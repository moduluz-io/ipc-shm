#include <iostream>

#include <fn_registry/fn.h>

int main() {
    std::cout << "Server example running..." << std::endl;

    IPC::Function fn("add", std::function<int(int, int)>([](int a, int b) { return a + b; }));
    fn.printInfo();

    std::vector<std::any> args = { 1, 2 };
    std::any result = fn.invoke(args);
    std::cout << "Result: " << std::any_cast<int>(result) << std::endl;

    return 0;
}