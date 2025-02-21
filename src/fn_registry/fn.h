#pragma once

#include <vector>
#include <functional>
#include <any>
#include <tuple>

namespace IPC {
    /**
     * This is a class that wraps a function and allows it to be called with a vector of std::any arguments.
     * The function is called with the arguments unpacked from the vector.
     */
    class Function {
    public:
        /**
         * Creates a Function object from a lambda or a function pointer.
         */
        template <typename Func>
        Function(Func&& f);

        std::any operator()(std::vector<std::any> args);

    private:
        /// Represents a function that takes a vector of std::any arguments and returns a std::any
        std::function<std::any(std::vector<std::any>)> func;

        // Call function with unpacked arguments and return the result
        template <typename Func, typename... Args>
        static std::any callWithArgs(Func& f, std::vector<std::any>&& args);

        // Convert std::vector<std::any> to a tuple of expected argument types
        template <typename... Args>
        static std::tuple<Args...> makeTuple(std::vector<std::any>&& args);

        // Unpack the std::any into the correct types for the tuple
        template <typename... Args, size_t... Is>
        static std::tuple<Args...> makeTupleHelper(std::vector<std::any>&& args, std::index_sequence<Is...>);
    };
}