#pragma once

#include <unistd.h> 
#include <stdexcept>
#include <sys/mman.h>
#include <cstring>
#include <fcntl.h> 

namespace IPC {
    class SharedMemoryManager {
    public:
        SharedMemoryManager(const char* name, size_t size, bool create = true);
        ~SharedMemoryManager();

        /// Write data to shared memory
        void writeData(const void* data, size_t size, size_t offset = 0);

        /// Read data from shared memory
        void readData(void* buffer, size_t size, size_t offset = 0);

        /// Get the pointer to the shared memory
        void* getMemoryPointer(size_t offset = 0);

        /// Remove shared memory (call this manually when done)
        void removeMemory();

    private:
        const char* shm_name;
        size_t shm_size;
        int shm_fd;
        void* shm_ptr;

        /// Create a new shared memory segment
        void createMemory();

        /// Open an existing shared memory segment
        void openMemory();

        /// Close the shared memory segment
        void closeMemory();
    };
}