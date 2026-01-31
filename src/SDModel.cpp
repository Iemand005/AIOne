#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "SDModel.hpp"

bool SDModel::saveImageAsPNG(const sd_image_t& image, const std::string& filename) {
        if (!image.data || image.width == 0 || image.height == 0) {
                std::cerr << "Invalid image data" << std::endl;
            return false;
        }

        int success = stbi_write_png(filename.c_str(),
                                    image.width,
                                    image.height,
                                    image.channel,  // RGB
                                    image.data,
                                    image.width * image.channel);

        if (success) {
            std::cout << "Saved image to: " << filename << std::endl;
            return true;
        } else {
            std::cerr << "Failed to save image: " << filename << std::endl;
            return false;
        }
    }