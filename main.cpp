
#include "./include/llama-cpp.h"

#include <string>

int main() {

  std::string model_path = "C:\\Users\\Lasse\\Documents\\AI\\models\\gguf\\Qwen\\Qwen2.5-0.5B-Instruct-GGUF\\qwen2.5-0.5b-instruct-q8_0.gguf";

  llama_model_params model_params = llama_model_default_params();
  llama_model *model = llama_model_load_from_file(model_path.c_str(), model_params);


}