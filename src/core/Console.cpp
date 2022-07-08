#include "../core.h"
#include <vector>
#include <string>
#include <sstream>

Console::Console()
{
    ClearLog();

    AutoScroll = true;
    ScrollToBottom = false;
}

Console::~Console()
{
    ClearLog();
}
void Console::AddCommand(std::string cmd, pHandler phandle, std::string pHelp)
{
    char space_char = ' ';
    std::vector<std::string> words{};

    std::stringstream sstream(cmd);
    std::string word;
    while (std::getline(sstream, word, space_char)) {
        word.erase(std::remove_if(word.begin(), word.end(), ispunct), word.end());
        words.push_back(word);
    }
    auto commandName = words[0];
    words.erase(words.begin());

    Command command(commandName, pHelp, phandle);
    
    _commandContainer.insert(std::pair<std::string, Command>(commandName, command));
}

void Console::HandleResult(CmdResult& result)
{
    switch (result.state)
    {
    case CmdState::COMMAND_SUCCEEDED:
        AddLog("Worked :D");

        break;

    case CmdState::COMMAND_NOT_FOUND:
        AddLog("Command not found :'(");
        break;

    case CmdState::COMMAND_FOUND_BAD_ARGS:
        AddLog("Command found but wrong arguments, try <%s> help!", result.cmd.GetName().c_str());
        break;

    case CmdState::COMMAND_RESULT_HELP:
        AddLog(result.cmd.GetHelp());
        break;
    }
}
void Console::AddLog(const char* fmt, ...)
{
    // FIXME-OPT
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);
    Items.push_back(Strdup(buf));
}
void Console::ShowConsoleWindow(const char* title, bool* p_open)
{
    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title, p_open))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Close Console"))
            *p_open = false;
        ImGui::EndPopup();
    }


    // Reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

    for (int i = 0; i < Items.Size; i++)
    {
        const char* item = Items[i];
        if (!Filter.PassFilter(item))
            continue;

        ImVec4 color;
        bool has_color = false;

        if (has_color)
            ImGui::PushStyleColor(ImGuiCol_Text, color);

        ImGui::TextUnformatted(item);
        if (has_color)
            ImGui::PopStyleColor();
    }


    if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
        ImGui::SetScrollHereY(1.0f);

    ScrollToBottom = false;

    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::Separator();

    // Command-line
    bool reclaim_focus = false;
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
    if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
    {
        // here run the commands
        char* s = InputBuf;

        if ((s != NULL) && (s[0] == '\0'))
            return;

        CmdResult result = ExecuteCommand(s);
        strcpy(s, "");
        reclaim_focus = true;

        HandleResult(result);

    }

    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
    if (reclaim_focus)
        ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

    ImGui::End();
}
CmdResult Console::ExecuteCommand(char* cmd)
{
    char space_char = ' ';
    std::vector<std::string> words{};

    std::stringstream sstream(cmd);
    std::string word;
    while (std::getline(sstream, word, space_char)) {
        word.erase(std::remove_if(word.begin(), word.end(), ispunct), word.end());
        words.push_back(word);
    }
    auto command = words[0];
    words.erase(words.begin());

    CmdResult result;

    if (words.size() > 0 && words[0] == "help")
    {
        result.state = CmdState::COMMAND_RESULT_HELP;
        return result;
    }

    if (_commandContainer.find(command) == _commandContainer.end())
    {
        result.state = CmdState::COMMAND_NOT_FOUND;
        return result;
    }

    if (_commandContainer[command].Invoke(words))
    {
        result.state = CmdState::COMMAND_SUCCEEDED;
        result.cmd = _commandContainer[command];
    }
    else
    {
        result.state = CmdState::COMMAND_FOUND_BAD_ARGS;
        result.cmd = _commandContainer[command];
    }

    return result;

}