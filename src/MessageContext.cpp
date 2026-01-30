#include "MessageContext.hpp"
#include "LLModel.hpp"

MessageContext::MessageContext(LLModel *modelWrapper, llama_context *context) : modelWrapper(modelWrapper), context(context) {
    seqId = modelWrapper->claimSeqId();
}

bool MessageContext::isValid() {
    return modelWrapper->isValid(context);
}

bool MessageContext::isConnectedTo(LLModel *model) {
    return modelWrapper == model;
}

void MessageContext::destroy() {
}
