
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
        sd_img_gen_params_t img_gen_params{};

        img_gen_params.loras = nullptr;
        img_gen_params.prompt = prompt.c_str();
        img_gen_params.negative_prompt = "";
        img_gen_params.clip_skip = -1;
        img_gen_params.ref_images = nullptr;

        sd_sample_params_t sample_params{};
        sd_sample_params_init(&sample_params);
        sample_params.sample_steps = 20;
        sample_params.guidance.txt_cfg = 7.0f;

        img_gen_params.sample_params = sample_params;
        img_gen_params.strength = 0.0f;
        img_gen_params.seed = 42;
        img_gen_params.batch_count = 1;
        img_gen_params.control_image = sd_image_t{0, 0, 3, nullptr};
        img_gen_params.control_strength = 0.0f;


        auto results = generate_image(ctx, &img_gen_params);
        int num_results = img_gen_params.batch_count;
    }
};
