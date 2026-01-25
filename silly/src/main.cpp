#include <iostream>
#include <vector>
#include <string>

#include "ggml-backend.h"
#include "llama.h"

int main(int argc, char* argv[]) {
  const std::string modelPath = argv[1];
  const std::string prompt = argv[2];
  const int maxTokens = 40;

  // init ggml
  ggml_backend_load_all();

  // load model
  auto modelParams = llama_model_default_params();
  modelParams.use_mmap = true;

  llama_model *model = llama_model_load_from_file(modelPath.c_str(), modelParams);

  if (model == NULL) {
    std::cerr << "Failed to load model" << std::endl;
    return 1;
  }

  // calculate amount of tokens in prompt to allocate
  const llama_vocab *vocab = llama_model_get_vocab(model);
  const int promptTokenLen = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), NULL, 0, true, true);

  // tokenize prompt
  std::vector<llama_token> promptTokens(promptTokenLen);
  if (llama_tokenize(vocab, prompt.c_str(), prompt.size(), promptTokens.data(), promptTokenLen, true, true) < 0) {
    std::cerr << "Failed to tokenize the prompt" << std::endl;
    return 2;
  }

  // create a new context
  auto contextParams = llama_context_default_params();
  contextParams.n_ctx = promptTokenLen + maxTokens - 1; // context size
  contextParams.n_batch = 512; // decoding batch size
  // TODO provide generation stats?
  //contextParams.no_perf = false;

  llama_context *context = llama_init_from_model(model, contextParams);

  if (context == NULL) {
    std::cerr << "Failed to initialize a context window" << std::endl;
    return 3;
  }

  // initialize the sampler
  auto samplerParams = llama_sampler_chain_default_params();
  samplerParams.no_perf = false; // TODO disable?

  llama_sampler *sampler = llama_sampler_chain_init(samplerParams);
  llama_sampler_chain_add(sampler, llama_sampler_init_greedy());

  // process input tokens one batch at a time
  llama_batch batch;

  for (size_t i = 0; i < promptTokenLen; i += contextParams.n_batch) {
    size_t remainingTokens = promptTokenLen - i;
    
    // create a new batch (size of remaining prompt tokens, max `n_batch` length)
    size_t batchSize = std::min(remainingTokens, (size_t)contextParams.n_batch);
    batch = llama_batch_get_one(promptTokens.data() + i, batchSize);
    
    //std::cout << "===================" << std::endl;
    //std::cout << "Processing " << std::to_string(batchSize) << " out of " << std::to_string(remainingTokens) << " tokens" << std::endl;
    //std::cout << "===================" << std::endl;
    
    // if the model has an encoder, process input using encoder
    // in that case, the decoder is used on the "decoder start token" after this loop
    // this seems to be for T5?
    // 
    // if the model does not have an encoder, the decoder will be used
    // in that case, the last batch will not be decoded, because that'll be done after this loop
    if (llama_model_has_encoder(model)) {
      if (llama_encode(context, batch)) {
        std::cerr << "Failed to evaluate input tokens" << std::endl;
        return 4;
      }
    } else {
      if (remainingTokens <= contextParams.n_batch) {
        // this is the last batch, which will be processed in the generation loop
        break;
      }

      if (llama_decode(context, batch)) {
        std::cerr << "Failed to evaluate input tokens" << std::endl;
        return 4;
      }
    }
  }

  if (llama_model_has_encoder(model)) {
    // set decoder start token to be decoded
    llama_token startTokenId = llama_model_decoder_start_token(model);
    if (startTokenId == LLAMA_TOKEN_NULL) {
      startTokenId = llama_vocab_bos(vocab);
    }

    // this batch will be processed in generation loop
    batch = llama_batch_get_one(&startTokenId, 1);
  }

  // generation loop
  size_t pos = 0;
  size_t maxPos = promptTokenLen + maxTokens;
  while (pos + batch.n_tokens < maxPos) {
    // decode last batch (either input or the newly generated token)
    if (llama_decode(context, batch)) {
      std::cerr << "Failed to evaluate input tokens" << std::endl;
      return 4;
    }

    pos += batch.n_tokens;

    // generate a new token
    llama_token newTokenId = llama_sampler_sample(sampler, context, -1);

    // if "end of generation" token emitted, stop generation
    if (llama_vocab_is_eog(vocab, newTokenId)) {
      break;
    }
    
    // convert token to text
    char buf[128];
    int n = llama_token_to_piece(vocab, newTokenId, buf, sizeof(buf), 0, true);
    if (n < 0) {
      std::cerr << "Failed to convert token to text" << std::endl;
      return 5;
    }

    // write text
    // TODO send to callback
    std::string text(buf, n);
    std::cout << text.c_str();

    // wrap token in batch for decoding
    batch = llama_batch_get_one(&newTokenId, 1);

    // TODO provide generation stats?
    //tokensGenerated++;
  }

  std::cout << std::endl;

  // free model
  llama_sampler_free(sampler);
  llama_free(context);
  llama_model_free(model);

  return 0;
}