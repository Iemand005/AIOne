#include "Model.hpp"

#include <ggml-backend.h>
#include <ggml.h>

#include <iostream>

// Model::Model() {
//   devices = std::vector<ggml_backend_dev_t>(1);
// }

ggml_backend_device* Model::getBackend() { return devices[0]; }

void Model::selectDevice(PreferredDevice preferred) {
  devices[1] = nullptr;
  // choose devices based on preferred in options
  // (leaks into next case if preferred device isn't available)
  switch (preferred) {
    case PreferredDevice::ANY:
    case PreferredDevice::DGPU:
      devices[0] = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_GPU);
      if (devices[0]) break;
    case PreferredDevice::IGPU:
      devices[0] = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_IGPU);
      if (devices[0]) break;
    case PreferredDevice::ACCELERATOR:
      devices[0] = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_ACCEL);
      if (devices[0]) break;
    case PreferredDevice::CPU:
      devices[0] = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_CPU);
      if (devices[0]) break;
    default:
      throw std::runtime_error("No compatible device found");
  }

  std::cout << "==============" << std::endl;
  std::cout << "Using device for inference: " << ggml_backend_dev_name(devices[0]) << std::endl;
  std::cout << "==============" << std::endl;
}
