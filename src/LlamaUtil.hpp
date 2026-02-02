#include <string>
#include <vector>

#include <common/chat.h>
#include <common/jinja/runtime.h>//8

/**
 * From llama.cpp "test-chat-template.cpp"
 */
static std::string applyJinjaTemplate(
  const llama_model *model,
  std::vector<common_chat_msg> &messages,
    bool addAss = true,
  std::vector<common_chat_tool> tools = {}
) {
  const std::string &jinjaTemplate = llama_model_chat_template(model, nullptr);

  common_chat_templates_ptr tmpls = common_chat_templates_init(nullptr, jinjaTemplate);
  common_chat_templates_inputs inputs;
  inputs.use_jinja = true;
  inputs.messages = messages;
  inputs.tools = tools;
  inputs.add_generation_prompt = addAss;

  return common_chat_templates_apply(tmpls.get(), inputs).prompt;
}
