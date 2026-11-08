cmake_minimum_required(VERSION 3.16)
project(AIOneCore VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# if(AIONE_UWP)
#     target_compile_definitions(llama PRIVATE WINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP)
    # set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0.22621.0" CACHE STRING "" FORCE)
    # set(WINAPI_FAMILY "WINAPI_FAMILY_DESKTOP_APP" CACHE STRING "" FORCE)
    # add_definitions(-DWINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP)
    # -D=
# endif()

option(AIONE_OPENCL "Enable OpenCL support via CLBlast" OFF)
option(AIONE_VULKAN "Enable Vulkan support" OFF)
option(AIONE_WEBGPU "Enable WebGPU support (only for emscripten)" OFF)
option(AIONE_MSVC_STATIC_RUNTIME "Use static MSVC runtime (/MT,/MTd) for AIOneCore" OFF)
option(AIONE_BUILD_SHARED_LIBS "Build AIOneCore as a shared library (DLL)" OFF)
option(AIONE_DEPS_SHARED "Build llama.cpp and stable-diffusion.cpp as shared libraries" OFF)
option(AIONE_UWP "Enable UWP support (Disables mmap)" OFF)
option(AIONE_ENABLE_OPENAI "Enable OpenAI/HTTPS provider and remote API support" OFF)
option(AIONE_ENABLE_STABLE_DIFFUSION "Enable stable-diffusion.cpp image generation support" OFF)
option(AIONE_ENABLE_TTS "Enable text-to-speech support (builds libmtmd)" OFF)
option(AIONE_ENABLE_TTS_INSTRUCT "Enable text-to-speech instruction support (requires custom llama.cpp)" OFF)
option(AIONE_VERBOSE "Enable verbose logging from AIOne and llama.cpp" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(AIONE_GGML_SHARED "Use static MSVC runtime (/MT,/MTd) for AIOneCore" OFF)
option(AIONE_SD_SHARED "Use static MSVC runtime (/MT,/MTd) for AIOneCore" OFF)
option(AIONE_BUILD_SHARED_LIBS "Build AIOneCore as a shared library (DLL)" OFF)
option(AIONE_DEPS_SHARED "Build llama.cpp and stable-diffusion.cpp as shared libraries" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(AIONE_PRELOAD_GGML "Use the `external/ggml` version of GGML instead of llama.cpp's" ON)

if(MSVC)
    # if(AIONE_MSVC_STATIC_RUNTIME)
    #     set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    # else()
    #     set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>")
    # endif()

    add_compile_options( # need to do it this way or else they leak into nvcc which thinks they are input files
        "$<$<COMPILE_LANGUAGE:C,CXX>:/FS;/bigobj;/MP;/W3;/wd4244;/wd4267>"
    )
    # add_compile_options(/GR- /EHs-) # no RTTI, no C++ exceptions
    # add_compile_options(/GR-) # no RTTI, no C++ exceptions
    # add_link_options(/MAP /MAPINFO:EXPORTS)
    add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)

    if(AIONE_UWP)
        # add_compile_options(/ZW)
        # Ensure platform.winmd is found for C++/CX builds
        if(DEFINED ENV{WindowsSdkDir} AND CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
            add_compile_options(
                "$<$<COMPILE_LANGUAGE:C,CXX>:/AI\"$ENV{WindowsSdkDir}UnionMetadata\\${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}\">"
                "$<$<COMPILE_LANGUAGE:C,CXX>:/AI\"$ENV{WindowsSdkDir}References\\${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}\">"
            )
        endif()
    endif()
else()
    if(DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
        unset(CMAKE_MSVC_RUNTIME_LIBRARY CACHE)
        unset(CMAKE_MSVC_RUNTIME_LIBRARY)
    endif()
endif()

set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES ON CACHE BOOL "" FORCE)

if(AIONE_UWP)
    set(LLAMA_UWP ON CACHE BOOL "Enable UWP code paths in llama.cpp" FORCE)
else()
    set(LLAMA_UWP OFF CACHE BOOL "Enable UWP code paths in llama.cpp" FORCE)
endif()

# set(LLAMA_CUBLAS OFF CACHE BOOL "" FORCE)
# set(LLAMA_VULKAN OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) # Default static in this subdir

# set(SD_CUDA OFF CACHE BOOL "" FORCE)
set(SD_HIPBLAS OFF CACHE BOOL "" FORCE)
set(SD_METAL OFF CACHE BOOL "" FORCE)
# set(SD_VULKAN OFF CACHE BOOL "" FORCE)
set(SD_OPENCL OFF CACHE BOOL "" FORCE)
set(SD_SYCL OFF CACHE BOOL "" FORCE)
set(SD_MUSA OFF CACHE BOOL "" FORCE)

# set(GGML_CPU ON CACHE BOOL "" FORCE)
# set(GGML_CUDA OFF CACHE BOOL "" FORCE)
set(GGML_HIP OFF CACHE BOOL "" FORCE)
set(GGML_METAL OFF CACHE BOOL "" FORCE)
# set(GGML_VULKAN OFF CACHE BOOL "" FORCE)
set(GGML_OPENCL OFF CACHE BOOL "" FORCE)
set(GGML_SYCL OFF CACHE BOOL "" FORCE)
if(EMSCRIPTEN)
    set(GGML_WEBGPU ON CACHE BOOL "" FORCE)
    set(GGML_NATIVE OFF CACHE BOOL "" FORCE)
    set(GGML_OPENMP OFF CACHE BOOL "" FORCE)
else()
    set(GGML_WEBGPU OFF CACHE BOOL "" FORCE)
endif()
set(GGML_ZDNN OFF CACHE BOOL "" FORCE)
set(GGML_ZENDNN OFF CACHE BOOL "" FORCE)
set(GGML_RPC OFF CACHE BOOL "" FORCE)
set(GGML_VIRTGPU OFF CACHE BOOL "" FORCE)
set(GGML_MUSA OFF CACHE BOOL "" FORCE)

get_filename_component(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external" ABSOLUTE)
set(LLAMA_CPP_DIR "${THIRD_PARTY_DIR}/llama.cpp")
set(STABLE_DIFFUSION_CPP_DIR "${THIRD_PARTY_DIR}/stable-diffusion.cpp")

unset(GGML_CPU CACHE)

if (AIONE_NOCPU)
    set(GGML_CPU OFF CACHE BOOL "" FORCE)
else()
    set(GGML_CPU ON CACHE BOOL "" FORCE)
endif()

# unset(LLAMA_CUBLAS CACHE)
unset(SD_CUDA CACHE)
unset(GGML_CUDA CACHE)

if (AIONE_CUDA)
    # set(LLAMA_CUBLAS ON CACHE BOOL "" FORCE) # will not build if set to on cuz llama cpp wants GGML_CUDA
    set(SD_CUDA ON CACHE BOOL "" FORCE)
    set(GGML_CUDA ON CACHE BOOL "" FORCE)
else()
    # set(LLAMA_CUBLAS OFF CACHE BOOL "" FORCE)
    set(SD_CUDA OFF CACHE BOOL "" FORCE)
    set(GGML_CUDA OFF CACHE BOOL "" FORCE)
endif()

unset(SD_VULKAN CACHE)
unset(LLAMA_VULKAN CACHE)
unset(GGML_VULKAN CACHE)

if(AIONE_VULKAN)
    # set(VULKAN_SDK_PATH "C:/VulkanSDK/1.4.335.0")
    set(CMAKE_PREFIX_PATH "${VULKAN_SDK_PATH};${CMAKE_PREFIX_PATH}" )

    set(LLAMA_VULKAN ON CACHE BOOL "Enable Vulkan for llama.cpp" FORCE)
    set(GGML_VULKAN ON CACHE BOOL "Enable Vulkan for ggml" FORCE)
    set(SD_VULKAN ON CACHE BOOL "Enable Vulkan for stable-diffusion.cpp" FORCE)
    message(STATUS "Vulkan support: ENABLED")

    set(GGML_VULKAN_VALIDATE OFF CACHE BOOL "" FORCE)
    set(GGML_VULKAN_CHECK_RESULTS OFF CACHE BOOL "" FORCE)

    set(GGML_VULKAN_SHADER_SOURCE ON CACHE BOOL "Compile Vulkan shaders from source" FORCE)
    set(GGML_VULKAN_SHADER_BINARY OFF CACHE BOOL "Use precompiled shader binaries" FORCE)

    add_definitions(-DGGML_USE_VULKAN -DSD_USE_VULKAN)
else()
    set(LLAMA_VULKAN OFF CACHE BOOL "" FORCE)
    set(GGML_VULKAN OFF CACHE BOOL "" FORCE)
    set(SD_VULKAN OFF CACHE BOOL "" FORCE)
    message(STATUS "Vulkan support: DISABLED")
endif()

if(AIONE_VULKAN)
    set(GGML_VULKAN_VALIDATE OFF CACHE BOOL "" FORCE)
    set(GGML_VULKAN_CHECK_RESULTS OFF CACHE BOOL "" FORCE)
    set(GGML_VULKAN_RUN_TESTS OFF CACHE BOOL "" FORCE)
    set(GGML_VULKAN_SHADER_DEBUG_INFO OFF CACHE BOOL "" FORCE)
endif()

if(VULKAN_SDK_PATH)
    set(Vulkan_ROOT "${VULKAN_SDK_PATH}")
    list(APPEND CMAKE_PREFIX_PATH "${VULKAN_SDK_PATH}")
endif()

if(AIONE_VULKAN AND WIN32)
    if(DEFINED ENV{VULKAN_SDK})
        set(VULKAN_SDK_PATH "$ENV{VULKAN_SDK}")
        message(STATUS "Found Vulkan SDK from environment: ${VULKAN_SDK_PATH}")
    else()
        file(GLOB vulkan_dirs RELATIVE "C:/VulkanSDK" "C:/VulkanSDK/*")
        
        set(newest_version "0.0.0.0")
        set(VULKAN_SDK_PATH "")

        foreach(dir ${vulkan_dirs})
            # Controleer of de mapnaam matcht met een versiestructuur (bijv. 1.4.363.0)
            if(dir MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$")
                if(dir VERSION_GREATER newest_version)
                    set(newest_version ${dir})
                    set(VULKAN_SDK_PATH "C:/VulkanSDK/${dir}")
                endif()
            endif()
        endforeach()

        if(NOT VULKAN_SDK_PATH AND EXISTS "C:/VulkanSDK")
            set(VULKAN_SDK_PATH "C:/VulkanSDK")
        endif()

        if(VULKAN_SDK_PATH)
            message(STATUS "Automatically selected newest Vulkan SDK: ${VULKAN_SDK_PATH} (Version: ${newest_version})")
        else()
            message(WARNING "Vulkan SDK not found. Please install it or set VULKAN_SDK environment variable")
        endif()
    endif()


    if(VULKAN_SDK_PATH AND EXISTS "${VULKAN_SDK_PATH}")
        message(STATUS "Using Vulkan SDK at: ${VULKAN_SDK_PATH}")
        set(CMAKE_PREFIX_PATH "${VULKAN_SDK_PATH};${CMAKE_PREFIX_PATH}")
        set(Vulkan_DIR "${VULKAN_SDK_PATH}/cmake")
    endif()
endif()

if(AIONE_VULKAN)

    
    set(Vulkan_INCLUDE_DIR "${VULKAN_SDK_PATH}/Include" CACHE PATH "Vulkan include dir" FORCE)
    set(Vulkan_LIBRARY "${VULKAN_SDK_PATH}/Lib/vulkan-1.lib" CACHE FILEPATH "Vulkan library" FORCE)

    find_package(Vulkan REQUIRED)
    if(Vulkan_FOUND)
        message(STATUS "Vulkan library found: ${Vulkan_LIBRARY}")
    else()
        message(FATAL_ERROR "Vulkan not found! Install Vulkan SDK")
    endif()
endif()

if(AIONE_OPENCL)
    set(LLAMA_CLBLAST ON CACHE BOOL "Enable CLBlast for OpenCL" FORCE)
    set(SD_OPENCL ON CACHE BOOL "Enable Stable Diffusion OpenCL" FORCE)
    set(GGML_OPENCL ON CACHE BOOL "Enable GGML OpenCL backend" FORCE)
    message(STATUS "OpenCL support: ENABLED")
endif()

set(LLAMA_METAL OFF CACHE BOOL "" FORCE)
set(LLAMA_CUDA OFF CACHE BOOL "" FORCE)

# set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(LLAMA_TOOLS_INSTALL OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_COMMON ON CACHE BOOL "" FORCE)

set(LLAMA_HTTPLIB OFF CACHE BOOL "" FORCE)
set(LLAMA_OPENSSL OFF CACHE BOOL "" FORCE)
set(LLAMA_LLGUIDANCE OFF CACHE BOOL "" FORCE)

set(LLAMA_ALL_WARNINGS OFF CACHE BOOL "" FORCE)

set(SD_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
if(AIONE_SD_SHARED)
    set(SD_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
else()
    set(SD_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
endif()

# Add the submodules (build as DLLs)
if(AIONE_GGML_SHARED)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
else()
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
endif()
# Both subprojects bundle a ggml copy, but only one "ggml" target can exist.
# Build one of the GGMLs first based on option so it's shared; the libs then
# reuse it instead of adding their own. GGML_MAX_NAME must match the value SD's
# CMake forces, or the two subprojects disagree on struct layouts.
add_definitions(-DGGML_MAX_NAME=160)

if(AIONE_PRELOAD_GGML)
    # need this guard cuz add_subdirectory from AIOneMetaProject makes it import multiple times
    # which fucks up the macro and causes recursion, so only define macro once per run
    get_property(_add_library_guarded GLOBAL PROPERTY __ADD_LIBRARY_GUARDED)
    if(NOT _add_library_guarded)
        set_property(GLOBAL PROPERTY __ADD_LIBRARY_GUARDED TRUE)

        # "INSANE CMAKE HACK" as gemini puts it
        macro(add_library target)
            if("${target}" STREQUAL "ggml" AND "${ARGV1}" STREQUAL "ALIAS")
                message(STATUS "Intercepted and suppressed: add_library(ggml ALIAS ...) to prevent collision")
            else()
                _add_library(${ARGV})
            endif()
        endmacro()
    endif()

    set(LLAMA_USE_SYSTEM_GGML ON CACHE BOOL "" FORCE)
    set(SD_USE_SYSTEM_GGML ON CACHE BOOL "" FORCE)

    include(FetchContent)
    FetchContent_Declare(ggml SOURCE_DIR "${THIRD_PARTY_DIR}/ggml" OVERRIDE_FIND_PACKAGE)
    FetchContent_MakeAvailable(ggml)
endif()

if (AIONE_ENABLE_TTS)
    set(LLAMA_BUILD_MTMD ON CACHE BOOL "" FORCE)
    set(MTMD_VIDEO OFF CACHE BOOL "" FORCE) # don't need video support for mtmd
endif()

add_subdirectory(${LLAMA_CPP_DIR} ${CMAKE_BINARY_DIR}/llama.cpp_build)

# stable-diffusion.cpp is optional; when disabled SDModel compiles to a no-op stub.
if(AIONE_ENABLE_STABLE_DIFFUSION)
    add_subdirectory(${STABLE_DIFFUSION_CPP_DIR} ${CMAKE_BINARY_DIR}/stable-diffusion.cpp_build)
endif()
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(AIONE_UWP)
    if(TARGET llama)
        target_compile_definitions(llama PUBLIC LLAMA_UWP=1)
    endif()
    if(TARGET common)
        target_compile_definitions(common PUBLIC LLAMA_UWP=1)
    endif()
endif()

# if(AIONE_UWP)
#     # target_compile_definitions(llama PRIVATE WINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP)
#     # set(LLAMA_UWP 
#     target_compile_definitions(llama LLAMA_UWP)
# endif()

# Automatically find all source files in src directory
file(GLOB_RECURSE CORE_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# OpenAI/HTTPS support is opt-in; only the local provider is built by default.
if(NOT AIONE_ENABLE_OPENAI)
    list(FILTER CORE_SOURCES EXCLUDE REGEX "OpenAIProvider\\.cpp")
endif()

# Automatically find all header files in src directory
file(GLOB_RECURSE CORE_HEADERS 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

message(STATUS "Found ${CORE_SOURCES} source files in AIOneCore/src")
message(STATUS "Found ${CORE_HEADERS} header files in AIOneCore/src")

if(AIONE_BUILD_SHARED_LIBS)
    add_library(AIOneCore SHARED
        ${CORE_SOURCES}
    )
else()
    add_library(AIOneCore STATIC
        ${CORE_SOURCES}
        src/ModelSession.cpp
    )
endif()

# if(AIONE_UWP)
#     target_compile_definitions(llama PUBLIC LLAMA_UWP)
# endif()

add_library(AIOne::Core ALIAS AIOneCore)

if(AIONE_ENABLE_OPENAI)
    target_compile_definitions(AIOneCore PRIVATE AIONE_ENABLE_OPENAI)
endif()

if(AIONE_ENABLE_STABLE_DIFFUSION)
    target_compile_definitions(AIOneCore PUBLIC AIONE_ENABLE_STABLE_DIFFUSION)
endif()

if(AIONE_ENABLE_TTS)
    target_compile_definitions(AIOneCore PUBLIC AIONE_ENABLE_TTS)

    if (AIONE_ENABLE_TTS_INSTRUCT)
        target_compile_definitions(AIOneCore PUBLIC AIONE_ENABLE_TTS_INSTRUCT)
    endif()
endif()

if(AIONE_BUILD_SHARED_LIBS)
    # CMake defines AIOneCore_EXPORTS for SHARED automatically.
    target_compile_definitions(AIOneCore PUBLIC AIONE_DYNAMIC)
else()
endif()

if(AIONE_VERBOSE)
    target_compile_definitions(AIOneCore PRIVATE AIONE_VERBOSE)
endif()

# target_compile_definitions(AIOneCore PUBLIC LLAMA_UWP)

# Add header files to the library (for IDE support)
# target_sources(AIOneCore PUBLIC
#     FILE_SET HEADERS
#     BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
#     FILES ${CORE_HEADERS}
# )

if(AIONE_VULKAN)
    target_link_libraries(AIOneCore PRIVATE Vulkan::Vulkan)
endif()

if(WIN32 AND NOT EMSCRIPTEN)
    target_link_libraries(AIOneCore PRIVATE windowsapp)
endif()

if(AIONE_ENABLE_STABLE_DIFFUSION)
    target_link_libraries(AIOneCore PRIVATE stable-diffusion)
endif()

if(AIONE_ENABLE_TTS)
    target_link_libraries(AIOneCore PRIVATE mtmd)
endif()

target_link_libraries(AIOneCore PRIVATE
    llama
    llama-common
)

target_include_directories(AIOneCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${LLAMA_CPP_DIR}>
    $<BUILD_INTERFACE:${LLAMA_CPP_DIR}/include>
    $<BUILD_INTERFACE:${LLAMA_CPP_DIR}/ggml/include>
    $<BUILD_INTERFACE:${LLAMA_CPP_DIR}/src>
    $<BUILD_INTERFACE:${LLAMA_CPP_DIR}/vendor>
    $<BUILD_INTERFACE:${LLAMA_CPP_DIR}/common>
    $<INSTALL_INTERFACE:include/AIOneCore>
)

if(AIONE_ENABLE_STABLE_DIFFUSION)
    target_include_directories(AIOneCore PUBLIC
        $<BUILD_INTERFACE:${STABLE_DIFFUSION_CPP_DIR}>
    )
endif()

if(AIONE_ENABLE_TTS)
    target_include_directories(AIOneCore PUBLIC
        $<BUILD_INTERFACE:${LLAMA_CPP_DIR}/tools/mtmd>
    )
endif()

include(GNUInstallDirs)

install(TARGETS AIOneCore
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/AIOne.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
