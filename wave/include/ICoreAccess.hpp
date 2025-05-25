#ifndef WAVE_INCLUDE_ICOREACCESS_HPP
#define WAVE_INCLUDE_ICOREACCESS_HPP

// Forward declarations of core system classes
// This avoids including all their headers directly in this interface file,
// reducing coupling if only pointers/references are needed.
namespace wave {
namespace core {
namespace eventbus { class EventBus; }
namespace configuration { class ConfigurationSystem; }
namespace logging { class LoggingSystem; }
namespace cli { class CLIEngine; }
namespace moduleloader { class ModuleLoaderSystem; }
} // namespace core
} // namespace wave

namespace wave {
// The ICoreAccess interface provides a unified way for different parts of the application,
// especially modules, to access core functionalities.
class ICoreAccess {
public:
    virtual ~ICoreAccess() = default;

    // Getter for the EventBus system
    virtual core::eventbus::EventBus* getEventBus() = 0;

    // Getter for the ConfigurationSystem
    virtual core::configuration::ConfigurationSystem* getConfigurationSystem() = 0;

    // Getter for the LoggingSystem
    virtual core::logging::LoggingSystem* getLoggingSystem() = 0;

    // Getter for the CLIEngine
    virtual core::cli::CLIEngine* getCLIEngine() = 0;

    // Getter for the ModuleLoaderSystem
    virtual core::moduleloader::ModuleLoaderSystem* getModuleLoaderSystem() = 0;

    // Const versions of getters might be useful if some users only need read-only access
    // For now, providing non-const access as modules might need to register commands, subscribe, etc.
    // virtual const core::eventbus::EventBus* getEventBus() const = 0;
    // virtual const core::configuration::ConfigurationSystem* getConfigurationSystem() const = 0;
    // ... and so on for other systems.
};

} // namespace wave

#endif // WAVE_INCLUDE_ICOREACCESS_HPP
