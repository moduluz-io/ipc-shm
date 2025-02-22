#pragma once

#include <map>

#include <fn_registry/fn.h>

namespace IPC {
    class FunctionRegistry {
    public:
        /**
         * Registers a function with the registry.
         */
        template <typename Ret, typename... Args>
        void registerFunction(std::string name, std::function<Ret(Args...)> func);

        /**
         * Calls a function with the given name and arguments.
         */
        std::any invokefunction(std::string name, const std::vector<std::any>& args);

        /**
         * Prints the details of all registered functions.
         */
        void printRegisteredFunctions();

    private:
        /**
         * All registered functions are stored in a map with the function name as the key.
         * The value is the Function object that wraps the function.
         */
        std::map<std::string, IPC::Function> registered_fns_;
    };
}