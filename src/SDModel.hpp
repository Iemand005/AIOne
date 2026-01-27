#pragma once
#include <string>
#include <cstdlib>

#include <stable-diffusion.h>

#include "model.h"

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

        params.model_path = modelPath.c_str();

        params.clip_l_path = "";
        params.clip_g_path = "";

        params.vae_path = "";

        params.wtype = SD_TYPE_F16;

        params.n_threads = 20;  // 0 = auto-detect

        params.rng_type = STD_DEFAULT_RNG;
        params.sampler_rng_type = STD_DEFAULT_RNG;

        params.prediction = EPS_PRED;

        params.vae_decode_only = true;
        params.free_params_immediately = false;

        // Performance settings
        params.offload_params_to_cpu = false;
        params.keep_vae_on_cpu = false;
        params.keep_clip_on_cpu = false;
        params.keep_control_net_on_cpu = false;
        params.enable_mmap = true;

        params.tensor_type_rules = "";

        ctx = new_sd_ctx(&params);

        if (!ctx) {
            std::cerr << "Failed to load model: " << path << std::endl;
            return false;
        }

        return true;
    }

    sd_image_t generateImage(const std::string &prompt, int width = 512, int height = 512, int steps = 25) {
        if (!ctx) {
            std::cerr << "Error: Model not loaded. Call loadModel() first!" << std::endl;
            return {};
        }

        // Store the prompt to keep it alive
        lastPrompt = prompt;

        sd_img_gen_params_t img_gen_params;
        sd_img_gen_params_init(&img_gen_params);

        img_gen_params.prompt = lastPrompt.c_str();  // Use stored prompt
        img_gen_params.negative_prompt = "";

        // SDXL works best with 512x512 or larger
        img_gen_params.width = width;
        img_gen_params.height = height;

        sd_sample_params_init(&img_gen_params.sample_params);
        img_gen_params.sample_params.sample_steps = steps;
        img_gen_params.sample_params.sample_method = EULER_A_SAMPLE_METHOD;
        img_gen_params.sample_params.scheduler = KARRAS_SCHEDULER;

        img_gen_params.clip_skip = -1;
        img_gen_params.strength = 0.75f;
        img_gen_params.seed = 42;
        img_gen_params.batch_count = 1;
        img_gen_params.control_strength = 1.0f;

        sd_cache_params_init(&img_gen_params.cache);

        img_gen_params.pm_params.id_images = nullptr;
        img_gen_params.pm_params.id_images_count = 0;
        img_gen_params.pm_params.id_embed_path = nullptr;
        img_gen_params.pm_params.style_strength = 0.0f;

        // CRITICAL FOR SDXL: Enable VAE tiling to handle large resolutions
        img_gen_params.vae_tiling_params.enabled = true;  // CHANGED: Enable VAE tiling
        img_gen_params.vae_tiling_params.tile_size_x = 512;
        img_gen_params.vae_tiling_params.tile_size_y = 512;
        img_gen_params.vae_tiling_params.target_overlap = 0.2f;
        img_gen_params.vae_tiling_params.rel_size_x = 1.0f;
        img_gen_params.vae_tiling_params.rel_size_y = 1.0f;

        img_gen_params.ref_images = nullptr;
        img_gen_params.ref_images_count = 0;
        img_gen_params.auto_resize_ref_image = true;
        img_gen_params.increase_ref_index = false;

        img_gen_params.loras = nullptr;
        img_gen_params.lora_count = 0;

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
