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

CmdState Console::ExecuteCommand(char* cmd)
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

    if (_commandContainer.find(command) == _commandContainer.end())
        return CmdState::COMMAND_NOT_FOUND;

    if (_commandContainer[command].Invoke(words))
        return CmdState::COMMAND_SUCCEEDED;
    else
        return CmdState::COMMAND_FOUND_BAD_ARGS;

}