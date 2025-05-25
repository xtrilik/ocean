#ifndef WAVE_CORE_CLI_CLI_ENGINE_HPP
#define WAVE_CORE_CLI_CLI_ENGINE_HPP

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <any> // For StructuredData
#include <optional> // For CommandResult::data
#include <iostream> // For startInteractiveSession
#include <sstream>  // For command parsing
#include <algorithm> // For std::remove for unregister by pointer (if needed)

namespace wave {
namespace core {
namespace cli {

// Using std::any for StructuredData as in other modules
using StructuredData = std::any;

// Result of a command execution
struct CommandResult {
    enum class Status {
        Success,
        Warning,
        Error
    };

    Status status;
    std::string message;
    std::optional<StructuredData> data;

    CommandResult(Status s, std::string msg, std::optional<StructuredData> d = std::nullopt)
        : status(s), message(std::move(msg)), data(std::move(d)) {}

    static std::string statusToString(Status s) {
        switch (s) {
            case Status::Success: return "Success";
            case Status::Warning: return "Warning";
            case Status::Error:   return "Error";
            default: return "Unknown";
        }
    }
};

// Interface for executable commands
class ICommand {
public:
    virtual ~ICommand() = default; // Important for proper cleanup of derived classes
    virtual CommandResult execute(const std::vector<std::string>& args) = 0;
    virtual std::string getHelp() const = 0;
    virtual std::string getName() const = 0; // Useful for a generic help command
};

// Command Registry and Execution Engine
class CLIEngine {
public:
    CLIEngine();
    ~CLIEngine();

    // Executes a command line string.
    // Parses the string into command name and arguments, then executes.
    CommandResult executeCommand(const std::string& commandLine);

    // Registers a command. The engine does NOT take ownership of the ICommand pointer.
    // The caller is responsible for managing the lifetime of the command object.
    void registerCommand(const std::string& name, ICommand* command);

    // Unregisters a command by its name.
    void unregisterCommand(const std::string& name);

    // Starts a simple interactive command line session. (Optional)
    void startInteractiveSession();
    
    // Gets a list of registered command names
    std::vector<std::string> getRegisteredCommands() const;


private:
    // CommandRegistry: Using a map to store commands by name.
    // The ICommand pointers are not owned by CLIEngine.
    std::map<std::string, ICommand*> CppCommandRegistry; // Renamed
    mutable std::mutex CppRegistryMutex; // Renamed, mutable for getRegisteredCommands

    // Helper to parse command line.
    // Returns true if parsing is successful, populates commandName and args.
    bool parseCommandLine(const std::string& commandLine, std::string& commandName, std::vector<std::string>& args) const;
};

} // namespace cli
} // namespace core
} // namespace wave

#endif // WAVE_CORE_CLI_CLI_ENGINE_HPP
