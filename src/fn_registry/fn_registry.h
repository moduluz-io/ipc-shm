#pragma once

#include <map>
#include <pthread.h>

#include <fn_registry/fn.h>
#include <shm_manager/shm_manager.h>

namespace IPC {
    class FunctionRegistry {
    public:
        FunctionRegistry(std::string channel_name);
        ~FunctionRegistry();

        /**
         * Registers a function with the registry.
         */
        template <typename Ret, typename... Args>
        void registerFunction(std::string name, std::function<Ret(Args...)> func);

        /**
         * Listen for the incoming function calls.
         *
         * If the fn_call data sh contains a non-zero value, then it means a function call is in progress.
         */
        void listen();
    private:
        /**
         * Calls a function with the given name and arguments.
         */
        std::any invokefunction(std::string name, const std::vector<std::any>& args);

        /**
         * Returns the number of arguments for the given function.
         * If the function is not registered, then it throws a runtime error.
         *
         * [name] The name of the function.
         */
        int getArgCount(std::string name);

        /**
         * Returns the argument type at the given index.
         *
         * [name] The name of the function.
         * [index] The index of the argument.
         */
        std::string getArgType(std::string name, int index);

        /**
         * Prints the details of all registered functions.
         */
        void printRegisteredFunctions();

        /**
         * All registered functions are stored in a map with the function name as the key.
         * The value is the Function object that wraps the function.
         */
        std::map<std::string, IPC::Function> registered_fns_;

        /**
         * The name of the registry, usually this is used for the IPC channel name (In this case
         * it is used for creating the shared memeory).
         */
        std::string channel_name_;

        /**
         * Shared memory used to store the function registry data.
         */
        SharedMemoryManager* sync_shm_manager_;
        SharedMemoryManager* fn_call_data_shm_manager_;

        /**
         * The mutex and the condition variable are used to synchronize access to the registry.
         */
        pthread_mutex_t* mtx_;
        pthread_cond_t* cv_;
    };
}