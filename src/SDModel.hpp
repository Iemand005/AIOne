#pragma once
#include <string>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include <ggml-backend.h>
#include <stable-diffusion.h>
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
    std::string modelPath;
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
    SDModel(std::string path) {
        loadModel(path);
    }

    void selectDevice(int device = 1) {
        setEnvironmentVariable("SD_VK_DEVICE", std::to_string(device));
    }

    bool loadModel(const std::string& path) {
        modelPath = path; 

        auto device = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_GPU);
        if (!device) 
        {
            device = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_IGPU);
            if (!device) {
                device = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_CPU);
            }
        }

        selectDevice();

        if (ctx) free_sd_ctx(ctx);

        sd_ctx_params_t params;
        sd_ctx_params_init(&params);

        // SDXL
        params.model_path = modelPath.c_str();
        params.clip_l_path = "";
        params.clip_g_path = "";
        params.clip_vision_path = "";
        params.t5xxl_path = "";
        params.llm_path = "";
        params.llm_vision_path = "";
        params.diffusion_model_path = "";
        params.high_noise_diffusion_model_path = "";
        params.vae_path = "";
        params.taesd_path = "";
        params.control_net_path = "";
        params.embeddings = nullptr;
        params.embedding_count = 0;
        params.photo_maker_path = "";
        params.tensor_type_rules = "";
        params.vae_decode_only = true;
        params.free_params_immediately = true;
        params.n_threads = 4;
        params.wtype = SD_TYPE_COUNT;
        params.rng_type = CUDA_RNG;
        params.sampler_rng_type = RNG_TYPE_COUNT;
        params.prediction = PREDICTION_COUNT;
        params.lora_apply_mode = LORA_APPLY_AUTO;
        params.offload_params_to_cpu = false;
        params.enable_mmap = false;
        params.keep_clip_on_cpu = false;
        params.keep_control_net_on_cpu = false;
        params.keep_vae_on_cpu = false;
        params.diffusion_flash_attn = false;
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
        params.flow_shift = INFINITY;


        // params.clip_l_path = modelPath.c_str();
        // params.clip_g_path = modelPath.c_str();
        // params.vae_path = modelPath.c_str();

        // params.vae_decode_only = false;

        // SD 1.5

        // params.vae_decode_only = true;

        // params.wtype = SD_TYPE_F16;

        // params.n_threads = 4;  // 0 = auto-detect

        // params.rng_type = STD_DEFAULT_RNG;
        // params.sampler_rng_type = STD_DEFAULT_RNG;

        // params.prediction = EPS_PRED;


        // params.free_params_immediately = false;

        // // Performance settings
        // params.offload_params_to_cpu = false;
        // params.keep_vae_on_cpu = false;
        // params.keep_clip_on_cpu = false;
        // params.keep_control_net_on_cpu = false;
        // params.enable_mmap = true;

        // params.tensor_type_rules = "";

        ctx = new_sd_ctx(&params);

        if (!ctx) {
            std::cerr << "Failed to load model: " << path << std::endl;
            return false;
        }

        return true;
    }

    bool saveImageAsPNG(const sd_image_t& image, const std::string& filename) ;

    sd_image_t generateImage(const std::string prompt, int width = 512, int height = 512, int steps = 10) {
        if (!ctx) {
            std::cerr << "Error: Model not loaded. Call loadModel() first!" << std::endl;
            return {};
        }

        sd_img_gen_params_t img_gen_params;
        sd_img_gen_params_init(&img_gen_params);
        img_gen_params.loras = 0;
        img_gen_params.lora_count = 0;
        img_gen_params.prompt = prompt.c_str();
        img_gen_params.negative_prompt = "";
        img_gen_params.clip_skip = -1;
        img_gen_params.init_image.width = 0;
        img_gen_params.init_image.height = 0;
        img_gen_params.init_image.channel = 3;
        img_gen_params.init_image.data = 0;
        img_gen_params.ref_images = 0;
        img_gen_params.ref_images_count = 0;
        img_gen_params.auto_resize_ref_image = true;
        img_gen_params.increase_ref_index = false;

        img_gen_params.width = 256;
        img_gen_params.height = 256;

        img_gen_params.mask_image.data = (uint8_t*)malloc(img_gen_params.width * img_gen_params.height);
        if (img_gen_params.mask_image.data == nullptr) {

            return {};
        }
        img_gen_params.mask_image.width  = img_gen_params.width;
        img_gen_params.mask_image.height = img_gen_params.height;
        memset(img_gen_params.mask_image.data, 255, img_gen_params.width * img_gen_params.height);

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



        // sd_img_gen_params_init(&img_gen_params);

        // img_gen_params.prompt = lastPrompt.c_str();  // Use stored prompt
        // img_gen_params.negative_prompt = "";

        // // img_gen_params.vae_tiling_params./*enabled*/ = false;

        // // SDXL works best with 512x512 or larger
        // img_gen_params.width = width;
        // img_gen_params.height = height;


        // img_gen_params.sample_params.sample_steps = steps;
        // img_gen_params.sample_params.sample_method = EULER_A_SAMPLE_METHOD;
        // img_gen_params.sample_params.scheduler = KARRAS_SCHEDULER;

        // img_gen_params.sample_params.guidance.txt_cfg = 5.0f;
        // img_gen_params.sample_params.guidance.distilled_guidance = 0.0f;

        // img_gen_params.clip_skip = -1;
        // img_gen_params.strength = 1.00f;
        // img_gen_params.seed = 42;
        // img_gen_params.batch_count = 1;
        // img_gen_params.control_strength = 1.0f;



        // img_gen_params.pm_params.id_images = nullptr;
        // img_gen_params.pm_params.id_images_count = 0;
        // img_gen_params.pm_params.id_embed_path = nullptr;
        // img_gen_params.pm_params.style_strength = 0.0f;

        // // CRITICAL FOR SDXL: Enable VAE tiling to handle large resolutions
        // img_gen_params.vae_tiling_params.enabled = false;
        // img_gen_params.vae_tiling_params.tile_size_x = 512;
        // img_gen_params.vae_tiling_params.tile_size_y = 512;
        // img_gen_params.vae_tiling_params.target_overlap = 0.2f;
        // img_gen_params.vae_tiling_params.rel_size_x = 1.0f;
        // img_gen_params.vae_tiling_params.rel_size_y = 1.0f;

        // img_gen_params.ref_images = nullptr;
        // img_gen_params.ref_images_count = 0;
        // img_gen_params.auto_resize_ref_image = true;
        // img_gen_params.increase_ref_index = false;

        // img_gen_params.loras = nullptr;
        // img_gen_params.lora_count = 0;

        sd_sample_params_init(&img_gen_params.sample_params);
        sd_cache_params_init(&img_gen_params.cache);

        img_gen_params.sample_params.guidance.txt_cfg = 12.0;
        img_gen_params.sample_params.guidance.distilled_guidance = 3.5;
        img_gen_params.sample_params.scheduler = SGM_UNIFORM_SCHEDULER;
        img_gen_params.sample_params.sample_method = EULER_A_SAMPLE_METHOD;
        img_gen_params.sample_params.sample_steps = 10;


        // Call the library function - do NOT store or delete the returned pointer!
        sd_image_t* results = generate_image(ctx, &img_gen_params);
        auto num_results = img_gen_params.batch_count;
        
        if (!num_results || !results || !results[0].data) {
            std::cerr << "Image generation failed or returned empty result" << std::endl;
            return {};
        }

        // CRITICAL: Copy the image data into our own buffer immediately
        // Do NOT store the pointer - the library manages that memory
        const sd_image_t& source = results[0];

        std::cout << "First 10 pixel values (R,G,B): ";
        for (int i = 0; i < 30 && i < source.width * source.height * 3; i += 3) {
            std::cout << "(" << (int)source.data[i] << ","
                      << (int)source.data[i+1] << ","
                      << (int)source.data[i+2] << ") ";
        }
        std::cout << std::endl;
        
        lastResult.width = source.width;
        lastResult.height = source.height;
        lastResult.channel = source.channel;
        
        // Copy pixel data
        size_t dataSize = (size_t)source.width * source.height * source.channel;
        lastResult.data.assign(source.data, source.data + dataSize);

        // Create a return value with our copied data
        sd_image_t safeCopy;
        safeCopy.width = lastResult.width;
        safeCopy.height = lastResult.height;
        safeCopy.channel = lastResult.channel;
        safeCopy.data = lastResult.data.data();  // Point to our safe buffer
        
        return safeCopy;
    }

    ~SDModel() {
        if (ctx) {
            free_sd_ctx(ctx);
            ctx = nullptr;
        }
    }
};
