#pragma once
#include <string>
#include <cstdlib>

#include <stable-diffusion.h>

#include "model.h"

// Cross-platform environment variable setter
inline void setEnvironmentVariable(const std::string& name, const std::string& value) {
#ifdef _WIN32
    // Use _putenv_s for thread-safe operation and proper memory handling
    _putenv_s(name.c_str(), value.c_str());
#else
    setenv(name.c_str(), value.c_str(), 1);
#endif
}

class SDModel : public Model {
    sd_ctx_t* ctx = nullptr;
    std::string modelPath;  // Store the path to keep it alive
public:
    SDModel(std::string path) {
        loadModel(path);
    }

    void selectDevice(int device = 1) {
        setEnvironmentVariable("SD_VK_DEVICE", std::to_string(device));
    }

    bool loadModel(const std::string& path) {
        modelPath = path;  // Store the path

        auto device = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_GPU);
        if (!device) 
        {
            device = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_IGPU);
            if (!device) {
                device = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_CPU);
            }
        }

        selectDevice();
        

        if (ctx) {
            free_sd_ctx(ctx);
            ctx = nullptr;
        }

        sd_ctx_params_t params;
        // params.sd_ctx_params_t.
        sd_ctx_params_init(&params);  // THIS IS CRITICAL - call the init function!

        // Set only the necessary fields for SD 1.5
        params.model_path = modelPath.c_str();  // Use stored path

        // For SD 1.5, clip paths should be empty (model contains CLIP)
        params.clip_l_path = "";
        params.clip_g_path = "";

        // VAE is usually inside the model, but can be external
        params.vae_path = "";

        // FP16 model needs FP16 computation
        params.wtype = SD_TYPE_F16;  // MUST match your model format

        // Threads
        params.n_threads = 20;  // 0 = auto-detect

        // RNG settings
        params.rng_type = STD_DEFAULT_RNG;
        params.sampler_rng_type = STD_DEFAULT_RNG;

        // Important for SD 1.5
        params.prediction = EPS_PRED;  // SD 1.5 uses epsilon prediction

        // Memory settings
        params.vae_decode_only = true;
        params.free_params_immediately = false;  // Set to false for stability

        // Performance settings
        params.offload_params_to_cpu = false;
        params.enable_mmap = true;

        // Tensor type rules - important for FP16 models
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

        sd_img_gen_params_t img_gen_params;
        sd_img_gen_params_init(&img_gen_params);

        img_gen_params.prompt = prompt.c_str();
        img_gen_params.negative_prompt = "";

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

        img_gen_params.vae_tiling_params.enabled = false;
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

        sd_image_t* results = generate_image(ctx, &img_gen_params);
        auto num_results = img_gen_params.batch_count;
        if (!num_results) return {};
        return results[0];
    }
};
