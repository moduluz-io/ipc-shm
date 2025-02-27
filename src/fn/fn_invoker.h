#pragma once

#include <string>
#include <any>
#include <vector>
#include <pthread.h>

#include <shm_manager/shm_manager.h>

namespace IPC {
    class FunctionInvoker {
    public:
        FunctionInvoker(std::string channel_name);
        ~FunctionInvoker();

        /**
         * Calls a function with the given name and arguments which is registered in the function registry
         * of another process.
         */
        template <typename Ret>
        Ret invoke(std::string name, const std::vector<std::any>& args);
    private:
        /**
             * The mutex and the condition variable are used to synchronize access to the registry.
             */
        pthread_mutex_t* mtx_;
        pthread_cond_t* cv_;

        /**Shared memory that hold the mutex and condition variable */
        SharedMemoryManager* sync_shm_manager_;

        /** Shared memory that holds the basic details of the function calls, like the SHM name and size
         * that holds the full function call details.
         */
        SharedMemoryManager* fn_call_data_shm_manager_;
    };
}