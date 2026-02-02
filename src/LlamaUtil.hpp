#include <string>
#include <vector>

#include <common/chat.h>
#include <common/jinja/runtime.h>//8

/**
 * From llama.cpp "test-chat-template.cpp"
 */
static std::string applyJinjaTemplate(
  const std::string &jinjaTemplate,
  std::vector<common_chat_msg> &messages,
    bool addAss = true,
  std::vector<common_chat_tool> tools = {},
  const std::string &bosToken = "",
  const std::string &eosToken = ""

) {
  common_chat_templates_ptr tmpls = common_chat_templates_init(nullptr, jinjaTemplate, bosToken, eosToken);
  common_chat_templates_inputs inputs;
  inputs.use_jinja = true;
  inputs.messages = messages;
  inputs.tools = tools;
  inputs.add_generation_prompt = addAss;
  // inputs.add_bos = false;
  // inputs.add_eos = true;
  // inputs.add_generation_prompt = false;

  return common_chat_templates_apply(tmpls.get(), inputs).prompt;
}
