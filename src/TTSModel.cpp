#include "TTSModel.hpp"
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>

#include "ModelOptions.h"
#include "TTSModelOptions.h"
#include "TTSSynthesisOptions.hpp"
#include "TTSSynthesisResult.hpp"
#include "TTSAudio.hpp"

#ifdef AIONE_ENABLE_TTS
#include <llama.h>
#include <llama-model.h>
#include <common.h>
#include <sampling.h>
#include <chat.h>
#include <mtmd.h>
#include <mtmd-helper.h>

TTSModel::TTSModel(const std::string& path, bool load) {
    modelPath = path;
    if (load) loadModel();
}

TTSModel::TTSModel(const std::string& path, TTSModelOptions options, bool load) : TTSModel(path, false) {
    modelOptions = options;
    if (load) loadModel();
}

TTSModel::TTSModel(const std::string& path, const std::string& mmprojPath, bool load) : TTSModel(path, false) {
    this->mmprojPath = mmprojPath;

    if (load) {
        if (loadModel()) loadMmproj();
    }
}

TTSModel::TTSModel(const std::string& path, TTSModelOptions options, const std::string& mmprojPath, ModelOptions mmprojOptions, bool load) : TTSModel(path, mmprojPath, false) {
    modelOptions = options;
    this->mmprojOptions = mmprojOptions;
    
    if (load) {
        if (loadModel()) loadMmproj();
    }
}

bool TTSModel::loadModel() {
    auto options = modelOptions;
    modelOptions.onProgress = nullptr;
    return loadModel(modelPath, options);
}

bool TTSModel::loadModel(const std::string& path, TTSModelOptions options) {
    if (loaded) {
        return true;
    }

#ifdef __EMSCRIPTEN__
    const int threadCount = std::min<int>(options.threadCount, 3);
#else
    const int threadCount = options.threadCount;
#endif

    // getBackend() is used by mmproj loading, so resolve ANY to an available device.
    selectDevice(options.device);
#ifdef __EMSCRIPTEN__
    std::cout << "[AIOne TTS] Selected device: " << ggml_backend_dev_name(devices[0])
              << "; GPU offload: all eligible layers; CPU threads: " << threadCount << std::endl;
#endif

    // load model
    common_params params;
    params.model.path = path;
#ifdef __EMSCRIPTEN__
    params.load_mode = LLAMA_LOAD_MODE_NONE;
    params.fit_params = false;
    params.n_gpu_layers = -1;
    params.n_ctx = 4096;
    params.n_batch = 512;
    params.n_ubatch = 128;
#endif
    params.devices = devices;
    params.flash_attn_type = (options.flashAttention ? LLAMA_FLASH_ATTN_TYPE_ENABLED : LLAMA_FLASH_ATTN_TYPE_DISABLED);
    params.cpuparams.n_threads = threadCount;
    params.cpuparams_batch.n_threads = threadCount;
    params.embedding = true; // required to pass hidden states to audio gen helper
    if (options.onProgress) {
        params.load_progress_callback = [](float progress, void* userData) {
            (*static_cast<ProgressCallback*>(userData))(progress, 0, 0);
            return true;
        };
        params.load_progress_callback_user_data = &options.onProgress;
    }

    postprocess_cpu_params(params.cpuparams);
    postprocess_cpu_params(params.cpuparams_batch, &params.cpuparams);

    llamaInit = common_init_from_params(params);
    model = llamaInit->model();
    if (!model) {
        std::cerr << "Failed to initialize TTS model" << std::endl;
        return false;
    }

    ctx = llamaInit->context();
    if (!ctx) {
        std::cerr << "Failed to initialize TTS context" << std::endl;
        return false;
    }

    sampler = llamaInit->sampler(0);

    loaded = true;
    return true;
}

bool TTSModel::loadMmproj() {
    auto options = mmprojOptions;
    mmprojOptions.onProgress = nullptr;
    return loadMmproj(mmprojPath, options);
}

bool TTSModel::loadMmproj(const std::string& path, ModelOptions options) {
    if (!loaded) {
        std::cerr << "Backbone model not loaded, call loadModel(...) first" << std::endl;
        return false;
    }

#ifdef __EMSCRIPTEN__
    const int threadCount = std::min<int>(options.threadCount, 3);
#else
    const int threadCount = options.threadCount;
#endif

    mtmd_context_params mtmdParams = mtmd_context_params_default();
    mtmdParams.device = getBackend();

    enum ggml_backend_dev_type deviceType = ggml_backend_dev_type(mtmdParams.device);
    mtmdParams.use_gpu = (deviceType == GGML_BACKEND_DEVICE_TYPE_GPU || deviceType == GGML_BACKEND_DEVICE_TYPE_IGPU);
    mtmdParams.flash_attn_type = (options.flashAttention ? LLAMA_FLASH_ATTN_TYPE_ENABLED : LLAMA_FLASH_ATTN_TYPE_DISABLED);
    mtmdParams.n_threads = threadCount;
    if (options.onProgress) {
        mtmdParams.progress_callback = [](float progress, void* userData) {
            (*static_cast<ProgressCallback*>(userData))(progress, 0, 0);
            return true;
        };
        mtmdParams.progress_callback_user_data = &options.onProgress;
    }
    
    mctx = mtmd::context_ptr(mtmd_init_from_file(path.c_str(), model, mtmdParams));
    if (!mctx) {
        std::cerr << "Failed to initialize TTS multimodal context" << std::endl;
        return false;
    }
    
    mctxInfo = mtmd_gen_audio_get_info(mctx.get());
    if (mctxInfo.type == MTMD_GEN_AUDIO_TYPE_NONE) {
        std::cerr << "Loaded mmproj does not support audio generation" << std::endl;
        return false;
    }

    loadedMmproj = true;
    return true;
}

std::vector<std::string> TTSModel::getVoices() {
    std::vector<std::string> voices;
    
    if (!requireFull()) {
        return voices;
    }

    for (int32_t i = 0; const char *name = mtmd_gen_audio_voice_name(mctx.get(), i); i++) {
        voices.push_back(std::string(name));
    }

    return voices;
}

TTSSynthesisResult TTSModel::synthesize(const std::string& prompt, TTSSynthesisOptions options) {
    if (!requireFull(true)) {
        return {};
    }

    /*
       stage 0: process speaker reference file, if any
    */

    mtmd::bitmap_ptr speakerBmp;

    if (!options.voice.referenceFile.empty()) {
        // load voice from a reference file
        mtmd_helper_bitmap_wrapper wrapper = mtmd_helper_bitmap_init_from_file(mctx.get(), options.voice.referenceFile.c_str(), false, mtmd_helper_init_opt_default());
        if (!wrapper.bitmap) {
            throw std::runtime_error("Failed to load reference file");
        }

        speakerBmp.reset(wrapper.bitmap);
    } else if (mctxInfo.n_voices > 0) {
        // load a built-in voice
        if (options.voice.builtInName.empty()) {
            options.voice.builtInName = mtmd_gen_audio_voice_name(mctx.get(), options.voice.builtInIndex);
        }

        speakerBmp.reset(mtmd_gen_audio_get_voice(mctx.get(), options.voice.builtInName.c_str()));

        if (!speakerBmp) {
            throw std::runtime_error("Failed to load built-in voice, call getVoices() to see what voices this model has");
        }
    }

    // put prompt through the tts chat template if the model has one (e.g. LFM3-Audio)
    common_chat_templates_ptr templates = common_chat_templates_init(model, "");
    std::string modelPrompt = common_chat_get_tts_prompt(templates.get(), prompt, options.lang, options.voice.referenceFile.empty() ? options.voice.builtInName : "");
    if (modelPrompt.empty()) {
        modelPrompt = prompt;
    }

    /*
       stage 1: process prompt via backbone model to generate semantic representation
    */
    mtmd_helper::gen_audio gen;
    gen.init(ctx, mctx.get());

    mtmd_helper_gen_audio_inp input = mtmd_helper_gen_audio_inp_default();
    input.seq_id = 0;
    input.prompt = prompt.c_str();
    input.prompt_len = prompt.size();
    input.speaker_ref = speakerBmp.get();
    input.lang = options.lang.empty() ? nullptr : options.lang.c_str();
    input.top_k = options.topK;
    input.top_p = options.topP;
    input.seed = options.seed;

    TTSAudioType actualOutputType = options.outputType;
    if (options.outputType == TTSAudioType::PCM) {
        input.out_type = MTMD_HELPER_GEN_AUDIO_OUTTYPE_PCM;
    } else {
        actualOutputType = TTSAudioType::WAV;
        input.out_type = MTMD_HELPER_GEN_AUDIO_OUTTYPE_WAV;
    }
    
    if (gen.set_input(&input) != 0) {
        throw std::runtime_error("Failed to set model input");
    }

    // process prompt
    const int64_t promptStartTime = ggml_time_us();
    while (true) {
        int32_t ret = gen.step_prompt(options.batchSize);
        if (ret < 0) {
            throw std::runtime_error("Failed to process prompt");
        }

        if (ret == 0) {
            break;
        }
    }
    const int64_t promptStopTime = ggml_time_us();

    // tts works with two codes, semantic and acoustic
    //   semantic is like the pacing of the voice, prompt meaning and features
    //   acoustic is how it's said and how the audio will turn out
    //   then this gets turned into audio waveform
    // we have to sample semantic ourselves but gen.step_gen does acoustic and waveform
    auto newSemanticCode = [&]() {
        llama_token t = common_sampler_sample(sampler, ctx, -1);
        common_sampler_accept(sampler, t, true);
        return t;
    };

    llama_token sampled = newSemanticCode();
    const float *hiddenState = llama_get_embeddings_ith(ctx, -1);
    const int maxNewToks = (options.maxTokens >= 0 ? options.maxTokens : 512);
    int frames = 0;
    bool stop = false;

    // generate audio
    const int64_t genStartTime = ggml_time_us();
    while (!stop && frames < maxNewToks) {
        const float *newHiddenState;
        if (gen.step_gen(sampled, hiddenState, &newHiddenState, &stop) != 0) {
            throw std::runtime_error("Failed to generate frame index " + std::to_string(frames));
        }

        if (!newHiddenState) {
            // stopped without generating a new frame
            break;
        }

        frames++;
        hiddenState = newHiddenState;
        sampled = newSemanticCode();

        if (options.onFrame) options.onFrame(frames - 1, gen); // pass `gen` incase listener wants to preview audio or smth (.get_output)
    }
    const int64_t genStopTime = ggml_time_us();
    
    // convert to wav
    TTSSynthesisResult result;
    const char *wav;
    size_t wavLen;

    const int64_t audioStartTime = ggml_time_us();
    if (gen.get_output(&result.sampleRate, &wav, &wavLen, &result.samples) != 0) {
        throw std::runtime_error("Failed to convert state to audio");
    }
    const int64_t audioStopTime = ggml_time_us();

    result.audio.type = actualOutputType;
    result.audio.data.assign(wav, wav + wavLen);

    result.promptEvalMs = (promptStopTime - promptStartTime) / 1e6;
    result.generationMs = (genStopTime - genStartTime) / 1e6;
    result.conversionMs = (audioStopTime - audioStartTime) / 1e6;
    return result;
}

bool TTSModel::saveAudioAsWAV(const TTSAudio& audio, const std::string& filename) {
    if (audio.type != TTSAudioType::WAV) {
        return false;
    }

    FILE *file = fopen(filename.c_str(), "wb");
    if (!file) {
        return false;
    }

    fwrite(audio.data.data(), 1, audio.data.size(), file);
    fclose(file);
    return true;
}

TTSModel::~TTSModel() {
    if (loadedMmproj) {
        mctx.reset();
    }

    if (loaded) {
        sampler = nullptr;
        ctx = nullptr;
        model = nullptr;
        llamaInit.reset();
    }
    
    loaded = false;
    loadedMmproj = false;
}

#else // !AIONE_ENABLE_TTS

static void throwMtmdDisabled() {
    throw std::runtime_error("libmtmd is disabled in this build");
}

TTSModel::TTSModel(const std::string& path, bool load) {
    modelPath = path;
    if (load) throwMtmdDisabled();
}

TTSModel::TTSModel(const std::string& path, TTSModelOptions options, bool load) : TTSModel(path, false) {
    modelOptions = options;
    if (load) throwMtmdDisabled();
}

TTSModel::TTSModel(const std::string& path, const std::string& mmprojPath, bool load) : TTSModel(path, false) {
    this->mmprojPath = mmprojPath;
    if (load) throwMtmdDisabled();
}

TTSModel::TTSModel(const std::string& path, TTSModelOptions options, const std::string& mmprojPath, ModelOptions mmprojOptions, bool load) : TTSModel(path, mmprojPath, false) {
    modelOptions = options;
    this->mmprojOptions = mmprojOptions;
    if (load) throwMtmdDisabled();
}

bool TTSModel::loadModel() {
    throwMtmdDisabled();
    return false;
}

bool TTSModel::loadModel(const std::string& path, TTSModelOptions options) {
    throwMtmdDisabled();
    return false;
}

bool TTSModel::loadMmproj() {
    throwMtmdDisabled();
    return false;
}

bool TTSModel::loadMmproj(const std::string& path, ModelOptions options) {
    throwMtmdDisabled();
    return false;
}

std::vector<std::string> TTSModel::getVoices() {
    return std::vector<std::string>();
}

TTSSynthesisResult TTSModel::synthesize(const std::string& prompt, TTSSynthesisOptions options) {
    throwMtmdDisabled();
    return {};
}

bool TTSModel::saveAudioAsWAV(const TTSAudio& audio, const std::string& filename) {
    throwMtmdDisabled();
    return false;
}

TTSModel::~TTSModel() {}

#endif // AIONE_ENABLE_TTS