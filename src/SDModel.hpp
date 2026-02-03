#pragma once
#include <string>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
#include <functional>
#include <thread>
#include <mutex >
#include <queue>


#include <ggml-backend.h>
#include <stable-diffusion.h>
#include "SDImageOptions.h"
#include "SDModelOptions.hpp"
// #ifndef STB_IMAGE_WRITE_IMPLEMENTATION
// #undef STB_IMAGE_WRITE_IMPLEMENTATION
// // #endif
// #include <stb_image_write.h>



#include "Model.hpp"

inline void setEnvironmentVariable(const std::string& name, const std::string& value) {
#ifdef _WIN32
    _putenv_s(name.c_str(), value.c_str());
#else
    setenv(name.c_str(), value.c_str(), 1);
#endif
}

class SDModel : public Model {
    sd_ctx_t* ctx = nullptr;
    std::string lastPrompt; 
    
    struct SafeImage {
        std::vector<uint8_t> data;
        int width = 0;
        int height = 0;
        int channel = 0;
    };
    SafeImage lastResult;

public:
    SDModel(){}
    SDModel(std::string path, SDModelOptions options = {}) {
        loadModel(path, options);
    }

    void selectDevice(int device = 1) {
        setEnvironmentVariable("SD_VK_DEVICE", std::to_string(device));
    }

    void setAllowSharedMemory(bool allow = true) {
        setEnvironmentVariable("GGML_VK_ALLOW_SYSMEM_FALLBACK", std::to_string(allow));
    }

    bool loadModel(const std::string path, SDModelOptions options = {}) {

        selectDevice();
        setAllowSharedMemory();

        if (ctx) free_sd_ctx(ctx);

        sd_ctx_params_t params;
        sd_ctx_params_init(&params);

        params.model_path = path.c_str();
        params.vae_path = options.vaePath.c_str();
        params.n_threads = options.threadCount;

        params.keep_clip_on_cpu = options.keepClipOnCpu;
        params.keep_control_net_on_cpu = options.keepControlNetOnCpu;
        params.keep_vae_on_cpu = options.keepVaeOnCpu;
        params.enable_mmap = options.useMmap;
        params.diffusion_flash_attn = options.flashAttention;

        // params.clip_l_path = "";
        // params.clip_g_path = "";
        // params.clip_vision_path = "";
        // params.t5xxl_path = "";
        // params.llm_path = "";
        // params.llm_vision_path = "";
        // params.diffusion_model_path = "";
        // params.high_noise_diffusion_model_path = "";
        params.taesd_path = "";
        params.control_net_path = "";
        params.embeddings = nullptr;
        params.embedding_count = 0;
        params.photo_maker_path = "";
        params.tensor_type_rules = "";
        params.vae_decode_only = false;
        params.free_params_immediately = false;
        // params.wtype = SD_TYPE_COUNT;
        params.wtype = SD_TYPE_COUNT;
        params.rng_type = CUDA_RNG;
        params.sampler_rng_type = RNG_TYPE_COUNT;
        params.prediction = PREDICTION_COUNT;
        params.lora_apply_mode = LORA_APPLY_AUTO;
        params.offload_params_to_cpu = false;
        params.tae_preview_only = false;
        params.diffusion_conv_direct = false;
        params.vae_conv_direct = false;
        params.circular_x = false;
        params.circular_y = false;
        params.force_sdxl_vae_conv_scale = false;
        params.chroma_use_dit_mask = true;
        params.chroma_use_t5_mask = false;
        params.chroma_t5_mask_pad = 1;
        params.qwen_image_zero_cond_t = false;
        // params.flow_shift = INFINITY;

        ctx = new_sd_ctx(&params);

        if (!ctx) {
            std::cerr << "Failed to load model: " << path << std::endl;
            return false;
        }

        return true;
    }

    bool saveImageAsPNG(const sd_image_t& image, const std::string& filename) ;

    using PreviewCallback = std::function<void(int step, int frameCount, sd_image_t* image, bool isNoisy)>;

    void setPreviewCallback(PreviewCallback previewCallback) {
        sd_set_preview_callback([](int step, int frameCount, sd_image_t* image, bool isNoisy, void* data) {
            auto callback = *(PreviewCallback *)data;
            if (callback) callback(step, frameCount, image, isNoisy);
        }, preview_t::PREVIEW_TAE, 1, true, true, (void*)&previewCallback);
    }

    std::thread generationWorker;

    std::vector<std::thread> generationThreads;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::thread generationThread;

    using ImageCompleteHandler = std::function<void(sd_image_t image)>;

    void generateAsync(const std::string positive, const std::string negative = "", SDImageOptions options = SDImageOptions{}, ImageCompleteHandler callback = nullptr) {
        // std::lock_guard<std::mutex> lock(queueMutex);
        // if (generationThread) generationThread.
        std::thread([this, positive, negative, options, callback]() {
            auto image = generateImage(positive, negative, options);
            if (callback) callback(image);
            // generationWorkers.
        }).detach();
    }

    sd_image_t generateImage(const std::string positive, const std::string negative = "", SDImageOptions options = SDImageOptions{}) {
        if (!ctx) {
            std::cerr << "Error: Model not loaded. Call loadModel() first!" << std::endl;
            return {};
        }

        sd_img_gen_params_t img_gen_params;
        sd_img_gen_params_init(&img_gen_params);
        // img_gen_params.loras = 0;
        // img_gen_params.lora_count = 0;
        img_gen_params.prompt = positive.c_str();
        img_gen_params.negative_prompt = negative.c_str();
        img_gen_params.clip_skip = options.clipSkip;
        img_gen_params.init_image.width = 0;
        img_gen_params.init_image.height = 0;
        img_gen_params.init_image.channel = 3;
        img_gen_params.init_image.data = 0;
        img_gen_params.ref_images = 0;
        img_gen_params.ref_images_count = 0;
        img_gen_params.auto_resize_ref_image = true;
        img_gen_params.increase_ref_index = false;

        img_gen_params.width = options.width;
        img_gen_params.height = options.height;

        img_gen_params.mask_image.data = (uint8_t*)malloc(img_gen_params.width * img_gen_params.height);
        if (img_gen_params.mask_image.data == nullptr) {

            return {};
        }
        img_gen_params.mask_image.width  = img_gen_params.width;
        img_gen_params.mask_image.height = img_gen_params.height;
        memset(img_gen_params.mask_image.data, 255, img_gen_params.width * img_gen_params.height);
        img_gen_params.mask_image.channel = 1;

        img_gen_params.strength = 0.750000000;
        img_gen_params.seed = 42;
        img_gen_params.batch_count = 1;
        img_gen_params.pm_params.id_images = 0;
        img_gen_params.pm_params.id_images_count = 0;
        img_gen_params.pm_params.id_embed_path = "";
        img_gen_params.pm_params.style_strength = 20.0;

        img_gen_params.vae_tiling_params.enabled = false;
        img_gen_params.vae_tiling_params.tile_size_x = 0;
        img_gen_params.vae_tiling_params.tile_size_y = 0;
        img_gen_params.vae_tiling_params.target_overlap = 0.5;
        img_gen_params.vae_tiling_params.rel_size_x = 0.0;
        img_gen_params.vae_tiling_params.rel_size_y = 0.0;

        sd_sample_params_init(&img_gen_params.sample_params);
        sd_cache_params_init(&img_gen_params.cache);

        std::vector<int> high_noise_skip_layers = {7, 8, 9};

        img_gen_params.sample_params.guidance.txt_cfg = 2;
        img_gen_params.sample_params.guidance.distilled_guidance = 3.5;
        img_gen_params.sample_params.guidance.slg.layer_count = high_noise_skip_layers.size();
        img_gen_params.sample_params.guidance.slg.layers = high_noise_skip_layers.data();
        img_gen_params.sample_params.guidance.slg.layer_start = 0.01;
        img_gen_params.sample_params.guidance.slg.layer_end = 0.2;
        img_gen_params.sample_params.guidance.slg.scale = 0;
        img_gen_params.sample_params.scheduler = SGM_UNIFORM_SCHEDULER;
        img_gen_params.sample_params.sample_method = EULER_A_SAMPLE_METHOD;
        img_gen_params.sample_params.sample_steps = options.stepCount;

        img_gen_params.control_image.channel = 3;


        sd_image_t* results = generate_image(ctx, &img_gen_params);
        auto num_results = img_gen_params.batch_count;
        
        if (!num_results || !results || !results[0].data) {
            std::cerr << "Image generation failed or returned empty result" << std::endl;
            return {};
        }

        const sd_image_t& source = results[0];
        
        lastResult.width = source.width;
        lastResult.height = source.height;
        lastResult.channel = source.channel;
        
        size_t dataSize = (size_t)source.width * source.height * source.channel;
        lastResult.data.assign(source.data, source.data + dataSize);

        // Create a return value with our copied data
        sd_image_t safeCopy;
        safeCopy.width = lastResult.width;
        safeCopy.height = lastResult.height;
        safeCopy.channel = lastResult.channel;
        safeCopy.data = lastResult.data.data();  // Point to our safe buffer

        this->saveImageAsPNG(safeCopy, positive + "rawr.png");
        
        return safeCopy;
    }

    ~SDModel() {
        if (ctx) {
            free_sd_ctx(ctx);
            ctx = nullptr;
        }
    }
};
