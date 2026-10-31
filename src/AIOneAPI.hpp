#pragma once

#if defined(_WIN32)
  #ifdef AIONE_DYNAMIC
    #ifdef AIOneCore_EXPORTS
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
