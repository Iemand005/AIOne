#pragma once
#include <string>

#include <stable-diffusion.h>

#include "model.h"

class SDModel : public Model {
    sd_ctx_t* ctx = nullptr;
public:
    SDModel(std::string path) {
        loadModel(path);
    }

    bool loadModel(const std::string& path) {
        if (ctx) {
            free_sd_ctx(ctx);
            ctx = nullptr;
        }
        
        // Set up context parameters
        sd_ctx_params_t params = {0};
        params.model_path = path.c_str();
        params.vae_path = nullptr;
        params.taesd_path = nullptr;
        params.control_net_path = nullptr;
        params.vae_decode_only = true;
        params.free_params_immediately = true;
        params.n_threads = -1;
        params.wtype = SD_TYPE_COUNT;
        params.rng_type = CPU_RNG;

        
        ctx = new_sd_ctx(&params);
        return ctx != nullptr;
    }

    void generateImage(const std::string &prompt) {
        sd_img_gen_params_t img_gen_params{};

        img_gen_params.prompt = prompt.c_str();
        img_gen_params.negative_prompt = "";

        img_gen_params.width = 100;
        img_gen_params.height = 100;

        sd_sample_params_init(&img_gen_params.sample_params);
        img_gen_params.sample_params.sample_steps = 1;
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
    }
};
