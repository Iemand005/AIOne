cmake_minimum_required(VERSION 3.16)
project(AIOneCore VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(AIONE_OPENCL "Enable OpenCL support via CLBlast" OFF)
option(AIONE_VULKAN "Enable Vulkan support" OFF)
option(AIONE_MSVC_STATIC_RUNTIME "Use static MSVC runtime (/MT,/MTd) for AIOneCore" OFF)
option(AIONE_BUILD_SHARED_LIBS "Build AIOneCore as a shared library (DLL)" OFF)
option(AIONE_DEPS_SHARED "Build llama.cpp and stable-diffusion.cpp as shared libraries" OFF)

if(MSVC)
    # if(AIONE_MSVC_STATIC_RUNTIME)
    #     set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    # else()
    #     set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>")
    # endif()

    set(CMAKE_C_FLAGS_INIT "/FS")
    set(CMAKE_CXX_FLAGS_INIT "/FS")
    add_compile_options(/FS /bigobj /MP /W3 /wd4244 /wd4267)
    # add_compile_options(/GR- /EHs-) # no RTTI, no C++ exceptions
    # add_compile_options(/GR-) # no RTTI, no C++ exceptions
    # add_link_options(/MAP /MAPINFO:EXPORTS)
    string(APPEND CMAKE_C_FLAGS " /FS")
    string(APPEND CMAKE_CXX_FLAGS " /FS")
    set(CMAKE_JOB_POOLS compile=1 link=1)
    add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /FS" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /FS" CACHE STRING "" FORCE)
else()
    if(DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
        unset(CMAKE_MSVC_RUNTIME_LIBRARY CACHE)
        unset(CMAKE_MSVC_RUNTIME_LIBRARY)
    endif()
endif()

set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(LLAMA_CUBLAS OFF CACHE BOOL "" FORCE)
# set(LLAMA_VULKAN OFF CACHE BOOL "" FORCE)
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) # Default static in this subdir

set(SD_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SD_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(SD_CUDA OFF CACHE BOOL "" FORCE)
set(SD_HIPBLAS OFF CACHE BOOL "" FORCE)
set(SD_METAL OFF CACHE BOOL "" FORCE)
# set(SD_VULKAN OFF CACHE BOOL "" FORCE)
set(SD_OPENCL OFF CACHE BOOL "" FORCE)
set(SD_SYCL OFF CACHE BOOL "" FORCE)
set(SD_MUSA OFF CACHE BOOL "" FORCE)

set(GGML_CPU ON CACHE BOOL "" FORCE)
set(GGML_CUDA OFF CACHE BOOL "" FORCE)
set(GGML_HIP OFF CACHE BOOL "" FORCE)
set(GGML_METAL OFF CACHE BOOL "" FORCE)
# set(GGML_VULKAN OFF CACHE BOOL "" FORCE)
set(GGML_OPENCL OFF CACHE BOOL "" FORCE)
set(GGML_SYCL OFF CACHE BOOL "" FORCE)
set(GGML_WEBGPU OFF CACHE BOOL "" FORCE)
set(GGML_ZDNN OFF CACHE BOOL "" FORCE)
set(GGML_ZENDNN OFF CACHE BOOL "" FORCE)
set(GGML_RPC OFF CACHE BOOL "" FORCE)
set(GGML_VIRTGPU OFF CACHE BOOL "" FORCE)
set(GGML_MUSA OFF CACHE BOOL "" FORCE)

get_filename_component(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external" ABSOLUTE)
set(LLAMA_CPP_DIR "${THIRD_PARTY_DIR}/llama.cpp")
set(STABLE_DIFFUSION_CPP_DIR "${THIRD_PARTY_DIR}/stable-diffusion.cpp")

unset(SD_VULKAN CACHE)
unset(LLAMA_VULKAN CACHE)
unset(GGML_VULKAN CACHE)

if(AIONE_VULKAN)
    set(VULKAN_SDK_PATH "C:/VulkanSDK/1.4.335.0")
    set(CMAKE_PREFIX_PATH "${VULKAN_SDK_PATH};${CMAKE_PREFIX_PATH}" )
    set(Vulkan_INCLUDE_DIR "${VULKAN_SDK_PATH}/Include" CACHE PATH "Vulkan include dir" FORCE)
    set(Vulkan_LIBRARY "${VULKAN_SDK_PATH}/Lib/vulkan-1.lib" CACHE FILEPATH "Vulkan library" FORCE)

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

if(AIONE_VULKAN AND WIN32)
    # Try multiple possible Vulkan SDK locations
    if(DEFINED ENV{VULKAN_SDK})
        set(VULKAN_SDK_PATH "$ENV{VULKAN_SDK}")
        message(STATUS "Found Vulkan SDK from environment: ${VULKAN_SDK_PATH}")
    else()
        if(EXISTS "C:/VulkanSDK/1.4.335.0")
            set(VULKAN_SDK_PATH "C:/VulkanSDK/1.4.335.0")
        elseif(EXISTS "C:/VulkanSDK/1.4.250.1")
            set(VULKAN_SDK_PATH "C:/VulkanSDK/1.4.250.1")
        elseif(EXISTS "C:/VulkanSDK")
            set(VULKAN_SDK_PATH "C:/VulkanSDK")
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
set(LLAMA_BUILD_COMMON OFF CACHE INTERNAL "") 

set(LLAMA_HTTPLIB OFF CACHE BOOL "" FORCE)
set(LLAMA_OPENSSL OFF CACHE BOOL "" FORCE)
set(LLAMA_LLGUIDANCE OFF CACHE BOOL "" FORCE)

set(LLAMA_ALL_WARNINGS OFF CACHE BOOL "" FORCE)


if(AIONE_DEPS_SHARED)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(SD_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
else()
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(SD_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
endif()
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(${STABLE_DIFFUSION_CPP_DIR} ${CMAKE_BINARY_DIR}/stable-diffusion.cpp_build)
add_subdirectory(${LLAMA_CPP_DIR} ${CMAKE_BINARY_DIR}/llama.cpp_build)
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Automatically find all source files in src directory
file(GLOB_RECURSE CORE_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

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
    )
endif()

add_library(AIOne::Core ALIAS AIOneCore)

if(AIONE_BUILD_SHARED_LIBS)
    # CMake defines AIOneCore_EXPORTS for SHARED automatically.
else()
    target_compile_definitions(AIOneCore PUBLIC AIONE_STATIC)
endif()

# Add header files to the library (for IDE support)
# target_sources(AIOneCore PUBLIC
#     FILE_SET HEADERS
#     BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
#     FILES ${CORE_HEADERS}
# )

if(AIONE_VULKAN)
    target_link_libraries(AIOneCore PRIVATE Vulkan::Vulkan)
endif()

target_link_libraries(AIOneCore PRIVATE
    stable-diffusion
    llama
    common
)

target_include_directories(AIOneCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${LLAMA_CPP_DIR}>
    $<BUILD_INTERFACE:${LLAMA_CPP_DIR}/include>
    $<BUILD_INTERFACE:${LLAMA_CPP_DIR}/ggml/include>
    $<BUILD_INTERFACE:${LLAMA_CPP_DIR}/src>
    $<BUILD_INTERFACE:${LLAMA_CPP_DIR}/vendor>
    $<BUILD_INTERFACE:${STABLE_DIFFUSION_CPP_DIR}>
    $<INSTALL_INTERFACE:include/AIOneCore>
)
