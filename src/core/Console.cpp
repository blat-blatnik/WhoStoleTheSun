#include "../core.h"
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <map>


static std::vector<std::string> SplitStringByCharacter(std::string string, char spacer)
{
    int i = 0;

    std::vector<std::string> output;
    std::stringstream ss;

    while (i < string.size())
    {
        if (string[i] == spacer)
            if (ss.str().length() > 0)
            {
                output.push_back(ss.str());
                ss.str(std::string());
            }


        if (string[i] != spacer)
        {
            ss << string[i];
        }
        i++;

        if ((i == string.size()) && (ss.str().length() > 0))
        {
            output.push_back(ss.str());
            ss.str(std::string());
        }
    }

    return output;
}

class Command
{
public:
    Command(std::string pcmd, std::string pHelp, CommandHandler handle) : name(pcmd), help(pHelp), handler(handle) {}
    Command()
    {
        handler = CommandHandler();
    }

    bool Invoke(List(const char*) args) { return handler(args); }

    void SetHelp(std::string pHelp) { help = pHelp; }
    const char* GetHelp() { return help.c_str(); }
    std::string GetName() { return name; }

private:

    std::string name;
    std::string help;
    CommandHandler handler;
    // maybe implement later, for now useless
    std::vector<std::string> _commandArgTypes; // %s %d %f etc

};

enum CmdState
{
    COMMAND_NOT_FOUND,
    COMMAND_FOUND_BAD_ARGS,
    COMMAND_SUCCEEDED,
    COMMAND_HANDLED_DO_NOTHING
};
struct CmdResult
{
    std::shared_ptr<Command> cmd;
    CmdState state;
};

class Console
{
public:

    std::map<std::string, std::shared_ptr<Command>> _commandContainer = std::map<std::string, std::shared_ptr<Command>>();

    Console()
    {
        ClearLog();

        AutoScroll = true;
        ScrollToBottom = false;
    }

    ~Console()
    {
        ClearLog();
    }


    void AddCommand(const char* cmd, CommandHandler handle, const char* pHelp = "")
    {
        auto words = SplitStringByCharacter(std::string(cmd), ' ');

        auto commandName = words[0];
        words.erase(words.begin());

        auto command = std::make_shared<Command>(commandName, std::string(pHelp), handle);

        _commandContainer.insert(std::pair<std::string, std::shared_ptr<Command>>(commandName, command));
    }

    CmdResult ExecuteCommand(const char* cmd)
    {
        auto words = SplitStringByCharacter(std::string(cmd), ' ');

        auto command = words[0];
        words.erase(words.begin());

        CmdResult result;

        if (command == "help")
        {
            AddLog("All commands:");
            for (auto &keyval : _commandContainer)
                AddLog("  %s", keyval.second->GetHelp());

            result.state = CmdState::COMMAND_HANDLED_DO_NOTHING;
            return result;
        }

        if (_commandContainer.find(command) == _commandContainer.end())
        {
            result.state = CmdState::COMMAND_NOT_FOUND;
            return result;
        }

        List(const char*) wordsList = NULL;
        ListSetAllocator((void **)&wordsList, TempRealloc, TempFree);


        for (int i = 0; i < words.size(); i++)
        {          
            std::replace(words[i].begin(), words[i].end(), '`', ' ');

            ListAdd(&wordsList, words[i].c_str());
        }

        if (_commandContainer[command]->Invoke(wordsList))
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

    char                        InputBuf[256];
    ImVector<char*>             Items;
    bool                        AutoScroll;
    bool                        ScrollToBottom;
    bool                        FocusOnLoad = true;

    static char* Strdup(const char* s) { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }

    void ClearLog()
    {
        for (int i = 0; i < Items.Size; i++)
            free(Items[i]);
        Items.clear();
    }

    void AddLog(const char* fmt, ...) IM_FMTARGS(2)
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

    void ShowConsoleGui()
    {
        // Reserve enough left-over height for 1 separator + 1 input text
        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

        for (int i = 0; i < Items.Size; i++)
        {
            const char* item = Items[i];
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
        
        // Always maintain keyboard focus on the console.
        if (FocusOnLoad)
        {
            ImGui::SetKeyboardFocusHere();
            FocusOnLoad = false;
        }

        ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
        if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
        {
            // here run the commands
            char* s = InputBuf;

            if ((s != NULL) && (s[0] == '\0'))
            {
                ImGui::End();
                return;
            }

            CmdResult result = ExecuteCommand(s);
            strcpy(s, "");

            HandleResult(result);
        }
    }

    void Reset()
    {
        FocusOnLoad = true;
    }

    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
    {
        UNUSED(data);
        return NULL; 
    }

    void HandleResult(CmdResult& result)
    {
        auto cmd = result.cmd;

        switch (result.state)
        {
        case CmdState::COMMAND_SUCCEEDED:
            AddLog("Worked :D");
            break;

        case CmdState::COMMAND_NOT_FOUND:
            AddLog("Command not found :'(");
            break;

        case CmdState::COMMAND_FOUND_BAD_ARGS:
            AddLog("Wrong arguments. Usage: %s.", cmd->GetHelp());
            break;
        
        case CmdState::COMMAND_HANDLED_DO_NOTHING:
            break;
        }
    }

};

Console g_console;

extern "C" bool ParseCommandBoolArg(const char *string, bool *outSuccess)
{
    *outSuccess = false;
    if (not string)
        return false;

    if (StringsEqual(string, "1") or StringsEqualNocase(string, "true") or StringsEqualNocase(string, "on"))
    {
        *outSuccess = true;
        return true;
    }
    if (StringsEqual(string, "0") or StringsEqualNocase(string, "false") or StringsEqualNocase(string, "off"))
    {
        *outSuccess = true;
        return false;
    }

    return false;
}

extern "C" int ParseCommandIntArg(const char *string, bool *outSuccess)
{
    *outSuccess = false;
    if (not string)
        return 0;

    char *endPtr;
    long result = strtol(string, &endPtr, 0);
    if (endPtr != string)
        *outSuccess = true;

    return (int)result;
}

extern "C" float ParseCommandFloatArg(const char *string, bool *outSuccess)
{
    *outSuccess = false;
    if (not string)
        return 0;

    char *endPtr;
    float result = strtof(string, &endPtr);
    if (endPtr != string)
        *outSuccess = true;

    return result;
}

extern "C" void AddCommand(const char *command, CommandHandler handle, const char *help)
{
    g_console.AddCommand(command, handle, help);
}

extern "C" void ExecuteCommand(const char* command)
{
    g_console.ExecuteCommand(command);
}

extern "C" void ShowConsoleGui()
{
    g_console.ShowConsoleGui();
}

extern "C" void ResetConsole()
{
    g_console.Reset();
}
void AddConsoleLog(const char* log)
{
    g_console.AddLog(log);
}
void ClearConsoleLog()
{
    g_console.ClearLog();
}
