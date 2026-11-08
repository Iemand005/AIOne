#include "AIOne.h"

#include "Chat.hpp"
#include "ChatStorage.hpp"
#include "Message.hpp"
#include "ModelFactory.hpp"
#include "ModelSession.hpp"
#include "TTSModel.hpp"

#include <ggml-backend.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

struct aione_session {
    AIOne::ModelSession session;
};

struct aione_tts {
    ModelFactory factory;
    TTSModelPtr model;
};

namespace {

thread_local std::string g_lastError;

void setError(const std::string& message) {
    g_lastError = message;
}

char* duplicateString(const std::string& value) {
    char* buffer = static_cast<char*>(std::malloc(value.size() + 1));
    if (buffer) {
        std::memcpy(buffer, value.c_str(), value.size() + 1);
    }
    return buffer;
}

std::mutex g_chatMutex;

bool isValidFolder(const std::string& folder) {
    if (folder.empty() || folder == "." || folder == "..") return false;
    return folder.find('/') == std::string::npos &&
           folder.find('\\') == std::string::npos &&
           folder.find(':') == std::string::npos;
}

std::string chatPath(const std::string& folder) {
    return ChatStorage::rootPath() + "/" + folder;
}

int64_t nowMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

nlohmann::json messageJson(const Message& message) {
    return {
        {"id", std::to_string(message.id)},
        {"parentId", std::to_string(message.parentId)},
        {"role", message.role},
        {"content", message.content},
        {"creationTime", message.timestamps.creationTime},
        {"finishTime", message.timestamps.modificationTime}
    };
}

nlohmann::json metadataJson(const ChatMetadata& meta) {
    return {
        {"folder", meta.folder},
        {"title", meta.title},
        {"model", meta.model},
        {"systemPrompt", meta.systemPrompt},
        {"created", meta.created},
        {"updated", meta.updated}
    };
}

AIOne::LoadOptions makeLoadOptions(const aione_load_model_options& options) {
    AIOne::LoadOptions result;
    result.noHost = options.no_host != 0;
    result.contextLength = options.context_length;
    return result;
}

AIOne::GenerateOptions makeGenerateOptions(const aione_generate_options& options) {
    AIOne::GenerateOptions result;
    result.maxTokens = options.max_tokens;
    result.minP = options.min_p;
    result.topK = static_cast<short>(options.top_k);
    result.temperature = options.temperature;
    result.seed = options.seed;
    return result;
}

AIOne::GenerateCallbacks makeCallbacks(const aione_generate_callbacks* callbacks) {
    AIOne::GenerateCallbacks result;
    if (!callbacks) return result;

    void* userData = callbacks->user_data;

    if (callbacks->on_token) {
        const aione_token_callback callback = callbacks->on_token;
        result.onToken = [callback, userData](const std::string& token) {
            callback(token.c_str(), userData);
        };
    }

    if (callbacks->on_token_reasoning) {
        const aione_token_reasoning_callback callback = callbacks->on_token_reasoning;
        result.onTokenReasoning = [callback, userData](const std::string& token, bool thinking) {
            callback(token.c_str(), thinking ? 1 : 0, userData);
        };
    }

    if (callbacks->on_input_eval) {
        const aione_progress_callback callback = callbacks->on_input_eval;
        result.onInputEval = [callback, userData](const float progress, const int step, const int steps) {
            callback(progress, userData);
        };
    }

    if (callbacks->on_generation_start) {
        const aione_generation_start_callback callback = callbacks->on_generation_start;
        result.onGenerationStart = [callback, userData]() {
            callback(userData);
        };
    }

    if (callbacks->on_think_state_change) {
        const aione_think_state_callback callback = callbacks->on_think_state_change;
        result.onThinkStateChange = [callback, userData](bool thinking) {
            callback(thinking ? 1 : 0, userData);
        };
    }

    if (callbacks->should_stop) {
        const aione_should_stop_callback callback = callbacks->should_stop;
        result.shouldStop = [callback, userData]() {
            return callback(userData) != 0;
        };
    }

    return result;
}

PreferredDevice makeTtsDevice(int device) {
    switch (device) {
        case AIONE_TTS_DEVICE_CPU: return PreferredDevice::CPU;
        case AIONE_TTS_DEVICE_IGPU: return PreferredDevice::IGPU;
        case AIONE_TTS_DEVICE_DGPU: return PreferredDevice::DGPU;
        case AIONE_TTS_DEVICE_ACCELERATOR: return PreferredDevice::ACCELERATOR;
        default: return PreferredDevice::ANY;
    }
}

TTSModelOptions makeTtsModelOptions(
    const aione_tts_load_options* options,
    aione_tts_load_progress_callback progressCallback,
    void* userData
) {
    TTSModelOptions result;
    if (options) {
        if (options->thread_count > 0) {
            result.threadCount = static_cast<short>(options->thread_count);
        }
        result.flashAttention = options->flash_attention != 0;
        result.device = makeTtsDevice(options->device);
    }
    if (progressCallback) {
        const auto callback = progressCallback;
        auto lastPercent = std::make_shared<int>(-1);
        result.onProgress = [callback, userData, lastPercent](float progress, int, int) {
            const int percent = static_cast<int>(progress * 100.0f);
            if (percent == *lastPercent) return;
            *lastPercent = percent;
            callback(AIONE_TTS_LOAD_STAGE_BACKBONE, progress, userData);
        };
    }
    return result;
}

ModelOptions makeTtsMmprojOptions(
    const aione_tts_load_options* options,
    aione_tts_load_progress_callback progressCallback,
    void* userData
) {
    ModelOptions result;
    if (options) {
        if (options->thread_count > 0) {
            result.threadCount = static_cast<short>(options->thread_count);
        }
        result.flashAttention = options->flash_attention != 0;
    }
    if (progressCallback) {
        const auto callback = progressCallback;
        auto lastPercent = std::make_shared<int>(-1);
        result.onProgress = [callback, userData, lastPercent](float progress, int, int) {
            const int percent = static_cast<int>(progress * 100.0f);
            if (percent == *lastPercent) return;
            *lastPercent = percent;
            callback(AIONE_TTS_LOAD_STAGE_MMPROJ, progress, userData);
        };
    }
    return result;
}

TTSSynthesisOptions makeTtsSynthesisOptions(const aione_tts_synthesis_options* options) {
    TTSSynthesisOptions result{};
    if (!options) return result;

    result.outputType = options->output_type == AIONE_TTS_AUDIO_WAV
        ? TTSAudioType::WAV
        : TTSAudioType::PCM;
    if (options->voice.reference_file) {
        result.voice.referenceFile = options->voice.reference_file;
    }
    if (options->voice.built_in_name) {
        result.voice.builtInName = options->voice.built_in_name;
    }
    result.voice.builtInIndex = options->voice.built_in_index;
    if (options->lang) {
        result.lang = options->lang;
    }
    result.topK = static_cast<short>(options->top_k);
    result.topP = options->top_p;
    result.seed = options->seed;
    result.batchSize = options->batch_size;
    result.maxTokens = options->max_tokens;
    return result;
}

}

extern "C" {

aione_session* aione_session_create(void) {
    try {
        return new aione_session();
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to create session");
    }
    return nullptr;
}

void aione_session_destroy(aione_session* session) {
    delete session;
}

int aione_session_load_model(
    aione_session* session,
    const char* path,
    const aione_load_model_options* options,
    aione_progress_callback on_progress,
    void* user_data
) {
    if (!session || !path || !options) {
        setError("Invalid argument");
        return 0;
    }

    try {
        ProgressCallback progress;
        if (on_progress) {
            progress = [on_progress, user_data](const float value, const int step, const int steps) {
                on_progress(value, user_data);
            };
        }

        return session->session.loadModelWithOptions(path, makeLoadOptions(*options), progress) ? 1 : 0;
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to load model");
    }
    return 0;
}

void aione_session_unload_model(aione_session* session) {
    if (!session) return;

    try {
        session->session.unloadModel();
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to unload model");
    }
}

int aione_session_is_loaded(const aione_session* session) {
    if (!session) return 0;
    return session->session.isLoaded() ? 1 : 0;
}

aione_generate_result aione_session_generate(
    aione_session* session,
    const char* prompt,
    const aione_generate_options* options,
    const aione_generate_callbacks* callbacks
) {
    aione_generate_result result{};

    if (!session || !prompt || !options) {
        setError("Invalid argument");
        return result;
    }

    try {
        const TextGenResult generated = session->session.generateWithOptions(
            prompt,
            makeGenerateOptions(*options),
            makeCallbacks(callbacks)
        );

        result.content = duplicateString(generated.output.content);
        result.tokens_evaluated = generated.tokensEvaluated;
        result.tokens_generated = generated.tokensGenerated;
        result.tokens_cached = generated.tokensCached;
        result.prompt_eval_ms = generated.promptEvalMs;
        result.generation_ms = generated.generationMs;
        result.prompt_tokens_per_second = generated.promptTokensPerSecond;
        result.tokens_per_second = generated.tokensPerSecond;
        result.ok = 1;
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to generate");
    }

    return result;
}

void aione_generate_result_free(aione_generate_result* result) {
    if (!result) return;
    std::free(result->content);
    result->content = nullptr;
}

char* aione_format_chat_prompt_json(
    aione_session* session,
    const char* messages_json,
    int add_generation_prompt
) {
    if (!session || !messages_json) {
        setError("Invalid argument");
        return nullptr;
    }

    try {
        const nlohmann::json input = nlohmann::json::parse(messages_json);
        std::vector<Message> messages;

        for (const auto& item : input) {
            Message message;
            message.role = item.value("role", "user");
            message.content = item.value("content", "");
            messages.push_back(std::move(message));
        }

        std::string prompt;

        if (add_generation_prompt) {
            prompt = session->session.formatChatPrompt(messages, true);
        } else {
            if (messages.empty()) {
                setError("No messages provided");
                return nullptr;
            }

            Message draft = messages.back();
            messages.pop_back();
            prompt = session->session.formatChatDraftPrompt(messages, draft);
        }

        return duplicateString(prompt);
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to format chat prompt");
    }

    return nullptr;
}

char* aione_chat_list_json(void) {
    try {
        std::lock_guard<std::mutex> lock(g_chatMutex);
        ChatStorage::init();

        nlohmann::json result = nlohmann::json::array();
        for (const auto& meta : ChatStorage::scan()) {
            result.push_back(metadataJson(meta));
        }

        return duplicateString(result.dump());
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to list chats");
    }
    return nullptr;
}

char* aione_chat_create_json(
    const char* title,
    const char* model,
    const char* system_prompt
) {
    try {
        std::lock_guard<std::mutex> lock(g_chatMutex);
        ChatStorage::init();

        ChatMetadata meta;
        meta.title = (title && *title) ? title : "New chat";
        meta.model = model ? model : "";
        meta.systemPrompt = system_prompt ? system_prompt : "";

        const std::string folder = ChatStorage::createChat(meta);

        if (!meta.systemPrompt.empty()) {
            Chat chat(meta.systemPrompt);
            chat.setFolder(chatPath(folder));
            auto messages = chat.getMessages();
            if (!messages.empty()) chat.saveMessage(messages[0]);
        }

        return duplicateString(metadataJson(ChatStorage::loadMetadata(folder)).dump());
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to create chat");
    }
    return nullptr;
}

char* aione_chat_load_json(const char* folder) {
    if (!folder || !isValidFolder(folder)) {
        setError("Invalid chat folder");
        return nullptr;
    }

    try {
        std::lock_guard<std::mutex> lock(g_chatMutex);
        ChatStorage::init();

        const ChatMetadata meta = ChatStorage::loadMetadata(folder);
        if (meta.folder.empty()) {
            setError("Chat not found");
            return nullptr;
        }

        nlohmann::json result = metadataJson(meta);
        nlohmann::json messages = nlohmann::json::array();
        const auto loadedMessages = ChatStorage::loadMessages(folder);
        Chat chat;
        chat.setMessages(loadedMessages);
        nlohmann::json activePath = nlohmann::json::array();
        for (const auto& message : chat.getActivePath()) {
            activePath.push_back(std::to_string(message.id));
        }
        for (const auto& message : loadedMessages) {
            messages.push_back(messageJson(message));
        }
        result["messages"] = std::move(messages);
        result["activePath"] = std::move(activePath);

        return duplicateString(result.dump());
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to load chat");
    }
    return nullptr;
}

char* aione_chat_append_with_parent_json(
    const char* folder,
    const char* role,
    const char* content,
    unsigned long long parent_id
) {
    if (!folder || !isValidFolder(folder)) {
        setError("Invalid chat folder");
        return nullptr;
    }
    if (!role) {
        setError("Invalid role");
        return nullptr;
    }

    const std::string roleValue(role);
    if (roleValue != "system" && roleValue != "user" && roleValue != "assistant") {
        setError("Invalid role");
        return nullptr;
    }

    try {
        std::lock_guard<std::mutex> lock(g_chatMutex);
        ChatStorage::init();

        const std::string path = chatPath(folder);
        if (!std::filesystem::is_directory(path)) {
            setError("Chat not found");
            return nullptr;
        }

        Chat chat;
        chat.setFolder(path);
        chat.setMessages(ChatStorage::loadMessages(folder));

        if (parent_id != 0) {
            chat.addMessage(Message(roleValue, content ? content : "", parent_id));
        } else {
            chat.addMessage(roleValue, content ? content : "");
        }

        return duplicateString(messageJson(chat.getLastMessage()).dump());
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to save chat message");
    }
    return nullptr;
}

char* aione_chat_append_json(
    const char* folder,
    const char* role,
    const char* content
) {
    return aione_chat_append_with_parent_json(folder, role, content, 0);
}

int aione_chat_update_metadata(
    const char* folder,
    const char* title,
    const char* model,
    const char* system_prompt
) {
    if (!folder || !isValidFolder(folder)) {
        setError("Invalid chat folder");
        return 0;
    }

    try {
        std::lock_guard<std::mutex> lock(g_chatMutex);
        ChatStorage::init();

        ChatMetadata meta = ChatStorage::loadMetadata(folder);
        if (meta.folder.empty()) {
            setError("Chat not found");
            return 0;
        }

        if (title) meta.title = title;
        if (model) meta.model = model;
        if (system_prompt) meta.systemPrompt = system_prompt;
        meta.updated = nowMillis();

        ChatStorage::saveMetadata(folder, meta);
        return 1;
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to update chat metadata");
    }
    return 0;
}

int aione_chat_delete(const char* folder) {
    if (!folder || !isValidFolder(folder)) {
        setError("Invalid chat folder");
        return 0;
    }

    try {
        std::lock_guard<std::mutex> lock(g_chatMutex);
        ChatStorage::init();

        const std::filesystem::path path = chatPath(folder);
        if (!std::filesystem::is_directory(path)) {
            setError("Chat not found");
            return 0;
        }

        std::filesystem::remove_all(path);
        return 1;
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to delete chat");
    }
    return 0;
}

char* aione_system_info_json(void) {
    try {
        ModelFactory factory;
        factory.loadBackends();
        factory.initLlama();

        nlohmann::json devices = nlohmann::json::array();

        for (size_t i = 0; i < ggml_backend_dev_count(); ++i) {
            ggml_backend_dev_t device = ggml_backend_dev_get(i);
            if (!device) continue;

            ggml_backend_dev_props props{};
            ggml_backend_dev_get_props(device, &props);

            const char* type = "unknown";
            switch (props.type) {
                case GGML_BACKEND_DEVICE_TYPE_CPU: type = "cpu"; break;
                case GGML_BACKEND_DEVICE_TYPE_GPU: type = "gpu"; break;
                case GGML_BACKEND_DEVICE_TYPE_IGPU: type = "igpu"; break;
                case GGML_BACKEND_DEVICE_TYPE_ACCEL: type = "accelerator"; break;
                case GGML_BACKEND_DEVICE_TYPE_META: type = "meta"; break;
            }

            devices.push_back({
                {"index", i},
                {"name", props.name ? props.name : ""},
                {"description", props.description ? props.description : ""},
                {"type", type},
                {"memoryFree", props.memory_free},
                {"memoryTotal", props.memory_total},
                {"deviceId", props.device_id ? props.device_id : ""}
            });
        }

        nlohmann::json result;
        result["devices"] = std::move(devices);
        result["systemInfo"] = factory.systemInfoStr();

        return duplicateString(result.dump());
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to read system information");
    }
    return nullptr;
}

aione_tts* aione_tts_create(void) {
    try {
        return new aione_tts();
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to create TTS handle");
    }
    return nullptr;
}

void aione_tts_destroy(aione_tts* tts) {
    delete tts;
}

static int loadTtsModel(
    aione_tts* tts,
    const char* model_path,
    const char* mmproj_path,
    const aione_tts_load_options* options,
    aione_tts_load_progress_callback progressCallback,
    void* userData
) {
    if (!tts || !model_path || !mmproj_path) {
        setError("Invalid argument");
        return 0;
    }

    try {
        tts->factory.loadBackends();
        tts->factory.initLlama();
        tts->factory.initCommon();

        auto model = std::make_unique<TTSModel>(
            model_path,
            makeTtsModelOptions(options, progressCallback, userData),
            mmproj_path,
            makeTtsMmprojOptions(options, progressCallback, userData),
            false
        );

        if (!model->loadModel()) {
            setError("Failed to load TTS backbone model");
            return 0;
        }
        if (progressCallback) {
            progressCallback(AIONE_TTS_LOAD_STAGE_BACKBONE_READY, 1.0f, userData);
        }

        if (!model->loadMmproj()) {
            setError("Failed to load TTS mmproj");
            return 0;
        }
        if (progressCallback) {
            progressCallback(AIONE_TTS_LOAD_STAGE_MMPROJ_READY, 1.0f, userData);
        }

        tts->model = std::move(model);
        return 1;
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to load TTS model");
    }
    return 0;
}

int aione_tts_load_model(
    aione_tts* tts,
    const char* model_path,
    const char* mmproj_path,
    const aione_tts_load_options* options
) {
    return loadTtsModel(tts, model_path, mmproj_path, options, nullptr, nullptr);
}

int aione_tts_load_model_with_progress(
    aione_tts* tts,
    const char* model_path,
    const char* mmproj_path,
    const aione_tts_load_options* options,
    aione_tts_load_progress_callback on_progress,
    void* user_data
) {
    return loadTtsModel(tts, model_path, mmproj_path, options, on_progress, user_data);
}

void aione_tts_unload_model(aione_tts* tts) {
    if (!tts) return;
    tts->model.reset();
}

int aione_tts_is_loaded(const aione_tts* tts) {
    if (!tts || !tts->model) return 0;
    return tts->model->isReady() ? 1 : 0;
}

char* aione_tts_voices_json(aione_tts* tts) {
    if (!tts || !tts->model) {
        setError("No TTS model loaded");
        return nullptr;
    }

    try {
        nlohmann::json result = nlohmann::json::array();
        for (const auto& voice : tts->model->getVoices()) {
            result.push_back(voice);
        }
        return duplicateString(result.dump());
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to list TTS voices");
    }
    return nullptr;
}

aione_tts_synthesis_result aione_tts_synthesize(
    aione_tts* tts,
    const char* prompt,
    const aione_tts_synthesis_options* options,
    aione_tts_frame_callback on_frame,
    void* user_data
) {
    aione_tts_synthesis_result result{};

    if (!tts || !tts->model || !prompt) {
        setError("Invalid argument");
        return result;
    }

    try {
        TTSSynthesisOptions synthesisOptions = makeTtsSynthesisOptions(options);
        if (on_frame) {
            synthesisOptions.onFrame = [on_frame, user_data](const int frameIdx, const mtmd_helper::gen_audio&) {
                on_frame(frameIdx, user_data);
            };
        }

        const TTSSynthesisResult generated = tts->model->synthesize(prompt, synthesisOptions);

        result.sample_rate = generated.sampleRate;
        result.samples = generated.samples;
        result.audio_type = static_cast<int>(generated.audio.type);
        result.data_size = static_cast<unsigned long long>(generated.audio.data.size());
        if (result.data_size > 0) {
            result.data = static_cast<char*>(std::malloc(static_cast<size_t>(result.data_size)));
            if (!result.data) {
                setError("Out of memory");
                result.data_size = 0;
                return result;
            }
            std::memcpy(result.data, generated.audio.data.data(), static_cast<size_t>(result.data_size));
        }
        result.prompt_eval_ms = generated.promptEvalMs;
        result.generation_ms = generated.generationMs;
        result.conversion_ms = generated.conversionMs;
        result.ok = 1;
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to synthesize speech");
    }

    return result;
}

void aione_tts_synthesis_result_free(aione_tts_synthesis_result* result) {
    if (!result) return;
    std::free(result->data);
    result->data = nullptr;
    result->data_size = 0;
    result->ok = 0;
}

int aione_tts_save_wav(
    aione_tts* tts,
    const char* filename,
    const char* data,
    unsigned long long data_size
) {
    if (!tts || !tts->model || !filename || (data_size > 0 && !data)) {
        setError("Invalid argument");
        return 0;
    }

    try {
        TTSAudio audio;
        audio.type = TTSAudioType::WAV;
        if (data_size > 0) {
            audio.data.assign(data, data + static_cast<size_t>(data_size));
        }

        return tts->model->saveAudioAsWAV(audio, filename) ? 1 : 0;
    } catch (const std::exception& e) {
        setError(e.what());
    } catch (...) {
        setError("Failed to save WAV file");
    }
    return 0;
}

void aione_string_free(char* value) {
    std::free(value);
}

const char* aione_last_error(void) {
    return g_lastError.c_str();
}

}
