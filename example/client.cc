#include <iostream>

#include <fn/fn.h>
#include <fn/fn_invoker.h>

int main() {
    std::cout << "Running IPC client..." << std::endl;

    IPC::FunctionInvoker invoker("sample-ipc");
    std::any ret = invoker.invoke<int>("add", { 1,2 });

    std::cout << "Result: " << std::any_cast<int>(ret) << std::endl;
    return 0;
}