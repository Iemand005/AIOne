#pragma once

#if defined(_WIN32)
  #if defined(AIONE_STATIC)
    #define AIONE_API
  #elif defined(AIOneCore_EXPORTS)
    #define AIONE_API __declspec(dllexport)
  #else
    #define AIONE_API __declspec(dllimport)
  #endif
#else
  #define AIONE_API
#endif
