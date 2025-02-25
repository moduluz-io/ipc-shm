#include <fn_registry/fn_registry.h>
#include <typeinfo>

#define FUNCTION_CALL_DATA_SHM_SIZE 256

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

    /**
     * The fn_call shm usually contains the a UUIDv4 which is 128 bytes long.
     * Taking 256 bytes as the size of the shared memory. (Including some extra space)
     *
     * Data Order: (Total 256 bits)
     * 1. Shared memory name that holds the function call related data (128 bits)
     * 2. Size of the shared memory (size_t)
     * 3- Padding
     */
    size_t fn_call_data_shm_size = FUNCTION_CALL_DATA_SHM_SIZE;

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

void IPC::FunctionRegistry::listen() {
    while (true) {
        pthread_mutex_lock(mtx_);
        pthread_cond_wait(cv_, mtx_);

        /// Check if a function call is in progress
        if (((char*)fn_call_data_shm_manager_->getMemoryPointer())[0] == 0) {
            pthread_mutex_unlock(mtx_);

            continue;
        }

        /// Read the function call data
        char* shm_name = new char[129];
        fn_call_data_shm_manager_->readData(shm_name, 0, 128);
        shm_name[128] = '\0';

        size_t shm_size = *(size_t*)(fn_call_data_shm_manager_->getMemoryPointer() + 128);

        /// Create a new shared memory manager for the function call data
        SharedMemoryManager* fn_details_shm_manager = new SharedMemoryManager(shm_name, shm_size, false);
        if (fn_details_shm_manager == NULL) {
            throw std::runtime_error("Failed to initialize function details data shared memory");
        }

        /// Read the function call data
        size_t offset = 0;
        size_t call_id_len = *(size_t*)fn_details_shm_manager->getMemoryPointer(offset);
        offset += sizeof(size_t);

        char* call_id = new char[call_id_len];
        fn_details_shm_manager->readData(call_id, call_id_len, offset);
        offset += call_id_len;

        size_t method_name_len = *(size_t*)fn_details_shm_manager->getMemoryPointer(offset);
        char* method_name = new char[method_name_len];
        fn_details_shm_manager->readData(method_name, method_name_len, offset);
        offset += method_name_len;

        if (registered_fns_.find(method_name) == registered_fns_.end()) {
            throw std::runtime_error("Function not found");
        }

        size_t num_args = *(size_t*)fn_details_shm_manager->getMemoryPointer(offset);
        offset += sizeof(size_t);

        if (num_args != registered_fns_[method_name].getArgCount()) {
            throw std::runtime_error("Incorrect number of arguments");
        }

        std::vector<std::any> args;
        for (size_t i = 0; i < num_args; i++) {
            size_t arg_len = *(size_t*)fn_details_shm_manager->getMemoryPointer(offset);
            offset += sizeof(size_t);

            char* arg_data = new char[arg_len];
            fn_details_shm_manager->readData(arg_data, arg_len, offset);
            offset += arg_len;

            std::string arg_type = getArgType(method_name, i);
            if (arg_type == typeid(std::string).name()) {
                args.push_back(std::string(arg_data, arg_len));
            }
            else if (arg_type == typeid(int).name()) {
                args.push_back(*(int*)arg_data);
            }
            else if (arg_type == typeid(double).name()) {
                args.push_back(*(double*)arg_data);
            }
            else if (arg_type == typeid(float).name()) {
                args.push_back(*(float*)arg_data);
            }
            else if (arg_type == typeid(bool).name()) {
                args.push_back(*(bool*)arg_data);
            }

            delete[] arg_data;
        }

        /// Invoke the function
        std::any ret = invokefunction(method_name, args);

        /// Write the return value to the shared memory
        SharedMemoryManager return_size_shm_manager(
            (std::string(call_id) + "_ret_size").c_str(), sizeof(size_t), true);
        size_t* ret_size = (size_t*)(return_size_shm_manager.getMemoryPointer());
        if (ret.type().name() == typeid(std::string).name()) {
            std::string ret_str = std::any_cast<std::string>(ret);
            *ret_size = ret_str.size();

            SharedMemoryManager ret_shm_manager(call_id, *ret_size, true);
            ret_shm_manager.writeData(ret_str.c_str(), ret_str.size());
        }
        else if (ret.type().name() == typeid(int).name()) {
            *ret_size = sizeof(int);

            SharedMemoryManager ret_shm_manager(call_id, *ret_size, true);
            *((int*)ret_shm_manager.getMemoryPointer()) = std::any_cast<int>(ret);
        }
        else if (ret.type().name() == typeid(double).name()) {
            *ret_size = sizeof(double);

            SharedMemoryManager ret_shm_manager(call_id, *ret_size, true);
            *((double*)ret_shm_manager.getMemoryPointer()) = std::any_cast<double>(ret);
        }
        else if (ret.type().name() == typeid(float).name()) {
            *ret_size = sizeof(float);

            SharedMemoryManager ret_shm_manager(call_id, *ret_size, true);
            *((float*)ret_shm_manager.getMemoryPointer()) = std::any_cast<float>(ret);
        }
        else if (ret.type().name() == typeid(bool).name()) {
            *ret_size = sizeof(bool);

            SharedMemoryManager ret_shm_manager(call_id, *ret_size, true);
            *((bool*)ret_shm_manager.getMemoryPointer()) = std::any_cast<bool>(ret);
        }

        /// Reset the function call data
        memset(fn_call_data_shm_manager_->getMemoryPointer(), 0, FUNCTION_CALL_DATA_SHM_SIZE);

        delete[] shm_name;
        fn_details_shm_manager->removeMemory();
        delete fn_details_shm_manager;

        pthread_cond_broadcast(cv_);
        pthread_mutex_unlock(mtx_);
    }
}

int IPC::FunctionRegistry::getArgCount(std::string name) {
    if (registered_fns_.find(name) == registered_fns_.end()) {
        throw std::runtime_error("Function not found");
    }

    return registered_fns_[name].getArgCount();
}

std::string IPC::FunctionRegistry::getArgType(std::string name, int index) {
    if (registered_fns_.find(name) == registered_fns_.end()) {
        throw std::runtime_error("Function not found");
    }

    return registered_fns_[name].getArgType(index);
}