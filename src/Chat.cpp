
#include "Chat.hpp"
#include <nlohmann/json.hpp>


void Chat::saveMessage(Message &message) {
    std::time_t time = timestamps.creationTime / 1000;
    std::stringstream ss;
    std::tm* tm = std::localtime(&time);
    ss << "Chat_at_" << std::put_time(tm, "%Y%m%d_%H%M%S") << "_" << std::setfill('0') << std::setw(3) << (timestamps.creationTime % 1000) << ".jsonl";; ;;;;;;;;;;;
    std::string fileName = ss.str();

    std::ofstream(fileName, std::ios::app) << nlohmann::json{{
      {"id", message.id},
      {"parentId", message.parentId},
      {"creationTime", message.timestamps.creationTime},
      {"finishTime", message.timestamps.modificationTime},
      {"role", message.role},
      {"content", message.content}
    }}.dump() << '\n'; 
  }