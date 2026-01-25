#include <iostream>
#include "llama.h"

int main(int argc, char* argv[]) {
  llama_backend_init();

  std::cout << "Llama.cpp System Info: " << llama_print_system_info() << std::endl;

  llama_backend_free();

  return 0;
}