#include "core/moduleloader/module_loader.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono> // For sleep
#include <cstdio> // For std::remove to clean up dummy module if copied

// Helper function to print test headers
void printTestHeader(const std::string& testName) {
    std::cout << "\n--- " << testName << " ---" << std::endl;
}

// Dummy ICoreAccess implementation for testing
class DummyCoreAccess : public wave::core::moduleloader::ICoreAccess {
public:
    void placeholder() const override {
        // std::cout << "[DummyCoreAccess] Placeholder method called." << std::endl;
    }
    // Add any methods modules might actually call if necessary for deeper testing
};

// Path to the dummy module (adjust if necessary based on build environment)
// Assumes tests are run from the root directory of the project (/app)
#ifdef _WIN32
    const std::string DUMMY_MODULE_FILENAME = "dummy_module.dll";
#else
    const std::string DUMMY_MODULE_FILENAME = "libdummy_module.so";
#endif
const std::string DUMMY_MODULE_PATH = "wave/tests/dummy_module/build/lib/" + DUMMY_MODULE_FILENAME;
const std::string NON_EXISTENT_MODULE_PATH = "wave/tests/dummy_module/build/lib/non_existent_module.so";


void testModuleLoadUnloadList() {
    printTestHeader("Module Load, Unload, and List Test");
    DummyCoreAccess coreAccess;
    wave::core::moduleloader::ModuleLoaderSystem loader(&coreAccess);

    // 1. Test Loading a valid module
    std::cout << "Attempting to load module: " << DUMMY_MODULE_PATH << std::endl;
    wave::core::moduleloader::ModuleResult loadRes = loader.loadModule(DUMMY_MODULE_PATH);

    std::cout << "Load message: " << loadRes.message << std::endl;
    assert(loadRes.status == wave::core::moduleloader::ModuleResult::Status::Success);
    assert(loadRes.module.has_value());
    assert(loadRes.module.value().name == "DummyModule");
    assert(loadRes.module.value().version == "1.0.0");
    assert(loadRes.module.value().path == DUMMY_MODULE_PATH);
    assert(loadRes.module.value().instance != nullptr);
    assert(loadRes.module.value().libraryHandle != nullptr);

    std::string loadedModuleName = loadRes.module.value().name;

    // 2. Test Listing modules
    std::vector<wave::core::moduleloader::ModuleInfo> modules = loader.listModules();
    assert(modules.size() == 1);
    assert(modules[0].name == loadedModuleName);
    assert(modules[0].path == DUMMY_MODULE_PATH);

    // 3. Test loading the same module path again (should fail)
    wave::core::moduleloader::ModuleResult loadAgainRes = loader.loadModule(DUMMY_MODULE_PATH);
    assert(loadAgainRes.status == wave::core::moduleloader::ModuleResult::Status::Error);
    assert(loadAgainRes.message.find("already loaded") != std::string::npos);

    // 4. Test Unloading the module
    wave::core::moduleloader::ModuleResult unloadRes = loader.unloadModule(loadedModuleName);
    assert(unloadRes.status == wave::core::moduleloader::ModuleResult::Status::Success);
    assert(unloadRes.module.has_value()); // Should contain info of the unloaded module
    assert(unloadRes.module.value().name == loadedModuleName);

    modules = loader.listModules();
    assert(modules.empty());

    // 5. Test Unloading a non-existent module
    wave::core::moduleloader::ModuleResult unloadNonExistentRes = loader.unloadModule("NonExistentModule");
    assert(unloadNonExistentRes.status == wave::core::moduleloader::ModuleResult::Status::NotFound);

    std::cout << "Module Load, Unload, and List Test: PASSED" << std::endl;
}

void testModuleEvents() {
    printTestHeader("Module Event Subscription Test");
    DummyCoreAccess coreAccess;
    wave::core::moduleloader::ModuleLoaderSystem loader(&coreAccess);

    std::atomic<int> loadedEvents(0);
    std::atomic<int> unloadedEvents(0);
    std::atomic<int> errorEvents(0);
    wave::core::moduleloader::ModuleInfo lastEventModuleInfo;
    std::string lastEventMessage;

    loader.subscribeToModuleEvents(
        [&](wave::core::moduleloader::ModuleEventType type, 
            const wave::core::moduleloader::ModuleInfo& info, 
            const std::string& message) {
            lastEventModuleInfo = info; // Be careful with lifetime if info.instance is accessed
            lastEventMessage = message;
            if (type == wave::core::moduleloader::ModuleEventType::Loaded) {
                loadedEvents++;
            } else if (type == wave::core::moduleloader::ModuleEventType::Unloaded) {
                unloadedEvents++;
            } else if (type == wave::core::moduleloader::ModuleEventType::ErrorLoading || type == wave::core::moduleloader::ModuleEventType::ErrorUnloading) {
                errorEvents++;
            }
        }
    );

    // Test Loaded event
    wave::core::moduleloader::ModuleResult loadRes = loader.loadModule(DUMMY_MODULE_PATH);
    assert(loadRes.status == wave::core::moduleloader::ModuleResult::Status::Success);
    assert(loadedEvents.load() == 1);
    assert(unloadedEvents.load() == 0);
    assert(errorEvents.load() == 0);
    assert(lastEventModuleInfo.name == "DummyModule");
    assert(lastEventMessage.find("loaded successfully") != std::string::npos);

    std::string moduleName = loadRes.module.value().name;

    // Test Unloaded event
    loader.unloadModule(moduleName);
    assert(loadedEvents.load() == 1);
    assert(unloadedEvents.load() == 1);
    assert(errorEvents.load() == 0);
    assert(lastEventModuleInfo.name == moduleName); // Info of the unloaded module
    assert(lastEventMessage.find("unloaded successfully") != std::string::npos);

    // Test ErrorLoading event
    loader.loadModule(NON_EXISTENT_MODULE_PATH);
    assert(errorEvents.load() == 1); // Total error events
    assert(lastEventModuleInfo.path == NON_EXISTENT_MODULE_PATH); // Info might contain path
    assert(lastEventMessage.find("Failed to load library") != std::string::npos);
    
    std::cout << "Module Event Subscription Test: PASSED" << std::endl;
}

void testModuleReload() {
    printTestHeader("Module Reload Test");
    DummyCoreAccess coreAccess;
    wave::core::moduleloader::ModuleLoaderSystem loader(&coreAccess);

    std::atomic<int> loadedCount(0), unloadedCount(0), reloadedCount(0), errorCount(0);
    std::string last_module_name_event;

    loader.subscribeToModuleEvents(
        [&](wave::core::moduleloader::ModuleEventType type, 
            const wave::core::moduleloader::ModuleInfo& info, 
            const std::string&) {
            last_module_name_event = info.name;
            switch(type) {
                case wave::core::moduleloader::ModuleEventType::Loaded: loadedCount++; break;
                case wave::core::moduleloader::ModuleEventType::Unloaded: unloadedCount++; break;
                case wave::core::moduleloader::ModuleEventType::Reloaded: reloadedCount++; break;
                case wave::core::moduleloader::ModuleEventType::ErrorLoading:
                case wave::core::moduleloader::ModuleEventType::ErrorUnloading: errorCount++; break;
            }
        }
    );

    // 1. Load the module first
    wave::core::moduleloader::ModuleResult loadRes = loader.loadModule(DUMMY_MODULE_PATH);
    assert(loadRes.status == wave::core::moduleloader::ModuleResult::Status::Success);
    assert(loadedCount.load() == 1);
    std::string moduleName = loadRes.module.value().name;

    // 2. Reload the module
    std::cout << "Attempting to reload module: " << moduleName << std::endl;
    wave::core::moduleloader::ModuleResult reloadRes = loader.reloadModule(moduleName);
    std::cout << "Reload message: " << reloadRes.message << std::endl;

    assert(reloadRes.status == wave::core::moduleloader::ModuleResult::Status::Success);
    assert(reloadRes.module.has_value());
    assert(reloadRes.module.value().name == moduleName); // Should be the same module (new instance)
    
    // Check events: Original load (1), then Reload (1)
    // The internalUnloadModule during reload is called with isReloading=true, so it shouldn't fire Unloaded.
    // The loadModule after that should fire Loaded.
    // Then reloadModule itself fires Reloaded.
    // So, Loaded: 1 (initial) + 1 (internal to reload) = 2
    // Unloaded: 0 (internal to reload is suppressed for this event type)
    // Reloaded: 1
    // This depends on the exact event logic in reloadModule and internalUnloadModule.
    // My current reloadModule:
    //   - public unloadModule (fires Unloaded) -> this is not what I wrote in module_loader.cpp, I used unlock/lock.
    //   - public loadModule (fires Loaded)
    //   - then broadcasts Reloaded.
    // Let's re-check module_loader.cpp for reloadModule:
    //   `CppModuleMutex.unlock(); loadRes = loadModule(modulePath); CppModuleMutex.lock();`
    //   `internalUnloadModule(moduleName, true)` was what I had *before* the deadlock fix.
    //   The current code calls public `loadModule` and (implicitly before that, by finding path) `internalUnloadModule`.
    //   No, `reloadModule` calls `internalUnloadModule(moduleName, true)` IF module found.
    //   Then it unlocks, calls `loadModule` (public), re-locks, then broadcasts `Reloaded`.
    //   So `internalUnloadModule(..., true)` means no "Unloaded" event.
    //   `loadModule(public)` means "Loaded" event.
    //   Then "Reloaded" event.
    // So:
    // Initial load: 1 Loaded event.
    // Reload: 1 Loaded event (from public loadModule call), 1 Reloaded event. No Unloaded.
    assert(loadedCount.load() == 2); // Initial load + load during reload
    assert(unloadedCount.load() == 0); // internalUnloadModule(..., true) suppresses Unloaded event
    assert(reloadedCount.load() == 1);
    assert(errorCount.load() == 0);
    assert(last_module_name_event == moduleName);


    // 3. Check if module is still listed
    auto modules = loader.listModules();
    assert(modules.size() == 1);
    assert(modules[0].name == moduleName);

    // 4. Reload a non-existent module
    wave::core::moduleloader::ModuleResult reloadNonExistentRes = loader.reloadModule("NonExistentForReload");
    assert(reloadNonExistentRes.status == wave::core::moduleloader::ModuleResult::Status::NotFound);
    assert(reloadedCount.load() == 1); // No new reloaded event
    assert(errorCount.load() == 0); // NotFound is not an error event from ErrorLoading/ErrorUnloading types

    // Clean up
    loader.unloadModule(moduleName);
    std::cout << "Module Reload Test: PASSED" << std::endl;
}

void testErrorConditions() {
    printTestHeader("Error Conditions Test");
    DummyCoreAccess coreAccess;
    wave::core::moduleloader::ModuleLoaderSystem loader(&coreAccess);

    // 1. Load non-existent module file
    wave::core::moduleloader::ModuleResult res = loader.loadModule(NON_EXISTENT_MODULE_PATH);
    assert(res.status == wave::core::moduleloader::ModuleResult::Status::Error);
    assert(res.message.find("Failed to load library") != std::string::npos);
    assert(!res.module.has_value() || res.module.value().path == NON_EXISTENT_MODULE_PATH);

    // 2. Load a file that is not a valid module (e.g., a text file)
    // This requires creating such a file. For now, this is harder to automate robustly.
    // Skipped for this iteration, but important for full testing.
    // Would likely fail at GetProcAddress/dlsym or when calling create_module_instance.

    // 3. Unload module that was never loaded
    res = loader.unloadModule("NeverLoadedModule");
    assert(res.status == wave::core::moduleloader::ModuleResult::Status::NotFound);

    std::cout << "Error Conditions Test: PASSED (basic cases)" << std::endl;
}


void testThreadSafety() {
    printTestHeader("Thread Safety Test (Basic)");
    DummyCoreAccess coreAccess;
    wave::core::moduleloader::ModuleLoaderSystem loader(&coreAccess);
    
    std::atomic<int> successfulLoads(0);
    std::atomic<int> successfulUnloads(0);
    std::atomic<int> eventCount(0);

    loader.subscribeToModuleEvents(
        [&](wave::core::moduleloader::ModuleEventType, 
            const wave::core::moduleloader::ModuleInfo&, 
            const std::string&) {
            eventCount++;
        }
    );

    const int numThreads = 5; // Reduced for faster test, can be increased
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&, i]() {
            // Each thread tries to load, then unload the module.
            // To make them operate on distinct "modules" if module names need to be unique,
            // we'd need multiple dummy module files or a module that can change its name.
            // The current DummyModule always has the same name.
            // So, only one thread will successfully load "DummyModule" at a time.
            // Others will fail to load if it's already loaded by name.
            // This tests concurrent access to the load/unload logic and registry.

            // Let's try loading the same module path. Only one should succeed.
            wave::core::moduleloader::ModuleResult lRes = loader.loadModule(DUMMY_MODULE_PATH);
            if (lRes.status == wave::core::moduleloader::ModuleResult::Status::Success) {
                successfulLoads++;
                std::string moduleName = lRes.module.value().name;
                std::this_thread::sleep_for(std::chrono::milliseconds(10 * i)); // Stagger unloads
                
                wave::core::moduleloader::ModuleResult uRes = loader.unloadModule(moduleName);
                if (uRes.status == wave::core::moduleloader::ModuleResult::Status::Success) {
                    successfulUnloads++;
                }
            } else {
                // It might fail if another thread loaded it (by path or by name if name is fixed)
                // Or if the file path was wrong, etc.
                 std::cout << "[Thread " << i << "] Load failed: " << lRes.message << std::endl;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Thread Safety Test: Successful loads: " << successfulLoads.load() << std::endl;
    std::cout << "Thread Safety Test: Successful unloads: " << successfulUnloads.load() << std::endl;
    std::cout << "Thread Safety Test: Total events: " << eventCount.load() << std::endl;

    // Because the dummy module always has the name "DummyModule", and module paths are also unique for loading,
    // only one thread can successfully load the module *by that path* at a time.
    // And only one thread can have "DummyModule" loaded *by that name*.
    // The current logic:
    // - loadModule(path) checks if path is already loaded.
    // - loadModule(path) then checks if moduleInstance->getName() is already loaded.
    // So, only the first thread to pass both checks will load.
    assert(successfulLoads.load() == 1); 
    assert(successfulUnloads.load() == 1);
    
    // Events: 1 Load, 1 Unload from the successful thread.
    // If other threads failed to load, they might trigger ErrorLoading events *if* the error
    // occurs after the point where an event is broadcast (e.g. file not found error).
    // The "module from this path is already loaded" error does not broadcast an event in current code.
    // The "Module with name ... already loaded" error *does* broadcast ErrorLoading.
    // So, (numThreads - 1) ErrorLoading events + 1 Loaded + 1 Unloaded = numThreads + 1 events.
    // This needs to be precise based on implementation.

    // Re-checking loadModule:
    // 1. Path check: `return ModuleResult(ModuleResult::Status::Error, "Module from this path is already loaded: " + modulePath, pair.second);` -> NO EVENT
    // 2. Name check: `broadcastEvent(ModuleEventType::ErrorLoading, errorInfo, errorMsg); return ModuleResult(...)` -> EVENT
    // So, the first thread loads. Other N-1 threads will hit the "path already loaded" error if they try to load *before* the first one unloads.
    // If they try to load *after* the first one unloads, one of them will succeed.
    // This test is a bit racy for exact event counts.
    // The key is that it doesn't crash and the final state is consistent (no modules loaded).
    
    assert(loader.listModules().empty()); // Ensure all modules are unloaded eventually.
    std::cout << "Thread Safety Test (Basic): COMPLETED (check for crashes/deadlocks)" << std::endl;
}


int main() {
    std::cout << "Starting ModuleLoaderSystem Test Suite..." << std::endl;
    std::cout << "Dummy module shared library expected at: " << DUMMY_MODULE_PATH << std::endl;
    
    // Check if dummy module lib actually exists before running tests
    std::ifstream f(DUMMY_MODULE_PATH.c_str());
    if (!f.good()) {
        std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        std::cerr << "!! ERROR: Dummy module library not found at: " << DUMMY_MODULE_PATH << std::endl;
        std::cerr << "!! Please ensure the dummy_module was compiled successfully before running tests." << std::endl;
        std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        // It might be useful to `ls` the directory here if possible from C++ for more diagnostics,
        // but system calls are outside typical test suite actions.
        // For now, just this error.
        return 1; // Indicate failure
    }
    f.close();


    testModuleLoadUnloadList();
    testModuleEvents();
    testModuleReload();
    testErrorConditions();
    testThreadSafety();

    std::cout << "\nModuleLoaderSystem Test Suite: ALL TESTS COMPLETED." << std::endl;
    return 0;
}
