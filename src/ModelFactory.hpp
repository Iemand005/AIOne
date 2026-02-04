#pragma once

#include <string>

#include <llama-cpp.h>

#include "LLModel.hpp"
#include "SDModel.hpp"





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




    bool convertSDModel(std::string source, QuantizationLevels level, std::string destination, bool convertName = false) {
        // std::string vaePath = "";

        // std::string tensor_type_rules = "";

        // bool success = convert(source.c_str(), vaePath.c_str(),destination.c_str(),getSDType(level),tensor_type_rules.c_str(),convertName);
        // return success;

        auto model = new SDModel(source);
        model->convertModel(destination, level, convertName);
    }

    const char *systemInfoStr() ;

};
