#include "core/cli/cli_engine.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono> // For sleep
#include <sstream> // For joining args in EchoCommand

// Helper function to print test headers
void printTestHeader(const std::string& testName) {
    std::cout << "\n--- " << testName << " ---" << std::endl;
}

// --- Dummy Commands for Testing ---

class EchoCommand : public wave::core::cli::ICommand {
public:
    std::string getName() const override { return "echo"; }
    std::string getHelp() const override { return "echo [args...] - echoes the arguments back."; }
    wave::core::cli::CommandResult execute(const std::vector<std::string>& args) override {
        std::stringstream ss;
        for (size_t i = 0; i < args.size(); ++i) {
            ss << args[i] << (i == args.size() - 1 ? "" : " ");
        }
        return wave::core::cli::CommandResult(
            wave::core::cli::CommandResult::Status::Success,
            "Echoed successfully.",
            wave::core::cli::StructuredData(ss.str())
        );
    }
};

class ExitCliTestCommand : public wave::core::cli::ICommand {
public:
    std::string getName() const override { return "exitclitest"; }
    std::string getHelp() const override { return "exitclitest - dummy command for testing."; }
    wave::core::cli::CommandResult execute(const std::vector<std::string>&) override {
        return wave::core::cli::CommandResult(
            wave::core::cli::CommandResult::Status::Success,
            "ExitCliTest command executed."
        );
    }
};

class FailCommand : public wave::core::cli::ICommand {
public:
    std::string getName() const override { return "fail"; }
    std::string getHelp() const override { return "fail - always returns an error."; }
    wave::core::cli::CommandResult execute(const std::vector<std::string>&) override {
        return wave::core::cli::CommandResult(
            wave::core::cli::CommandResult::Status::Error,
            "This command always fails."
        );
    }
};

class HelpTestCommand : public wave::core::cli::ICommand {
private:
    wave::core::cli::CLIEngine* engine_ptr; // Non-owning pointer to the engine
public:
    HelpTestCommand(wave::core::cli::CLIEngine* engine = nullptr) : engine_ptr(engine) {}
    std::string getName() const override { return "helptest"; }
    std::string getHelp() const override { 
        return "helptest [command_name] - displays help for a command. If no command_name, shows general help."; 
    }
    wave::core::cli::CommandResult execute(const std::vector<std::string>& args) override {
        if (!engine_ptr) {
             return wave::core::cli::CommandResult(wave::core::cli::CommandResult::Status::Error, "HelpTestCommand not initialized with CLIEngine.");
        }
        if (args.empty()) {
            std::string all_commands_help = "Available commands:\n";
            for (const auto& cmd_name : engine_ptr->getRegisteredCommands()) {
                // To get help for each, we'd need to access the ICommand objects.
                // The CLIEngine doesn't directly expose them. This test command
                // would need a way to get ICommand* from name, or CLIEngine needs a getCommandHelp(name) method.
                // For simplicity, this test help will just list names.
                all_commands_help += "- " + cmd_name + "\n";
            }
            return wave::core::cli::CommandResult(wave::core::cli::CommandResult::Status::Success, all_commands_help);
        }
        
        // This part is tricky: to get help for a *specific* command by name,
        // the CLIEngine would need to provide access to the ICommand objects or their help strings.
        // The current CLIEngine::executeCommand is for execution, not metadata retrieval.
        // Let's assume for this test that if an argument is given, we try to "execute" `command_name --help`
        // or similar, which isn't how ICommand is structured.
        // So, this specific help command will be limited for now.
        // A real help command in the engine might iterate CppCommandRegistry.
        // For now, let's simulate getting help for "echo" if it's registered.
        if (args[0] == "echo") {
             EchoCommand temp_echo; // Create a temporary instance just to get help
             return wave::core::cli::CommandResult(wave::core::cli::CommandResult::Status::Success, temp_echo.getHelp());
        }

        return wave::core::cli::CommandResult(wave::core::cli::CommandResult::Status::Warning, "Help for specific command '" + args[0] + "' not fully implemented in this test command.");
    }
};


void testRegistrationAndUnregistration() {
    printTestHeader("Command Registration and Unregistration Test");
    wave::core::cli::CLIEngine engine;
    EchoCommand echoCmd;
    ExitCliTestCommand exitCmd;

    engine.registerCommand("echo", &echoCmd);
    engine.registerCommand("exitclitest", &exitCmd);

    auto registered = engine.getRegisteredCommands();
    assert(registered.size() == 2);
    bool echoFound = false, exitFound = false;
    for(const auto& name : registered) {
        if (name == "echo") echoFound = true;
        if (name == "exitclitest") exitFound = true;
    }
    assert(echoFound && exitFound);

    // Test registering with existing name (should be ignored)
    EchoCommand echoCmd2; // Different instance, same name
    engine.registerCommand("echo", &echoCmd2); 
    registered = engine.getRegisteredCommands();
    assert(registered.size() == 2); // Size should not change

    engine.unregisterCommand("echo");
    registered = engine.getRegisteredCommands();
    assert(registered.size() == 1);
    assert(registered[0] == "exitclitest");

    engine.unregisterCommand("nonexistent"); // Should not crash or change anything
    registered = engine.getRegisteredCommands();
    assert(registered.size() == 1);

    engine.unregisterCommand("exitclitest");
    registered = engine.getRegisteredCommands();
    assert(registered.empty());

    std::cout << "Command Registration and Unregistration Test: PASSED" << std::endl;
}

void testCommandExecution() {
    printTestHeader("Command Execution Test");
    wave::core::cli::CLIEngine engine;
    EchoCommand echoCmd;
    FailCommand failCmd;

    engine.registerCommand("echo", &echoCmd);
    engine.registerCommand("fail", &failCmd);

    // Test successful execution with args
    wave::core::cli::CommandResult result = engine.executeCommand("echo Hello Wave World");
    assert(result.status == wave::core::cli::CommandResult::Status::Success);
    assert(result.message == "Echoed successfully.");
    assert(result.data.has_value());
    try {
        assert(std::any_cast<std::string>(result.data.value()) == "Hello Wave World");
    } catch (const std::bad_any_cast&) { assert(false && "Bad any_cast in echo test"); }

    // Test successful execution without args
    result = engine.executeCommand("echo");
    assert(result.status == wave::core::cli::CommandResult::Status::Success);
    assert(result.data.has_value());
    try {
        assert(std::any_cast<std::string>(result.data.value()).empty());
    } catch (const std::bad_any_cast&) { assert(false && "Bad any_cast in echo empty test"); }

    // Test command that fails
    result = engine.executeCommand("fail");
    assert(result.status == wave::core::cli::CommandResult::Status::Error);
    assert(result.message == "This command always fails.");

    // Test non-existent command
    result = engine.executeCommand("nonexistentcmd arg1 arg2");
    assert(result.status == wave::core::cli::CommandResult::Status::Error);
    assert(result.message.find("Command not found") != std::string::npos);
    
    // Test empty command line
    result = engine.executeCommand("");
    assert(result.status == wave::core::cli::CommandResult::Status::Error);
    assert(result.message.find("Command line cannot be empty") != std::string::npos);

    // Test command line with only spaces (should also be treated as empty or parse error)
    result = engine.executeCommand("   ");
     assert(result.status == wave::core::cli::CommandResult::Status::Error);
    // The message might be "Failed to parse command line." or "Command line cannot be empty."
    // depending on how parseCommandLine handles all-space input.
    // Current parseCommandLine would make commandName "   " if not trimmed first, or fail to extract.
    // Let's assume it would fail to parse if the command name itself is effectively empty after internal processing.
    // The current parseCommandLine will extract "   " as commandName if it's the only token.
    // Then it will fail at lookup. If CLIEngine trims commandName before lookup, it might be "not found".
    // If it's `iss >> commandName` and line is "   ", commandName might remain empty.
    // Let's check current behavior: `iss >> commandName` for "   " results in commandName being empty.
    // So, it should be "Failed to parse command line." if commandName is empty after extraction.
    assert(result.message.find("Failed to parse command line.") != std::string::npos || result.message.find("Command not found: ") != std::string::npos);


    std::cout << "Command Execution Test: PASSED" << std::endl;
}

void testHelpMessages() {
    printTestHeader("Help Messages Test");
    wave::core::cli::CLIEngine engine;
    EchoCommand echoCmd;
    HelpTestCommand helpCmd(&engine); // Pass engine pointer

    engine.registerCommand("echo", &echoCmd);
    engine.registerCommand("helptest", &helpCmd);

    // Test help for a specific command (simulated via HelpTestCommand)
    wave::core::cli::CommandResult result = engine.executeCommand("helptest echo");
    assert(result.status == wave::core::cli::CommandResult::Status::Success);
    assert(result.message == echoCmd.getHelp());

    // Test general help (listing commands)
    result = engine.executeCommand("helptest");
    assert(result.status == wave::core::cli::CommandResult::Status::Success);
    std::cout << "General Help Output:\n" << result.message << std::endl;
    assert(result.message.find("- echo") != std::string::npos);
    assert(result.message.find("- helptest") != std::string::npos);
    
    // Test direct getHelp() from a known command instance
    assert(echoCmd.getHelp() == "echo [args...] - echoes the arguments back.");

    std::cout << "Help Messages Test: PASSED (visual check for general help recommended)" << std::endl;
}


void testThreadSafety() {
    printTestHeader("Thread Safety Test (Basic)");
    wave::core::cli::CLIEngine engine;
    EchoCommand echoCmd; // Shared instance for some threads
    
    const int numThreads = 10;
    const int operationsPerThread = 50;
    std::vector<std::thread> threads;
    std::atomic<int> successfulExecutions(0);
    std::atomic<int> registrationAttempts(0);

    // Initial registration
    engine.registerCommand("echo", &echoCmd);

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&engine, &echoCmd, i, &successfulExecutions, &registrationAttempts]() {
            EchoCommand localEchoCmd; // Each thread can also have its own commands
            FailCommand localFailCmd;

            for (int j = 0; j < operationsPerThread; ++j) {
                int op = j % 4;
                std::string cmdName = "echo"; // Most operations on "echo"
                
                if (op == 0) { // Execute echo
                    wave::core::cli::CommandResult res = engine.executeCommand("echo Thread " + std::to_string(i) + " Op " + std::to_string(j));
                    if (res.status == wave::core::cli::CommandResult::Status::Success) {
                        successfulExecutions++;
                    }
                } else if (op == 1) { // Register a thread-local command
                    registrationAttempts++;
                    engine.registerCommand("localEcho" + std::to_string(i), &localEchoCmd);
                    engine.registerCommand("localFail" + std::to_string(i), &localFailCmd);
                } else if (op == 2) { // Unregister a thread-local command
                    engine.unregisterCommand("localEcho" + std::to_string(i));
                } else { // Execute another command, possibly local or common "fail"
                    if (j % 2 == 0) engine.executeCommand("localFail" + std::to_string(i)); // Might fail if not registered yet or already unregistered
                    else engine.executeCommand("fail"); // This one is not registered in this basic test setup initially, should fail "not found"
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Thread Safety Test: Successful 'echo' executions: " << successfulExecutions.load() << std::endl;
    std::cout << "Thread Safety Test: Registration attempts for local commands: " << registrationAttempts.load() << std::endl;
    
    // Expected successful 'echo' executions: numThreads * (operationsPerThread / 4)
    // This is approximate as other ops might interact. The main goal is no crashes.
    assert(successfulExecutions.load() > 0); 

    // Check final state of registered commands (can be complex to predict exact number)
    auto registeredCmds = engine.getRegisteredCommands();
    std::cout << "Registered commands after test: " << registeredCmds.size() << std::endl;
    for(const auto& name : registeredCmds) std::cout << "- " << name << std::endl;
    assert(!registeredCmds.empty()); // Should have at least "echo", maybe some "localFail" commands.

    std::cout << "Thread Safety Test (Basic): COMPLETED (check for crashes/deadlocks)" << std::endl;
}


int main() {
    std::cout << "Starting CLIEngine Test Suite..." << std::endl;

    testRegistrationAndUnregistration();
    testCommandExecution();
    testHelpMessages();
    testThreadSafety();

    // Note: startInteractiveSession() is harder to test automatically.
    // It can be tested manually by uncommenting:
    // if (false) { // Set to true to manually test interactive session
    //     printTestHeader("Manual Interactive Session Test");
    //     wave::core::cli::CLIEngine engine;
    //     EchoCommand echoCmd;
    //     FailCommand failCmd;
    //     engine.registerCommand("echo", &echoCmd);
    //     engine.registerCommand("fail", &failCmd);
    //     engine.startInteractiveSession();
    // }
    
    std::cout << "\nCLIEngine Test Suite: ALL TESTS COMPLETED." << std::endl;
    return 0;
}
