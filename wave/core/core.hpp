#ifndef WAVE_CORE_CORE_HPP
#define WAVE_CORE_CORE_HPP

#include "include/ICoreAccess.hpp" // Use correct path for ICoreAccess.hpp

// Include headers for all managed core systems
#include "core/eventbus/eventbus.hpp"
#include "core/configuration/configuration.hpp"
#include "core/logging/logging.hpp"
#include "core/cli/cli_engine.hpp"
#include "core/moduleloader/module_loader.hpp"

#include <string> // For potential config file paths, etc.
#include <memory> // For std::unique_ptr if choosing that for ownership

namespace wave {
namespace core {

class Core : public wave::ICoreAccess {
public:
    Core();
    ~Core() override;

    // Initialization and Shutdown methods for the Core
    // These could load main configurations, start systems in order, etc.
    void initialize(const std::string& configFilePath = ""); // Optional config path
    void shutdown();

    // --- Implementation of ICoreAccess interface ---
    eventbus::EventBus* getEventBus() override;
    configuration::ConfigurationSystem* getConfigurationSystem() override;
    logging::LoggingSystem* getLoggingSystem() override;
    cli::CLIEngine* getCLIEngine() override;
    moduleloader::ModuleLoaderSystem* getModuleLoaderSystem() override;

private:
    // Core system instances
    // Using direct instances or unique_ptr for ownership.
    // Direct instances are simpler if their lifetime is strictly tied to Core.
    // Order of declaration can matter for destruction if there are dependencies.
    // LoggingSystem often comes early and goes late.
    // ModuleLoaderSystem might need to be shut down before other systems that modules use.

    // Option 1: Direct instances (requires default constructibility or initialization in Core constructor list)
    // logging::LoggingSystem CppLoggingSystem;
    // configuration::ConfigurationSystem CppConfigurationSystem;
    // eventbus::EventBus CppEventBus;
    // cli::CLIEngine CppCliEngine;
    // moduleloader::ModuleLoaderSystem CppModuleLoaderSystem; // Needs ICoreAccess*

    // Option 2: Unique_ptr for more flexible initialization order in constructor body
    // and explicit control over deletion order in destructor if needed.
    std::unique_ptr<logging::LoggingSystem> CppLoggingSystem_ptr;
    std::unique_ptr<configuration::ConfigurationSystem> CppConfigurationSystem_ptr;
    std::unique_ptr<eventbus::EventBus> CppEventBus_ptr;
    std::unique_ptr<cli::CLIEngine> CppCliEngine_ptr;
    std::unique_ptr<moduleloader::ModuleLoaderSystem> CppModuleLoaderSystem_ptr;

    bool CppIsInitialized;
};

} // namespace core
} // namespace wave

#endif // WAVE_CORE_CORE_HPP
