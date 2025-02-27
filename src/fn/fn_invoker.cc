#include <fn/fn_invoker.h>

#include <random>
#include <sstream>
#include <iomanip>
#include <typeinfo>

#ifndef FUNCTION_CALL_DATA_SHM_SIZE
#define FUNCTION_CALL_DATA_SHM_SIZE 256
#endif


std::string generate_uuid_v4() {
    std::random_device rd;
    std::mt19937_64 gen(rd());  // 64-bit Mersenne Twister PRNG
    std::uniform_int_distribution<uint64_t> dis(0, UINT64_MAX);

    uint8_t uuid[16];

    // Generate 16 random bytes
    for (int i = 0; i < 16; i += 8) {
        uint64_t part = dis(gen);
        std::memcpy(uuid + i, &part, sizeof(part));
    }

    // Set the version (4) -> Bits 12-15 of time_hi_and_version
    uuid[6] = (uuid[6] & 0x0F) | 0x40;

    // Set the variant (RFC 4122) -> Bits 6-7 of clock_seq_hi_and_reserved
    uuid[8] = (uuid[8] & 0x3F) | 0x80;

    // Convert to UUID string format
    std::ostringstream oss;
    for (int i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) oss << "-";
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)uuid[i];
    }

    return oss.str();
}

IPC::FunctionInvoker::FunctionInvoker(std::string channel_name) {
    size_t sync_shm_size = sizeof(pthread_mutex_t) + sizeof(pthread_cond_t) + 128;

    /// Create the shared memory for storing the data synchronization details.
    sync_shm_manager_ = new SharedMemoryManager((channel_name + "_sync").c_str(), sync_shm_size, false);
    if (sync_shm_manager_ == NULL) {
        throw std::runtime_error("Failed to initialize sync shared memory");
    }

    /// Get the mutext and condition variable from the shared memory
    mtx_ = (pthread_mutex_t*)sync_shm_manager_->getMemoryPointer(0);
    cv_ = (pthread_cond_t*)sync_shm_manager_->getMemoryPointer(sizeof(pthread_mutex_t));

    if (this->mtx_ == NULL || this->cv_ == NULL) {
        throw std::runtime_error("Failed to load mutex or condition variable");
    }

    /// Create the shared memory manager for storing the function call data
    fn_call_data_shm_manager_ =
        new SharedMemoryManager(channel_name.c_str(), FUNCTION_CALL_DATA_SHM_SIZE, false);
}

IPC::FunctionInvoker::~FunctionInvoker() {
    fn_call_data_shm_manager_->~SharedMemoryManager();
    delete fn_call_data_shm_manager_;

    sync_shm_manager_->~SharedMemoryManager();
    delete sync_shm_manager_;
}

template <typename Ret>
Ret IPC::FunctionInvoker::invoke(std::string name, const std::vector<std::any>& args) {
    while (true) {
        pthread_mutex_lock(mtx_);

        /// Check if a function call is in progress
        if (((char*)fn_call_data_shm_manager_->getMemoryPointer())[0] != 0) {
            pthread_mutex_unlock(mtx_);

            continue;
        }
        else {
            /// No other function calls are in progress, so we can proceed with this function call
            break;
        }
    }

    /// UUID for the function call
    std::string call_id = generate_uuid_v4();

    size_t total_shm_size = 0;
    total_shm_size += sizeof(size_t);  // Size of the call_id
    total_shm_size += call_id.size();  // The call_id
    total_shm_size += sizeof(size_t);  // Size of the method name
    total_shm_size += name.size();     // The method name
    total_shm_size += sizeof(size_t);  // Size of the number of arguments
    total_shm_size += sizeof(size_t) * args.size();  // Size of the argument sizes
    for (const auto& arg : args) {
        if (arg.type().name() == typeid(std::string).name()) {
            total_shm_size += std::any_cast<std::string>(arg).size();
        }
        else if (arg.type().name() == typeid(int).name()) {
            total_shm_size += sizeof(int);
        }
        else if (arg.type().name() == typeid(double).name()) {
            total_shm_size += sizeof(double);
        }
        else if (arg.type().name() == typeid(float).name()) {
            total_shm_size += sizeof(float);
        }
        else if (arg.type().name() == typeid(bool).name()) {
            total_shm_size += sizeof(bool);
        }
    }

    /// Create the shm manager and write the function call data
    SharedMemoryManager fn_call_details_shm_manager(call_id.c_str(), total_shm_size, true);

    /// Write the call_id length
    size_t offset = 0;
    size_t call_id_len = call_id.size();
    fn_call_details_shm_manager.writeData((void*)&call_id_len, sizeof(size_t), offset);
    offset += sizeof(size_t);

    /// Write the call_id
    fn_call_details_shm_manager.writeData((void*)call_id.c_str(), call_id_len, offset);
    offset += call_id_len;

    /// Write the method name length
    size_t method_name_len = name.size();
    fn_call_details_shm_manager.writeData((void*)&method_name_len, sizeof(size_t), offset);
    offset += sizeof(size_t);

    /// Write the method name
    fn_call_details_shm_manager.writeData((void*)name.c_str(), method_name_len, offset);
    offset += method_name_len;

    /// Write the number of arguments
    size_t num_args = args.size();
    fn_call_details_shm_manager.writeData((void*)&num_args, sizeof(size_t), offset);
    offset += sizeof(size_t);

    /// Write the arguments
    for (const auto& arg : args) {
        if (arg.type().name() == typeid(std::string).name()) {
            std::string arg_str = std::any_cast<std::string>(arg);
            size_t arg_len = arg_str.size();
            fn_call_details_shm_manager.writeData((void*)&arg_len, sizeof(size_t), offset);
            offset += sizeof(size_t);

            fn_call_details_shm_manager.writeData((void*)arg_str.c_str(), arg_len, offset);
            offset += arg_len;
        }
        else if (arg.type().name() == typeid(int).name()) {
            int arg_int = std::any_cast<int>(arg);
            size_t arg_len = sizeof(int);

            fn_call_details_shm_manager.writeData((void*)&arg_len, sizeof(size_t), offset);
            offset += sizeof(size_t);

            fn_call_details_shm_manager.writeData((void*)&arg_int, arg_len, offset);
            offset += arg_len;
        }
        else if (arg.type().name() == typeid(double).name()) {
            double arg_double = std::any_cast<double>(arg);
            size_t arg_len = sizeof(double);

            fn_call_details_shm_manager.writeData((void*)&arg_len, sizeof(size_t), offset);
            offset += sizeof(size_t);

            fn_call_details_shm_manager.writeData((void*)&arg_double, arg_len, offset);
            offset += arg_len;
        }
        else if (arg.type().name() == typeid(float).name()) {
            float arg_float = std::any_cast<float>(arg);

            size_t arg_len = sizeof(float);

            fn_call_details_shm_manager.writeData((void*)&arg_len, sizeof(size_t), offset);
            offset += sizeof(size_t);

            fn_call_details_shm_manager.writeData((void*)&arg_float, arg_len, offset);
            offset += arg_len;
        }
        else if (arg.type().name() == typeid(bool).name()) {
            bool arg_bool = std::any_cast<bool>(arg);
            size_t arg_len = sizeof(bool);

            fn_call_details_shm_manager.writeData((void*)&arg_len, sizeof(size_t), offset);
            offset += sizeof(size_t);

            fn_call_details_shm_manager.writeData((void*)&arg_bool, arg_len, offset);
            offset += arg_len;
        }
    }

    /// Write the function call data
    offset = 0;
    fn_call_data_shm_manager_->writeData((void*)call_id.c_str(), 128, offset);
    offset += 128;

    /// Write the size of the shm
    fn_call_data_shm_manager_->writeData((void*)&total_shm_size, sizeof(size_t), offset);
    offset += sizeof(size_t);

    /// Awake the listener
    pthread_cond_broadcast(cv_);
    pthread_mutex_unlock(mtx_);

    /// Wait for the response
    while (true) {
        pthread_mutex_lock(mtx_);
        pthread_cond_wait(cv_, mtx_);

        /// Check if a function call is completed
        if (((char*)fn_call_data_shm_manager_->getMemoryPointer())[0] != 0) {
            pthread_mutex_unlock(mtx_);
            continue;
        }
        else {
            break;
        }
    }

    if (typeid(Ret) == typeid(void)) {
        return;
    }

    /// Read the return value
    SharedMemoryManager return_size_shm_manager(
        (call_id + "_ret_size").c_str(), sizeof(size_t), false);
    size_t* ret_size = (size_t*)(return_size_shm_manager.getMemoryPointer());

    Ret ret;
    if (*ret_size > 0) {
        SharedMemoryManager ret_shm_manager((call_id + "_ret").c_str(), *ret_size, false);

        if (typeid(Ret) == typeid(std::string)) {
            ret = std::string((char*)ret_shm_manager.getMemoryPointer(), *ret_size);
        }
        else if (typeid(Ret) == typeid(int)) {
            ret = *((int*)ret_shm_manager.getMemoryPointer());
        }
        else if (typeid(Ret) == typeid(double)) {
            ret = *((double*)ret_shm_manager.getMemoryPointer());
        }
        else if (typeid(Ret) == typeid(float)) {
            ret = *((float*)ret_shm_manager.getMemoryPointer());
        }
        else if (typeid(Ret) == typeid(bool)) {
            ret = *((bool*)ret_shm_manager.getMemoryPointer());
        }
    }

    return ret;
}