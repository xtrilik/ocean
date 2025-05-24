# Core: Event Bus System PRD (v2)

## 1. Purpose

The Event Bus System is a central sub-component of the Launcher Core, designed to facilitate decoupled, asynchronous communication between different parts of the application (core sub-systems, external modules, CLI, and GUI). It allows components to publish and subscribe to events with structured payloads, supporting both synchronous and asynchronous delivery, and provides extensibility events for GUI and other frontends.

**This v2 PRD fully integrates requirements for thread safety, async event delivery, structured event payloads and error reporting, extensibility events for GUI, localization/i18n, and testability. The Event Bus and all APIs are explicitly frontend-agnostic and ready for both CLI and GUI use.**

## 2. Key Responsibilities

*   Providing a thread-safe mechanism for components to broadcast and subscribe to events using string-based event names and structured payloads.
*   Supporting both synchronous and asynchronous event delivery, ensuring that long-running event handlers do not block the main thread or event publisher.
*   Maintaining a registry of event types and their current subscribers, with all modifications thread-safe.
*   Dispatching published events to all registered subscribers, with support for both direct (sync) and queued (async) delivery.
*   Managing subscriber lifecycles, ensuring automatic unsubscription when subscribers are destroyed.
*   Providing extensibility events/callbacks for GUI and other frontends to update dynamically on event type registration, subscription changes, or event publication.
*   Supporting structured event payloads and structured error reporting for event delivery failures.
*   All user-facing event names, descriptions, and error messages must be localizable.
*   All APIs must be testable independently of the frontend.

## 3. Key Internal Interfaces/Classes (Conceptual)

*   **EventBus**: The main orchestrating class for event publication and subscription. Exposes a frontend-agnostic API:
    *   `void publish(const std::string& eventName, const StructuredData& payload, DeliveryMode mode = DeliveryMode::Async)`
    *   `SubscriptionId subscribe(const std::string& eventName, EventCallback callback, DeliveryMode mode = DeliveryMode::Async)`
    *   `void unsubscribe(SubscriptionId id)`
    *   All methods must be thread-safe.
    *   DeliveryMode can be `Sync` or `Async` (default is `Async` for GUI safety).
*   **EventCallback**: A function or functor receiving a structured payload and returning a structured result or error.
*   **StructuredData**: A serializable, extensible data structure (e.g., variant, JSON-like, or C++ struct) for event payloads.
*   **Extensibility Events**: The EventBus must emit events/callbacks for event type registration, subscription changes, and event publication, so GUI and other frontends can update dynamically.
*   **Localization**: All event names, descriptions, and error messages must be localizable.

### 3.1. Structured Event Payloads & Error Reporting

*   All events must use structured payloads (e.g., C++ structs, JSON, or variant types) for easy consumption by CLI, GUI, and modules.
*   Event delivery failures must be reported as structured errors, not just text.
*   Example:
    ```cpp
    struct EventResult {
        enum class Status { Delivered, Warning, Error };
        Status status;
        std::string message; // Localizable
        std::optional<StructuredData> data;
    };
    ```

### 3.2. Threading & Async Delivery

*   All EventBus APIs must be thread-safe.
*   Event delivery must support both synchronous and asynchronous modes.
*   Async delivery must use safe mechanisms (e.g., Qt signals/slots, message queues) to avoid blocking the main thread or publisher.
*   Subscribers can specify their preferred delivery mode.

### 3.3. Extensibility Events

*   The EventBus must provide events/callbacks for:
    *   Event type registration/unregistration
    *   Subscription addition/removal
    *   Event publication (for monitoring, diagnostics, or GUI updates)
*   GUI and other frontends must be able to subscribe to these events to update menus, dashboards, or event viewers dynamically.

### 3.4. Localization & Output Formatting

*   All user-facing event names, descriptions, and error messages must be prepared for localization (i18n/l10n).
*   Event payloads and results must support flexible formatting for CLI, GUI, and logging.

### 3.5. Testability & Automation

*   All EventBus APIs must be testable independently of the frontend.
*   Automated tests should cover event publication, subscription, delivery (sync/async), error handling, and extensibility events.

## 4. Interactions with Other Core Sub-components

*   **Module Loader System:**
    *   Modules can publish and subscribe to events via the EventBus.
    *   Module lifecycle events (e.g., `module.loaded`, `module.unloaded`) are published for monitoring and GUI updates.
*   **CLI Engine:**
    *   CLI commands can publish events (e.g., `cli.command.executed`) and subscribe to events for output or automation.
*   **Logging System:**
    *   Logs significant event publications, delivery failures, and diagnostics.
*   **Configuration System:**
    *   Can publish events on config reloads or changes.
*   **ICoreAccess Interface:**
    *   Provides access to the EventBus for modules and frontends.

## 5. Output Formatting Standards

*   All event-related output must be localizable and support flexible formatting for CLI, GUI, and logs.

## 6. Error Handling

*   All event delivery errors must be reported as structured results (see above), not just as text.
*   The EventBus must log detailed error information for diagnostics via the Logging System.

## 7. Open Questions & Considerations

*   (Update as needed to reflect new requirements for async, structured payloads, extensibility events, localization, and testability.)

---

**This v2 PRD ensures the Event Bus System is robust, extensible, and ready for both CLI and GUI frontends, with all modern requirements fully integrated.** 