#ifdef AIONE_ENABLE_STABLE_DIFFUSION

#include <thread>

#include <ggml-backend.h>
#include <stable-diffusion.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "SDModel.hpp"

struct SDModel::Impl {
    sd_ctx_t* ctx = nullptr;


    sd_type_t getSDType(QuantTypes level) {
        switch (level) {
            default:
        case QuantTypes::Q4_0: return SD_TYPE_Q4_0;
        case QuantTypes::Q8_0: return SD_TYPE_Q8_0;
        case QuantTypes::Q4_1: return SD_TYPE_Q4_1;
        case QuantTypes::Q5_0: return SD_TYPE_Q5_0;
        case QuantTypes::Q5_1: return SD_TYPE_Q5_1;
        case QuantTypes::F16: return SD_TYPE_F16;
        case QuantTypes::F32: return SD_TYPE_F32;
        }

    }

    SDImage toSDImage(sd_image_t oldImage) {
        SDImage image;
        image.channel = oldImage.channel;
        image.data = oldImage.data;
        image.height = oldImage.height;
        image.width = oldImage.width;
        return image;
    }

    sd_image_t fromSDImage(SDImage oldImage) {
sd_image_t image;
        image.channel = oldImage.channel;
        image.data = oldImage.data;
        image.height = oldImage.height;
        image.width = oldImage.width;
        return image;
    }

    preview_t toPreviewT(SDPreviewMode mode) {
        switch (mode) {
            default:
            case SDPreviewMode::None: return PREVIEW_NONE;
            case SDPreviewMode::Proj: return PREVIEW_PROJ;
            case SDPreviewMode::VAE: return PREVIEW_VAE;
            case SDPreviewMode::TAE: return PREVIEW_TAE;
        }
    }
};

SDModel::SDModel() {
    initSdCpp();
    impl = std::make_unique<Impl>();
}

SDModel::SDModel(std::string path, bool load) : modelPath(path) {
    initSdCpp();
    impl = std::make_unique<Impl>();
    if (load) loadModel();
}

SDModel::SDModel(std::string path, SDModelOptions options, bool load) : SDModel(path, false) {
    this->options = options;
    if (load) loadModel();
}



SDModel::~SDModel() {
    freeContext();
    clearPreviewCallback();
    clearProgressCallback();
}

void SDModel::freeContext(){
        if (impl->ctx) {
            free_sd_ctx(impl->ctx);
            impl->ctx = nullptr;
        }
    }

bool SDModel::saveImageAsPNG(const SDImage& imageo, const std::string& filename) {
    sd_image_t image = impl->fromSDImage(imageo);
        if (!image.data || image.width == 0 || image.height == 0) {
            std::cerr << "Invalid image data" << std::endl;
            return false;
        }

        int success = stbi_write_png(filename.c_str(), image.width, image.height, image.channel, image.data, image.width * image.channel);

        if (success) {
#ifdef AIONE_VERBOSE
            std::cout << "Saved image to: " << filename << std::endl;
#endif
            return true;
        } else {
#ifdef AIONE_VERBOSE
            std::cerr << "Failed to save image: " << filename << std::endl;
#endif
            return false;
        }
    }

    void SDModel::setPreviewCallback(PreviewCallback callback, SDPreviewMode mode ) {
        previewCallback = callback;
        sd_set_preview_callback([](int step, int batchSize, sd_image_t* image, bool isNoisy, void* data) {
            auto model = (SDModel *)data;
            SDImage sdImage = model->impl->toSDImage(*image);
            if (model->previewCallback) model->previewCallback(step, &sdImage, isNoisy);
            else model->clearPreviewCallback();
        }, impl->toPreviewT(mode), 1, true, true, this);
    }

    bool SDModel::loadModel(const std::string path, SDModelOptions options ) {
        selectDevice();
        setAllowSharedMemory();
        freeContext();

        if (options.onProgress) setProgressCallback(options.onProgress);

        sd_ctx_params_t params;
        sd_ctx_params_init(&params);

        params.model_path = path.c_str();
        params.vae_path = options.vaePath.c_str();
        params.taesd_path = options.taePath.c_str();
        params.n_threads = options.threadCount;

        params.diffusion_model_path = path.c_str();
        params.clip_l_path = options.clipLPath.c_str();
        params.clip_g_path = options.clipGPath.c_str();
        params.t5xxl_path = options.t5xxlPath.c_str();
        params.backend = options.backend.c_str();

        // params.keep_clip_on_cpu = options.keepClipOnCpu; // TODO removed by sd.cpp, find alt?
        // params.keep_control_net_on_cpu = options.keepControlNetOnCpu; // TODO removed by sd.cpp, find alt?
        // params.keep_vae_on_cpu = options.keepVaeOnCpu; // TODO removed by sd.cpp, find alt?
        params.enable_mmap = options.useMmap;
        params.diffusion_flash_attn = options.flashAttention;
        // params.vae_decode_only = options.vaeDecodeOnly; // TODO removed by sd.cpp, find alt?

        // params.clip_vision_path = "";
        // params.llm_path = "";
        // params.llm_vision_path = "";
        // params.high_noise_diffusion_model_path = "";

        params.control_net_path = "";
        params.embeddings = nullptr;
        params.embedding_count = 0;
        params.photo_maker_path = "";
        params.tensor_type_rules = "";
        // params.free_params_immediately = false; // TODO removed by sd.cpp, find alt?
        params.wtype = SD_TYPE_COUNT;
        // params.wtype = SD_TYPE_Q4_K;
        params.rng_type = CUDA_RNG;
        params.sampler_rng_type = RNG_TYPE_COUNT;
        params.prediction = PREDICTION_COUNT;
        params.lora_apply_mode = LORA_APPLY_AUTO;
        // params.offload_params_to_cpu = false; // TODO removed by sd.cpp, find alt?
        params.tae_preview_only = false;
        params.diffusion_conv_direct = false;
        params.vae_conv_direct = false;
        // params.circular_x = false; // TODO removed by sd.cpp, find alt?
        // params.circular_y = false; // TODO removed by sd.cpp, find alt?
        params.force_sdxl_vae_conv_scale = false;
        // params.chroma_use_dit_mask = true; // TODO removed by sd.cpp, find alt?
        // params.chroma_use_t5_mask = false; // TODO removed by sd.cpp, find alt?
        // params.chroma_t5_mask_pad = 1; // TODO removed by sd.cpp, find alt?
        // params.qwen_image_zero_cond_t = false; // TODO removed by sd.cpp, find alt?
        // params.flow_shift = INFINITY;

        // if (options)

        impl->ctx = new_sd_ctx(&params);

        if (!impl->ctx) {
            std::cerr << "Failed to load model: " << path << std::endl;
            return false;
        }

        return true;
    }

    void SDModel::setProgressCallback(ProgressCallback callback) {
        // progressCallback = callback;
        super()->setProgressCallback(callback);

        sd_set_progress_callback([](int step, int steps, float time, void *data){
            auto model = (SDModel *)data;
            auto onProgress = model->progressCallback();
            if (onProgress) onProgress((float)step / (float)steps, step, steps);
            else model->clearProgressCallback();
        }, this);
    }
    void SDModel::clearProgressCallback() {
        sd_set_progress_callback(nullptr, nullptr);
        // progressCallback = nullptr;
        super()->clearProgressCallback();
    }
    void SDModel::clearPreviewCallback() {
        sd_set_preview_callback(nullptr, PREVIEW_NONE, 0, false, false, nullptr);
        previewCallback = nullptr;
    }


    bool SDModel::exportToGGUF(std::string destination, QuantTypes level , bool convertTensorsName ) {
        std::string tensor_type_rules = "";
        return convert(modelPath.c_str(), options.vaePath.c_str(), destination.c_str(),impl->getSDType(level), tensor_type_rules.c_str(), convertTensorsName);
    }

    
    SDImage SDModel::generateImage(const std::string positive, const std::string negative , SDImageOptions options) {
        if (!impl->ctx) {
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
        // img_gen_params.auto_resize_ref_image = true; // TODO removed by sd.cpp, find alt?
        // img_gen_params.increase_ref_index = false; // TODO removed by sd.cpp, find alt?

        img_gen_params.width = options.width;
        img_gen_params.height = options.height;



        // if ()
        // img_gen_params.seed = !options.randomSeed ? options.seed : ;
        img_gen_params.seed = options.seed;


        img_gen_params.mask_image.data = (uint8_t*)malloc(img_gen_params.width * img_gen_params.height);
        if (img_gen_params.mask_image.data == nullptr) {

            return {};
        }
        img_gen_params.mask_image.width  = img_gen_params.width;
        img_gen_params.mask_image.height = img_gen_params.height;
        memset(img_gen_params.mask_image.data, 255, img_gen_params.width * img_gen_params.height);
        img_gen_params.mask_image.channel = 1;

        img_gen_params.strength = 0.750000000;

        
        img_gen_params.batch_count = options.batchSize;
        img_gen_params.pm_params.id_images = 0;
        img_gen_params.pm_params.id_images_count = 0;
        img_gen_params.pm_params.id_embed_path = "";
        img_gen_params.pm_params.style_strength = 20.0;

        img_gen_params.vae_tiling_params.enabled = options.tiling.enabled;
        // img_gen_params.vae_tiling_params.tile_size_x = options.tiling.tileWidth; // TODO removed by sd.cpp, find alt?
        // img_gen_params.vae_tiling_params.tile_size_y = options.tiling.tileHeight; // TODO removed by sd.cpp, find alt?
        img_gen_params.vae_tiling_params.target_overlap = options.tiling.overlap;
        // img_gen_params.vae_tiling_params.rel_size_x = 0.0; // TODO removed by sd.cpp, find alt?
        // img_gen_params.vae_tiling_params.rel_size_y = 0.0; // TODO removed by sd.cpp, find alt?

        sd_sample_params_init(&img_gen_params.sample_params);
        sd_cache_params_init(&img_gen_params.cache);

        std::vector<int> high_noise_skip_layers = {7, 8, 9};

        img_gen_params.sample_params.guidance.txt_cfg = 2;
        img_gen_params.sample_params.guidance.distilled_guidance = 3.5;
        img_gen_params.sample_params.guidance.slg.layer_count = high_noise_skip_layers.size();
        img_gen_params.sample_params.guidance.slg.layers = high_noise_skip_layers.data();
        img_gen_params.sample_params.guidance.slg.layer_start = 0.01f;
        img_gen_params.sample_params.guidance.slg.layer_end = 0.2f;
        img_gen_params.sample_params.guidance.slg.scale = 0;
        img_gen_params.sample_params.scheduler = SGM_UNIFORM_SCHEDULER;
        img_gen_params.sample_params.sample_method = EULER_A_SAMPLE_METHOD;
        img_gen_params.sample_params.sample_steps = options.stepCount;

        img_gen_params.control_image.channel = 3;

        if (options.previewMode != SDPreviewMode::None)
            setPreviewCallback(previewCallback, options.previewMode);

        sd_image_generation_result_t *results = nullptr;
        bool genSuccess = generate_image(impl->ctx, &img_gen_params, &results);
        int num_results;
        
        if (!genSuccess || !(num_results = results->count) || results->items == nullptr) {
            std::cerr << "Image generation failed or returned empty result" << std::endl;
            return {};
        }

        // TODO there's also items[0].generation_info, maybe cool to return as well?
        const sd_image_t& source = results->items[0].image;
        
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
        // std::chars_format
        // this->saveImageAsPNG(impl->toSDImage(safeCopy), positive +""+ std::to_string(img_gen_params.seed) + "rawr.png");
        
        return impl->toSDImage(safeCopy);
    }

    void SDModel::initSdCpp() {
#ifdef AIONE_VERBOSE
        sd_set_log_callback(sdCppLogger, nullptr);
#endif
    }

    void SDModel::sdCppLogger(enum sd_log_level_t level, const char* text, void* data) {
#ifdef AIONE_VERBOSE
        if (level == sd_log_level_t::SD_LOG_ERROR) {
            std::cerr << "[sd.cpp] " << text;
        } else {
            std::cout << "[sd.cpp] " << text;
        }
#endif
    }

#else // !AIONE_ENABLE_STABLE_DIFFUSION

#include "SDModel.hpp"

struct SDModel::Impl {};

static void warnStableDiffusionDisabled() {
    std::cerr << "Stable diffusion support is disabled in this build" << std::endl;
}

SDModel::SDModel() : impl(std::make_unique<Impl>()) {}

SDModel::SDModel(std::string path, bool load) : modelPath(path), impl(std::make_unique<Impl>()) {}

SDModel::~SDModel() = default;

void SDModel::freeContext() {}

bool SDModel::loadModel(const std::string, SDModelOptions) {
    warnStableDiffusionDisabled();
    return false;
}

bool SDModel::exportToGGUF(std::string, QuantTypes, bool) {
    warnStableDiffusionDisabled();
    return false;
}

bool SDModel::saveImageAsPNG(const SDImage&, const std::string&) {
    warnStableDiffusionDisabled();
    return false;
}

void SDModel::setProgressCallback(ProgressCallback callback) {
    super()->setProgressCallback(callback);
}

void SDModel::setPreviewCallback(PreviewCallback callback, SDPreviewMode) {
    previewCallback = callback;
}

void SDModel::clearProgressCallback() {
    super()->clearProgressCallback();
}

void SDModel::clearPreviewCallback() {
    previewCallback = nullptr;
}

SDImage SDModel::generateImage(const std::string, const std::string, SDImageOptions) {
    warnStableDiffusionDisabled();
    return {};
}

void SDModel::initSdCpp() {}
void SDModel::sdCppLogger(enum sd_log_level_t level, const char* text, void* data) {}

#endif // AIONE_ENABLE_STABLE_DIFFUSION
