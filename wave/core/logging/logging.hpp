#ifndef WAVE_CORE_LOGGING_LOGGING_HPP
#define WAVE_CORE_LOGGING_LOGGING_HPP

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <chrono>
#include <any>
#include <iostream> // For console output
#include <fstream>  // For file output
#include <sstream>  // For formatting log messages
#include <iomanip>  // For std::put_time

namespace wave {
namespace core {
namespace logging {

// Enum for log levels
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    None // Special level to disable logging for a category
};

// Structure for log entries
struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string category;
    std::string message;
    std::any structuredData; // Optional structured data

    LogEntry(LogLevel lvl, std::string cat, std::string msg, std::any data = std::any())
        : timestamp(std::chrono::system_clock::now()),
          level(lvl),
          category(std::move(cat)),
          message(std::move(msg)),
          structuredData(std::move(data)) {}
};

// Callback function type for log events
using LogEventCallback = std::function<void(const LogEntry&)>;

class LoggingSystem {
public:
    LoggingSystem();
    ~LoggingSystem();

    // Logs a message.
    // The entry's category will be checked against its configured log level.
    void log(const LogEntry& entry);

    // Sets the log level for a specific category.
    // "default" category can be used to set the fallback log level.
    void setLogLevel(const std::string& category, LogLevel level);

    // Gets the log level for a specific category.
    // If the category is not explicitly set, it returns the "default" log level.
    LogLevel getLogLevel(const std::string& category) const; // Made const

    // Subscribes to log events. Callbacks are invoked after a log entry is processed.
    void subscribeToLogEvents(LogEventCallback callback);

    // Enables logging to a file.
    void enableFileLogging(const std::string& filePath);

    // Disables file logging.
    void disableFileLogging();

    // Helper to convert LogLevel to string
    static std::string logLevelToString(LogLevel level);

private:
    mutable std::mutex CppLogMutex; // Renamed, made mutable for const getLogLevel
    std::map<std::string, LogLevel> CppCategoryLogLevels; // Renamed
    LogLevel CppDefaultLogLevel; // Renamed
    std::vector<LogEventCallback> CppLogEventCallbacks; // Renamed
    std::ofstream CppLogFileStream; // Renamed
    bool CppFileLoggingEnabled; // Renamed

    void outputToConsole(const LogEntry& entry) const;
    void outputToFile(const LogEntry& entry);
    void broadcastLogEvent(const LogEntry& entry); // No const because CppLogMutex is used non-const way
    std::string formatLogEntry(const LogEntry& entry) const;
};

} // namespace logging
} // namespace core
} // namespace wave

#endif // WAVE_CORE_LOGGING_LOGGING_HPP
