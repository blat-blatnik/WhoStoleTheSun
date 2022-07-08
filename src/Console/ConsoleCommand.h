#pragma once
#include <vector>
#include <string>
#include "ChatHandler.h"

class ConsoleCommand
{
	typedef bool(*pHandler)(ChatHandler*, char const*);

public:

    ConsoleCommand(char const* name, char const* help, pHandler handler, std::vector<ConsoleCommand> childCommands = std::vector<ConsoleCommand>())
        : Name(name), Help(std::move(help)), Handler(handler), ChildCommands(std::move(childCommands)) { };


    char const* Name;
    pHandler Handler;
    std::string Help;
    std::vector<ConsoleCommand> ChildCommands;
};

