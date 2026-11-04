#include "AIOne.h"

#include "Chat.hpp"
#include "ChatStorage.hpp"
#include "Message.hpp"
#include "ModelFactory.hpp"
#include "ModelSession.hpp"

#include <ggml-backend.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

struct aione_session {
    AIOne::ModelSession session;
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
        {"id", message.id},
        {"parentId", message.parentId},
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
        result.onInputEval = [callback, userData](const float progress) {
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
    aione_progress_callback on_progress,
    void* user_data
) {
    if (!session || !path) {
        setError("Invalid argument");
        return 0;
    }

    try {
        ProgressCallback progress;
        if (on_progress) {
            progress = [on_progress, user_data](const float value) {
                on_progress(value, user_data);
            };
        }

        return session->session.loadModel(path, progress) ? 1 : 0;
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
    unsigned int max_tokens,
    const aione_generate_callbacks* callbacks
) {
    aione_generate_result result{};

    if (!session || !prompt) {
        setError("Invalid argument");
        return result;
    }

    try {
        const TextGenResult generated = session->session.generate(
            prompt,
            max_tokens,
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
        for (const auto& message : ChatStorage::loadMessages(folder)) {
            messages.push_back(messageJson(message));
        }
        result["messages"] = std::move(messages);

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

void aione_string_free(char* value) {
    std::free(value);
}

const char* aione_last_error(void) {
    return g_lastError.c_str();
}

}
