#include <fn_registry/fn.h>

template <typename Func>
IPC::Function::Function(Func&& f) : func([f = std::forward<Func>(f)](std::vector<std::any> args) -> std::any {
    return callWithArgs(f, std::move(args));
    }) {
}

std::any IPC::Function::operator()(std::vector<std::any> args) {
    return func(std::move(args));
}

template <typename Func, typename... Args>
std::any IPC::Function::callWithArgs(Func& f, std::vector<std::any>&& args) {
    if (sizeof...(Args) == args.size()) {
        return std::apply(f, makeTuple<Args...>(std::move(args)));
    }
    return {}; // Return empty std::any if argument count mismatches
}

template <typename... Args>
std::tuple<Args...> IPC::Function::makeTuple(std::vector<std::any>&& args) {
    return makeTupleHelper<Args...>(std::move(args), std::index_sequence_for<Args...>{});
}

template <typename... Args, size_t... Is>
std::tuple<Args...> IPC::Function::makeTupleHelper(std::vector<std::any>&& args, std::index_sequence<Is...>) {
    return std::make_tuple(std::any_cast<Args>(args[Is])...);
}