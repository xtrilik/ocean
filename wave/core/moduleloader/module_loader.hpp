#ifndef WAVE_CORE_MODULELOADER_MODULE_LOADER_HPP
#define WAVE_CORE_MODULELOADER_MODULE_LOADER_HPP

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <any>
#include <optional>
#include <chrono> // For potential future use in ModuleInfo (e.g. load time)

// Platform-specific includes for dynamic library loading
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace wave {
namespace core {
namespace moduleloader {

// Forward declaration
class ICoreAccess;
class ILauncherModule;

// Re-using StructuredData definition
using StructuredData = std::any;

// Module Information
struct ModuleInfo {
    std::string name;
    std::string version;
    std::string path; // Filesystem path to the loaded module file
    void* libraryHandle; // OS-specific handle (HMODULE or void*)
    ILauncherModule* instance; // Pointer to the module instance

    ModuleInfo() : libraryHandle(nullptr), instance(nullptr) {}
    // Making ModuleInfo movable and copyable (default is fine for now, but consider ownership of instance if not raw pointer)
};

// Result of module operations
struct ModuleResult {
    enum class Status {
        Success,
        NotFound, // Module not found (e.g., for unload/reload) or file not found for load
        Error     // General error (failed to load, init failed, etc.)
    };

    Status status;
    std::optional<ModuleInfo> module; // Contains info on success, or if relevant to error
    std::string message;
    std::optional<StructuredData> data; // Additional data, e.g., error codes

    ModuleResult(Status s, std::string msg, std::optional<ModuleInfo> mi = std::nullopt, std::optional<StructuredData> d = std::nullopt)
        : status(s), message(std::move(msg)), module(std::move(mi)), data(std::move(d)) {}
};

// Module event types
enum class ModuleEventType {
    Loaded,
    Unloaded,
    Reloaded, // Could be implemented as Unloaded then Loaded events
    ErrorLoading,
    ErrorUnloading
};

// Callback for module events
using ModuleEventCallback = std::function<void(ModuleEventType type, const ModuleInfo& info, const std::string& message)>;

// Interface for modules (to be implemented by actual modules)
class ILauncherModule {
public:
    virtual ~ILauncherModule() = default;
    virtual void initialize(ICoreAccess* coreAccess) = 0;
    virtual void shutdown() = 0;
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
};

// Dummy interface for core access (to be provided by the core to modules)
class ICoreAccess {
public:
    virtual ~ICoreAccess() = default;
    // Example: virtual LoggingSystem* getLoggingSystem() = 0;
    // For now, it's empty as per requirements.
    virtual void placeholder() const { /* Does nothing */ } // To make it have at least one virtual method if needed
};


// Module Loader System
class ModuleLoaderSystem {
public:
    ModuleLoaderSystem(ICoreAccess* coreAccess); // Provide core access to modules
    ~ModuleLoaderSystem();

    ModuleResult loadModule(const std::string& modulePath);
    ModuleResult unloadModule(const std::string& moduleName);
    ModuleResult reloadModule(const std::string& moduleName); // Convenience: unload then load

    std::vector<ModuleInfo> listModules() const;
    void subscribeToModuleEvents(ModuleEventCallback callback);

    // Define function pointer types for module entry points
    // These are functions that each module shared library is expected to export.
    using CreateModuleFunc = ILauncherModule* (*)();
    using DestroyModuleFunc = void (*)(ILauncherModule*);


private:
    mutable std::mutex CppModuleMutex; // Renamed
    std::map<std::string, ModuleInfo> CppLoadedModules; // Keyed by module name (from ILauncherModule::getName()) // Renamed
    std::vector<ModuleEventCallback> CppEventCallbacks; // Renamed
    ICoreAccess* CppCoreAccess; // Non-owning pointer to core access interface // Renamed

    void broadcastEvent(ModuleEventType type, const ModuleInfo& info, const std::string& message);
    
    // Internal helper to unload a module, assumes lock is held or not needed if called from public method that locks
    ModuleResult internalUnloadModule(const std::string& moduleName, bool isReloading = false);
};

} // namespace moduleloader
} // namespace core
} // namespace wave

#endif // WAVE_CORE_MODULELOADER_MODULE_LOADER_HPP
