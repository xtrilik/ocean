#include "logging.hpp"
#include <iostream> // For std::cerr, std::cout
#include <algorithm> // For std::find_if, not strictly needed with map
#include <iomanip>   // For std::put_time for formatting time

namespace wave {
namespace core {
namespace logging {

// Static helper implementation
std::string LoggingSystem::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARNING";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::None:    return "NONE";
        default:                return "UNKNOWN";
    }
}

LoggingSystem::LoggingSystem()
    : CppDefaultLogLevel(LogLevel::Info), CppFileLoggingEnabled(false) {
    // Default log level for "default" category is Info.
    CppCategoryLogLevels["default"] = LogLevel::Info;
}

LoggingSystem::~LoggingSystem() {
    std::lock_guard<std::mutex> lock(CppLogMutex);
    if (CppLogFileStream.is_open()) {
        CppLogFileStream.close();
    }
}

std::string LoggingSystem::formatLogEntry(const LogEntry& entry) const {
    std::stringstream ss;
    // Time formatting
    auto t = std::chrono::system_clock::to_time_t(entry.timestamp);
    // Using std::put_time for formatting. Note: std::gmtime or std::localtime might return static buffer.
    // For thread safety with time formatting, C++20 introduces thread-safe versions.
    // Here, we'll use std::localtime which is common but be aware of potential issues in highly threaded legacy systems.
    // A safer approach might involve custom time formatting or using a library.
    // For this scope, std::put_time with std::localtime result is generally acceptable.
    // To be absolutely safe from static buffer issues with localtime:
    std::tm timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &t); // Windows specific
#else
    localtime_r(&t, &timeinfo); // POSIX specific
#endif

    ss << "[" << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S") << "] ";
    ss << "[" << logLevelToString(entry.level) << "] ";
    ss << "[" << entry.category << "] ";
    ss << entry.message;

    // Optionally, serialize std::any structuredData if it's present and simple type
    if (entry.structuredData.has_value()) {
        // Example: if it's a string, append it.
        // Real serialization of std::any is complex and depends on expected types.
        // This is a placeholder for more complex structured data handling.
        try {
            if (entry.structuredData.type() == typeid(std::string)) {
                ss << " {Data: " << std::any_cast<std::string>(entry.structuredData) << "}";
            } else if (entry.structuredData.type() == typeid(const char*)) {
                ss << " {Data: " << std::any_cast<const char*>(entry.structuredData) << "}";
            } else if (entry.structuredData.type() == typeid(int)) {
                 ss << " {Data: " << std::any_cast<int>(entry.structuredData) << "}";
            }
            // Add more types as needed for structured data display
        } catch (const std::bad_any_cast& e) {
            ss << " {StructuredData: Opaque/Type Error}";
        }
    }
    return ss.str();
}

void LoggingSystem::outputToConsole(const LogEntry& entry) const {
    // Output to std::cout or std::cerr based on level
    std::ostream& stream = (entry.level == LogLevel::Error || entry.level == LogLevel::Warning) ? std::cerr : std::cout;
    stream << formatLogEntry(entry) << std::endl;
}

void LoggingSystem::outputToFile(const LogEntry& entry) {
    // This method assumes CppLogMutex is already held by the caller (log method)
    if (CppFileLoggingEnabled && CppLogFileStream.is_open()) {
        CppLogFileStream << formatLogEntry(entry) << std::endl;
    }
}

void LoggingSystem::broadcastLogEvent(const LogEntry& entry) {
    // This method assumes CppLogMutex is already held by the caller (log method)
    // Create a copy of callbacks to iterate over, in case a callback tries to unsubscribe
    // or subscribe, which could modify CppLogEventCallbacks.
    // This avoids iterator invalidation issues if callbacks modify the list.
    // However, the PRD for EventBus did not specify such robustness for its callbacks.
    // For logging, it's good practice.
    std::vector<LogEventCallback> callbacks_copy;
    // No, the lock is already held, so copying inside the lock is fine.
    // The issue is if a callback itself calls subscribe/unsubscribe.
    // The current simple vector copy is fine for preventing iterator invalidation
    // during the loop, but doesn't prevent deadlocks if a callback tries to acquire CppLogMutex.
    // For simplicity, we assume callbacks are well-behaved and don't call back into LoggingSystem methods
    // that would re-lock the mutex.
    
    callbacks_copy = CppLogEventCallbacks; // Copy while lock is held.

    // Release lock before calling callbacks? No, PRD implies callbacks are part of the log operation.
    // If callbacks are slow, this holds the lock longer. Alternative is async broadcast.
    // For now, synchronous broadcast as per typical logging system design.

    for (const auto& callback : callbacks_copy) {
        try {
            callback(entry);
        } catch (const std::exception& e) {
            // Log callback exception to stderr directly to avoid re-entering logging system
            std::cerr << "[LoggingSystem] Exception in log event callback: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[LoggingSystem] Unknown exception in log event callback." << std::endl;
        }
    }
}


void LoggingSystem::log(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(CppLogMutex);

    LogLevel categoryLogLevel = CppDefaultLogLevel; // Start with default
    auto it = CppCategoryLogLevels.find(entry.category);
    if (it != CppCategoryLogLevels.end()) {
        categoryLogLevel = it->second;
    } else { // Fallback to "default" category's level if specific category not found
        it = CppCategoryLogLevels.find("default");
        if (it != CppCategoryLogLevels.end()) {
            categoryLogLevel = it->second;
        }
        // If "default" is also not there (shouldn't happen with constructor), CppDefaultLogLevel is used.
    }


    if (entry.level >= categoryLogLevel && categoryLogLevel != LogLevel::None) {
        // Output to console
        outputToConsole(entry);

        // Output to file if enabled
        if (CppFileLoggingEnabled) {
            outputToFile(entry);
        }

        // Broadcast to subscribers
        broadcastLogEvent(entry);
    }
}

void LoggingSystem::setLogLevel(const std::string& category, LogLevel level) {
    std::lock_guard<std::mutex> lock(CppLogMutex);
    if (category.empty()) { // Or handle error for empty category name
        return;
    }
    CppCategoryLogLevels[category] = level;
    if (category == "default") {
        CppDefaultLogLevel = level; // Update the cached default level
    }
}

LogLevel LoggingSystem::getLogLevel(const std::string& category) const {
    std::lock_guard<std::mutex> lock(CppLogMutex); // CppLogMutex needs to be mutable for const methods
    auto it = CppCategoryLogLevels.find(category);
    if (it != CppCategoryLogLevels.end()) {
        return it->second;
    }
    // If specific category not found, return the "default" level
    auto default_it = CppCategoryLogLevels.find("default");
    if (default_it != CppCategoryLogLevels.end()) {
        return default_it->second;
    }
    // Fallback if "default" somehow isn't in the map (should be set by constructor)
    return CppDefaultLogLevel; 
}

void LoggingSystem::subscribeToLogEvents(LogEventCallback callback) {
    std::lock_guard<std::mutex> lock(CppLogMutex);
    CppLogEventCallbacks.push_back(callback);
}

void LoggingSystem::enableFileLogging(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(CppLogMutex);
    if (CppLogFileStream.is_open()) {
        CppLogFileStream.close(); // Close any existing file stream
    }
    CppLogFileStream.open(filePath, std::ios::app); // Open in append mode
    if (CppLogFileStream.is_open()) {
        CppFileLoggingEnabled = true;
        // Optionally log a message indicating file logging has started
        // Be careful: calling log() here would re-lock CppLogMutex causing deadlock.
        // Direct output or a non-locking internal log is needed if logging this action.
        // For now, just set the flag.
    } else {
        CppFileLoggingEnabled = false;
        // Output error to console directly if file can't be opened
        std::cerr << "[LoggingSystem] Error: Could not open log file: " << filePath << std::endl;
    }
}

void LoggingSystem::disableFileLogging() {
    std::lock_guard<std::mutex> lock(CppLogMutex);
    if (CppLogFileStream.is_open()) {
        CppLogFileStream.close();
    }
    CppFileLoggingEnabled = false;
}

} // namespace logging
} // namespace core
} // namespace wave
