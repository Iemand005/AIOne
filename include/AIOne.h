#ifndef AIONE_CAPI_H
#define AIONE_CAPI_H

#ifndef AIONE_API
#  if defined(_WIN32)
#    if defined(AIONE_DYNAMIC)
#      if defined(AIOneCore_EXPORTS)
#        define AIONE_API __declspec(dllexport)
#      else
#        define AIONE_API __declspec(dllimport)
#      endif
#    else
#      define AIONE_API
#    endif
#  else
#    define AIONE_API
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aione_session aione_session;

typedef void (*aione_progress_callback)(float progress, void* user_data);
typedef void (*aione_token_callback)(const char* token, void* user_data);
typedef void (*aione_token_reasoning_callback)(const char* token, int thinking, void* user_data);
typedef void (*aione_generation_start_callback)(void* user_data);
typedef void (*aione_think_state_callback)(int thinking, void* user_data);

typedef struct aione_generate_callbacks {
    aione_token_callback on_token;
    aione_token_reasoning_callback on_token_reasoning;
    aione_progress_callback on_input_eval;
    aione_generation_start_callback on_generation_start;
    aione_think_state_callback on_think_state_change;
    void* user_data;
} aione_generate_callbacks;

typedef struct aione_generate_result {
    char* content;
    unsigned long long tokens_evaluated;
    unsigned long long tokens_generated;
    unsigned long long tokens_cached;
    double prompt_eval_ms;
    double generation_ms;
    double prompt_tokens_per_second;
    double tokens_per_second;
    int ok;
} aione_generate_result;

AIONE_API aione_session* aione_session_create(void);
AIONE_API void aione_session_destroy(aione_session* session);
AIONE_API int aione_session_load_model(
    aione_session* session,
    const char* path,
    aione_progress_callback on_progress,
    void* user_data
);
AIONE_API void aione_session_unload_model(aione_session* session);
AIONE_API int aione_session_is_loaded(const aione_session* session);
AIONE_API aione_generate_result aione_session_generate(
    aione_session* session,
    const char* prompt,
    unsigned int max_tokens,
    const aione_generate_callbacks* callbacks
);
AIONE_API void aione_generate_result_free(aione_generate_result* result);
AIONE_API const char* aione_last_error(void);

#ifdef __cplusplus
}
#endif

#endif
