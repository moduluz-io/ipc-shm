#include <fn_registry/fn_registry.h>

IPC::FunctionRegistry::FunctionRegistry(std::string channel_name) : channel_name_(channel_name) {

    /**
     * SHARED MEMEORY STRUCTURE DETAILS
     * (The shared memory contains the following data in the given order)
     *
     * 1. Mutex
     * 2. Condition Variable
     *
     * 3. Method call ID length (size_t)
     * 4. Method call ID (char*) - This is used as the channel_name for the shared memory that should
     * be created by the client to store the return value of the function.
     *
     * 5. Method name length (size_t)
     * 6. Method name (char*)
     *
     * 7. Number of arguments (size_t)
     * 8. Argument 1 length (size_t)
     * 9. Argument 1 (char*)
     * 10. Argument 2 length (size_t)
     * 11. Argument 2 (char*)
     * ...
     */

     /// 128 bytes is taken as the extra size for the shared memory.
    size_t sync_shm_size = sizeof(pthread_mutex_t) + sizeof(pthread_cond_t) + 128;

    /// The fn_call shm usually contains the a UUIDv4 which is 128 bytes long.
    /// Taking 256 bytes as the size of the shared memory. (Including some extra space)
    size_t fn_call_data_shm_size = 256;

    /// Create the shared memory for storing the data synchronization details.
    sync_shm_manager_ = new SharedMemoryManager((channel_name_ + "_sync").c_str(), sync_shm_size, true);
    if (sync_shm_manager_ == NULL) {
        throw std::runtime_error("Failed to initialize sync shared memory");
    }

    /// Initialize the mutex and condition variable
    mtx_ = new(sync_shm_manager_->getMemoryPointer(0)) pthread_mutex_t;
    cv_ = new(sync_shm_manager_->getMemoryPointer(sizeof(pthread_mutex_t))) pthread_cond_t;

    pthread_mutex_init(mtx_, NULL);
    pthread_cond_init(cv_, NULL);

    if (this->mtx_ == NULL || this->cv_ == NULL) {
        throw std::runtime_error("Failed to initialize mutex or condition variable");
    }

    /// Initialize the function call related data shm
    fn_call_data_shm_manager_ = new SharedMemoryManager(channel_name_.c_str(), fn_call_data_shm_size, true);
    if (fn_call_data_shm_manager_ == NULL) {
        throw std::runtime_error("Failed to initialize function call data shared memory");
    }

    /**
     * Initialize the function call data shared memory with all zeros.
     *
     * 0 means no function call is currently in progress.
     */
    memset(fn_call_data_shm_manager_->getMemoryPointer(), 0, fn_call_data_shm_size);
}

IPC::FunctionRegistry::~FunctionRegistry() {
    delete mtx_;
    delete cv_;

    sync_shm_manager_->removeMemory();
    delete sync_shm_manager_;
}

template <typename Ret, typename... Args>
void  IPC::FunctionRegistry::registerFunction(std::string name, std::function<Ret(Args...)> func) {
    registered_fns_[name] = Function<Ret, ...Args>(name, func);
}

std::any IPC::FunctionRegistry::invokefunction(std::string name, const std::vector<std::any>& args) {
    if (registered_fns_.find(name) == registered_fns_.end()) {
        throw std::runtime_error("Function not found");
    }

    return registered_fns_[name].invoke(args);
}

void IPC::FunctionRegistry::printRegisteredFunctions() {
    for (const auto& [name, fn] : registered_fns_) {
        fn.printInfo();
    }
}