#include "ChatStorage.hpp"
#include "Message.hpp"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <iostream>

namespace fs = std::filesystem;

static std::string getAppDataDir() {
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    return appdata ? std::string(appdata) + "/AIOne" : std::string(std::getenv("USERPROFILE")) + "/AppData/Roaming/AIOne";
#else
    const char* home = std::getenv("HOME");
    return home ? std::string(home) + "/.local/share/AIOne" : "/tmp/AIOne";
#endif
}

std::string ChatStorage::rootPath() {
    return getAppDataDir() + "/chats";
}

void ChatStorage::init() {
    fs::create_directories(rootPath());
    fs::create_directories(getAppDataDir());
}

static std::string sanitizeFilename(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            out += '_';
        else
            out += c;
    }
    return out.empty() ? "Untitled" : out;
}

std::string ChatStorage::createChat(const ChatMetadata& meta) {
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    std::stringstream ss;
    ss << sanitizeFilename(meta.title) << "_"
       << std::put_time(tm, "%Y%m%d_%H%M%S");
    std::string folderName = ss.str();
    std::string path = rootPath() + "/" + folderName;
    fs::create_directories(path);

    ChatMetadata m = meta;
    m.folder = folderName;
    m.created = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    m.updated = m.created;

    saveMetadata(folderName, m);
    return folderName;
}

void ChatStorage::saveMetadata(const std::string& folder, const ChatMetadata& meta) {
    std::string path = rootPath() + "/" + folder + "/chat.json";
    nlohmann::json j = {
        {"version", 1},
        {"title", meta.title},
        {"created", meta.created},
        {"updated", meta.updated},
        {"model", meta.model},
        {"systemPrompt", meta.systemPrompt},
        {"parameters", {
            {"maxTokens", meta.params.maxTokens},
            {"temperature", meta.params.temperature},
            {"minP", meta.params.minP},
            {"seed", meta.params.seed}
        }}
    };
    std::ofstream(path) << j.dump(2) << std::endl;
}

ChatMetadata ChatStorage::loadMetadata(const std::string& folder) {
    std::string path = rootPath() + "/" + folder + "/chat.json";
    std::ifstream file(path);
    if (!file.is_open()) return {};

    nlohmann::json j;
    file >> j;

    ChatMetadata meta;
    meta.folder = folder;
    meta.title = j.value("title", "Untitled");
    meta.created = j.value("created", 0LL);
    meta.updated = j.value("updated", 0LL);
    meta.model = j.value("model", "");
    meta.systemPrompt = j.value("systemPrompt", "");

    auto p = j.value("parameters", nlohmann::json::object());
    meta.params.maxTokens = p.value("maxTokens", 50000);
    meta.params.temperature = p.value("temperature", 0.8f);
    meta.params.minP = p.value("minP", 0.05f);
    meta.params.seed = p.value("seed", 0xFFFFFFFF);

    return meta;
}

std::vector<Message> ChatStorage::loadMessages(const std::string& folder) {
    std::vector<Message> messages;
    std::string path = rootPath() + "/" + folder + "/messages.jsonl";
    std::ifstream file(path);
    if (!file.is_open()) return messages;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        try {
            auto j = nlohmann::json::parse(line);
            Message msg;
            msg.id = j.value("id", 0ULL);
            msg.parentId = j.value("parentId", 0ULL);
            msg.role = j.value("role", "");
            msg.content = j.value("content", "");
            msg.timestamps.creationTime = j.value("creationTime", 0LL);
            msg.timestamps.modificationTime = j.value("finishTime", 0LL);
            messages.push_back(std::move(msg));
        } catch (...) {
            std::cerr << "Failed to parse message line: " << line << std::endl;
        }
    }
    return messages;
}

std::vector<ChatMetadata> ChatStorage::scan() {
    std::vector<ChatMetadata> list;
    std::string path = rootPath();
    if (!fs::is_directory(path)) return list;

    for (const auto& entry : fs::directory_iterator(path)) {
        if (!entry.is_directory()) continue;
        std::string folder = entry.path().filename().string();
        std::string jsonPath = path + "/" + folder + "/chat.json";
        if (!fs::exists(jsonPath)) continue;
        auto meta = loadMetadata(folder);
        if (!meta.folder.empty())
            list.push_back(std::move(meta));
    }

    std::sort(list.begin(), list.end(), [](const ChatMetadata& a, const ChatMetadata& b) {
        return a.updated > b.updated;
    });

    return list;
}

void ChatStorage::exportChat(const std::string& folder, const std::string& exportPath) {
    auto meta = loadMetadata(folder);
    auto messages = loadMessages(folder);

    nlohmann::json j;
    j["version"] = 1;
    j["exported"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    j["title"] = meta.title;
    j["model"] = meta.model;
    j["systemPrompt"] = meta.systemPrompt;
    j["created"] = meta.created;
    j["updated"] = meta.updated;
    j["parameters"] = {
        {"maxTokens", meta.params.maxTokens},
        {"temperature", meta.params.temperature},
        {"minP", meta.params.minP},
        {"seed", meta.params.seed}
    };

    nlohmann::json msgArray = nlohmann::json::array();
    for (const auto& msg : messages) {
        msgArray.push_back({
            {"role", msg.role},
            {"content", msg.content},
            {"timestamp", msg.timestamps.creationTime}
        });
    }
    j["messages"] = std::move(msgArray);

    std::ofstream(exportPath) << j.dump(2) << std::endl;
}

AppSettings ChatStorage::loadSettings() {
    std::string path = getAppDataDir() + "/settings.json";
    std::ifstream file(path);
    if (!file.is_open()) return {};

    try {
        nlohmann::json j;
        file >> j;
        AppSettings s;
        s.apiKey = j.value("apiKey", "");
        s.apiBaseUrl = j.value("apiBaseUrl", "api.groq.com/openai");
        s.lastAIModel = j.value("lastAIModel", "");
        s.lastChatFolder = j.value("lastChatFolder", "");
        return s;
    } catch (...) {
        return {};
    }
}

void ChatStorage::saveSettings(const AppSettings& settings) {
    std::string path = getAppDataDir() + "/settings.json";
    nlohmann::json j = {
        {"apiKey", settings.apiKey},
        {"apiBaseUrl", settings.apiBaseUrl},
        {"lastAIModel", settings.lastAIModel},
        {"lastChatFolder", settings.lastChatFolder}
    };
    std::ofstream(path) << j.dump(2) << std::endl;
}
