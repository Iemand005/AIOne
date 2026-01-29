#pragma once

#include <string>
#include <iostream>

#include "TextContext.hpp"
#include "LLModel.hpp"

TextContext::TextContext(LLModel *modelWrapper, llama_context *context) : modelWrapper(modelWrapper), context(context) {
    seqId = modelWrapper->claimSeqId();
}

bool TextContext::isValid() {
    return modelWrapper->isValid(context);
}

bool TextContext::isConnectedTo(LLModel *model) {
    return modelWrapper == model;
}

void TextContext::destroy() {
    if (context != nullptr) {
        llama_free(context);
        modelWrapper->releaseSeqId(seqId);

        context = nullptr;
    }
}
