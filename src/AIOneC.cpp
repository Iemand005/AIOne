#include "AIOne.h"

#include "ModelSession.hpp"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

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

const char* aione_last_error(void) {
    return g_lastError.c_str();
}

}
