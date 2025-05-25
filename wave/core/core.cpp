#include "core.hpp"
#include <iostream> // For basic debug messages during init/shutdown

namespace wave {
namespace core {

Core::Core() : CppIsInitialized(false) {
    // Order of initialization can be important.
    // 1. LoggingSystem: So other systems can log during their construction/init.
    CppLoggingSystem_ptr = std::make_unique<logging::LoggingSystem>();
    
    // 2. ConfigurationSystem: May be needed by other systems.
    CppConfigurationSystem_ptr = std::make_unique<configuration::ConfigurationSystem>();

    // 3. EventBus: For inter-system communication.
    CppEventBus_ptr = std::make_unique<eventbus::EventBus>();

    // 4. CLIEngine: May need access to other systems via ICoreAccess if commands are complex,
    //    but CLIEngine itself is fairly standalone.
    CppCliEngine_ptr = std::make_unique<cli::CLIEngine>();

    // 5. ModuleLoaderSystem: Needs ICoreAccess (this), so it's typically initialized last among systems
    //    that don't depend on modules being loaded at Core construction.
    //    Or, it can be initialized earlier if other systems need to subscribe to module events
    //    during their own construction (less common for Core construction phase).
    //    Passing 'this' (ICoreAccess*) to ModuleLoaderSystem.
    CppModuleLoaderSystem_ptr = std::make_unique<moduleloader::ModuleLoaderSystem>(this);

    // std::cout << "[Core] Constructor: All core systems instantiated." << std::endl;
}

Core::~Core() {
    // std::cout << "[Core] Destructor: Shutting down..." << std::endl;
    if (CppIsInitialized) {
        shutdown(); // Ensure shutdown logic is called if not explicitly done.
    }
    // unique_ptrs will automatically delete managed objects in reverse order of declaration.
    // If specific shutdown sequence beyond simple destruction is needed for the objects
    // themselves (e.g., ModuleLoaderSystem::unloadAllModules before LoggingSystem stops),
    // that should be in the Core::shutdown() method.
    // The reverse order of declaration for unique_ptr members is:
    // 1. CppModuleLoaderSystem_ptr
    // 2. CppCliEngine_ptr
    // 3. CppEventBus_ptr
    // 4. CppConfigurationSystem_ptr
    // 5. CppLoggingSystem_ptr
    // This order is generally good (logging last to go).
    // std::cout << "[Core] Destructor: Cleanup complete." << std::endl;
}

void Core::initialize(const std::string& configFilePath) {
    if (CppIsInitialized) {
        // std::cout << "[Core] Already initialized." << std::endl;
        if (CppLoggingSystem_ptr) {
             CppLoggingSystem_ptr->log(logging::LogEntry(logging::LogLevel::Warning, "Core", "Core::initialize() called multiple times."));
        }
        return;
    }

    // Initialize ConfigurationSystem first if a path is provided
    if (CppConfigurationSystem_ptr && !configFilePath.empty()) {
        CppConfigurationSystem_ptr->setConfigSource(configFilePath);
        CppConfigurationSystem_ptr->reloadConfig([this](bool success, const std::string& message) {
            if (CppLoggingSystem_ptr) {
                logging::LogLevel level = success ? logging::LogLevel::Info : logging::LogLevel::Error;
                CppLoggingSystem_ptr->log(logging::LogEntry(level, "Core", "Config reload from initialize: " + message));
            }
        });
    }
    
    // Example: Set default log level from config after loading it.
    // if (CppLoggingSystem_ptr && CppConfigurationSystem_ptr) {
    //     auto logLevelConf = CppConfigurationSystem_ptr->getValue("Logging", "DefaultLevel");
    //     if (logLevelConf.success && logLevelConf.value.has_value()) {
    //         try {
    //             std::string levelStr = std::any_cast<std::string>(logLevelConf.value.value());
    //             // Convert string to LogLevel enum and set it
    //             // For now, this is just a placeholder for such logic
    //         } catch (const std::bad_any_cast&) { /* ignore */ }
    //     }
    // }


    // Other initializations can go here.
    // For example, registering built-in CLI commands,
    // loading default/essential modules, etc.

    CppIsInitialized = true;
    if (CppLoggingSystem_ptr) {
        CppLoggingSystem_ptr->log(logging::LogEntry(logging::LogLevel::Info, "Core", "Core initialized successfully."));
    }
    // std::cout << "[Core] Initialized." << std::endl;
}

void Core::shutdown() {
    if (!CppIsInitialized) {
        // std::cout << "[Core] Already shutdown or not initialized." << std::endl;
         if (CppLoggingSystem_ptr) { // Logging system might still be there if never initialized
             CppLoggingSystem_ptr->log(logging::LogEntry(logging::LogLevel::Warning, "Core", "Core::shutdown() called without prior initialization or multiple times."));
        }
        return;
    }

    if (CppLoggingSystem_ptr) {
        CppLoggingSystem_ptr->log(logging::LogEntry(logging::LogLevel::Info, "Core", "Core shutting down..."));
    }
    // std::cout << "[Core] Shutting down..." << std::endl;

    // Order of shutdown is important:
    // 1. Unload all modules: Modules might depend on other core systems.
    if (CppModuleLoaderSystem_ptr) {
        // std::cout << "[Core] Unloading all modules..." << std::endl;
        // This part of ModuleLoaderSystem's destructor already handles unloading.
        // However, calling it explicitly here ensures it happens before other systems are potentially nulled out
        // if we were not using unique_ptrs or had more complex dependencies during system destructor phase.
        // For unique_ptrs, their destructors are called in reverse order of declaration.
        // CppModuleLoaderSystem_ptr's destructor will be called first among the unique_ptrs,
        // which should unload modules. So, explicit call here might be redundant IF
        // ModuleLoaderSystem's destructor is robust enough.
        // Let's assume ModuleLoaderSystem's destructor handles unloading all modules.
        // If specific actions are needed beyond that, call them here.
        // For example, if ModuleLoaderSystem had a specific `unloadAllModules()` method:
        // CppModuleLoaderSystem_ptr->unloadAllModules();
    }
    
    // 2. Shutdown other systems if they have explicit shutdown methods.
    // Most of my systems manage resources via RAII and their destructors are sufficient.
    // e.g., CLIEngine, EventBus don't have explicit shutdown methods in their current design.
    // LoggingSystem's destructor closes the log file.
    // ConfigurationSystem has no explicit shutdown.

    // After this, unique_ptrs will handle deletion in reverse order of declaration in core.hpp
    // CppModuleLoaderSystem_ptr -> CppCliEngine_ptr -> CppEventBus_ptr -> CppConfigurationSystem_ptr -> CppLoggingSystem_ptr.
    // This order seems reasonable.

    CppIsInitialized = false;
    if (CppLoggingSystem_ptr) { // Log after CppIsInitialized is set to false but before logger is gone.
        CppLoggingSystem_ptr->log(logging::LogEntry(logging::LogLevel::Info, "Core", "Core shutdown complete."));
    }
    // std::cout << "[Core] Shutdown complete." << std::endl;
}

// --- Implementation of ICoreAccess interface ---

eventbus::EventBus* Core::getEventBus() {
    return CppEventBus_ptr.get();
}

configuration::ConfigurationSystem* Core::getConfigurationSystem() {
    return CppConfigurationSystem_ptr.get();
}

logging::LoggingSystem* Core::getLoggingSystem() {
    return CppLoggingSystem_ptr.get();
}

cli::CLIEngine* Core::getCLIEngine() {
    return CppCliEngine_ptr.get();
}

moduleloader::ModuleLoaderSystem* Core::getModuleLoaderSystem() {
    return CppModuleLoaderSystem_ptr.get();
}

} // namespace core
} // namespace wave
