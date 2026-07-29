struct ChatMessage {
    std::string role;
    std::string content;
};

struct ModelInfo {
    std::string id;
    std::string provider;
};

class ILLMProvider {
public:
    virtual ~ILLMProvider() = default;

    virtual std::vector<ModelInfo> getModels() = 0;

    virtual std::string complete(
        const std::string& model,
        const std::vector<ChatMessage>& messages
    ) = 0;
};