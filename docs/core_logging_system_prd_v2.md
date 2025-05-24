# Core: Logging System PRD (v2)

## 1. Purpose

The Logging System is a core sub-component of the Launcher, providing centralized, thread-safe, and extensible logging for all core systems, modules, CLI, and GUI. It supports structured log entries, async operation, extensibility events for GUI, localization/i18n, and is fully testable and frontend-agnostic.

## 2. Key Responsibilities

*   Providing a thread-safe API for logging messages from any thread or component (core, modules, CLI, GUI).
*   Supporting both synchronous and asynchronous logging (e.g., log queuing and background writing) to avoid blocking the main thread or performance-critical paths.
*   Accepting structured log entries (not just plain strings), including status, message, category, timestamp, and optional structured data.
*   Providing extensibility events/callbacks for GUI and other frontends (e.g., new log entries, log level changes) so log viewers and dashboards can update in real time.
*   Supporting per-module and per-category log levels, dynamic log level changes, and runtime configuration reloads.
*   All user-facing log messages and categories must be localizable.
*   All APIs must be testable independently of the frontend.

## 3. Key Internal Interfaces/Classes (Conceptual)

*   **LoggingSystem**: The main orchestrating class for logging. Exposes a frontend-agnostic API:
    *   `void log(const LogEntry& entry)`
    *   `void setLogLevel(const std::string& category, LogLevel level)`
    *   `LogLevel getLogLevel(const std::string& category) const`
    *   `void subscribeToLogEvents(LogEventCallback callback)`
    *   All methods must be thread-safe.
*   **LogEntry**: A structured log entry, e.g.:
    ```cpp
    struct LogEntry {
        enum class Status { Info, Warning, Error, Debug };
        Status status;
        std::string message; // Localizable
        std::string category;
        std::chrono::system_clock::time_point timestamp;
        std::optional<StructuredData> data;
    };
    ```
*   **LogEventCallback**: A function or functor receiving new log entries or log level changes (for GUI log viewers, dashboards, etc.).
*   **Async Operation**: LoggingSystem must support async log queuing and background writing to avoid blocking the main thread.
*   **Localization**: All log messages and categories must be localizable.

### 3.1. Structured Log Entries & Error Reporting

*   All log entries must be structured (see above) for easy consumption by CLI, GUI, and external tools.
*   Log errors (e.g., file write failures) must be reported as structured results, not just text.

### 3.2. Threading & Async Operation

*   All LoggingSystem APIs must be thread-safe.
*   Logging must support async queuing and background writing.
*   Log event callbacks (for GUI) must be delivered safely across threads.

### 3.3. Extensibility Events

*   The LoggingSystem must provide events/callbacks for:
    *   New log entries
    *   Log level changes
    *   Log target changes (e.g., file, console, network)
*   GUI and other frontends must be able to subscribe to these events to update log viewers and dashboards dynamically.

### 3.4. Localization & Output Formatting

*   All user-facing log messages and categories must be prepared for localization (i18n/l10n).
*   Log output formatting must be flexible: plain text for CLI, structured/styled for GUI, and machine-readable for external tools.

### 3.5. Testability & Automation

*   All LoggingSystem APIs must be testable independently of the frontend.
*   Automated tests should cover log entry creation, async delivery, error handling, and extensibility events.

## 4. Interactions with Other Core Sub-components

*   **Module Loader System:**
    *   Modules can log messages, set log levels, and subscribe to log events.
*   **CLI Engine:**
    *   CLI commands can log messages and subscribe to log events for output or automation.
*   **Event Bus:**
    *   Can publish log-related events (e.g., log entry published, log level changed).
*   **Configuration System:**
    *   Can trigger log level changes or log target changes on config reload.
*   **ICoreAccess Interface:**
    *   Provides access to the LoggingSystem for modules and frontends.

## 5. Output Formatting Standards

*   All log output must be localizable and support flexible formatting for CLI, GUI, and external tools.

## 6. Error Handling

*   All logging errors (e.g., file write failures) must be reported as structured results (see above), not just as text.
*   The LoggingSystem must log detailed error information for diagnostics.

## 7. Open Questions & Considerations

*   (Update as needed to reflect new requirements for async, structured log entries, extensibility events, localization, and testability.)

---

**This v2 PRD ensures the Logging System is robust, extensible, and ready for both CLI and GUI frontends, with all modern requirements fully integrated.** 