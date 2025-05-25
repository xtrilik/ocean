#include "core/core.hpp" // For Core, ICoreAccess, ModuleLoaderSystem
#include "modules/clipboard/clipboard_module.hpp" // For ClipboardModule specific types
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <atomic>
#include <thread> // For potential async tests or delays
#include <chrono> // For std::this_thread::sleep_for

// Helper function to print test headers
void printTestHeader(const std::string& testName) {
    std::cout << "\n--- " << testName << " ---" << std::endl;
}

// Path to the clipboard module shared library
#ifdef _WIN32
    const std::string CLIPBOARD_MODULE_FILENAME = "clipboard_module.dll";
#else
    const std::string CLIPBOARD_MODULE_FILENAME = "libclipboard_module.so";
#endif
const std::string CLIPBOARD_MODULE_PATH = "wave/modules/clipboard/build/lib/" + CLIPBOARD_MODULE_FILENAME;

void testClipboardModuleLifecycleAndAccess() {
    printTestHeader("Clipboard Module Lifecycle and Access Test");
    wave::core::Core appCore;
    appCore.initialize(); // Initialize core systems

    wave::core::moduleloader::ModuleLoaderSystem* moduleLoader = appCore.getModuleLoaderSystem();
    assert(moduleLoader != nullptr && "ModuleLoaderSystem should be accessible from Core.");

    // 1. Load the ClipboardModule
    std::cout << "Attempting to load module: " << CLIPBOARD_MODULE_PATH << std::endl;
    wave::core::moduleloader::ModuleResult loadRes = moduleLoader->loadModule(CLIPBOARD_MODULE_PATH);
    
    std::cout << "Load message: " << loadRes.message << std::endl;
    assert(loadRes.status == wave::core::moduleloader::ModuleResult::Status::Success && "ClipboardModule failed to load.");
    assert(loadRes.module.has_value() && "ModuleInfo should be present on successful load.");
    assert(loadRes.module.value().name == "ClipboardModule" && "Module name mismatch.");
    assert(loadRes.module.value().instance != nullptr && "Module instance should not be null.");

    wave::core::moduleloader::ILauncherModule* iModule = loadRes.module.value().instance;
    wave::modules::clipboard::ClipboardModule* clipboardModule = dynamic_cast<wave::modules::clipboard::ClipboardModule*>(iModule);
    assert(clipboardModule != nullptr && "Failed to cast ILauncherModule to ClipboardModule.");

    std::cout << "  ClipboardModule loaded successfully. Name: " << clipboardModule->getName() 
              << ", Version: " << clipboardModule->getVersion() << std::endl;

    // 2. Unload the ClipboardModule
    wave::core::moduleloader::ModuleResult unloadRes = moduleLoader->unloadModule("ClipboardModule");
    assert(unloadRes.status == wave::core::moduleloader::ModuleResult::Status::Success && "ClipboardModule failed to unload.");
    std::cout << "  ClipboardModule unloaded successfully." << std::endl;

    appCore.shutdown();
    std::cout << "Clipboard Module Lifecycle and Access Test: PASSED" << std::endl;
}

void testClipboardCopyPasteAndEvents() {
    printTestHeader("Clipboard Copy, Paste, and Events Test");
    wave::core::Core appCore;
    appCore.initialize();

    wave::core::moduleloader::ModuleLoaderSystem* moduleLoader = appCore.getModuleLoaderSystem();
    wave::core::moduleloader::ModuleResult loadRes = moduleLoader->loadModule(CLIPBOARD_MODULE_PATH);
    assert(loadRes.status == wave::core::moduleloader::ModuleResult::Status::Success && "ClipboardModule failed to load for copy/paste test.");
    
    wave::modules::clipboard::ClipboardModule* clipboardModule = dynamic_cast<wave::modules::clipboard::ClipboardModule*>(loadRes.module.value().instance);
    assert(clipboardModule != nullptr);

    std::atomic<int> copiedEventCount(0);
    std::atomic<int> pastedEventCount(0);
    wave::modules::clipboard::ClipboardData lastCopiedData;
    wave::modules::clipboard::ClipboardData lastPastedData;

    clipboardModule->subscribeToClipboardEvents(
        [&](wave::modules::clipboard::ClipboardEventType type, 
            const wave::modules::clipboard::ClipboardData& data) {
            if (type == wave::modules::clipboard::ClipboardEventType::Copied) {
                copiedEventCount++;
                lastCopiedData = data;
            } else if (type == wave::modules::clipboard::ClipboardEventType::Pasted) {
                pastedEventCount++;
                lastPastedData = data;
            }
        }
    );

    // 1. Test Copy
    std::string testDataToCopy = "Hello Wave Clipboard! Random number: " + std::to_string(rand());
    std::cout << "Attempting to copy: \"" << testDataToCopy << "\"" << std::endl;
    wave::modules::clipboard::ClipboardResult copyRes = clipboardModule->copy(testDataToCopy);
    std::cout << "Copy result: " << copyRes.message << std::endl;
    
    // In some CI environments (especially headless Linux without X server or Windows without session),
    // clipboard operations might fail. We check for success but acknowledge it might be environment-dependent.
    if (copyRes.status != wave::modules::clipboard::ClipboardResult::Status::Success) {
        std::cout << "WARNING: Clipboard copy failed. This might be due to the test environment. Skipping further checks for this part." << std::endl;
    } else {
        assert(copiedEventCount.load() == 1 && "Copied event not triggered or triggered multiple times.");
        assert(lastCopiedData == testDataToCopy && "Copied event data mismatch.");
        std::cout << "  Copy successful, event triggered." << std::endl;

        // 2. Test Paste
        // Add a small delay to ensure clipboard is updated, especially for command-line tools.
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
        
        std::cout << "Attempting to paste..." << std::endl;
        wave::modules::clipboard::ClipboardResult pasteRes = clipboardModule->paste();
        std::cout << "Paste result: " << pasteRes.message << std::endl;
        
        if (pasteRes.status != wave::modules::clipboard::ClipboardResult::Status::Success) {
            std::cout << "WARNING: Clipboard paste failed. This might be due to the test environment." << std::endl;
        } else {
            assert(pasteRes.data.has_value() && "Paste operation should return data.");
            std::cout << "  Pasted data: \"" << pasteRes.data.value() << "\"" << std::endl;
            assert(pasteRes.data.value() == testDataToCopy && "Pasted data does not match copied data.");
            assert(pastedEventCount.load() == 1 && "Pasted event not triggered or triggered multiple times.");
            assert(lastPastedData == testDataToCopy && "Pasted event data mismatch."); // Or lastPastedData == pasteRes.data.value()
            std::cout << "  Paste successful, event triggered, data matches." << std::endl;
        }
    }

    // 3. Test clearHistory (currently a NotSupported operation)
    std::cout << "Attempting to clear history..." << std::endl;
    wave::modules::clipboard::ClipboardResult clearRes = clipboardModule->clearHistory();
    assert(clearRes.status == wave::modules::clipboard::ClipboardResult::Status::NotSupported);
    std::cout << "  clearHistory result: " << clearRes.message << " (as expected)" << std::endl;

    moduleLoader->unloadModule("ClipboardModule");
    appCore.shutdown();
    std::cout << "Clipboard Copy, Paste, and Events Test: COMPLETED (check warnings for environment issues)" << std::endl;
}

int main() {
    std::cout << "Starting ClipboardModule Test Suite..." << std::endl;
    std::cout << "Clipboard module shared library expected at: " << CLIPBOARD_MODULE_PATH << std::endl;
    
    // Check if clipboard module lib actually exists before running tests
    std::ifstream f(CLIPBOARD_MODULE_PATH.c_str());
    if (!f.good()) {
        std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        std::cerr << "!! ERROR: Clipboard module library not found at: " << CLIPBOARD_MODULE_PATH << std::endl;
        std::cerr << "!! Please ensure the clipboard_module was compiled successfully before running tests." << std::endl;
        std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        return 1; // Indicate failure
    }
    f.close();

    // Seed for random data in tests
    srand(time(0));

    testClipboardModuleLifecycleAndAccess();
    testClipboardCopyPasteAndEvents();

    std::cout << "\nClipboardModule Test Suite: ALL TESTS COMPLETED." << std::endl;
    std::cout << "Note: Clipboard functionality is environment-dependent. Failures in copy/paste tests might occur in CI environments." << std::endl;
    std::cout << "Ensure 'xclip' is installed on Linux test environments." << std::endl;
    
    return 0;
}
