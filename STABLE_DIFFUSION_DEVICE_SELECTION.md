# Device Selection Guide for stable-diffusion.cpp

## Overview
**stable-diffusion.cpp has a more LIMITED device selection API compared to llama.cpp.** Device selection is primarily done through:
1. **Compile-time flags** - Which backends are enabled at build time (SD_USE_CUDA, SD_USE_VULKAN, etc.)
2. **Environment variables** - Runtime device selection for backends that support it
3. **Model parameters** - Limited device configuration options via `sd_ctx_params_t`

---

## Key Files

### Public API Header
- **[thirdparty/stable-diffusion.cpp/stable-diffusion.h](thirdparty/stable-diffusion.cpp/stable-diffusion.h)** - Main API with parameters

### Backend Implementation
- **[thirdparty/stable-diffusion.cpp/stable-diffusion.cpp](thirdparty/stable-diffusion.cpp/stable-diffusion.cpp)** - Backend initialization logic (lines 160-220)
- **[thirdparty/stable-diffusion.cpp/upscaler.cpp](thirdparty/stable-diffusion.cpp/upscaler.cpp)** - Similar backend initialization for upscaler

---

## Device Selection Mechanism

### 1. Compile-Time Backend Selection

The library selects backends based on compile-time flags in this priority order:

```c
#ifdef SD_USE_CUDA
    backend = ggml_backend_cuda_init(0);
#endif

#ifdef SD_USE_METAL
    backend = ggml_backend_metal_init();
#endif

#ifdef SD_USE_VULKAN
    backend = ggml_backend_vk_init(device);  // Can select device via env var
#endif

#ifdef SD_USE_OPENCL
    backend = ggml_backend_opencl_init();
#endif

#ifdef SD_USE_SYCL
    backend = ggml_backend_sycl_init(0);
#endif

// Fallback to CPU if no GPU backends are available or enabled
if (!backend) {
    backend = ggml_backend_cpu_init();
}
```

**Supported backends:**
- CUDA (NVIDIA GPUs)
- Metal (Apple GPUs)
- Vulkan (Cross-platform)
- OpenCL (AMD, Intel, etc.)
- SYCL (Intel)
- CPU (Fallback)

### 2. Runtime Device Selection (Vulkan only)

For Vulkan, you can select which device to use via environment variable:

```bash
# Select Vulkan device 0 (default)
SD_VK_DEVICE=0

# Select Vulkan device 1
SD_VK_DEVICE=1

# Example usage
export SD_VK_DEVICE=0
./my_stable_diffusion_app
```

**Implementation details:**
```c
const char* SD_VK_DEVICE = getenv("SD_VK_DEVICE");
if (SD_VK_DEVICE != nullptr) {
    device = std::stoull(sd_vk_device_str);  // Convert string to int
}
const int device_count = ggml_backend_vk_get_device_count();
if (device >= device_count) {
    // Falls back to device 0 if invalid device specified
    device = 0;
}
backend = ggml_backend_vk_init(device);
```

---

## Model Parameters for Device Control

### sd_ctx_params_t

```c
typedef struct {
    // ... model paths ...
    
    int n_threads;                  // Number of CPU threads
    
    // Memory/performance
    bool offload_params_to_cpu;     // Keep model params on CPU (saves VRAM)
    bool enable_mmap;               // Use memory mapping for model loading
    bool keep_clip_on_cpu;          // Keep CLIP encoder on CPU
    bool keep_control_net_on_cpu;   // Keep control net on CPU
    bool keep_vae_on_cpu;           // Keep VAE on CPU
    
    // Optional offloading flags
    bool free_params_immediately;   // Free params after loading
    
    // Other parameters...
} sd_ctx_params_t;
```

**Important:** Unlike llama.cpp, stable-diffusion.cpp does NOT provide:
- ❌ Direct device selection in API
- ❌ Multi-GPU support
- ❌ Tensor splitting across devices
- ❌ Backend selection at runtime

---

## How to Use Device Selection

### 1. Build-time Configuration

When building stable-diffusion.cpp, enable only the backends you want:

```bash
# CUDA only
cmake -DBUILD_SHARED_LIBS=ON -DSD_USE_CUDA=ON ..

# Vulkan only
cmake -DBUILD_SHARED_LIBS=ON -DSD_USE_VULKAN=ON ..

# Metal only (macOS)
cmake -DBUILD_SHARED_LIBS=ON -DSD_USE_METAL=ON ..

# CPU only
cmake -DBUILD_SHARED_LIBS=ON ..  # (default, all GPU backends disabled)

# Multiple backends (it will try them in order)
cmake -DBUILD_SHARED_LIBS=ON -DSD_USE_CUDA=ON -DSD_USE_VULKAN=ON -DSD_USE_OPENCL=ON ..
```

### 2. Basic Usage Example

```c
#include "stable-diffusion.h"

int main() {
    // Initialize parameters with defaults
    sd_ctx_params_t params = {0};
    sd_ctx_params_init(&params);
    
    // Set model paths
    params.model_path = "model.safetensors";
    params.vae_path = "vae.safetensors";
    
    // Optional: Configure device offloading
    params.offload_params_to_cpu = false;  // Keep params in VRAM
    params.keep_clip_on_cpu = false;       // Use GPU for CLIP
    params.keep_vae_on_cpu = false;        // Use GPU for VAE
    
    // Create context (will use automatically selected backend)
    sd_ctx_t* ctx = new_sd_ctx(&params);
    if (!ctx) {
        printf("Failed to create context\n");
        return 1;
    }
    
    // ... use context ...
    
    free_sd_ctx(ctx);
    return 0;
}
```

### 3. Vulkan Device Selection Example

```c
#include <stdlib.h>
#include "stable-diffusion.h"

int main() {
    // Select Vulkan device 1 (if available)
    setenv("SD_VK_DEVICE", "1", 1);
    
    sd_ctx_params_t params = {0};
    sd_ctx_params_init(&params);
    params.model_path = "model.safetensors";
    
    sd_ctx_t* ctx = new_sd_ctx(&params);
    if (!ctx) {
        fprintf(stderr, "Failed to create context with Vulkan device 1\n");
        return 1;
    }
    
    // ... use context ...
    
    free_sd_ctx(ctx);
    return 0;
}
```

### 4. Memory Optimization (VRAM Conservation)

```c
sd_ctx_params_t params = {0};
sd_ctx_params_init(&params);

// Use these flags to keep models on CPU and move them to GPU as needed
params.offload_params_to_cpu = true;      // Save VRAM
params.keep_clip_on_cpu = true;            // CLIP stays on CPU
params.keep_control_net_on_cpu = true;     // ControlNet stays on CPU
params.keep_vae_on_cpu = true;             // VAE stays on CPU

// The main diffusion model and text encoder will still use GPU if available
```

---

## Checking Available Devices (Vulkan)

Unfortunately, stable-diffusion.cpp doesn't expose device enumeration in its public API. However, you can check device count via environment:

```c
// For Vulkan, you can enumerate devices by trying different device IDs
// until ggml_backend_vk_init(device) returns NULL
const int max_devices = 8;  // Arbitrary max
for (int i = 0; i < max_devices; i++) {
    // You would need to directly use ggml-vulkan functions
    // which are not exposed through stable-diffusion.h public API
}
```

For more detailed device information, you would need to use the lower-level GGML backend API directly (see the llama.cpp guide).

---

## Comparison: stable-diffusion.cpp vs llama.cpp

| Feature | stable-diffusion.cpp | llama.cpp |
|---------|----------------------|-----------|
| Runtime device selection | ❌ Limited (Vulkan only via env var) | ✅ Full API support |
| Device enumeration | ❌ Not exposed | ✅ `ggml_backend_dev_get()` |
| Multi-GPU support | ❌ No | ✅ Yes (LLAMA_SPLIT_MODE_LAYER) |
| Tensor splitting | ❌ No | ✅ Yes |
| Device in params | ❌ No | ✅ Yes (`llama_model_params.devices`) |
| Backend selection | Compile-time + env var | Runtime API |
| GPU layer offloading | Via `keep_*_on_cpu` flags | Via `n_gpu_layers` |

---

## Advanced: Using GGML Backend API Directly

If you need more control, you can use the underlying GGML backend API (same as llama.cpp):

```c
#include "ggml-backend.h"

// Enumerate devices
for (size_t i = 0; i < ggml_backend_dev_count(); i++) {
    ggml_backend_dev_t dev = ggml_backend_dev_get(i);
    struct ggml_backend_dev_props props = {0};
    ggml_backend_dev_get_props(dev, &props);
    printf("Device %zu: %s\n", i, props.name);
}

// However, stable-diffusion.cpp doesn't expose a way to pass 
// selected devices to new_sd_ctx(), so this is mainly for inspection
```

---

## Build Configuration Location

The cmake configuration that enables/disables backends is typically in:

- **[thirdparty/stable-diffusion.cpp/CMakeLists.txt](thirdparty/stable-diffusion.cpp/CMakeLists.txt)** - Check for `SD_USE_*` options

Search for lines like:
```cmake
option(SD_USE_CUDA "Build with CUDA support" OFF)
option(SD_USE_VULKAN "Build with Vulkan support" OFF)
option(SD_USE_METAL "Build with Metal support" OFF)
```

---

## Summary

**stable-diffusion.cpp device selection:**
1. **Primary method**: Compile-time backend flags (SD_USE_CUDA, SD_USE_VULKAN, etc.)
2. **Secondary method**: Environment variable for Vulkan device selection (`SD_VK_DEVICE`)
3. **Parameter control**: `sd_ctx_params_t` flags to keep parts of model on CPU

**Comparison to llama.cpp**: Much less flexible at runtime. If you need sophisticated device management, you may need to either:
- Build multiple versions with different backends
- Use environment variables for Vulkan
- Extend the API to accept device parameters (requires code modification)
- Use the low-level GGML backend API directly
