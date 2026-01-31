#include <string>
#include <vector>

#include <common/chat.h>

/**
 * From llama.cpp "test-chat-template.cpp"
 */
static std::string applyJinjaTemplate(
  const std::string &jinjaTemplate,
  std::vector<common_chat_msg> &messages,
  std::vector<common_chat_tool> tools = {},
  const std::string &bosToken = "",
  const std::string &eosToken = ""
) {
  common_chat_templates_ptr tmpls = common_chat_templates_init(nullptr, jinjaTemplate, bosToken, eosToken);
  common_chat_templates_inputs inputs;
  inputs.use_jinja = true;
  inputs.messages = messages;
  inputs.tools = tools;
  inputs.add_generation_prompt = true;
  return common_chat_templates_apply(tmpls.get(), inputs).prompt;
}