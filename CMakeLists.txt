cmake_minimum_required(VERSION 3.19)  # llama.cpp requires 3.13+
project(MyLlamaProject)

# Set your preferred C++ standard (llama.cpp uses C++11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CRITICAL: Disable llama.cpp's tests and examples (optional but recommended)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Set any llama.cpp options BEFORE adding subdirectory
set(LLAMA_CUBLAS OFF CACHE BOOL "" FORCE)      # Disable CUDA by default
set(LLAMA_VULKAN OFF CACHE BOOL "" FORCE)      # Disable Vulkan by default
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) # Static library

# Now add the submodule
add_subdirectory(llama.cpp)

# Your executable
add_executable(app
    main.cpp
    # ... other source files
)

# Link against llama.cpp's library
target_link_libraries(app PRIVATE llama)

# Include llama.cpp's headers
target_include_directories(app PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/llama.cpp
)

# Optional: Add definitions to match llama.cpp's config
target_compile_definitions(app PRIVATE
    $<$<BOOL:${LLAMA_CUBLAS}>:GGML_USE_CUBLAS>
    $<$<BOOL:${LLAMA_VULKAN}>:GGML_USE_VULKAN>
)