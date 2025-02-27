#include <iostream>

#include <fn/fn.h>
#include <fn/fn_registry.h>

int sum(int a, int b) {
    return a + b;
}

int main() {
    std::cout << "Server example running..." << std::endl;

    IPC::FunctionRegistry registry("sample-ipc");
    registry.registerFunction<int, int, int>(std::string("add"), std::function<int(int, int)>(sum));

    registry.listen();

    return 0;
}