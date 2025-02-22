#include <fn_registry/fn.h>

#include <stdexcept>

template <typename Ret, typename... Args>
IPC::Function::Function(std::function<Ret(Args...)> func) {
    /**
     * Creates an anonymous function that takes a vector of std::any arguments
     * and calls the function with the arguments unpacked from the vector.
     */
    function_ = [func](const std::vector<std::any>& args) -> std::any {
        if (args.size() != sizeof...(Args)) throw std::runtime_error("Incorrect argument count");
        auto tuple_args = vector_to_tuple<Args...>(args);
        return std::apply(func, tuple_args);
        };
}

std::any IPC::Function::invoke(const std::vector<std::any>& args) {
    return function_(args);
}