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
typedef int (*aione_should_stop_callback)(void* user_data);

typedef struct aione_generate_callbacks {
    aione_token_callback on_token;
    aione_token_reasoning_callback on_token_reasoning;
    aione_progress_callback on_input_eval;
    aione_generation_start_callback on_generation_start;
    aione_think_state_callback on_think_state_change;
    aione_should_stop_callback should_stop;
    void* user_data;
} aione_generate_callbacks;

typedef struct aione_load_model_options {
    int no_host;
    unsigned int context_length;
} aione_load_model_options;

typedef struct aione_generate_options {
    unsigned int max_tokens;
    float min_p;
    int top_k;
    float temperature;
    unsigned int seed;
} aione_generate_options;

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
    const aione_load_model_options* options,
    aione_progress_callback on_progress,
    void* user_data
);
AIONE_API void aione_session_unload_model(aione_session* session);
AIONE_API int aione_session_is_loaded(const aione_session* session);
AIONE_API aione_generate_result aione_session_generate(
    aione_session* session,
    const char* prompt,
    const aione_generate_options* options,
    const aione_generate_callbacks* callbacks
);
AIONE_API void aione_generate_result_free(aione_generate_result* result);
AIONE_API char* aione_format_chat_prompt_json(
    aione_session* session,
    const char* messages_json,
    int add_generation_prompt
);
AIONE_API const char* aione_last_error(void);

/* Hardware information reported by the llama.cpp/ggml backends. */
AIONE_API char* aione_system_info_json(void);

/* Chat history storage, using the same on-disk format as the desktop app. */
AIONE_API char* aione_chat_list_json(void);
AIONE_API char* aione_chat_create_json(
    const char* title,
    const char* model,
    const char* system_prompt
);
AIONE_API char* aione_chat_load_json(const char* folder);
AIONE_API char* aione_chat_append_json(
    const char* folder,
    const char* role,
    const char* content
);
AIONE_API char* aione_chat_append_with_parent_json(
    const char* folder,
    const char* role,
    const char* content,
    unsigned long long parent_id
);
AIONE_API int aione_chat_update_metadata(
    const char* folder,
    const char* title,
    const char* model,
    const char* system_prompt
);
AIONE_API int aione_chat_delete(const char* folder);
AIONE_API void aione_string_free(char* value);

/* Text-to-speech (TTS) models. Requires an AIOne build with AIONE_ENABLE_TTS. */
typedef struct aione_tts aione_tts;

typedef enum aione_tts_audio_type {
    AIONE_TTS_AUDIO_PCM = 0,
    AIONE_TTS_AUDIO_WAV = 1
} aione_tts_audio_type;

typedef enum aione_tts_device {
    AIONE_TTS_DEVICE_ANY = 0,
    AIONE_TTS_DEVICE_CPU = 1,
    AIONE_TTS_DEVICE_IGPU = 2,
    AIONE_TTS_DEVICE_DGPU = 3,
    AIONE_TTS_DEVICE_ACCELERATOR = 4
} aione_tts_device;

typedef void (*aione_tts_frame_callback)(int frame_index, void* user_data);
typedef void (*aione_tts_load_progress_callback)(int stage, float progress, void* user_data);

typedef enum aione_tts_load_stage {
    AIONE_TTS_LOAD_STAGE_BACKBONE = 0,
    AIONE_TTS_LOAD_STAGE_MMPROJ = 1
} aione_tts_load_stage;

typedef struct aione_tts_load_options {
    int thread_count;
    int flash_attention;
    int device; /* aione_tts_device */
} aione_tts_load_options;

typedef struct aione_tts_voice {
    const char* reference_file;
    const char* built_in_name;
    int built_in_index;
} aione_tts_voice;

typedef struct aione_tts_synthesis_options {
    int output_type; /* aione_tts_audio_type */
    aione_tts_voice voice;
    const char* lang;
    int top_k;
    float top_p;
    unsigned int seed;
    int batch_size;
    int max_tokens;
} aione_tts_synthesis_options;

typedef struct aione_tts_synthesis_result {
    int sample_rate;
    long long samples;
    int audio_type; /* aione_tts_audio_type */
    char* data;
    unsigned long long data_size;
    double prompt_eval_ms;
    double generation_ms;
    double conversion_ms;
    int ok;
} aione_tts_synthesis_result;

AIONE_API aione_tts* aione_tts_create(void);
AIONE_API void aione_tts_destroy(aione_tts* tts);
AIONE_API int aione_tts_load_model(
    aione_tts* tts,
    const char* model_path,
    const char* mmproj_path,
    const aione_tts_load_options* options
);
/* Reports backbone or mmproj loading progress; progress is between 0 and 1. */
AIONE_API int aione_tts_load_model_with_progress(
    aione_tts* tts,
    const char* model_path,
    const char* mmproj_path,
    const aione_tts_load_options* options,
    aione_tts_load_progress_callback on_progress,
    void* user_data
);
AIONE_API void aione_tts_unload_model(aione_tts* tts);
AIONE_API int aione_tts_is_loaded(const aione_tts* tts);
AIONE_API char* aione_tts_voices_json(aione_tts* tts);
AIONE_API aione_tts_synthesis_result aione_tts_synthesize(
    aione_tts* tts,
    const char* prompt,
    const aione_tts_synthesis_options* options,
    aione_tts_frame_callback on_frame,
    void* user_data
);
AIONE_API void aione_tts_synthesis_result_free(aione_tts_synthesis_result* result);
AIONE_API int aione_tts_save_wav(
    aione_tts* tts,
    const char* filename,
    const char* data,
    unsigned long long data_size
);

#ifdef __cplusplus
}
#endif

#endif
