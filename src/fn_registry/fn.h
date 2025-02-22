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
        template <typename Ret, typename... Args>
        Function(std::function<Ret(Args...)> func);

        /**
         * Calls the function with the arguments unpacked from the vector.
         */
        std::any invoke(const std::vector<std::any>& args);

    private:
        std::function<std::any(const std::vector<std::any>&)> function_;
    };
}