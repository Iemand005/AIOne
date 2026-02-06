#pragma once

#include "LLModel.hpp"

struct LLModel::Impl {
    llama_model_ptr model;
    llama_context_ptr context = nullptr;
    llama_sampler_ptr sampler;

    const llama_vocab *vocab = nullptr;

    std::deque<llama_seq_id> freeSeqIds;
    llama_seq_id biggestSeqId = 0;

    // void initBatch(llama_batch &batch, size_t maxBatchSize, llama_seq_id seqId) ;

    // llama_model *getLlamaModel() ;

    /**
     * Warning: you are responsible for ensuring the `batchSize` does not exceed the batch's
     * `maxBatchSize`, else you will leak memory
     * 
     * Also, DO NOT EDIT the `tokens` vector AT ALL until you call `freeBatch` because
     * it just sets the tokens pointer to the vector data
     */
    // void setBatch(llama_batch &batch, std::vector<llama_token> &tokens, size_t index, size_t batchSize) ;
    
    // /**
    //  * Warning: you are responsible for ensuring the `batchSize` does not exceed the batch's
    //  * `maxBatchSize`, else you will leak memory
    //  */
    // void setBatch(llama_batch &batch, llama_token *tokensStart, size_t batchSize) ;

    // void freeBatch(llama_batch &batch) ;

    llama_seq_id claimSeqId() {
        // get the first explicitly released seq id, or a new seq id
        if (freeSeqIds.empty()) {
            if (biggestSeqId == 0xFFFF) throw std::runtime_error("Maximum number of sequences reached (" + std::to_string(biggestSeqId) + ")");
            
            return biggestSeqId++; // no explicitly released seq ids, get a new one
        }

        // a seq id somewhere in the middle was released, use that one
        llama_seq_id seqId = freeSeqIds.front();
        freeSeqIds.pop_front();

        return seqId;
    }

    // TODO this method can be optimized for memory, though not too important:
    // - claim seqId (0)
    // - claim seqId (1)
    // - release seqId 0 (added to freeSeqIds)
    // - release seqId 1 (decremented biggestSeqId)
    // result: all seqIds released, but `freeSeqIds` is not empty and `biggestSeqId` is 1
    // I don't really care about this much personally but feel free to fix it
    void releaseSeqId(llama_seq_id seqId) {

        // if the seq id to be released was the last one that was claimed,
        // just decrement the last claimed seq id
        if (biggestSeqId == seqId + 1) {
            biggestSeqId--;
            return;
        }

        // this seq id was somewhere in the middle, so add it to released ids
        freeSeqIds.push_back(seqId);
    }

    void initBatch(llama_batch &batch, size_t maxBatchSize, llama_seq_id seqId){
    // note: `llama_batch_get_one` doesn't allow changing the sequence id (it's always 0)
    // another note: `llama_batch_init` mallocs memory for tokens while the vector
    // already has that memory too, so I'm gonna be a bad kitty and just set the
    // struct manually to point to the vector's data for memory efficiency
    // (it's not much but let me have my 4 kB of memory savings okay)

    if (maxBatchSize <= 0) {
        throw std::runtime_error("Cannot create a batch with maximum size " + maxBatchSize);
    }
    
    batch.n_tokens = maxBatchSize;

    // array with size `n_seq_id(maxBatchSize)`
    // each item signals the amount of `seq_id`s a token belongs to
    // in this case, always 1 (set in the loop a little further down the function)
    batch.n_seq_id = (int32_t*)llamaMalloc(sizeof(int32_t) * maxBatchSize);

    // this array is used for all items
    llama_seq_id* seqIdArray = (llama_seq_id*)llamaMalloc(sizeof(llama_seq_id));
    seqIdArray[0] = seqId;
    
    batch.seq_id = (llama_seq_id**)llamaMalloc(sizeof(llama_seq_id*) * (maxBatchSize + 1));
    for (size_t i = 0; i < maxBatchSize; ++i) {
        batch.n_seq_id[i] = 1;
        batch.seq_id[i] = seqIdArray;
    }
    batch.seq_id[maxBatchSize] = nullptr; // `llama_batch_init` does this so, so shall I
}


// llama_model *LLModel::getLlamaModel() {
//     return model.get();
// }
/**
 * Warning: you are responsible for ensuring the `batchSize` does not exceed the batch's
 * `maxBatchSize`, else you will leak memory
 * 
 * Also, DO NOT EDIT the `tokens` vector AT ALL until you call `freeBatch` because
 * it just sets the tokens pointer to the vector data
 */
void setBatch(llama_batch &batch, std::vector<llama_token> &tokens, size_t index, size_t batchSize) {
    setBatch(batch, tokens.data() + index, batchSize);
}

/**
 * Warning: you are responsible for ensuring the `batchSize` does not exceed the batch's
 * `maxBatchSize`, else you will leak memory
 */
void setBatch(llama_batch &batch, llama_token *tokensStart, size_t batchSize) {
    if (batchSize != batch.n_tokens) {
        batch.seq_id[batch.n_tokens] = batch.seq_id[0]; // reset previous nullptr seq_id to the actual seq_id
        batch.n_tokens = batchSize;
        batch.seq_id[batchSize] = nullptr; // set new last seq_id to nullptr
    }
    
    batch.token = tokensStart;
}

void freeBatch(llama_batch &batch) {
    if (batch.seq_id && batch.seq_id[0])
        llamaFree(batch.seq_id[0]); // free the one array that's reused for all other seq_id items
    if (batch.seq_id)
        llamaFree(batch.seq_id); // free the array that was holding the pointers to that one array
    if (batch.n_seq_id)
        llamaFree(batch.n_seq_id);
    batch.token = nullptr; // stop referencing the vector
}
};

void *LLModel::getSecretThingy() {
        return impl.get();
    }