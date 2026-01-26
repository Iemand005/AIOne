
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
        sd_ctx_params_t params = {};
        params.model_path = path.c_str();
        params.vae_path = nullptr;
        params.taesd_path = nullptr;
        params.control_net_path = nullptr,
        params.vae_decode_only = true,
        params.free_params_immediately = true;
        params.n_threads = -1,  // auto-detect
        params.wtype = SD_TYPE_COUNT,  // auto-detect from fil;
        params.rng_type = CPU_RNG;

        
        ctx = new_sd_ctx(&params);
        return ctx != nullptr;
    }

    void generateImage(const std::string &prompt) {
        sd_img_gen_params_t img_gen_params = {
            gen_params.lora_vec.data(),
            static_cast<uint32_t>(gen_params.lora_vec.size()),
            gen_params.prompt.c_str(),
            gen_params.negative_prompt.c_str(),
            gen_params.clip_skip,
            init_image,
            ref_images.data(),
            (int)ref_images.size(),
            gen_params.auto_resize_ref_image,
            gen_params.increase_ref_index,
            mask_image,
            gen_params.get_resolved_width(),
            gen_params.get_resolved_height(),
            gen_params.sample_params,
            gen_params.strength,
            gen_params.seed,
            gen_params.batch_count,
            control_image,
            gen_params.control_strength,
            {
                pmid_images.data(),
                (int)pmid_images.size(),
                gen_params.pm_id_embed_path.c_str(),
                gen_params.pm_style_strength,
            },  // pm_params
            ctx_params.vae_tiling_params,
            gen_params.cache_params,
        };

        results     = generate_image(sd_ctx, &img_gen_params);
        num_results = gen_params.batch_count;
    }
};
