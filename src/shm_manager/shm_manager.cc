#include <shm_manager/shm_manager.h>

IPC::SharedMemoryManager::SharedMemoryManager::SharedMemoryManager(std::string name, size_t size, bool create)
    : shm_name(name), shm_size(size), shm_fd(-1), shm_ptr(nullptr) {
    if (create) {
        createMemory();
    }
    else {
        openMemory();
    }
}

IPC::SharedMemoryManager::~SharedMemoryManager() {
    closeMemory();
}

void IPC::SharedMemoryManager::writeData(const void* data, size_t size, size_t offset) {
    if (!shm_ptr || size > shm_size || offset + size > shm_size) {
        throw std::runtime_error("Invalid shared memory write operation.");
    }

    memcpy((char*)shm_ptr + offset, data, size);
}

void IPC::SharedMemoryManager::readData(void* buffer, size_t size, size_t offset) {
    if (!shm_ptr || size > shm_size || offset + size > shm_size) {
        throw std::runtime_error("Invalid shared memory read operation.");
    }

    memcpy(buffer, (void*)((char*)shm_ptr + offset), size);
}

void* IPC::SharedMemoryManager::getMemoryPointer(size_t offset) {
    if (!shm_ptr || offset > shm_size) {
        throw std::runtime_error("Invalid shared memory pointer operation.");
    }

    return (char*)shm_ptr + offset;
}

void IPC::SharedMemoryManager::removeMemory() {
    shm_unlink(shm_name.c_str());
}

void IPC::SharedMemoryManager::createMemory() {
    shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        throw std::runtime_error("Failed to create shared memory.");
    }

    // Set the size of the shared memory
    if (ftruncate(shm_fd, shm_size) == -1) {
        throw std::runtime_error("Failed to set shared memory size.");
    }

    // Map shared memory into process address space
    shm_ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        throw std::runtime_error("Failed to map shared memory.");
    }
}

void IPC::SharedMemoryManager::openMemory() {
    while (true) {
        shm_fd = shm_open(shm_name.c_str(), O_RDWR, 0666);

        if (shm_fd != -1) {
            break;
        }
    }

    shm_ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        throw std::runtime_error("Failed to map shared memory.");
    }
}

void IPC::SharedMemoryManager::closeMemory() {
    if (shm_ptr) {
        munmap(shm_ptr, shm_size);
        shm_ptr = nullptr;
    }
    if (shm_fd != -1) {
        close(shm_fd);
        shm_fd = -1;
    }
}