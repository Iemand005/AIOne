#include "Chat.hpp"

void Chat::saveMessage(Message &message) {
    if (m_chatFolder.empty()) return;
    std::string path = m_chatFolder + "/messages.jsonl";
    std::ofstream(path, std::ios::app) << nlohmann::json{{
        {"id", message.id},
        {"parentId", message.parentId},
        {"creationTime", message.timestamps.creationTime},
        {"finishTime", message.timestamps.modificationTime},
        {"role", message.role},
        {"content", message.content}
    }}.dump() << '\n';
}

nlohmann::json Chat::toJson() const {
    nlohmann::json msgArray = nlohmann::json::array();
    for (const auto& msg : messages) {
        msgArray.push_back({
            {"id", msg.id},
            {"parentId", msg.parentId},
            {"role", msg.role},
            {"content", msg.content},
            {"creationTime", msg.timestamps.creationTime},
            {"modificationTime", msg.timestamps.modificationTime}
        });
    }

    return {
        {"version", 1},
        {"model", m_model},
        {"messages", std::move(msgArray)}
    };
}

Chat Chat::fromJson(const nlohmann::json& j) {
    Chat chat;
    chat.m_model = j.value("model", "");

    auto msgs = j.value("messages", nlohmann::json::array());
    for (const auto& mj : msgs) {
        Message msg;
        msg.id = mj.value("id", 0ULL);
        msg.parentId = mj.value("parentId", 0ULL);
        msg.role = mj.value("role", "");
        msg.content = mj.value("content", "");
        msg.timestamps.creationTime = mj.value("creationTime", 0LL);
        msg.timestamps.modificationTime = mj.value("modificationTime", 0LL);
        chat.messages.push_back(std::move(msg));
    }

    if (!chat.messages.empty())
        chat.timestamps = chat.messages.back().timestamps;

    return chat;
}
