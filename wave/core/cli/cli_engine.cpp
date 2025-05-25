#include "cli_engine.hpp"
#include <iostream> // For std::cout, std::cin in startInteractiveSession
#include <sstream>  // For std::istringstream in parseCommandLine
#include <algorithm> // For std::remove if needed (not for map directly)

namespace wave {
namespace core {
namespace cli {

CLIEngine::CLIEngine() {
    // Constructor, if any specific initialization is needed for CppCommandRegistry or mutexes.
    // For now, default member initialization is sufficient.
}

CLIEngine::~CLIEngine() {
    // Destructor. Since CLIEngine does not own the ICommand pointers,
    // it should NOT delete them here. Callers are responsible for command object lifetimes.
    // If there were any resources owned by CLIEngine (e.g., dynamically allocated commands
    // if ownership model changes), they would be cleaned up here.
    std::lock_guard<std::mutex> lock(CppRegistryMutex);
    CppCommandRegistry.clear(); // Clear the map, but again, does not delete ICommand objects.
}

// Helper to parse command line.
// Splits by space. First word is command, rest are args.
// Handles simple cases, not advanced quoting or escaping.
bool CLIEngine::parseCommandLine(const std::string& commandLine, std::string& commandName, std::vector<std::string>& args) const {
    args.clear();
    commandName.clear();

    std::istringstream iss(commandLine);
    std::string token;

    if (!(iss >> commandName)) {
        return false; // Empty command line or failed to read command name
    }

    while (iss >> token) {
        args.push_back(token);
    }
    return true;
}

CommandResult CLIEngine::executeCommand(const std::string& commandLine) {
    std::string commandName;
    std::vector<std::string> args;

    if (commandLine.empty()) {
        return CommandResult(CommandResult::Status::Error, "Command line cannot be empty.");
    }

    if (!parseCommandLine(commandLine, commandName, args)) {
        return CommandResult(CommandResult::Status::Error, "Failed to parse command line.");
    }

    ICommand* command = nullptr;
    {
        std::lock_guard<std::mutex> lock(CppRegistryMutex);
        auto it = CppCommandRegistry.find(commandName);
        if (it != CppCommandRegistry.end()) {
            command = it->second;
        } else {
            // Check for a generic "help" command if the specific command is not found
            // and the requested command was "help" itself with arguments.
            // Or, provide a list of commands.
            // For now, simple "command not found".
            return CommandResult(CommandResult::Status::Error, "Command not found: " + commandName);
        }
    }

    // Execute command outside the lock if possible, unless command execution itself needs
    // to interact with the registry in a complex way. ICommand::execute is independent.
    if (command) {
        try {
            return command->execute(args);
        } catch (const std::exception& e) {
            return CommandResult(CommandResult::Status::Error, "Command execution failed with exception: " + std::string(e.what()));
        } catch (...) {
            return CommandResult(CommandResult::Status::Error, "Command execution failed with unknown exception.");
        }
    }
    // Should not be reached if logic is correct, but as a fallback:
    return CommandResult(CommandResult::Status::Error, "Internal error: Command pointer was null after lookup.");
}

// Registers a command with the given name.
// The CLIEngine does not take ownership of the ICommand pointer.
void CLIEngine::registerCommand(const std::string& name, ICommand* command) {
    if (!command) {
        // Optionally log an error or throw an exception
        // For now, silently ignore null command pointers.
        return;
    }
    if (name.empty()) {
        // Optionally log an error: command name cannot be empty
        // Or return a CommandResult indicating failure if this method were to return one.
        return;
    }

    std::lock_guard<std::mutex> lock(CppRegistryMutex);
    if (CppCommandRegistry.find(name) == CppCommandRegistry.end()) {
        CppCommandRegistry[name] = command;
    } else {
        // Command with this name already registered.
        // Current behavior: do nothing (ignore the new command).
        // Alternative: throw, log a warning, or replace existing.
        // For now, this matches the previous logic of not overwriting.
    }
}


void CLIEngine::unregisterCommand(const std::string& name) {
    if (name.empty()) return;

    std::lock_guard<std::mutex> lock(CppRegistryMutex);
    CppCommandRegistry.erase(name);
}

std::vector<std::string> CLIEngine::getRegisteredCommands() const {
    std::lock_guard<std::mutex> lock(CppRegistryMutex);
    std::vector<std::string> commandNames;
    for (const auto& pair : CppCommandRegistry) {
        commandNames.push_back(pair.first);
    }
    return commandNames;
}


void CLIEngine::startInteractiveSession() {
    std::string line;
    std::cout << "Wave CLI Engine Interactive Mode. Type 'exitcli' to quit." << std::endl; // Added exit command info
    
    // Simple "exit" command for the interactive session itself.
    // This is a built-in concept for the session, not necessarily a registered ICommand.
    // However, it's better to make "exitcli" (or similar) a registered command.
    // For this basic version, we'll handle "exitcli" directly here.

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            // EOF or error
            break; 
        }

        if (line.empty()) {
            continue;
        }
        
        // Trim leading/trailing whitespace from line for "exitcli" check
        line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
        line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

        if (line == "exitcli") { // Special command to exit interactive mode
            std::cout << "Exiting interactive session." << std::endl;
            break;
        }

        CommandResult result = executeCommand(line);
        
        std::cout << "[" << CommandResult::statusToString(result.status) << "] "
                  << result.message << std::endl;
        
        if (result.data.has_value()) {
            // Try to print data if it's a string, for demonstration
            try {
                if (result.data.value().type() == typeid(std::string)) {
                    std::cout << "Data: " << std::any_cast<std::string>(result.data.value()) << std::endl;
                } else if (result.data.value().type() == typeid(const char*)) {
                     std::cout << "Data: " << std::any_cast<const char*>(result.data.value()) << std::endl;
                }
                // Add more types if needed for interactive display
            } catch (const std::bad_any_cast& e) {
                std::cout << "Data: (Opaque/Cannot display type)" << std::endl;
            }
        }
    }
}

} // namespace cli
} // namespace core
} // namespace wave
