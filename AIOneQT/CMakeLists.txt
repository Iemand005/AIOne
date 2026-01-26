cmake_minimum_required(VERSION 3.16)

project(AIOneQT VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(AIONE_OPENCL "Enable OpenCL support via CLBlast" OFF)
option(AIONE_VULKAN "Enable Vulkan support" ON)

if(MSVC)
    # set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
    #     set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb")
    # set(CMAKE_MSVC_PARALLEL 4)
    # add_compile_options(/FS)
    add_compile_options(/FS)
    string(APPEND CMAKE_C_FLAGS " /FS")
    string(APPEND CMAKE_CXX_FLAGS " /FS")
    set(CMAKE_JOB_POOLS compile=2 link=1)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS")
        add_compile_options(/wd4244)
        add_compile_options(/wd4267)
    # # Fix for large object files in stable-diffusion.cpp
    add_compile_options(/bigobj /MP /W3)
    
    # add_compile_options(/MP)  # Enable parallel builds
    add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
endif()

get_filename_component(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty" ABSOLUTE)

set(LLAMA_CPP_DIR "${THIRD_PARTY_DIR}/llama.cpp")
set(STABLE_DIFFUSION_CPP_DIR "${THIRD_PARTY_DIR}/stable-diffusion.cpp")

# SET(GGML_MAX_NAME 128 CACHE /)
# set(GGML_DIR "${THIRD_PARTY_DIR}/ggml")
set(SD_USE_SYSTEM_GGML OFF CACHE BOOL "" FORCE)


enable_language(C)

# set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Disable tests" FORCE)
# set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static lib" FORCE)

# set(GGML_NATIVE OFF CACHE BOOL "Disable platform-specific optimizations" FORCE)
# set(LLAMA_NATIVE OFF CACHE BOOL "Disable native CPU optimizations" FORCE)

set(SD_VULKAN OFF CACHE BOOL "" FORCE)
# set(LLAMA_VULKAN ON CACHE BOOL "Enable Vulkan" FORCE)
# set(LLAMA_VULKAN ON CACHE BOOL "Enable Vulkan support" FORCE)
# set(LLAMA_GGML_VULKAN ON CACHE BOOL "" FORCE)
# set(SD_VULKAN ON CACHE BOOL "" FORCE)
# set(LLAMA_GGML_VULKAN ON CACHE BOOL "" FORCE)
set(GGML_VULKAN OFF CACHE BOOL "" FORCE)

if (AIONE_VULKAN)
    set(SD_VULKAN ON CACHE BOOL "" FORCE)
    # set(GGML_VULKAN ON CACHE BOOL "" FORCE)
else()
    set(SD_VULKAN OFF CACHE BOOL "" FORCE)
    set(GGML_VULKAN OFF CACHE BOOL "" FORCE)
endif()

set(SD_BUILD_SHARED_GGML_LIB ON CACHE BOOL "" FORCE)

if(AIONE_OPENCL)
    set(LLAMA_CLBLAST ON CACHE BOOL "Enable CLBlast for OpenCL" FORCE)
    set(SD_OPENCL ON CACHE BOOL "Enable Stable Diffusion OpenCL" FORCE)
    set(GGML_OPENCL ON CACHE BOOL "Enable GGML OpenCL backend" FORCE)
else()
    set(LLAMA_CLBLAST OFF CACHE BOOL "Disable CLBlast" FORCE)
    set(SD_OPENCL OFF CACHE BOOL "Disable Stable Diffusion OpenCL" FORCE)
    set(GGML_OPENCL OFF CACHE BOOL "Disable GGML OpenCL" FORCE)
endif()

if(WIN32)
    set(OPENCL_SDK_DIR "${THIRD_PARTY_DIR}/OpenCL-SDK")
    # Common Vulkan SDK locations on Windows
    set(VULKAN_SDK_PATH "$ENV{VULKAN_SDK}")
    if(NOT VULKAN_SDK_PATH)
        set(VULKAN_SDK_PATH "C:/VulkanSDK/1.4.321")  # Adjust version
    endif()

    if(EXISTS "${VULKAN_SDK_PATH}")
        message(STATUS "Using Vulkan SDK at: ${VULKAN_SDK_PATH}")
        set(Vulkan_INCLUDE_DIR "${VULKAN_SDK_PATH}/Include")
        set(Vulkan_LIBRARY "${VULKAN_SDK_PATH}/Lib/vulkan-1.lib")
        set(glslc_EXECUTABLE "${VULKAN_SDK_PATH}/Bin/glslc.exe")
    endif()


endif()

if (AIONE_OPENCL)
    if(NOT TCLAP_INCLUDE_PATH)
        set(TCLAP_INCLUDE_PATH "../thirdparty/tclap/include")
    endif()

    if(NOT Stb_INCLUDE_PATH)
        set(Stb_INCLUDE_PATH "../thirdparty/stb")
    endif()

    add_subdirectory(${OPENCL_SDK_DIR} ${CMAKE_BINARY_DIR}/OpenCL-SDK_build)

    set(OpenCL_TARGET OpenCL)

    set(OpenCL_ROOT ${CMAKE_BINARY_DIR}/OpenCL-SDK_build)
    set(OpenCL_INCLUDE_DIR "${OpenCL_ROOT}/include")
    set(OpenCL_LIBRARY "${OpenCL_ROOT}/lib/OpenCL.lib")

    # add_dependencies(llama ${OpenCL_TARGET})
endif()

set(LLAMA_METAL OFF CACHE BOOL "" FORCE)
set(LLAMA_CUDA OFF CACHE BOOL "" FORCE)
# set(LLAMA_CLBLAST OFF CACHE BOOL "" FORCE) # oPENcl

# Other llama.cpp options
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# add_subdirectory(${GGML_DIR} ${CMAKE_BINARY_DIR}/ggml_build)

set(GGML_VULKAN OFF CACHE BOOL "" FORCE)
add_subdirectory(${LLAMA_CPP_DIR} ${CMAKE_BINARY_DIR}/llama.cpp_build)

if(AIONE_VULKAN)
# set(SD_VULKAN ON CACHE BOOL "" FORCE)
else()
    set(SD_VULKAN OFF CACHE BOOL "" FORCE)
endif()
# set(GGML_VULKAN ON CACHE BOOL "" FORCE)

add_subdirectory(${STABLE_DIFFUSION_CPP_DIR} ${CMAKE_BINARY_DIR}/stable-diffusion.cpp_build)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appAIOneQT
    main.cpp
)

qt_add_qml_module(appAIOneQT
    URI AIOneQT
    QML_FILES
        Main.qml
        SOURCES inputhandler.h inputhandler.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appAIOneQT PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appAIOneQT
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE

    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

find_package(Vulkan REQUIRED)
target_link_libraries(appAIOneQT PRIVATE Vulkan::Vulkan)

target_link_libraries(appAIOneQT
    PRIVATE Qt6::Quick
    # PRIVATE ggml
    PRIVATE llama
    PRIVATE stable-diffusion
)

target_include_directories(appAIOneQT PRIVATE
    ${LLAMA_CPP_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/llama.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/stable-diffusion.cpp
)

include(GNUInstallDirs)
install(TARGETS appAIOneQT
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
