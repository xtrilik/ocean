#include "configuration.hpp"
#include <fstream>
#include <sstream> // For string stream manipulations
#include <algorithm> // For std::remove_if, std::isspace

// Helper function to trim leading/trailing whitespace from a string
std::string trim(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return ""; // string is all whitespace
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, (end - start + 1));
}


namespace wave {
namespace core {
namespace configuration {

ConfigurationSystem::ConfigurationSystem() : CppConfigFilePath("") {}

ConfigurationSystem::ConfigurationSystem(const std::string& filePath) : CppConfigFilePath(filePath) {
    reloadConfig(); // Load initial config
}

ConfigurationSystem::~ConfigurationSystem() {
    // Destructor logic, if any needed (e.g., ensuring callbacks are cleared or threads joined if any were used internally)
}

void ConfigurationSystem::setConfigSource(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(CppConfigMutex);
    CppConfigFilePath = filePath;
    // Optionally, trigger a reload here or leave it to the user to call reloadConfig.
    // For now, just setting the path.
}

ConfigResult ConfigurationSystem::getValue(const std::string& section, const std::string& key) {
    std::lock_guard<std::mutex> lock(CppConfigMutex);
    auto sec_it = CppConfigData.find(section);
    if (sec_it != CppConfigData.end()) {
        auto key_it = sec_it->second.find(key);
        if (key_it != sec_it->second.end()) {
            // For now, we store strings and return them as std::any.
            // The user of getValue would need to know the expected type.
            // A more advanced system might store std::any directly or handle conversions.
            return ConfigResult(true, ConfigValue(key_it->second), "Value retrieved successfully.");
        }
        return ConfigResult(false, std::nullopt, "Key not found in section.");
    }
    return ConfigResult(false, std::nullopt, "Section not found.");
}

void ConfigurationSystem::setValue(const std::string& section, const std::string& key, const ConfigValue& value) {
    std::string valueStr;
    try {
        // Attempt to cast ConfigValue to string. This is a simplification.
        // A more robust system might handle various types or require `value` to be string.
        if (value.type() == typeid(std::string)) {
            valueStr = std::any_cast<std::string>(value);
        } else if (value.type() == typeid(const char*)) {
            valueStr = std::any_cast<const char*>(value);
        } else if (value.type() == typeid(int)) {
            valueStr = std::to_string(std::any_cast<int>(value));
        } else if (value.type() == typeid(double)) {
            valueStr = std::to_string(std::any_cast<double>(value));
        } else if (value.type() == typeid(bool)) {
            valueStr = std::any_cast<bool>(value) ? "true" : "false";
        }
        // Add more type handlers as needed...
        else {
            // If type is unknown or not easily convertible to string for INI storage
            // For now, we'll skip setting it and could log an error.
            // Or, one might choose to throw an exception.
            // For this implementation, we'll assume values are provided in a string-convertible form.
            // If not, the value won't be set in the map.
            // This is a simplification for the current scope.
            // A production system would need clearer rules on type handling for setValue.
            // For now, if it's not a directly supported type for string conversion, we don't update.
            // This means that if `value` is some complex user-defined type, it won't be stored
            // unless it's converted to a string *before* calling setValue.
            // The PRD says ConfigValue can be std::string or std::any. If it's std::any,
            // we need a defined way to serialize it to string for our ConfigMap.
            // We'll assume for now that any `std::any` passed can be represented as a string,
            // or the application layer handles this. The example above handles common types.
             if (valueStr.empty() && value.has_value()) {
                 // If valueStr is still empty but value is not (e.g. type not handled above)
                 // This indicates a type that we didn't explicitly convert.
                 // For simplicity, we'll effectively ignore setting such values in the string map.
                 // A more robust implementation would define behavior for arbitrary std::any types.
                 return; 
             }
        }
    } catch (const std::bad_any_cast& e) {
        // Failed to cast, cannot store as string.
        // In a real system, might log this error.
        return; // Or handle error appropriately
    }

    std::lock_guard<std::mutex> lock(CppConfigMutex);
    CppConfigData[section][key] = valueStr;
    
    // Broadcast change event
    // The `value` passed to broadcast is the original `std::any` value
    broadcastConfigEvent("changed", section, key, value);
}


void ConfigurationSystem::reloadConfig(AsyncCallback callback) {
    std::string path_to_load;
    {
        std::lock_guard<std::mutex> lock(CppConfigMutex); // Protect access to CppConfigFilePath
        path_to_load = CppConfigFilePath;
    }

    if (path_to_load.empty()) {
        if (callback) callback(false, "Configuration file path not set.");
        return;
    }

    std::ifstream configFileStream(path_to_load);
    if (!configFileStream.is_open()) {
        if (callback) callback(false, "Failed to open configuration file: " + path_to_load);
        return;
    }

    ConfigMap tempConfigData;
    bool parseSuccess = parseIniFile(configFileStream, tempConfigData);
    configFileStream.close();

    if (parseSuccess) {
        std::lock_guard<std::mutex> lock(CppConfigMutex);
        CppConfigData = tempConfigData; // Atomically update the configuration
        // Broadcast "reloaded" event for each key/value pair that was loaded.
        // Or a single "reloaded_all" event. For now, let's do one generic event.
        // The PRD implies a single "reloaded" event.
        // It's ambiguous if it should contain all data or just be a signal.
        // Let's make it a general signal without specific key/value.
        broadcastConfigEvent("reloaded", "", "", std::any()); // No specific section/key for a full reload event
        if (callback) callback(true, "Configuration reloaded successfully.");
    } else {
        if (callback) callback(false, "Failed to parse configuration file.");
    }
}

void ConfigurationSystem::subscribeToConfigEvents(ConfigEventCallback callback) {
    std::lock_guard<std::mutex> lock(CppConfigMutex);
    CppEventCallbacks.push_back(callback);
}

// Internal helper to parse INI data
bool ConfigurationSystem::parseIniFile(std::istream& stream, ConfigMap& tempConfigData) {
    // tempConfigData is passed by reference and will be populated here.
    // This method does not need its own mutex lock if it's only called from reloadConfig,
    // which should already hold the lock or manage data safely before swapping.
    // However, reloadConfig currently calls this *before* acquiring the main lock for CppConfigData update.
    // This is fine as tempConfigData is local to reloadConfig until the swap.

    std::string currentSection;
    std::string line;
    int lineNumber = 0;

    while (std::getline(stream, line)) {
        lineNumber++;
        line = trim(line);

        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue; // Skip empty lines and comments
        }

        if (line[0] == '[' && line.back() == ']') {
            // Section header
            currentSection = trim(line.substr(1, line.length() - 2));
            if (currentSection.empty()) {
                // Invalid section header, e.g., "[]"
                // Optionally log an error: "Parse error: Empty section name at line " << lineNumber
                return false; // Indicate parse error
            }
            tempConfigData[currentSection]; // Ensure section exists, even if empty
        } else {
            // Key-value pair
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = trim(line.substr(0, equalsPos));
                std::string value = trim(line.substr(equalsPos + 1));

                if (key.empty()) {
                    // Invalid key-value pair (empty key)
                    // Optionally log an error: "Parse error: Empty key at line " << lineNumber << " in section " << currentSection
                    return false; // Indicate parse error
                }
                if (currentSection.empty()) {
                    // Key-value pair outside of any section (global or error, depending on INI spec)
                    // For this implementation, we'll consider it an error or require a default/global section.
                    // Let's assume keys must be within a section.
                    // Optionally log: "Parse error: Key-value pair outside of section at line " << lineNumber
                    return false; // Or assign to a global section if desired
                }
                tempConfigData[currentSection][key] = value;
            } else {
                // Line is not a comment, not a section, and not a valid key-value pair
                // Optionally log: "Parse error: Malformed line " << lineNumber << ": " << line
                // Depending on strictness, could return false or just ignore. Let's be strict.
                return false;
            }
        }
    }
    return true; // Successfully parsed
}

// Helper to broadcast events
void ConfigurationSystem::broadcastConfigEvent(const std::string& eventType, 
                                              const std::string& section, 
                                              const std::string& key, 
                                              const ConfigValue& value) {
    // This function is called from methods that already hold CppConfigMutex,
    // so direct access to CppEventCallbacks should be safe.
    // If it were called from somewhere else, it would need its own lock or care.
    for (const auto& callback : CppEventCallbacks) {
        try {
            callback(eventType, section, key, value);
        } catch (const std::exception& e) {
            // Log error from callback: std::cerr << "Exception in config event callback: " << e.what() << std::endl;
            // Decide if one callback failing should affect others. Usually not.
        }
    }
}

} // namespace configuration
} // namespace core
} // namespace wave
