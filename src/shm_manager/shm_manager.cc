#include <shm_manager/shm_manager.h>

IPC::SharedMemoryManager::SharedMemoryManager::SharedMemoryManager(const char* name, size_t size, bool create = true)
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

void IPC::SharedMemoryManager::writeData(const void* data, size_t size) {
    if (!shm_ptr || size > shm_size) {
        throw std::runtime_error("Invalid shared memory write operation.");
    }

    memcpy(shm_ptr, data, size);
}

void IPC::SharedMemoryManager::readData(void* buffer, size_t size) {
    if (!shm_ptr || size > shm_size) {
        throw std::runtime_error("Invalid shared memory read operation.");
    }

    memcpy(buffer, shm_ptr, size);
}

void IPC::SharedMemoryManager::removeMemory() {
    shm_unlink(shm_name);
}

void IPC::SharedMemoryManager::createMemory() {
    shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
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
    shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        throw std::runtime_error("Failed to open shared memory.");
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