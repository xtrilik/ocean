#ifndef WAVE_CORE_CONFIGURATION_CONFIGURATION_HPP
#define WAVE_CORE_CONFIGURATION_CONFIGURATION_HPP

#include <string>
#include <any>
#include <map>
#include <vector>
#include <mutex>
#include <functional>
#include <optional> // For ConfigResult value

namespace wave {
namespace core {
namespace configuration {

// Type for configuration values
using ConfigValue = std::any;

// Result structure for getValue
struct ConfigResult {
    bool success;
    std::optional<ConfigValue> value;
    std::string message;
    std::optional<std::any> additionalData; // For any extra info, e.g., error codes

    ConfigResult(bool s, std::optional<ConfigValue> v, std::string m)
        : success(s), value(std::move(v)), message(std::move(m)), additionalData(std::nullopt) {}
};

// Callback for asynchronous operations like reloadConfig
using AsyncCallback = std::function<void(bool success, const std::string& message)>;

// Callback for configuration change events
// Parameters: event type (e.g., "changed", "reloaded"), section, key, new value
using ConfigEventCallback = std::function<void(const std::string& eventType, 
                                               const std::string& section, 
                                               const std::string& key, 
                                               const ConfigValue& newValue)>;

class ConfigurationSystem {
public:
    ConfigurationSystem();
    explicit ConfigurationSystem(const std::string& filePath); // Constructor to load initial config
    ~ConfigurationSystem();

    // Retrieves a configuration value.
    ConfigResult getValue(const std::string& section, const std::string& key);

    // Sets a configuration value. This will also trigger a config change event.
    // Note: For simplicity, this in-memory setValue might not persist to the INI file
    // unless a specific save/export method is also implemented (outside current scope).
    void setValue(const std::string& section, const std::string& key, const ConfigValue& value);

    // Reloads the configuration from the source file.
    // Triggers a "reloaded" event upon completion.
    // If callback is provided, it's called after reload attempt.
    void reloadConfig(AsyncCallback callback = nullptr);

    // Subscribes to configuration change events.
    void subscribeToConfigEvents(ConfigEventCallback callback);
    
    // Sets the path to the configuration file.
    void setConfigSource(const std::string& filePath);

private:
    // Internal representation of configuration data: map<section, map<key, value_string>>
    // Values are stored as strings initially from INI, conversion happens at getValue/setValue
    using ConfigMap = std::map<std::string, std::map<std::string, std::string>>;
    ConfigMap CppConfigData; // Renamed to avoid conflicts
    std::string CppConfigFilePath; // Renamed
    std::mutex CppConfigMutex; // Renamed
    std::vector<ConfigEventCallback> CppEventCallbacks; // Renamed

    // Internal helper to parse INI data from a stream (e.g., file stream)
    bool parseIniFile(std::istream& stream, ConfigMap& tempConfigData);

    // Helper to broadcast events
    void broadcastConfigEvent(const std::string& eventType, 
                              const std::string& section, 
                              const std::string& key, 
                              const ConfigValue& value);
};

} // namespace configuration
} // namespace core
} // namespace wave

#endif // WAVE_CORE_CONFIGURATION_CONFIGURATION_HPP
