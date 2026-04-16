#pragma once
#include <unistd.h>

// FileDescriptor — RAII wrapper around a raw file descriptor.
//
// Ensures that the file descriptor is always closed when the object
// goes out of scope, regardless of the exit path (return, exception, etc.).
//
// Non-copyable by design: two objects owning the same fd would cause
// a double close() on destruction — undefined behaviour.
// Use get() to pass the raw fd to syscalls.
//
// The constructor is explicit to prevent silent implicit conversions.
// Without it, any plain int could silently become a FileDescriptor:
//   afficher(42);            // sans explicit : compile et ferme fd 42 en silence
//   FileDescriptor fd = 42;  // sans explicit : compile
//   FileDescriptor fd(42);   // toujours ok
//   afficher(fd);            // toujours ok
//
// close() lets you close the fd manually before the object is destroyed.
// Both close() and the destructor check _fd != -1 to prevent double close:
// if close() is called manually, _fd is set to -1 so the destructor skips it.
//
// Usage:
//   int fd = socket(AF_INET, SOCK_STREAM, 0);
//   if (fd < 0) throw perror(...);
//   FileDescriptor serverFd(fd);
//   if (bind(serverFd.get(), ...) < 0)
//       throw perror(...); // close() called automatically
//   // end of scope -> close() called automatically

class FileDescriptor {
    public:
        explicit FileDescriptor(int fd) : _fd(fd) {}

        ~FileDescriptor() {
            if (_fd != -1)
                ::close(_fd);
        }

        int  get() const { return _fd; }

        void close() {
            if (_fd != -1) {
                ::close(_fd);
                _fd = -1;
            }
        }

    private:
        int _fd;
        FileDescriptor(const FileDescriptor&);
        FileDescriptor& operator=(const FileDescriptor&);
};

