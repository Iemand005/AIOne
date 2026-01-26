# Device Selection Guide for llama.cpp

## Overview
llama.cpp has a generic, high-level device selection API through the **GGML backend system**. The device selection happens at multiple levels:
1. **Device enumeration** - discovering available devices
2. **Backend registry** - managing different compute backends (CPU, CUDA, Vulkan, etc.)
3. **Model parameters** - specifying which devices to use when loading models

---

## Key Files

### Public API Header
- **[include/ggml-backend.h](include/ggml-backend.h)** - Core device and backend management API
- **[thirdparty/llama.cpp/include/llama.h](thirdparty/llama.cpp/include/llama.h)** - High-level llama.cpp API with model parameters

### Backend Implementation
- **[thirdparty/llama.cpp/ggml/src/ggml-vulkan/ggml-vulkan.cpp](thirdparty/llama.cpp/ggml/src/ggml-vulkan/ggml-vulkan.cpp)** - Vulkan backend (the file you're looking at)
- Other backends: CPU, CUDA, OpenCL, Metal, etc. in similar locations

---

## Device Selection API

### 1. Device Enumeration

```c
// Count available devices
size_t ggml_backend_dev_count(void);

// Get device by index
ggml_backend_dev_t ggml_backend_dev_get(size_t index);

// Get device by name
ggml_backend_dev_t ggml_backend_dev_by_name(const char * name);

// Get device by type
ggml_backend_dev_t ggml_backend_dev_by_type(enum ggml_backend_dev_type type);
```

### 2. Device Types

```c
enum ggml_backend_dev_type {
    GGML_BACKEND_DEVICE_TYPE_CPU,    // CPU device using system memory
    GGML_BACKEND_DEVICE_TYPE_GPU,    // GPU device using dedicated memory
    GGML_BACKEND_DEVICE_TYPE_IGPU,   // Integrated GPU using host memory
    GGML_BACKEND_DEVICE_TYPE_ACCEL   // Accelerators (BLAS, AMX, etc.)
};
```

### 3. Device Properties

```c
struct ggml_backend_dev_props {
    const char * name;              // Device name
    const char * description;       // Device description
    size_t memory_free;             // Free memory in bytes
    size_t memory_total;            // Total memory in bytes
    enum ggml_backend_dev_type type;
    const char * device_id;         // PCI ID (domain:bus:device.function)
    struct ggml_backend_dev_caps caps;  // Capabilities
};

// Get device properties
void ggml_backend_dev_get_props(ggml_backend_dev_t device, struct ggml_backend_dev_props * props);

// Quick access functions
const char * ggml_backend_dev_name(ggml_backend_dev_t device);
const char * ggml_backend_dev_description(ggml_backend_dev_t device);
void ggml_backend_dev_memory(ggml_backend_dev_t device, size_t * free, size_t * total);
enum ggml_backend_dev_type ggml_backend_dev_type(ggml_backend_dev_t device);
```

### 4. Backend Initialization

```c
// Initialize backend from device
ggml_backend_t ggml_backend_dev_init(ggml_backend_dev_t device, const char * params);

// Convenience functions
ggml_backend_t ggml_backend_init_by_name(const char * name, const char * params);
ggml_backend_t ggml_backend_init_by_type(enum ggml_backend_dev_type type, const char * params);
ggml_backend_t ggml_backend_init_best(void);  // Automatically picks GPU or CPU
```

### 5. Backend Registry (Multiple backends/devices)

```c
// Count available backends/registrations
size_t ggml_backend_reg_count(void);

// Get backend registration by index
ggml_backend_reg_t ggml_backend_reg_get(size_t index);

// Get devices from a backend registration
size_t ggml_backend_reg_dev_count(ggml_backend_reg_t reg);
ggml_backend_dev_t ggml_backend_reg_dev_get(ggml_backend_reg_t reg, size_t index);
```

---

## Using Devices with llama.cpp Models

### Model Loading Parameters

```c
struct llama_model_params {
    // NULL-terminated list of devices to use for offloading
    // If NULL, all available devices are used
    ggml_backend_dev_t * devices;

    // Number of GPU layers to offload
    int32_t n_gpu_layers;

    // Split mode for multi-GPU
    enum llama_split_mode split_mode;

    // Main GPU when split_mode is NONE
    int32_t main_gpu;

    // Proportion of model to offload to each GPU
    const float * tensor_split;

    // Other parameters...
};
```

### Split Modes

```c
enum llama_split_mode {
    LLAMA_SPLIT_MODE_NONE  = 0,  // Single GPU
    LLAMA_SPLIT_MODE_LAYER = 1,  // Split layers across GPUs
    LLAMA_SPLIT_MODE_ROW   = 2,  // Split layers/KV + tensor parallelism
};
```

---

## Example: Selecting and Using Devices

```c
// 1. Initialize backend system
llama_backend_init();

// 2. Enumerate available devices
printf("Available devices:\n");
for (size_t i = 0; i < ggml_backend_dev_count(); i++) {
    ggml_backend_dev_t dev = ggml_backend_dev_get(i);
    struct ggml_backend_dev_props props = {0};
    ggml_backend_dev_get_props(dev, &props);
    
    printf("Device %zu: %s (%s)\n", i, props.name, props.description);
    printf("  Type: %d, Memory: %.1f GB / %.1f GB\n",
        props.type,
        props.memory_free / (1024.0 * 1024.0 * 1024.0),
        props.memory_total / (1024.0 * 1024.0 * 1024.0));
}

// 3. Select specific devices or use auto-detection
ggml_backend_dev_t gpu_device = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_GPU);
if (!gpu_device) {
    gpu_device = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_IGPU);
}
if (!gpu_device) {
    gpu_device = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_CPU);
}

// 4. Create model with device selection
struct llama_model_params mparams = llama_model_default_params();
ggml_backend_dev_t devices[] = {gpu_device, NULL};
mparams.devices = devices;
mparams.n_gpu_layers = -1;  // Offload all layers

struct llama_model * model = llama_model_load_from_file("model.gguf", mparams);

// 5. Cleanup
llama_model_free(model);
llama_backend_free();
```

---

## Advanced: Multi-GPU Setup

```c
// Collect multiple devices
ggml_backend_dev_t devices[3];
devices[0] = ggml_backend_dev_by_name("NVIDIA RTX 4090");
devices[1] = ggml_backend_dev_by_name("NVIDIA RTX 3090");
devices[2] = NULL;  // NULL-terminated

// Set tensor split proportions (must match number of devices)
float tensor_split[] = {0.6, 0.4};  // 60% on first GPU, 40% on second

struct llama_model_params mparams = llama_model_default_params();
mparams.devices = devices;
mparams.split_mode = LLAMA_SPLIT_MODE_LAYER;
mparams.tensor_split = tensor_split;
mparams.n_gpu_layers = -1;

struct llama_model * model = llama_model_load_from_file("model.gguf", mparams);
```

---

## Backend Registry Information

The backend registry allows you to list all registered backends and their supported devices:

```c
printf("Available backends:\n");
for (size_t i = 0; i < ggml_backend_reg_count(); i++) {
    ggml_backend_reg_t reg = ggml_backend_reg_get(i);
    printf("Backend: %s\n", ggml_backend_reg_name(reg));
    printf("  Devices: %zu\n", ggml_backend_reg_dev_count(reg));
    
    for (size_t j = 0; j < ggml_backend_reg_dev_count(reg); j++) {
        ggml_backend_dev_t dev = ggml_backend_reg_dev_get(reg, j);
        printf("    - %s\n", ggml_backend_dev_name(dev));
    }
}
```

---

## Loading Backends Dynamically

```c
// Load a specific backend
ggml_backend_reg_t vulkan_backend = ggml_backend_load("vulkan");

// Load all available backends from standard directories
ggml_backend_load_all();

// Load from custom directory
ggml_backend_load_all_from_path("/path/to/backends");

// Unload a backend
ggml_backend_unload(vulkan_backend);
```

---

## Summary

The device selection API follows this hierarchy:
1. **Device Level** - Low-level device enumeration and properties
2. **Backend Level** - Backend implementations that wrap devices
3. **Model Parameters** - High-level llama.cpp API for specifying devices

For most use cases with llama.cpp:
- Enumerate devices with `ggml_backend_dev_get()` and `ggml_backend_dev_by_type()`
- Pass selected devices via `llama_model_params.devices`
- Specify GPU layer count with `n_gpu_layers`
- Use split modes and tensor_split for multi-GPU scenarios
