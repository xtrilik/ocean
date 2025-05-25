#include "core/configuration/configuration.hpp"
#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono> // For sleep
#include <any>

// Helper function to print test headers
void printTestHeader(const std::string& testName) {
    std::cout << "\n--- " << testName << " ---" << std::endl;
}

// Helper to create a dummy INI file for testing
void createDummyIniFile(const std::string& filePath, const std::string& content) {
    std::ofstream outFile(filePath);
    if (outFile.is_open()) {
        outFile << content;
        outFile.close();
    } else {
        std::cerr << "Failed to create dummy INI file: " << filePath << std::endl;
    }
}

const std::string TEST_INI_PATH = "test_config.ini";
const std::string TEST_INI_MALFORMED_PATH = "test_config_malformed.ini";

void testIniParsingAndGetValue() {
    printTestHeader("INI Parsing and getValue Test");
    std::string iniContent = 
        "[General]\n"
        "appName = WaveEditor\n"
        "version = 1.0\n"
        "; This is a comment\n"
        "settingWithoutValue = \n"
        "\n"
        "[Display]\n"
        "resolution = 1920x1080\n"
        "# Another comment\n"
        "fullscreen = true\n"
        "brightness = 0.75\n";
    createDummyIniFile(TEST_INI_PATH, iniContent);

    wave::core::configuration::ConfigurationSystem config(TEST_INI_PATH);

    // Test existing values
    auto appNameRes = config.getValue("General", "appName");
    assert(appNameRes.success && appNameRes.value.has_value());
    assert(std::any_cast<std::string>(appNameRes.value.value()) == "WaveEditor");

    auto versionRes = config.getValue("General", "version");
    assert(versionRes.success && versionRes.value.has_value());
    assert(std::any_cast<std::string>(versionRes.value.value()) == "1.0");
    
    auto fullscreenRes = config.getValue("Display", "fullscreen");
    assert(fullscreenRes.success && fullscreenRes.value.has_value());
    assert(std::any_cast<std::string>(fullscreenRes.value.value()) == "true"); // Stored as string

    auto brightnessRes = config.getValue("Display", "brightness");
    assert(brightnessRes.success && brightnessRes.value.has_value());
    assert(std::any_cast<std::string>(brightnessRes.value.value()) == "0.75");

    // Test value that might be empty in INI
    auto emptyValRes = config.getValue("General", "settingWithoutValue");
    assert(emptyValRes.success && emptyValRes.value.has_value());
    assert(std::any_cast<std::string>(emptyValRes.value.value()).empty());


    // Test non-existent key
    auto noKeyRes = config.getValue("General", "nonExistentKey");
    assert(!noKeyRes.success && !noKeyRes.value.has_value());
    assert(noKeyRes.message.find("Key not found") != std::string::npos);

    // Test non-existent section
    auto noSectionRes = config.getValue("NonExistentSection", "someKey");
    assert(!noSectionRes.success && !noSectionRes.value.has_value());
    assert(noSectionRes.message.find("Section not found") != std::string::npos);

    std::cout << "INI Parsing and getValue Test: PASSED" << std::endl;
}

void testSetValueAndEvents() {
    printTestHeader("setValue and Events Test");
    wave::core::configuration::ConfigurationSystem config; // Start with empty config
    
    std::atomic<int> eventCount(0);
    std::string lastEventType, lastEventSection, lastEventKey;
    std::string lastEventValueStr;

    config.subscribeToConfigEvents(
        [&](const std::string& eventType, const std::string& section, 
            const std::string& key, const wave::core::configuration::ConfigValue& newValue) {
            eventCount++;
            lastEventType = eventType;
            lastEventSection = section;
            lastEventKey = key;
            if (newValue.has_value()) {
                try {
                    if (newValue.type() == typeid(std::string)) {
                        lastEventValueStr = std::any_cast<std::string>(newValue);
                    } else if (newValue.type() == typeid(int)) {
                        lastEventValueStr = std::to_string(std::any_cast<int>(newValue));
                    } else if (newValue.type() == typeid(const char*)) {
                         lastEventValueStr = std::any_cast<const char*>(newValue);
                    }
                    // Add more types if testing specific conversions in setValue
                } catch (const std::bad_any_cast& e) {
                    lastEventValueStr = "cast_error";
                }
            } else {
                lastEventValueStr = "no_value";
            }
        }
    );

    // Set a new string value
    config.setValue("User", "username", std::string("testuser"));
    assert(eventCount.load() == 1);
    assert(lastEventType == "changed");
    assert(lastEventSection == "User");
    assert(lastEventKey == "username");
    assert(lastEventValueStr == "testuser");

    auto userRes = config.getValue("User", "username");
    assert(userRes.success && std::any_cast<std::string>(userRes.value.value()) == "testuser");

    // Set an integer value (will be stored as string by current setValue)
    config.setValue("Settings", "timeout", 30); // int
    assert(eventCount.load() == 2);
    assert(lastEventType == "changed");
    assert(lastEventSection == "Settings");
    assert(lastEventKey == "timeout");
    assert(lastEventValueStr == "30"); // callback received original int, converted to string for check

    auto timeoutRes = config.getValue("Settings", "timeout");
    assert(timeoutRes.success && std::any_cast<std::string>(timeoutRes.value.value()) == "30");


    // Update an existing value
    config.setValue("User", "username", std::string("anotheruser"));
    assert(eventCount.load() == 3);
    assert(lastEventType == "changed");
    assert(lastEventSection == "User");
    assert(lastEventKey == "username");
    assert(lastEventValueStr == "anotheruser"); // callback received original string

    auto updatedUserRes = config.getValue("User", "username");
    assert(updatedUserRes.success && std::any_cast<std::string>(updatedUserRes.value.value()) == "anotheruser");

    std::cout << "setValue and Events Test: PASSED" << std::endl;
}

void testReloadConfig() {
    printTestHeader("reloadConfig Test");
    std::string initialContent = "[Core]\nstatus = initial\n";
    std::string updatedContent = "[Core]\nstatus = updated\nnewKey = true\n";
    createDummyIniFile(TEST_INI_PATH, initialContent);

    wave::core::configuration::ConfigurationSystem config;
    config.setConfigSource(TEST_INI_PATH); // Set source first

    std::atomic<bool> reloadSuccess(false);
    std::atomic<int> reloadEventCount(0);
    std::string reloadMessage;

    config.subscribeToConfigEvents(
        [&](const std::string& eventType, const std::string&, const std::string&, const wave::core::configuration::ConfigValue&) {
            if (eventType == "reloaded") {
                reloadEventCount++;
            }
        }
    );
    
    // Initial load via reloadConfig
    config.reloadConfig([&](bool success, const std::string& message) {
        reloadSuccess = success;
        reloadMessage = message;
    });

    assert(reloadSuccess.load());
    assert(reloadMessage.find("reloaded successfully") != std::string::npos);
    assert(reloadEventCount.load() == 1); // "reloaded" event

    auto statusRes = config.getValue("Core", "status");
    assert(statusRes.success && std::any_cast<std::string>(statusRes.value.value()) == "initial");

    // Modify INI file and reload
    createDummyIniFile(TEST_INI_PATH, updatedContent);
    reloadSuccess = false; // Reset for next callback
    config.reloadConfig([&](bool success, const std::string& message) {
        reloadSuccess = success;
        reloadMessage = message;
    });
    
    assert(reloadSuccess.load());
    assert(reloadEventCount.load() == 2); // Another "reloaded" event

    statusRes = config.getValue("Core", "status");
    assert(statusRes.success && std::any_cast<std::string>(statusRes.value.value()) == "updated");
    auto newKeyRes = config.getValue("Core", "newKey");
    assert(newKeyRes.success && std::any_cast<std::string>(newKeyRes.value.value()) == "true");

    // Test reload failure (malformed file)
    std::string malformedContent = "[Section\nkey=value"; // Missing closing bracket
    createDummyIniFile(TEST_INI_MALFORMED_PATH, malformedContent);
    config.setConfigSource(TEST_INI_MALFORMED_PATH);
    reloadSuccess = true; // Expect failure
    reloadMessage.clear();

    config.reloadConfig([&](bool success, const std::string& message) {
        reloadSuccess = success;
        reloadMessage = message;
    });
    assert(!reloadSuccess.load());
    assert(reloadMessage.find("Failed to parse") != std::string::npos);
    // Data should ideally remain from last successful load or be cleared,
    // depending on desired behavior. Current implementation keeps old data on parse fail.
    statusRes = config.getValue("Core", "status"); // Should still be "updated" from previous successful load
    assert(statusRes.success && std::any_cast<std::string>(statusRes.value.value()) == "updated");


    // Test reload failure (file not found)
    config.setConfigSource("non_existent_file.ini");
    reloadSuccess = true; // Expect failure
    reloadMessage.clear();
    config.reloadConfig([&](bool success, const std::string& message) {
        reloadSuccess = success;
        reloadMessage = message;
    });
    assert(!reloadSuccess.load());
    assert(reloadMessage.find("Failed to open") != std::string::npos);


    std::cout << "reloadConfig Test: PASSED" << std::endl;
}


void testThreadSafety() {
    printTestHeader("Thread Safety Test (Basic)");
    std::string iniContent = "[Test]\ncounter = 0\n";
    createDummyIniFile(TEST_INI_PATH, iniContent);

    wave::core::configuration::ConfigurationSystem config(TEST_INI_PATH);
    std::atomic<int> eventCallbackCount(0);

    config.subscribeToConfigEvents(
        [&](const std::string&, const std::string&, const std::string&, const wave::core::configuration::ConfigValue&) {
            eventCallbackCount++;
        }
    );

    const int numThreads = 5;
    std::vector<std::thread> threads;

    // Simulate concurrent reads and writes
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&config, i]() {
            for (int j = 0; j < 20; ++j) {
                if (j % 4 == 0) { // Some threads write
                    config.setValue("Test", "counter", std::to_string(i * 100 + j));
                } else { // Others read
                    config.getValue("Test", "counter");
                }
                if (j % 10 == 0 && i == 0) { // One thread occasionally reloads
                    // To make reload more meaningful, we'd need to change the file content concurrently
                    // For simplicity, this reload might just reload the same or last written content.
                    // The main goal is to check for crashes or deadlocks.
                    config.reloadConfig(); 
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Small delay
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
    
    // Final check: get the value and see if it's one of the written values
    auto finalCounter = config.getValue("Test", "counter");
    assert(finalCounter.success); // Should have a value
    std::cout << "Final counter value: " << std::any_cast<std::string>(finalCounter.value.value()) << std::endl;
    std::cout << "Total events triggered: " << eventCallbackCount.load() << std::endl;
    // The exact number of events can be non-deterministic due to race conditions in updates
    // and which setValue call "wins" last for a given key before a read.
    // The key is that it runs without crashing or deadlocking.
    assert(eventCallbackCount.load() > 0); // At least some events should have fired.

    std::cout << "Thread Safety Test (Basic): COMPLETED (check for crashes/deadlocks)" << std::endl;
}


int main() {
    std::cout << "Starting ConfigurationSystem Test Suite..." << std::endl;

    testIniParsingAndGetValue();
    testSetValueAndEvents();
    testReloadConfig();
    testThreadSafety(); // Basic test

    // Cleanup dummy files
    std::remove(TEST_INI_PATH.c_str());
    std::remove(TEST_INI_MALFORMED_PATH.c_str());

    std::cout << "\nConfigurationSystem Test Suite: ALL TESTS COMPLETED." << std::endl;
    return 0;
}
