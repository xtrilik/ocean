#include "clipboard_module.hpp"
#include "wave/core/logging/logging.hpp" // Required for LogEntry, LogLevel
#include <iostream> // For debugging, remove for production
#include <cstdio>   // For popen, pclose, fgets
#include <memory>   // For std::unique_ptr for popen

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#else
#include <array> // For reading popen output
#endif

namespace wave {
namespace modules {
namespace clipboard {

ClipboardModule::ClipboardModule() 
    : CppCoreAccess(nullptr), 
      CppModuleName("ClipboardModule"), 
      CppModuleVersion("1.0.0") {
    // std::cout << "[ClipboardModule] Constructor." << std::endl;
}

ClipboardModule::~ClipboardModule() {
    // std::cout << "[ClipboardModule] Destructor." << std::endl;
}

void ClipboardModule::initialize(wave::core::moduleloader::ICoreAccess* moduleCoreAccess) {
    // The module loader passes a ICoreAccess defined within its own scope (dummy one).
    // We need to cast it to the main ICoreAccess (from wave/include) to access full core features.
    CppCoreAccess = dynamic_cast<wave::ICoreAccess*>(moduleCoreAccess);

    if (!CppCoreAccess && moduleCoreAccess) {
        // This means the passed pointer was not of the expected wave::Core type (or compatible).
        // Log an error if possible, though logging system itself might not be accessible if CppCoreAccess is null.
        std::cerr << "[ClipboardModule] CRITICAL ERROR: Failed to dynamic_cast ICoreAccess. Module will not function correctly." << std::endl;
        // Depending on policy, could throw or set an internal error state.
    }

    // if (CppCoreAccess && CppCoreAccess->getLoggingSystem()) { // Now CppCoreAccess is wave::ICoreAccess*
    //     CppCoreAccess->getLoggingSystem()->log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Info, CppModuleName, "ClipboardModule initialized."));
    // }
    // std::cout << "[ClipboardModule] Initialized." << std::endl;
}

void ClipboardModule::shutdown() {
    // if (CppCoreAccess && CppCoreAccess->getLoggingSystem()) {
    //     CppCoreAccess->getLoggingSystem()->log(wave::core::logging::LogEntry(wave::core::logging::LogLevel::Info, CppModuleName, "ClipboardModule shutdown."));
    // }
    // std::cout << "[ClipboardModule] Shutdown." << std::endl;
}

std::string ClipboardModule::getName() const {
    return CppModuleName;
}

std::string ClipboardModule::getVersion() const {
    return CppModuleVersion;
}

ClipboardResult ClipboardModule::copy(const ClipboardData& data) {
    std::lock_guard<std::mutex> lock(CppModuleMutex);
    ClipboardResult result = platformCopy(data);
    if (result.status == ClipboardResult::Status::Success) {
        broadcastEvent(ClipboardEventType::Copied, data);
    }
    return result;
}

ClipboardResult ClipboardModule::paste() {
    std::lock_guard<std::mutex> lock(CppModuleMutex);
    ClipboardResult result = platformPaste();
    if (result.status == ClipboardResult::Status::Success && result.data.has_value()) {
        broadcastEvent(ClipboardEventType::Pasted, result.data.value());
    }
    return result;
}

ClipboardResult ClipboardModule::clearHistory() {
    std::lock_guard<std::mutex> lock(CppModuleMutex);
    // Basic implementation: No actual history maintained in this version.
    // CppClipboardHistory.clear(); // If history was implemented
    broadcastEvent(ClipboardEventType::HistoryCleared, ""); // Broadcast with empty data
    return ClipboardResult(ClipboardResult::Status::NotSupported, "Clipboard history feature is not implemented in this version.");
}

void ClipboardModule::subscribeToClipboardEvents(ClipboardEventCallback callback) {
    std::lock_guard<std::mutex> lock(CppModuleMutex);
    CppEventCallbacks.push_back(callback);
}

void ClipboardModule::broadcastEvent(ClipboardEventType type, const ClipboardData& eventData) {
    // Assumes CppModuleMutex is already held by the calling public method.
    std::vector<ClipboardEventCallback> callbacks_copy = CppEventCallbacks; // Copy for safe iteration
    
    for (const auto& callback : callbacks_copy) {
        try {
            callback(type, eventData);
        } catch (const std::exception& e) {
            // Handle callback error (e.g., log via CoreAccess if available and safe)
            // std::cerr << "Exception in ClipboardEventCallback: " << e.what() << std::endl;
             if (CppCoreAccess && CppCoreAccess->getLoggingSystem()) {
                 CppCoreAccess->getLoggingSystem()->log(wave::core::logging::LogEntry(
                     wave::core::logging::LogLevel::Error, CppModuleName, 
                     "Exception in clipboard event callback: " + std::string(e.what())));
             }
        } catch (...) {
            // std::cerr << "Unknown exception in ClipboardEventCallback." << std::endl;
            if (CppCoreAccess && CppCoreAccess->getLoggingSystem()) {
                 CppCoreAccess->getLoggingSystem()->log(wave::core::logging::LogEntry(
                     wave::core::logging::LogLevel::Error, CppModuleName, 
                     "Unknown exception in clipboard event callback."));
            }
        }
    }
}

// --- Platform-specific implementations ---

#ifdef _WIN32
ClipboardResult ClipboardModule::platformCopy(const ClipboardData& data) {
    if (!OpenClipboard(nullptr)) {
        return ClipboardResult(ClipboardResult::Status::Error, "Cannot open clipboard (WinAPI).");
    }
    EmptyClipboard();
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, data.size() + 1);
    if (!hg) {
        CloseClipboard();
        return ClipboardResult(ClipboardResult::Status::Error, "GlobalAlloc failed (WinAPI).");
    }
    memcpy(GlobalLock(hg), data.c_str(), data.size() + 1);
    GlobalUnlock(hg);
    SetClipboardData(CF_TEXT, hg);
    CloseClipboard();
    // GlobalFree(hg); // System owns it after SetClipboardData.
    return ClipboardResult(ClipboardResult::Status::Success, "Text copied to clipboard (WinAPI).");
}

ClipboardResult ClipboardModule::platformPaste() {
    if (!OpenClipboard(nullptr)) {
        return ClipboardResult(ClipboardResult::Status::Error, "Cannot open clipboard (WinAPI).");
    }
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == nullptr) {
        CloseClipboard();
        return ClipboardResult(ClipboardResult::Status::Error, "Cannot get clipboard data (WinAPI).", ""); // Return empty data
    }
    char* pszText = static_cast<char*>(GlobalLock(hData));
    if (pszText == nullptr) {
        CloseClipboard();
        return ClipboardResult(ClipboardResult::Status::Error, "GlobalLock failed (WinAPI).", "");
    }
    std::string text(pszText);
    GlobalUnlock(hData);
    CloseClipboard();
    return ClipboardResult(ClipboardResult::Status::Success, "Text pasted from clipboard (WinAPI).", text);
}

#else // Linux and macOS (using command-line tools)

// Helper function to execute a command and get its output (for paste) or send input (for copy)
std::string execCommand(const char* cmd, const std::string* inputData = nullptr) {
    std::string result;
    FILE* pipe = nullptr;

    if (inputData) { // For copy: write inputData to the command's stdin
        pipe = popen(cmd, "w");
        if (!pipe) return "ERROR_POPEN_W";
        fwrite(inputData->c_str(), 1, inputData->length(), pipe);
    } else { // For paste: read output from the command's stdout
        pipe = popen(cmd, "r");
        if (!pipe) return "ERROR_POPEN_R";
        std::array<char, 128> buffer;
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
    }

    int returnCode = pclose(pipe);
    if (returnCode != 0) {
        // Error during pclose or command execution itself returned non-zero
        // For paste, result might contain partial data or error messages from the tool.
        // For copy, this indicates the copy command might have failed.
        if (inputData) return "ERROR_CMD_FAILED_W"; // Copy command failed
        // For paste, we might still have some data, but also an error.
        // If result is empty, it's a clear error. If not, it's ambiguous without parsing stderr.
        // For simplicity, if command fails and result is empty, it's an error.
        if (result.empty()) return "ERROR_CMD_FAILED_R"; 
    }
    return result;
}


ClipboardResult ClipboardModule::platformCopy(const ClipboardData& data) {
    std::string cmd;
#ifdef __APPLE__
    cmd = "pbcopy";
#else // Linux
    cmd = "xclip -selection clipboard -in";
    // Check if xclip is available? Could log a warning if command fails.
#endif
    std::string res = execCommand(cmd.c_str(), &data);
    if (res.find("ERROR_") == 0) { // Check for our internal error prefixes
        return ClipboardResult(ClipboardResult::Status::Error, "Failed to copy using command-line tool: " + res);
    }
    return ClipboardResult(ClipboardResult::Status::Success, "Text copied using " + cmd + ".");
}

ClipboardResult ClipboardModule::platformPaste() {
    std::string cmd;
#ifdef __APPLE__
    cmd = "pbpaste";
#else // Linux
    cmd = "xclip -selection clipboard -out";
#endif
    std::string pastedText = execCommand(cmd.c_str());
    if (pastedText.find("ERROR_") == 0) { // Check for our internal error prefixes
         return ClipboardResult(ClipboardResult::Status::Error, "Failed to paste using command-line tool: " + pastedText, "");
    }
    // Trim trailing newline typically added by xclip -o or pbpaste
    if (!pastedText.empty() && pastedText.back() == '\n') {
        pastedText.pop_back();
    }
    return ClipboardResult(ClipboardResult::Status::Success, "Text pasted using " + cmd + ".", pastedText);
}
#endif // _WIN32 / POSIX

} // namespace clipboard
} // namespace modules
} // namespace wave


// Exported C functions for dynamic loading
extern "C" {
    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    wave::core::moduleloader::ILauncherModule* create_module_instance() {
        return new wave::modules::clipboard::ClipboardModule();
    }

    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    void destroy_module_instance(wave::core::moduleloader::ILauncherModule* moduleInstance) {
        delete static_cast<wave::modules::clipboard::ClipboardModule*>(moduleInstance);
    }
}
