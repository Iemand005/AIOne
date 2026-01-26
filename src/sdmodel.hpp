
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
        sd_img_gen_params_t img_gen_params = {0};

        results     = generate_image(sd_ctx, &img_gen_params);
        num_results = gen_params.batch_count;
    }
};
