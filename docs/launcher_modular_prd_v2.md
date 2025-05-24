# Launcher Modular Re-Architecture PRD (v2)

## 1. Introduction & Vision

This document defines the requirements and architecture for re-architecting the Launcher application into a modular, Asterisk-inspired, event-driven system. The goals are:
- Modular, dynamically loadable features (modules)
- INI-based configuration, with conventions inspired by Asterisk
- A powerful CLI for all management and diagnostics
- Robust logging, diagnostics, and operational transparency
- A system that is easy to extend and maintain
- **Full support for both CLI and GUI frontends, with all core systems designed for thread safety, async operation, and structured results.**

## 2. High-Level Architecture

**Core Components:**
- **Event Bus:** Central mechanism for inter-module communication. Must be thread-safe and support async event delivery for GUI responsiveness.
- **Module Loader:** Loads/unloads modules at startup and runtime. Provides events/callbacks for module load/unload so GUIs can update dynamically.
- **CLI Engine:** Interactive command-line interface. Exposes a frontend-agnostic API for command processing, supporting both synchronous and asynchronous execution, and returning structured results.
- **Configuration System:** Loads and validates INI config files. All config APIs must be thread-safe or clearly documented as main-thread-only.
- **Logging System:** Centralized logging with per-module control. Logging APIs must be thread-safe and support structured log entries for advanced GUI filtering.

**System Flow:**
- On startup, the core loads configuration and modules as specified in modules.conf.
- Modules register their CLI commands and capabilities using structured result types and error reporting.
- The CLI provides operational control and diagnostics, and is accessible from both CLI and GUI frontends.
- All errors and events are logged centrally, with optional CLI/GUI display.

## 2.1. Planned Modules

Below is a comprehensive list of planned modules for the Launcher, based on the current architecture and feature set. This table will be updated as the project evolves. Each module will have its own PRD as development progresses.

| Module Name         | Category        | Purpose/Scope                                      | Notes/Dependencies                |
|---------------------|----------------|----------------------------------------------------|-----------------------------------|
| **core**            | Core           | Event bus, module loader, CLI, config, logging. Bundles essential resource-like services for other modules. | Root of all modules               |
| filepreview         | Services       | Preview files of various types                     |                                   |
| clipboard           | Services       | Clipboard management and history                   |                                   |
| scanservice         | Services       | File system scanning/indexing                      |                                   |
| directorymanager    | Services       | Directory navigation and file tree                 |                                   |
| database            | Resource Modules | Database access and management                     | SQLite or other DB                |
| calendar            | Services       | Calendar integration and event display             | May depend on Outlook/COM module  |
| eventmonitor        | UI/Utilities   | UI/CLI for monitoring events on the event bus      |                                   |
| windowmonitor       | UI/Utilities   | List and inspect open windows                      | Platform-specific                 |
| oppmanagement       | Services       | Opportunity management (CRM-like features)         | Depends on database               |
| comservices         | Resource Modules | Outlook/COM integration and resource access        | Windows only                      |
| theming             | UI             | UI theming and customization                       |                                   |
| updater             | Utilities      | Auto-update module                                 |                                   |
| notification        | Utilities      | System/user notifications                          |                                   |
| pluginmanager       | Utilities      | (Future) 3rd-party plugin support                  |                                   |

**As each module is designed/implemented, create a PRD for it and link it here.**

## 3. File and Directory Structure

A clear file and directory structure is essential for maintainability, onboarding, and operational clarity. The following layout is recommended for the modular launcher.

### 3.1. Source Tree Structure

```plaintext
wave/
  core/                      # Core logic and essential systems
    cli/                   # CLI Engine System
    logging/               # Logging System
    configuration/         # Configuration System
    eventbus/              # Event Bus System (Example, if it gets its own dir)
    moduleloader/          # Module Loader System (Example, if it gets its own dir)
    core.hpp               # Main core interfaces/definitions
    core.cpp               # Main core implementation
    ...
  modules/                   # All feature modules, one subdirectory per module
    filepreview/             # Example: FilePreview module
    clipboard/               # Example: Clipboard module
    comservices/             # Example: COMServices module
    scanservice/             # Example: ScanService module
    eventmonitor/            # Example: EventMonitor module
    ...
  include/                   # Shared headers/interfaces (e.g., ICoreAccess.hpp)
  conf/                      # All .conf files (one per module)
    modules.conf
    filepreview.conf
    clipboard.conf
    scanservice.conf
    ...
  docs/                      # PRDs, architecture docs, module docs
  tests/                     # Unit/integration tests (if any, per project decision)
  scripts/                   # Build/test scripts (if any)
  resources/                 # Icons, images, etc.
  logs/                      # Log files (created at runtime)
  CMakeLists.txt / Makefile  # Build system
  README.md
```

### 3.2. Runtime/Deployment Layout

```plaintext
/opt/launcher/bin/                   # Main executable
/opt/launcher/modules/               # All compiled .so/.dll modules
/opt/launcher/conf/                  # All .conf files (copied from source conf/)
  modules.conf
  filepreview.conf
  clipboard.conf
  ...
/opt/launcher/logs/                  # Log files
/opt/launcher/resources/             # Icons, images, etc.
```

### 3.3. Configuration Files
- All configuration files (`modules.conf`, `<modulename>.conf`) live in the top-level `conf/` directory.
- Each module reads its config from `conf/<modulename>.conf`.
- The core reads `conf/modules.conf` and `conf/launcher.conf` (or a similarly named core configuration file) for global settings.

### 3.4. Documentation/PRDs
- All PRDs and architecture docs live in `docs/`.
- Each module's PRD is a separate file (e.g., `docs/module_clipboard_prd.md`) and is linked from the "Planned Modules" table.

## 4. Core System Requirements

### 4.1. Threading & Concurrency
- All core system APIs must be thread-safe or clearly documented as main-thread-only.
- For cross-thread communication (e.g., GUI to core), use Qt signals/slots, message queues, or similar safe mechanisms.
- Long-running operations must not block the main GUI thread; use background threads or async patterns.

### 4.2. Asynchronous Operations & Responsiveness
- Command handlers and core APIs must support asynchronous execution for long-running tasks, enabling GUI responsiveness.
- For CLI, synchronous execution is acceptable, but the same APIs must be usable from background threads in GUI contexts.
- Consider returning futures/promises or using callbacks for async results.

### 4.3. Structured Results & Error Reporting
- Command handlers and core APIs must return structured results, not just plain strings. Example:
  ```cpp
  struct CommandResult {
      enum class Status { Success, Warning, Error };
      Status status;
      std::string message;
      std::optional<StructuredData> data;
  };
  ```
- This enables both CLI and GUI frontends to consume results easily, display errors, and present data in tables or dialogs.
- All errors should be reported with status, message, and optional data, not just as text.

### 4.4. Extensibility Events & Dynamic Updates
- The core must provide events or callbacks for module load/unload and command registry changes.
- GUI frontends must be able to subscribe to these events to update menus, toolbars, or command palettes dynamically.

### 4.5. Output Formatting
- Output formatting should be flexible: CLI uses plain text, but GUI may require structured or styled output.

## 5. Subsystem-Specific Requirements

### 5.1. Event Bus

The Event Bus is the central mechanism for decoupled, inter-module and core-component communication. 
- It must be thread-safe and support asynchronous event delivery, which is crucial for GUI responsiveness and overall system stability.
- Events can be published from any thread; delivery to subscribers (which can be the CLI, GUI components, or other modules) must be managed safely and avoid blocking critical threads like the main GUI thread.
- Event payloads should support structured data to enable advanced data passing and handling, particularly for UI elements or complex inter-module interactions.
- The system should define clear patterns for event naming, payload structure, and subscription management.

For detailed specifications on event types, subscription mechanisms, and payload handling, refer to the `core_event_bus_system_prd.md` document.

### 5.2. Module Loader

The Module Loader is responsible for the dynamic loading, unloading, and lifecycle management of all external modules (typically shared libraries).
- It loads/unloads modules at startup (based on configuration, e.g., `modules.conf`) and can support runtime loading/unloading if required.
- It must provide events or callbacks for module load and unload events. This allows other parts of the system, especially GUIs or the CLI Engine, to dynamically update their available commands, features, or UI elements.
- All module lifecycle methods invoked by the loader must be handled safely, considering thread-safety implications if modules operate in their own threads.
- When modules are registered or unregistered, the Module Loader must coordinate with the Command Registry (likely via the CLI Engine or a shared core mechanism) to ensure commands are correctly added or removed, and that all frontends are notified of these changes.

For detailed specifications on the `ILauncherModule` interface, `ICoreAccess` provision, dependency handling, and error management related to module loading, refer to the `core_module_loader_system_prd.md` document.

### 5.3. CLI Engine

The CLI Engine provides the interactive command-line interface and processes command inputs.
- It exposes a frontend-agnostic API for command processing, allowing it to be driven by a direct console interaction, a GUI frontend, or other programmatic means.
  - This typically includes a method for direct string output (suitable for simple CLI) and a method that returns structured results (for GUIs or advanced consumers).
- It supports both synchronous and asynchronous command execution to prevent blocking, especially for long-running commands initiated from a GUI.
- It maintains a Command Registry of available commands, including their names, any aliases, help text, and the associated handler functions or objects.
- Command handlers, whether part of the core or registered by modules, must return structured results (status, message, optional data) and be designed to support asynchronous execution if they perform long-running operations.
- The CLI Engine should provide events for command registry changes (e.g., when modules load/unload commands), allowing dynamic updates in GUIs or other clients.

For detailed specifications on command parsing, the Command Registry, `ICommand` interface/definition, help system, and output formatting, refer to the `core_cli_engine_system_prd.md` document.

### 5.4. Configuration System

The Configuration System is responsible for loading, parsing, and providing access to INI-based configuration files, which allow runtime customization of core behavior and module features.
- It loads and validates INI config files (e.g., `launcher.conf`, `modules.conf`, and per-module `.conf` files from a `conf/` directory).
- All configuration access APIs should be thread-safe or clearly documented regarding their threading model (e.g., main-thread-only access or thread-safe read access after initial load).
- It should provide events for configuration reloads, allowing modules and UI components to dynamically update their settings if the configuration is changed and reloaded at runtime.
- The system must support structured error reporting for configuration file parsing and validation issues.
- It should support common INI features like comments and potentially path variable expansion (e.g., `${HOME}`).

For detailed specifications on INI parsing rules, type-safe value retrieval, error handling, and reload mechanisms, refer to the `core_configuration_system_prd.md` document.

### 5.5. Logging System

The Logging System provides centralized logging capabilities with per-module control, inspired by systems like Asterisk and utilizing features of the underlying framework (e.g., Qt's QLoggingCategory).
- It offers centralized logging with configurable logging levels (e.g., INFO, WARNING, ERROR, DEBUG) for the core and per-module.
- Logging APIs must be thread-safe to allow logging from any part of the application.
- Log entries should be structured, including elements like timestamp, severity level, category (identifying the source, e.g., core or specific module), the log message, and optionally, additional structured data for diagnostic purposes.
- The system should allow runtime control of logging verbosity, typically via CLI commands.
- It provides events for log level changes and potentially for new log entries, which can be used by GUI log viewers or other diagnostic tools.
- Modules can report their preferred initial logging state (e.g., via flags in their `.conf` files), which the Logging System merges with baseline rules from a central logger configuration (e.g., `logger.conf`) to establish effective logging rules.

For detailed specifications on the logging architecture, configuration (including `logger.conf` and module-specific flags), rule merging, and integration with any platform-specific logging mechanisms, refer to the `core_logging_system_prd.md` document.

## 6. Module System

The Launcher's functionality is extended via dynamically loaded modules (typically shared libraries like `.dll` or `.so`). Modules are discovered and loaded based on settings in a core configuration file (e.g., `modules.conf`).

Each module must implement an `ILauncherModule` interface (or a similarly named contract). This interface defines standard methods for the module lifecycle, such as:
- Initialization (e.g., `initialize`)
- Shutdown (e.g., `shutdown`)
- Configuration loading (e.g., `loadConfig`)
- Command registration (e.g., `registerCommands`)

During its initialization, each module receives an `ICoreAccess` interface pointer (or a similar mechanism to access core functionalities). 

**`ICoreAccess` Interface:**
*   **Purpose:** This interface acts as a stable contract, insulating modules from internal core implementation details. It ensures modules access core functionalities in a controlled and consistent manner, also facilitating easier testing by allowing core services to be mocked.
*   **Conceptual Methods:** `ICoreAccess` provides methods for modules to obtain pointers or references to the core service interfaces, such as those for:
    *   The Logging System
    *   The Configuration System
    *   The Event Bus
    *   The Command Registry (for registering CLI commands)
    *   A mechanism for modules to report their preferred logging rules to the Core Logging System.
*   Modules should consult the specific PRDs of services obtained via `ICoreAccess` for their detailed interaction contracts, including threading models and API usage.

Modules can declare dependencies (though detailed dependency management is TBD or covered in the Module Loader PRD), interact with other components via the Event Bus, and register their own CLI commands (which should return structured results).

Modules must be able to register/unregister commands at runtime. The core system, via the Module Loader and CLI Engine, must ensure that frontends are notified of these changes (e.g., through events) to allow dynamic updates to command lists or UI elements.

Module APIs themselves should be designed with thread-safety in mind or be clearly documented regarding their threading constraints. Commands provided by modules should support asynchronous execution where appropriate and return structured results, consistent with the overall CLI engine design.

**For full details on the module loading mechanism, the specific `ILauncherModule` and `ICoreAccess` interface definitions, lifecycle management, dependency handling, reloading, and error handling related to modules, please refer to the Core Module Loader System PRD (e.g., `core_module_loader_system_prd.md`).**

## 7. Configuration Management

- All configuration files use the INI format and are stored in the `conf/` directory.
- All config APIs must be thread-safe or clearly documented.
- Config changes are only applied on explicit reload, and all frontends are notified of changes.
- Modules provide default values for missing options and validate config on reload, reporting errors in a structured way.

## 8. Command-Line Interface (CLI)

- The CLI engine is frontend-agnostic and can be used from both CLI and GUI.
- Command handlers must return structured results and support async execution.
- The CLI supports operational control and diagnostics.
- All output and error messages must support flexible formatting.
- The CLI provides events for command registry changes and command execution results.

## 9. Logging, Diagnostics, and Error Handling

- Centralized logging system with per-module control.
- All log APIs must be thread-safe and support structured log entries.
- Errors are reported with status, message, and optional data.
- The logging system provides events for new log entries and log level changes.

## 10. High-Level Core Initialization Sequence

This outlines the general order in which core components should be initialized during application startup. Dependencies may require slight adjustments, but this provides the overall flow:

1.  **Pre-initialization:** Basic application setup (e.g., QApplication instance for Qt-based projects, command-line argument parsing if applicable).
2.  **Configuration System Initialization:**
    *   Create the Configuration System instance.
    *   Load baseline configurations (e.g., a main application config like `launcher.conf`, `logger.conf`, `modules.conf`). This provides essential paths and initial settings needed by other systems.
3.  **Logging System Initialization:**
    *   Create the Logging System instance.
    *   If applicable, install any custom message handlers (e.g., `qInstallMessageHandler` for Qt).
    *   Configure the system based on settings from the loaded logger configuration (e.g., log file path, message pattern).
    *   Apply the *baseline* logging rules (e.g., from a `[rules]` section in `logger.conf`).
4.  **Event Bus Initialization:**
    *   Create the Event Bus instance.
    *   Event Bus is now ready for subscriptions and emissions.
5.  **CLI Engine Initialization:**
    *   Create the CLI Engine and its associated Command Registry.
    *   Register any core CLI commands provided by the engine itself (e.g., `help`, basic `core` commands).
6.  **Module Loader Initialization:**
    *   Create the Module Loader instance.
    *   Provide it with access to the `ICoreAccess` interface (or the necessary core service pointers like Configuration, Logging, EventBus, CommandRegistry).
    *   The Module Loader uses the Configuration System to determine which modules to load (e.g., from `modules.conf`).
7.  **Module Loading & Initialization Loop:**
    *   The Module Loader iterates through the modules specified for loading.
    *   For each module:
        *   Load the module library.
        *   Instantiate the `ILauncherModule` implementation.
        *   Call the module's initialization method (e.g., `ILauncherModule::initialize()`), passing the `ICoreAccess` pointer.
        *   Inside the module's initialization routines (e.g., `initialize()`, `loadConfig()`, `registerCommands()`):
            *   The module uses `ICoreAccess` to get the Configuration System and reads its own configuration file.
            *   It may read its logging flags/preferences from its configuration, translate them to appropriate rule strings, and report these to the Core Logging System via `ICoreAccess`.
            *   It uses `ICoreAccess` to get the Command Registry and registers its commands. Handler functions are expected to receive `ICoreAccess*` (or similar) to interact with core services.
            *   It uses `ICoreAccess` to get the Event Bus and subscribes to or prepares to publish events.
            *   Performs other module-specific setup.
8.  **Post-Module Load Logging Update (if applicable):**
    *   After all modules are initialized, if modules can report their own logging preferences, the Logging System finalizes and applies the *effective* ruleset by merging module-specific rules with the baseline rules.
9.  **Core Startup Complete:**
    *   Emit a `core.started` event via the Event Bus.
    *   The application is now fully initialized and operational.

This sequence ensures that essential services like Configuration and Logging are available before modules that depend on them are loaded.

## 11. Versioning & Compatibility

- The launcher and each module should have a clear version number (e.g., Major.Minor.Patch).
- Compatibility aims to be maintained within a major version. Breaking changes across major versions should be clearly documented in release notes.
- Modules may query the core version, and the core may be aware of module versions, though strict compatibility enforcement mechanisms are TBD.
- Version information should be accessible, for instance, via CLI commands like `core show version` or by inspecting module properties.

## 12. Security & Permissions

- At this stage, no special OS-level permissions are anticipated for loading modules or using most CLI commands beyond what's needed to run the application and access its own files.
- There is no granular, user-based permissions model for CLI commands within the application itself at this time.
- If remote CLI access is implemented, it should default to local-only connections (e.g., localhost/127.0.0.1) and any external access should require explicit configuration and security considerations (though detailed remote security is TBD).
- No specific "safe mode" startup or detailed audit trail for CLI/config changes is planned unless a specific need arises.
- Dangerous or destructive commands may implement a confirmation prompt (e.g., "Are you sure?") as a basic safeguard.

## 13. Migration Plan

- If migrating from a previous system, a side-by-side approach is generally preferred, where the new modular architecture is developed and stabilized.
- There is no strict requirement to maintain compatibility with old configuration files, data formats, or module structures from any pre-existing, non-modular version, unless explicitly stated for a particular feature.
- Features will be migrated or implemented in the new system based on project priorities, likely in incremental milestones.

## 14. Open Questions & Future Ideas

- **GUI Frontend:** How will the GUI frontend be developed (Qt, web-based, etc.)? How will it interact with the core systems and modules, especially regarding asynchronous operations and structured data? (Partially addressed by requiring thread-safe core APIs, structured results, and event-driven updates).
- **Advanced Module Dependencies:** How will complex module dependencies (beyond simple load order) be managed? Will there be a formal dependency resolution mechanism? (Currently TBD, refer to Module Loader PRD).
- **Security Model for Modules:** If 3rd-party plugins are supported, what security model will be in place? (Future concern).
- **Configuration Schema/Validation:** Formalized schema definition and validation for .conf files beyond basic parsing.
- **Performance Profiling Hooks:** Standardized ways for modules and core systems to expose performance metrics.
- **Internationalization/Localization:** While removed for now, this might be revisited in the distant future if the application's scope expands significantly.

## 15. Component PRDs

The core architecture is composed of several key systems, each with its own detailed Product Requirements Document (PRD). These documents provide in-depth specifications for each component's responsibilities, interfaces, configuration, and behavior.

- **Core Module Loader System PRD:**
    - *Document:* `docs/core_module_loader_system_prd.md`
    - *Scope:* Defines the `ILauncherModule` and `ICoreAccess` interfaces, module lifecycle management (loading, unloading, initialization, shutdown), dependency considerations, configuration via `modules.conf`, and error handling for module operations.
- **Core Configuration System PRD:**
    - *Document:* `docs/core_configuration_system_prd.md`
    - *Scope:* Specifies INI file parsing, type-safe value retrieval, configuration sections (e.g., `[General]`, `[Paths]`), support for variables (e.g., `${APP_DIR}`), configuration reloading, and error reporting for `launcher.conf`, `modules.conf`, and per-module `.conf` files.
- **Core Logging System PRD:**
    - *Document:* `docs/core_logging_system_prd.md`
    - *Scope:* Details the logging framework, log levels (INFO, DEBUG, WARNING, ERROR), per-module log control, structured logging, configuration via `logger.conf` and module-specific settings, and integration with potential UI log viewers.
- **Core CLI Engine System PRD:**
    - *Document:* `docs/core_cli_engine_system_prd.md`
    - *Scope:* Describes command registration, parsing, execution (synchronous and asynchronous), `ICommand` interface, help system, structured command results, command history, and integration with core and module-provided commands.
- **Core Event Bus System PRD:**
    - *Document:* `docs/core_event_bus_system_prd.md`
    - *Scope:* Defines the event publication and subscription mechanisms, event types, payload structures, thread-safety considerations, and its role in decoupled inter-module and core-component communication.

Each feature module (e.g., File Preview, Clipboard, Scan Service) will also have its own PRD, linked from the "Planned Modules" table (Section 2.1), as its detailed design and implementation are undertaken.

---

**This version (v2) fully integrates GUI and modular extensibility requirements throughout the architecture, ensuring the system is robust, future-proof, and ready for both CLI and GUI frontends.** 