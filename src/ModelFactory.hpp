#pragma once

#include <string>

#include <llama-cpp.h>

#include "LLModel.hpp"
#include "SDModel.hpp"

enum QuantizationLevels {
    Q8,
    Q4,
    Q3,
};



class ModelFactory {
    bool loadedBackends = false;
    bool initializedLlama = false;
public:
    void initLlama();
    void loadBackends();

    std::unique_ptr<LLModel> loadLLM(const std::string path) {
        initLlama();
        auto model = std::make_unique<LLModel>(path);
        return model;
    }

    std::unique_ptr<SDModel> loadSDM(const std::string path) {
        auto model = std::make_unique<SDModel>(path);
        return model;
    }

    sd_type_t getSDType(QuantizationLevels level) {
        switch (level) {
        case QuantizationLevels::Q4: return SD_TYPE_Q4_K;
        case QuantizationLevels::Q8: return SD_TYPE_Q8_K;
        case QuantizationLevels::Q3: return SD_TYPE_Q3_K;
        }
    }


    bool convertSDModel(std::string source, QuantizationLevels level, std::string destination, bool convertName = false) {
        std::string vaePath = "";

        std::string tensor_type_rules = "";

        bool success = convert(source.c_str(), vaePath.c_str(),destination.c_str(),getSDType(level),tensor_type_rules.c_str(),convertName);
        return success;
    }

    const char *systemInfoStr() ;

};
