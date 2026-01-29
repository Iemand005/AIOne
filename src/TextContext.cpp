#pragma once

#include <string>
#include <iostream>

#include "TextContext.hpp"
#include "LLModel.hpp"

TextContext::TextContext(LLModel *modelWrapper, std::shared_ptr<llama_context> context) : modelWrapper(modelWrapper) {
    seqId = modelWrapper->claimSeqId();
    this->context = context;
}

bool TextContext::isValid() {
    return modelWrapper->isValid(context.get());
}

bool TextContext::isConnectedTo(LLModel *model) {
    return modelWrapper == model;
}

void TextContext::destroy() {
    if (context != nullptr) {
        llama_free(context.get());
        modelWrapper->releaseSeqId(seqId);

        context = nullptr;
    }
}
