#ifndef WAVE_MODULES_CLIPBOARD_CLIPBOARD_MODULE_HPP
#define WAVE_MODULES_CLIPBOARD_CLIPBOARD_MODULE_HPP

#include "wave/core/moduleloader/module_loader.hpp" // For ILauncherModule
#include "wave/include/ICoreAccess.hpp" // For ICoreAccess
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <mutex>
#include <any> // For potential future structured data

namespace wave {
namespace modules {
namespace clipboard {

// Data type for clipboard content (currently text)
using ClipboardData = std::string;

// Result of clipboard operations
struct ClipboardResult {
    enum class Status {
        Success,
        Error,
        NotSupported // e.g., for history clear if not implemented
    };

    Status status;
    std::string message;
    std::optional<ClipboardData> data; // For paste operations

    ClipboardResult(Status s, std::string msg, std::optional<ClipboardData> d = std::nullopt)
        : status(s), message(std::move(msg)), data(std::move(d)) {}
};

// Event types for clipboard actions
enum class ClipboardEventType {
    Copied,
    Pasted, // This event might carry the pasted data
    HistoryCleared // If history is implemented
    // Future: SelectionChanged (if monitoring system clipboard)
};

// Callback for clipboard events
// Parameters: event type, clipboard data associated with the event (e.g., copied/pasted text)
using ClipboardEventCallback = std::function<void(ClipboardEventType type, const ClipboardData& eventData)>;

class ClipboardModule : public wave::core::moduleloader::ILauncherModule {
public:
    ClipboardModule();
    ~ClipboardModule() override;

    // --- ILauncherModule interface ---
    void initialize(wave::core::moduleloader::ICoreAccess* coreAccess) override;
    void shutdown() override;
    std::string getName() const override;
    std::string getVersion() const override;

    // --- Clipboard specific methods ---
    ClipboardResult copy(const ClipboardData& data);
    ClipboardResult paste();
    ClipboardResult clearHistory(); // Basic version, may not have extensive history
    void subscribeToClipboardEvents(ClipboardEventCallback callback);

private:
    wave::ICoreAccess* CppCoreAccess; // Renamed - Use the main ICoreAccess
    std::string CppModuleName; // Renamed
    std::string CppModuleVersion; // Renamed
    
    // For simplicity, current clipboard content is not stored internally in a variable here,
    // as it's directly interacting with the system clipboard.
    // If we were to implement advanced features like history or internal buffers,
    // we would need member variables for that.
    // std::vector<ClipboardData> CppClipboardHistory; // Example if history was implemented

    std::vector<ClipboardEventCallback> CppEventCallbacks; // Renamed
    mutable std::mutex CppModuleMutex; // Renamed, for protecting callbacks and any shared state

    void broadcastEvent(ClipboardEventType type, const ClipboardData& eventData);

    // Platform-specific helper methods
    ClipboardResult platformCopy(const ClipboardData& data);
    ClipboardResult platformPaste();
};

} // namespace clipboard
} // namespace modules
} // namespace wave

// Required for dynamic loading
extern "C" {
    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    wave::core::moduleloader::ILauncherModule* create_module_instance();

    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    void destroy_module_instance(wave::core::moduleloader::ILauncherModule* moduleInstance);
}

#endif // WAVE_MODULES_CLIPBOARD_CLIPBOARD_MODULE_HPP
