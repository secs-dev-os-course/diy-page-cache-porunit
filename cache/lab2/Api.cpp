#include <sys/types.h>

#include <cstddef>

#include "./Cache.hpp"

static lab2::FIFOCache cache(1024);

#ifdef __cplusplus
extern "C" {
#endif

int lab2_open(const char* path) {
  return cache.OpenFile(path);
}

int lab2_close(int fd) {
  return cache.CloseFile(fd);
}

ssize_t lab2_read(int fd, void* buf, size_t count) {
  return cache.ReadFile(fd, static_cast<char*>(buf), count);
}

ssize_t lab2_write(int fd, const void* buf, size_t count) {
  return cache.WriteFile(fd, static_cast<const char*>(buf), count);
}
off_t lab2_lseek(int fd, off_t offset, int whence) {
  return cache.LSeek(fd, offset, whence);
}

int lab2_fsync(int fd) {
  return cache.SyncFile(fd);
}

#ifdef __cplusplus
}
#endif