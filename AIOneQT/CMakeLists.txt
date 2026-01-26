cmake_minimum_required(VERSION 3.16)

project(AIOneQT VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # Fix for large object files in stable-diffusion.cpp
    add_compile_options(/bigobj)
    
    add_compile_options(/MP)  # Enable parallel builds
    add_definitions(-DNOMINMAX)  # Disable min/max macros
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)  # Disable secure CRT warnings
endif()

get_filename_component(LLAMA_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/llama.cpp" ABSOLUTE)
get_filename_component(STABLE_DIFFUSION_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/stable-diffusion.cpp" ABSOLUTE)

enable_language(C)

# set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Disable tests" FORCE)
# set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static lib" FORCE)

# set(GGML_NATIVE OFF CACHE BOOL "Disable platform-specific optimizations" FORCE)
# set(LLAMA_NATIVE OFF CACHE BOOL "Disable native CPU optimizations" FORCE)


set(LLAMA_VULKAN ON CACHE BOOL "Enable Vulkan" FORCE)
set(LLAMA_VULKAN ON CACHE BOOL "Enable Vulkan support" FORCE)
set(LLAMA_GGML_VULKAN ON CACHE BOOL "" FORCE)


set(LLAMA_METAL OFF CACHE BOOL "" FORCE)
set(LLAMA_CUDA OFF CACHE BOOL "" FORCE)
set(LLAMA_CLBLAST OFF CACHE BOOL "" FORCE) # oPENcl

# Other llama.cpp options
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(${LLAMA_CPP_DIR} ${CMAKE_BINARY_DIR}/llama.cpp_build)
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

target_link_libraries(appAIOneQT
    PRIVATE Qt6::Quick
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
