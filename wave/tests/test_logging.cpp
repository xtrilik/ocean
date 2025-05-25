#include "core/logging/logging.hpp"
#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono> // For sleep
#include <sstream> // For capturing cout/cerr (optional, can be complex)
#include <cstdio>  // For std::remove with file paths

// Helper function to print test headers
void printTestHeader(const std::string& testName) {
    std::cout << "\n--- " << testName << " ---" << std::endl;
}

const std::string TEST_LOG_FILE_PATH = "test_app.log";

// Helper to check if file contains a string
bool fileContainsString(const std::string& filePath, const std::string& searchString) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for searching: " << filePath << std::endl;
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(searchString) != std::string::npos) {
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}


void testBasicLoggingAndLevels() {
    printTestHeader("Basic Logging and Log Level Filtering Test");
    wave::core::logging::LoggingSystem logger;

    // Default level is Info. Debug messages should not appear unless level is changed.
    std::cout << "Testing with default log level (INFO)..." << std::endl;
    logger.log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Debug, "TestCategory", "This is a DEBUG message. (Should not be visible by default)"));
    logger.log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Info, "TestCategory", "This is an INFO message. (Should be visible)"));
    logger.log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Warning, "TestCategory", "This is a WARNING message. (Should be visible)"));
    logger.log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Error, "TestCategory", "This is an ERROR message. (Should be visible)"));

    std::cout << "\nSetting log level for 'TestCategory' to DEBUG..." << std::endl;
    logger.setLogLevel("TestCategory", wave::core::logging::LogLevel::Debug);
    assert(logger.getLogLevel("TestCategory") == wave::core::logging::LogLevel::Debug);
    logger.log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Debug, "TestCategory", "This DEBUG message for TestCategory should now be visible."));

    std::cout << "\nSetting log level for 'TestCategory' to WARNING..." << std::endl;
    logger.setLogLevel("TestCategory", wave::core::logging::LogLevel::Warning);
    assert(logger.getLogLevel("TestCategory") == wave::core::logging::LogLevel::Warning);
    logger.log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Info, "TestCategory", "This INFO message for TestCategory should NOT be visible now."));
    logger.log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Warning, "TestCategory", "This WARNING message for TestCategory should be visible."));

    std::cout << "\nTesting 'default' category log level..." << std::endl;
    logger.setLogLevel("default", wave::core::logging::LogLevel::Error);
    assert(logger.getLogLevel("default") == wave::core::logging::LogLevel::Error);
    assert(logger.getLogLevel("AnotherCategory") == wave::core::logging::LogLevel::Error); // Inherits from default
    logger.log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Info, "AnotherCategory", "INFO for AnotherCategory (Should NOT be visible due to default ERROR)."));
    logger.log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Error, "AnotherCategory", "ERROR for AnotherCategory (Should be visible)."));
    
    std::cout << "\nSetting log level for 'TestCategory' to NONE..." << std::endl;
    logger.setLogLevel("TestCategory", wave::core::logging::LogLevel::None);
    assert(logger.getLogLevel("TestCategory") == wave::core::logging::LogLevel::None);
    logger.log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Error, "TestCategory", "This ERROR message for TestCategory should NOT be visible (Level NONE)."));


    // Reset default for other tests
    logger.setLogLevel("default", wave::core::logging::LogLevel::Info);
    std::cout << "Basic Logging and Log Level Filtering Test: PASSED (visual check of console output)" << std::endl;
}

void testLogEventSubscription() {
    printTestHeader("Log Event Subscription Test");
    wave::core::logging::LoggingSystem logger;
    logger.setLogLevel("EventTest", wave::core::logging::LogLevel::Debug);

    std::atomic<int> eventCount(0);
    wave::core::logging::LogEntry lastEntry(wave::core::logging::LogLevel::Debug, "", ""); // Dummy init

    logger.subscribeToLogEvents([&](const wave::core::logging::LogEntry& entry) {
        eventCount++;
        lastEntry = entry;
    });

    wave::core::logging::LogEntry testEntry1(wave::core::logging::LogLevel::Info, "EventTest", "First event message");
    logger.log(testEntry1);
    assert(eventCount.load() == 1);
    assert(lastEntry.message == "First event message");
    assert(lastEntry.category == "EventTest");
    assert(lastEntry.level == wave::core::logging::LogLevel::Info);

    // This message should be filtered out by log level, so no event
    logger.setLogLevel("EventTest", wave::core::logging::LogLevel::Warning);
    wave::core::logging::LogEntry testEntry2(wave::core::logging::LogLevel::Info, "EventTest", "Second event message (filtered)");
    logger.log(testEntry2);
    assert(eventCount.load() == 1); // Count should not change

    wave::core::logging::LogEntry testEntry3(wave::core::logging::LogLevel::Warning, "EventTest", "Third event message (not filtered)", std::string("StructuredDataHere"));
    logger.log(testEntry3);
    assert(eventCount.load() == 2);
    assert(lastEntry.message == "Third event message (not filtered)");
    assert(lastEntry.structuredData.has_value());
    try {
        assert(std::any_cast<std::string>(lastEntry.structuredData) == "StructuredDataHere");
    } catch(const std::bad_any_cast& e) {
        assert(false && "Bad any_cast for structuredData in test");
    }


    std::cout << "Log Event Subscription Test: PASSED" << std::endl;
}

void testFileLogging() {
    printTestHeader("File Logging Test");
    // Clean up log file from previous runs if it exists
    std::remove(TEST_LOG_FILE_PATH.c_str());

    wave::core::logging::LoggingSystem logger;
    logger.setLogLevel("FileTest", wave::core::logging::LogLevel::Debug);

    logger.enableFileLogging(TEST_LOG_FILE_PATH);
    
    std::string msg1 = "Message 1 for file logging.";
    std::string msg2 = "Message 2 with DEBUG level for file.";
    std::string msg3 = "Message 3 filtered out for file.";

    logger.log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Info, "FileTest", msg1));
    logger.log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Debug, "FileTest", msg2));
    
    logger.setLogLevel("FileTest", wave::core::logging::LogLevel::Info); // Change level
    logger.log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Debug, "FileTest", msg3));

    logger.disableFileLogging(); // Important to flush and close before checking

    // Check file content
    assert(fileContainsString(TEST_LOG_FILE_PATH, msg1));
    assert(fileContainsString(TEST_LOG_FILE_PATH, msg2));
    assert(!fileContainsString(TEST_LOG_FILE_PATH, msg3)); // Should be filtered

    // Test logging after disabling file logging
    logger.log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Info, "FileTest", "This message should not be in file."));
    assert(!fileContainsString(TEST_LOG_FILE_PATH, "This message should not be in file."));
    
    // Test re-enabling
    logger.enableFileLogging(TEST_LOG_FILE_PATH); // Re-opens or appends
    std::string msg4 = "Message 4 after re-enabling file logging.";
    logger.log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Info, "FileTest", msg4));
    logger.disableFileLogging();
    assert(fileContainsString(TEST_LOG_FILE_PATH, msg4));


    std::cout << "File Logging Test: PASSED" << std::endl;
    // Clean up
    std::remove(TEST_LOG_FILE_PATH.c_str());
}


void testThreadSafety() {
    printTestHeader("Thread Safety Test (Basic)");
    wave::core::logging::LoggingSystem logger;
    logger.setLogLevel("default", wave::core::logging::LogLevel::Debug); // Log everything for this test
    logger.enableFileLogging(TEST_LOG_FILE_PATH); // Include file I/O in thread safety test

    std::atomic<int> eventCount(0);
    logger.subscribeToLogEvents([&](const wave::core::logging::LogEntry&) {
        eventCount++;
    });

    const int numThreads = 10;
    const int messagesPerThread = 50;
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&logger, i, messagesPerThread]() {
            for (int j = 0; j < messagesPerThread; ++j) {
                std::string message = "Thread " + std::to_string(i) + " message " + std::to_string(j);
                wave::core::logging::LogLevel level = static_cast<wave::core::logging::LogLevel>(j % 4); // Cycle through levels
                logger.log(wave::core::logging::LogEntry(level, "ThreadTest", message, i * 1000 + j));
                
                if (j % 10 == 0) { // Occasionally change log levels
                    logger.setLogLevel("Category" + std::to_string(i), static_cast<wave::core::logging::LogLevel>((j / 10) % 4));
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    logger.disableFileLogging(); // Close file before checking

    std::cout << "Thread Safety Test: Total log events processed: " << eventCount.load() << std::endl;
    // Expected events: numThreads * messagesPerThread, if all levels were >= Debug
    // Since levels cycle and default is Debug, all messages should be logged and generate events.
    assert(eventCount.load() == numThreads * messagesPerThread);

    // Basic check: ensure the log file was created and has some content.
    // More thorough checks would involve counting lines or specific messages,
    // but that's more complex if message order isn't guaranteed.
    std::ifstream logFile(TEST_LOG_FILE_PATH);
    assert(logFile.is_open());
    logFile.seekg(0, std::ios::end);
    assert(logFile.tellg() > 0); // File is not empty
    logFile.close();

    std::cout << "Thread Safety Test (Basic): COMPLETED (check for crashes, deadlocks, and event count)" << std::endl;
    std::remove(TEST_LOG_FILE_PATH.c_str());
}

int main() {
    std::cout << "Starting LoggingSystem Test Suite..." << std::endl;

    testBasicLoggingAndLevels();
    testLogEventSubscription();
    testFileLogging();
    testThreadSafety();

    std::cout << "\nLoggingSystem Test Suite: ALL TESTS COMPLETED." << std::endl;
    std::cout << "Note: Some tests rely on visual inspection of console output for full verification." << std::endl;
    
    return 0;
}
