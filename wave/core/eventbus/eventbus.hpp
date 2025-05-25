#ifndef WAVE_CORE_EVENTBUS_EVENTBUS_HPP
#define WAVE_CORE_EVENTBUS_EVENTBUS_HPP

#include <any>
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <thread> // For std::this_thread::sleep_for during async testing or handling
#include <chrono> // For std::chrono::milliseconds

namespace wave {
namespace core {
namespace eventbus {

// Represents a flexible data structure for event payloads
using StructuredData = std::any;

// Callback function type for event subscribers
using EventCallback = std::function<void(const StructuredData&)>;

// Unique identifier for subscriptions
using SubscriptionId = uint64_t;

// Delivery mode for events
enum class DeliveryMode {
    Sync,  // Event is delivered synchronously in the publisher's thread
    Async  // Event is delivered asynchronously in a separate thread (or thread pool)
};

class EventBus {
public:
    EventBus();
    ~EventBus();

    // Publishes an event to all subscribed listeners.
    // eventName: The name of the event to publish.
    // payload: The data associated with the event.
    // mode: The delivery mode (Sync or Async). Async is default.
    void publish(const std::string& eventName, const StructuredData& payload, DeliveryMode mode = DeliveryMode::Async);

    // Subscribes a callback to an event.
    // eventName: The name of the event to subscribe to.
    // callback: The function to call when the event is published.
    // mode: The delivery mode preference for this subscription. Note: if publisher forces Sync, this might be overridden.
    // Returns a unique ID for the subscription, which can be used to unsubscribe.
    SubscriptionId subscribe(const std::string& eventName, EventCallback callback, DeliveryMode mode = DeliveryMode::Async);

    // Unsubscribes a callback from an event.
    // id: The unique ID of the subscription to remove.
    void unsubscribe(SubscriptionId id);

private:
    struct Subscription {
        SubscriptionId id;
        EventCallback callback;
        DeliveryMode mode;
        // Could add other details like eventName here if needed for unsubscribe optimization,
        // but current design uses id mapping directly to subscription details.
    };

    std::mutex CppMutex; // Renamed to avoid conflict with potential system macros
    std::map<std::string, std::vector<Subscription>> CppSubscribers; // Renamed
    std::map<SubscriptionId, std::pair<std::string, size_t>> CppSubscriptionMap; // Maps ID to (eventName, index in CppSubscribers[eventName]) // Renamed
    std::atomic<SubscriptionId> CppNextSubscriptionId; // Renamed
    
    // Helper to find a subscription's details by ID for unsubscribe
    // Returns true if found, and populates eventName and index.
    bool findSubscriptionDetails(SubscriptionId id, std::string& eventName, size_t& index);
};

} // namespace eventbus
} // namespace core
} // namespace wave

#endif // WAVE_CORE_EVENTBUS_EVENTBUS_HPP
