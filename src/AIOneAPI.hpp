#pragma once

#if defined(_WIN32)
  #if defined(AIONE_DYNAMIC)
    #if defined(AIOneCore_EXPORTS)
      #define AIONE_API __declspec(dllexport)
    #else
      #define AIONE_API __declspec(dllimport)
    #endif
  #else
    #define AIONE_API
  #endif
#else
  #define AIONE_API
#endif
