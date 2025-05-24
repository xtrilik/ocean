# Core: Module Loader System PRD (v2)

## 1. Purpose

The Module Loader System is a core sub-component of the Launcher, responsible for discovering, loading, initializing, and unloading modules at runtime. It is thread-safe, supports async operations (e.g., module load/unload), structured results and error reporting, extensibility events for GUI, localization/i18n, and is fully testable and frontend-agnostic.

## 2. Key Responsibilities

*   Providing a thread-safe API for discovering, loading, initializing, and unloading modules from any thread or component (core, CLI, GUI).
*   Supporting both synchronous and asynchronous module load/unload operations to avoid blocking the main thread or performance-critical paths.
*   Returning structured results for all module operations, including status, module info, and structured error reporting.
*   Providing extensibility events/callbacks for GUI and other frontends (e.g., module lifecycle notifications, errors) so module managers and dashboards can update in real time.
*   Supporting per-module configuration, dynamic module reloads, and runtime module management.
*   All user-facing module names, descriptions, and error messages must be localizable.
*   All APIs must be testable independently of the frontend.

## 3. Key Internal Interfaces/Classes (Conceptual)

*   **ModuleLoaderSystem**: The main orchestrating class for module management. Exposes a frontend-agnostic API:
    *   `ModuleResult loadModule(const std::string& modulePath)`
    *   `ModuleResult unloadModule(const std::string& moduleName)`
    *   `ModuleResult reloadModule(const std::string& moduleName)`
    *   `std::vector<ModuleInfo> listModules()`
    *   `void subscribeToModuleEvents(ModuleEventCallback callback)`
    *   All methods must be thread-safe.
*   **ModuleResult**: A structured result for module operations, e.g.:
    ```cpp
    struct ModuleResult {
        enum class Status { Success, NotFound, Error };
        Status status;
        std::optional<ModuleInfo> module;
        std::string message; // Localizable
        std::optional<StructuredData> data;
    };
    ```
*   **ModuleEventCallback**: A function or functor receiving module lifecycle notifications, errors, or status changes (for GUI module managers, dashboards, etc.).
*   **Async Operation**: ModuleLoaderSystem must support async module load/unload to avoid blocking the main thread.
*   **Localization**: All module names, descriptions, and error messages must be localizable.

### 3.1. Structured Results & Error Reporting

*   All module operations must return structured results (see above) for easy consumption by CLI, GUI, and modules.
*   Module errors (e.g., load failures, dependency issues) must be reported as structured results, not just text.

### 3.2. Threading & Async Operation

*   All ModuleLoaderSystem APIs must be thread-safe.
*   Module load/unload must support async operation and safe delivery of events/callbacks.
*   Module event callbacks (for GUI) must be delivered safely across threads.

### 3.3. Extensibility Events

*   The ModuleLoaderSystem must provide events/callbacks for:
    *   Module loaded/unloaded/reloaded
    *   Module errors
    *   Status changes
*   GUI and other frontends must be able to subscribe to these events to update module managers and dashboards dynamically.

### 3.4. Localization & Output Formatting

*   All user-facing module names, descriptions, and error messages must be prepared for localization (i18n/l10n).
*   Module output formatting must be flexible: plain text for CLI, structured/styled for GUI, and machine-readable for external tools.

### 3.5. Testability & Automation

*   All ModuleLoaderSystem APIs must be testable independently of the frontend.
*   Automated tests should cover module load/unload, async operations, error handling, and extensibility events.

## 4. Interactions with Other Core Sub-components

*   **Configuration System:**
    *   Can provide per-module config and trigger module reloads on config changes.
*   **CLI Engine:**
    *   CLI commands can load/unload modules and subscribe to module events for output or automation.
*   **Event Bus:**
    *   Can publish module-related events (e.g., module loaded, error).
*   **Logging System:**
    *   Can log module load/unload, errors, and status changes.
*   **ICoreAccess Interface:**
    *   Provides access to the ModuleLoaderSystem for modules and frontends.

## 5. Output Formatting Standards

*   All module output must be localizable and support flexible formatting for CLI, GUI, and external tools.

## 6. Error Handling

*   All module errors (e.g., load failures, dependency issues) must be reported as structured results (see above), not just as text.
*   The ModuleLoaderSystem must log detailed error information for diagnostics.

## 7. Open Questions & Considerations

*   (Update as needed to reflect new requirements for async, structured results, extensibility events, localization, and testability.)

---

**This v2 PRD ensures the Module Loader System is robust, extensible, and ready for both CLI and GUI frontends, with all modern requirements fully integrated.** 