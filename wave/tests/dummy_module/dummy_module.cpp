#include "../../core/moduleloader/module_loader.hpp" // To get ILauncherModule and ICoreAccess
#include <iostream> // For basic output from the module

// Concrete implementation of ILauncherModule
class DummyModule : public wave::core::moduleloader::ILauncherModule {
private:
    wave::core::moduleloader::ICoreAccess* core_ = nullptr;
    std::string name_ = "DummyModule";
    std::string version_ = "1.0.0";
public:
    DummyModule() {
        // std::cout << "[DummyModule] Constructor called." << std::endl;
    }

    ~DummyModule() override {
        // std::cout << "[DummyModule] Destructor called." << std::endl;
    }

    void initialize(wave::core::moduleloader::ICoreAccess* coreAccess) override {
        core_ = coreAccess;
        // std::cout << "[DummyModule] Initialized. CoreAccess ptr: " << core_ << std::endl;
        if (core_) {
            // core_->placeholder(); // Example of using core access
        }
    }

    void shutdown() override {
        // std::cout << "[DummyModule] Shutdown." << std::endl;
    }

    std::string getName() const override {
        return name_;
    }

    std::string getVersion() const override {
        return version_;
    }

    void setName(const std::string& newName) { name_ = newName; } // Custom method for testing
};

// Exported C functions to create and destroy the module instance
extern "C" {
    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    wave::core::moduleloader::ILauncherModule* create_module_instance() {
        return new DummyModule();
    }

    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    void destroy_module_instance(wave::core::moduleloader::ILauncherModule* moduleInstance) {
        // std::cout << "[DummyModule] destroy_module_instance called." << std::endl;
        delete static_cast<DummyModule*>(moduleInstance);
    }
}
