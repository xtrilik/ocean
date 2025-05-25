#include "core/core.hpp" // This should bring in ICoreAccess via core.hpp's include
#include <iostream>
#include <cassert>
#include <string>

// Helper function to print test headers
void printTestHeader(const std::string& testName) {
    std::cout << "\n--- " << testName << " ---" << std::endl;
}

void testCoreInstantiationAndAccess() {
    printTestHeader("Core Instantiation and System Access Test");

    std::cout << "Creating Core instance..." << std::endl;
    wave::core::Core appCore;

    std::cout << "Initializing Core..." << std::endl;
    // Create a dummy config file for initialization test
    std::string dummyConfigPath = "test_core_config.ini";
    std::ofstream configFile(dummyConfigPath);
    if (configFile.is_open()) {
        configFile << "[TestSection]\n";
        configFile << "TestKey = TestValue\n";
        configFile.close();
    } else {
        std::cerr << "Failed to create dummy config file for core test." << std::endl;
    }
    appCore.initialize(dummyConfigPath); // Initialize with a dummy config file path

    // 1. Verify ICoreAccess getters return non-null pointers
    std::cout << "Verifying system getters..." << std::endl;
    
    wave::core::logging::LoggingSystem* logger = appCore.getLoggingSystem();
    assert(logger != nullptr && "LoggingSystem should be instantiated.");
    std::cout << "  LoggingSystem accessible." << std::endl;

    wave::core::configuration::ConfigurationSystem* config = appCore.getConfigurationSystem();
    assert(config != nullptr && "ConfigurationSystem should be instantiated.");
    std::cout << "  ConfigurationSystem accessible." << std::endl;

    wave::core::eventbus::EventBus* eventBus = appCore.getEventBus();
    assert(eventBus != nullptr && "EventBus should be instantiated.");
    std::cout << "  EventBus accessible." << std::endl;

    wave::core::cli::CLIEngine* cliEngine = appCore.getCLIEngine();
    assert(cliEngine != nullptr && "CLIEngine should be instantiated.");
    std::cout << "  CLIEngine accessible." << std::endl;

    wave::core::moduleloader::ModuleLoaderSystem* moduleLoader = appCore.getModuleLoaderSystem();
    assert(moduleLoader != nullptr && "ModuleLoaderSystem should be instantiated.");
    std::cout << "  ModuleLoaderSystem accessible." << std::endl;

    // 2. Perform a trivial operation with a system (e.g., Logging)
    std::cout << "Performing trivial operation with LoggingSystem via Core..." << std::endl;
    logger->log(wave::core::logging::LogEntry(
        wave::core::logging::LogLevel::Info,
        "CoreTest",
        "Message logged via LoggingSystem obtained from Core."
    ));
    // Visual check: This message should appear in console output if default log level is Info or Debug.

    // 3. Perform a trivial operation with ConfigurationSystem
    std::cout << "Performing trivial operation with ConfigurationSystem via Core..." << std::endl;
    auto configVal = config->getValue("TestSection", "TestKey");
    assert(configVal.success);
    assert(configVal.value.has_value());
    try {
         assert(std::any_cast<std::string>(configVal.value.value()) == "TestValue");
         std::cout << "  Successfully retrieved config value: TestSection.TestKey = " 
                   << std::any_cast<std::string>(configVal.value.value()) << std::endl;
    } catch(const std::bad_any_cast& e) {
        assert(false && "Failed to cast config value in test.");
    }


    std::cout << "Shutting down Core..." << std::endl;
    appCore.shutdown();

    // Clean up dummy config file
    std::remove(dummyConfigPath.c_str());

    std::cout << "Core Instantiation and System Access Test: PASSED" << std::endl;
}

// Dummy main for testing ICoreAccess through Core
// This also serves to ensure all necessary headers are included and linkable.
class TestModuleUsingCore : public wave::core::moduleloader::ILauncherModule {
    wave::ICoreAccess* coreAccess_ = nullptr;
public:
    void initialize(wave::ICoreAccess* coreAccess) override {
        coreAccess_ = coreAccess;
        assert(coreAccess_ != nullptr && "CoreAccess provided to module should not be null.");
        
        // Modules would use the coreAccess to get other systems
        wave::core::logging::LoggingSystem* logger = coreAccess_->getLoggingSystem();
        assert(logger != nullptr && "Module could access LoggingSystem.");
        logger->log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Debug, "TestModule", "TestModule initialized via Core."));
    }
    void shutdown() override { /* ... */ }
    std::string getName() const override { return "TestModuleUsingCore"; }
    std::string getVersion() const override { return "1.0"; }
};


void testCoreWithModuleLoader() {
    printTestHeader("Core with ModuleLoader Test");
    wave::core::Core appCore;
    appCore.initialize();

    wave::core::moduleloader::ModuleLoaderSystem* moduleLoader = appCore.getModuleLoaderSystem();
    assert(moduleLoader != nullptr);

    // This test assumes DUMMY_MODULE_PATH is valid and dummy_module is compiled.
    // It's more of an integration test snippet.
    // For now, just check if moduleLoader is accessible.
    // A full module load test via Core would be more involved here.
    std::cout << "  ModuleLoaderSystem obtained from Core. Further module loading tests are in test_module_loader.cpp." << std::endl;
    
    // Example: If we had a dummy module instance to pass to ModuleLoaderSystem constructor for some reason
    // (not how it's designed, but as an example of using a system from Core)
    // TestModuleUsingCore testModule;
    // moduleLoader->loadModule(...); // This would use the actual module loading mechanism.
    
    appCore.shutdown();
    std::cout << "Core with ModuleLoader Test: PASSED (basic check)" << std::endl;
}


int main() {
    std::cout << "Starting Core Test Suite..." << std::endl;

    testCoreInstantiationAndAccess();
    testCoreWithModuleLoader(); // Very basic check

    std::cout << "\nCore Test Suite: ALL TESTS COMPLETED." << std::endl;
    std::cout << "Note: Some tests rely on visual inspection of console output (e.g., log messages)." << std::endl;
    
    return 0;
}
