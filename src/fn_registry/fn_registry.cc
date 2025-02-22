#include <fn_registry/fn_registry.h>

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