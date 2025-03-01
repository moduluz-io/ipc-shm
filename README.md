# IPC Shm

A library written in C++ that allows a process to expose its functions through a shared memory so that other processes running in the same device can call them while the process is running.

# Basics

- This IPC is based on a concept where one process will expose some functions that can be executed by other processes in the device.
- All other process can connect to the process which exposes the functions and call any of the exposed functions.
- A shared memory is used to store the data that needs to be transferred between the processes.

## Supported Data types

- String, int, double, float, bool are the data types that you can use for the return type/parameter types for the exposed functions.

## Usage

- Example of a process exposing its functions:

```cpp
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
```

> NOTE: Here "sample-ipc" is the channel name that is used for the communication between the processes. Both client and server process should use the same channel name.

- Example of a client that consumes the functions exposed by the server:

```cpp
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
```
