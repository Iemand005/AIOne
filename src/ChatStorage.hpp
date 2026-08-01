#pragma once

#include <string>
#include <vector>
#include <map>

#include "TextGenOptions.hpp"

struct ChatMetadata {
    std::string folder;
    std::string title;
    std::string model;
    std::string systemPrompt;
    TextGenOptionsBase params;
    int64_t created = 0;
    int64_t updated = 0;
};

struct AppSettings {
    std::string apiKey;
    std::string apiBaseUrl = "api.groq.com/openai";
    std::map<std::string, std::string> apiKeys; // base URL -> API key
    std::string lastAIModel;
    std::string lastChatFolder;
};

namespace ChatStorage {

std::string rootPath();
void init();
std::string createChat(const ChatMetadata& meta);
void saveMetadata(const std::string& folder, const ChatMetadata& meta);
ChatMetadata loadMetadata(const std::string& folder);
std::vector<Message> loadMessages(const std::string& folder);
std::vector<ChatMetadata> scan();
void exportChat(const std::string& folder, const std::string& exportPath);

AppSettings loadSettings();
void saveSettings(const AppSettings& settings);

} // namespace ChatStorage
