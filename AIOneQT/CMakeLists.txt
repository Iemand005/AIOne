cmake_minimum_required(VERSION 3.16)

if(MSVC)
    set(CMAKE_C_FLAGS_INIT "/FS")
    set(CMAKE_CXX_FLAGS_INIT "/FS")
endif()

project(AIOne VERSION 0.1 LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(AIONE_OPENCL "Enable OpenCL support via CLBlast" OFF)
option(AIONE_VULKAN "Enable Vulkan support" OFF) # Here's da vulkaannn

set(QT_VERSION, "6.10.1")

set(Qt6_DIR "/Qt/6.10.1/msvc2022_64/lib/cmake/Qt6")
set(CMAKE_PREFIX_PATH "C:/Qt/${QT_VERSION}/msvc2022_64" ${CMAKE_PREFIX_PATH})

if(MSVC)
    add_compile_options(/FS /bigobj /MP /W3 /wd4244 /wd4267)
    string(APPEND CMAKE_C_FLAGS " /FS")
    string(APPEND CMAKE_CXX_FLAGS " /FS")
    set(CMAKE_JOB_POOLS compile=1 link=1)
    add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug" CACHE STRING "" FORCE)
endif()

get_filename_component(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty" ABSOLUTE)
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
    # set(SD_VK_DEVICE ON CACHE BOOL "" FORCE)
    message(STATUS "Vulkan support: ENABLED")
else()
    set(LLAMA_VULKAN OFF CACHE BOOL "" FORCE)
    set(GGML_VULKAN OFF CACHE BOOL "" FORCE)
    set(SD_VULKAN OFF CACHE BOOL "" FORCE)
    message(STATUS "Vulkan support: DISABLED")
endif()

if(AIONE_OPENCL)
    set(LLAMA_CLBLAST ON CACHE BOOL "Enable CLBlast for OpenCL" FORCE)
    set(SD_OPENCL ON CACHE BOOL "Enable Stable Diffusion OpenCL" FORCE)
    set(GGML_OPENCL ON CACHE BOOL "Enable GGML OpenCL backend" FORCE)
    message(STATUS "OpenCL support: ENABLED")
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

set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(LLAMA_METAL OFF CACHE BOOL "" FORCE)
set(LLAMA_CUDA OFF CACHE BOOL "" FORCE)

if(AIONE_VULKAN)
    set(GGML_VULKAN_VALIDATE OFF CACHE BOOL "" FORCE)
    set(GGML_VULKAN_CHECK_RESULTS OFF CACHE BOOL "" FORCE)
    set(GGML_VULKAN_RUN_TESTS OFF CACHE BOOL "" FORCE)
    set(GGML_VULKAN_SHADER_DEBUG_INFO OFF CACHE BOOL "" FORCE)
endif()

if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /FS" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /FS" CACHE STRING "" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(QT_QML_DEBUG)
    add_compile_definitions(QT_DECLARATIVE_DEBUG)
endif()

add_subdirectory(${LLAMA_CPP_DIR} ${CMAKE_BINARY_DIR}/llama.cpp_build)

add_subdirectory(${STABLE_DIFFUSION_CPP_DIR} ${CMAKE_BINARY_DIR}/stable-diffusion.cpp_build)

if(AIONE_VULKAN)

    find_package(Vulkan REQUIRED)
    if(Vulkan_FOUND)
        message(STATUS "Vulkan library found: ${Vulkan_LIBRARY}")
    else()
        message(FATAL_ERROR "Vulkan not found! Install Vulkan SDK")
    endif()
endif()

# Qt setup
find_package(Qt6 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(AIOne
    main.cpp
)

qt_add_qml_module(AIOne
    URI AIOne
    QML_FILES
        Main.qml
        SOURCES UIHandler.h UIHandler.cpp
        QML_FILES AIOButton.ui.qml
)

if(AIONE_VULKAN)
    target_link_libraries(AIOne PRIVATE Vulkan::Vulkan)
endif()

target_link_libraries(AIOne
    PRIVATE Qt6::Quick
    PRIVATE llama
    PRIVATE stable-diffusion
)

target_include_directories(AIOne PRIVATE
    ${LLAMA_CPP_DIR}
    ${STABLE_DIFFUSION_CPP_DIR}
)

set_target_properties(AIOne PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    DEBUG_POSTFIX "d"
)
