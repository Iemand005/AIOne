#include <string>
#include <vector>

#include <common/chat.h>
#include <common/jinja/runtime.h>//8

/**
 * From common/chat.cpp
 */
struct common_chat_template {
    jinja::program prog;
    std::string bos_tok;
    std::string eos_tok;
    std::string src;
};

struct common_chat_templates {
    bool add_bos;
    bool add_eos;
    bool has_explicit_template; // Model had builtin template or template overridde was specified.
    std::unique_ptr<common_chat_template> template_default; // always set (defaults to chatml)
    std::unique_ptr<common_chat_template> template_tool_use;
};

/**
 * From llama.cpp "test-chat-template.cpp"
 */
static std::string applyJinjaTemplate(
  const std::string &jinjaTemplate,
  std::vector<common_chat_msg> &messages,
    bool addEos = true,
    bool addBos = true,
  std::vector<common_chat_tool> tools = {},
  const std::string &bosToken = "",
  const std::string &eosToken = ""

) {
  common_chat_templates_ptr tmpls = common_chat_templates_init(nullptr, jinjaTemplate, bosToken, eosToken);
  common_chat_templates_inputs inputs;
  inputs.use_jinja = true;
  inputs.messages = messages;
  inputs.tools = tools;
  // inputs.add_generation_prompt = true;
  inputs.add_bos = false;
  inputs.add_eos = true;
  inputs.add_generation_prompt = false;
  // tmpls->add_bos = false;
  // tmpls->add_eos = true;

  std::string prompt = common_chat_templates_apply(tmpls.get(), inputs).prompt;

  std::string eos_tok = tmpls->template_default->eos_tok;


  return prompt;
}
