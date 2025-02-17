#include "./Cache.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace lab2 {

FIFOCache::FIFOCache(size_t capacity) : capacity_(capacity) {
}

FIFOCache::~FIFOCache() {
  Flush();
  // Close all open file descriptors
  for (auto& [user_fd, os_fd] : open_files_) {
    close(os_fd);
  }
}

void FIFOCache::Flush() {
  const std::unique_lock<std::shared_mutex> lock(cache_mutex_);

  for (auto& block : cache_list_) {
    if (block.is_dirty) {
      // If the block is marked as dirty, write it back to disk
      if (WriteBlockToDisk(block) == -1) {
        throw std::runtime_error("Failed to flush dirty block to disk");
      }
      block.is_dirty = false;
    }
  }
}

int FIFOCache::OpenFile(const std::string& path) {
  const std::unique_lock<std::shared_mutex> lock(cache_mutex_);

  const int os_fd = open(path.c_str(), O_RDWR | O_CREAT | O_DIRECT, 0644);
  if (os_fd == -1) {
    return -1;
  }

  const int user_fd = next_fd_++;
  open_files_[user_fd] = os_fd;
  file_positions_[user_fd] = 0;

  return user_fd;
}

int FIFOCache::CloseFile(int fd) {
  const std::unique_lock<std::shared_mutex> lock(cache_mutex_);

  auto iter = open_files_.find(fd);
  if (iter == open_files_.end()) {
    return -1;
  }

  // Flush all blocks related to this file
  for (auto block_it = cache_list_.begin(); block_it != cache_list_.end();) {
    if ((block_it->block_id >> KFdOffset) == fd) {
      if (block_it->is_dirty) {
        WriteBlockToDisk(*block_it);
      }

      const uint64_t block_id = block_it->block_id;
      map_.erase(block_id);
      block_it = cache_list_.erase(block_it);

      continue;
    }
    ++block_it;
  }

  // Close the OS file descriptor
  if (close(iter->second) != 0) {
    return -1;
  }

  // Remove the file from open_files_ and file_positions_
  open_files_.erase(iter);
  file_positions_.erase(fd);

  return 0;
}

ssize_t FIFOCache::ReadFile(int fd, char* buf, size_t size) {
  const std::unique_lock<std::shared_mutex> lock(cache_mutex_);

  auto iter = open_files_.find(fd);
  if (iter == open_files_.end()) {
    return -1;  // Invalid file descriptor
  }

  const int os_fd = iter->second;
  off_t current_pos = file_positions_[fd];
  size_t bytes_read_total = 0;

  while (bytes_read_total < size) {
    const int block_num = current_pos / KBlockSize;
    const size_t block_offset = current_pos % KBlockSize;
    const size_t bytes_to_read = std::min(KBlockSize - block_offset, size - bytes_read_total);

    // Create a unique block identifier, e.g., (fd << 32) | block_num
    const uint64_t block_id = (static_cast<uint64_t>(fd) << KFdOffset) | block_num;

    Block* block = GetBlock(block_id);
    if (block == nullptr) {
      // Load block from disk
      AlignedVec block_data(KBlockSize, 0);
      const ssize_t bytes_read =
          pread(os_fd, block_data.data(), KBlockSize, block_num * KBlockSize);
      if (bytes_read == -1) {
        return -1;  // Read error
      }
      block_data.resize(bytes_read);
      block = LoadBlock(block_id, block_data);
      if (block == nullptr) {
        return -1;  // Failed to load block
      }
    }

    // Copy data from block to buffer
    const size_t copy_size = std::min(bytes_to_read, block->data.size() - block_offset);
    std::memcpy(buf + bytes_read_total, block->data.data() + block_offset, copy_size);
    bytes_read_total += copy_size;
    current_pos += copy_size;

    if (copy_size < bytes_to_read) {
      break;  // Reached EOF
    }
  }

  file_positions_[fd] = current_pos;
  return bytes_read_total;
}

ssize_t FIFOCache::WriteFile(int fd, const char* buf, size_t size) {
  const std::unique_lock<std::shared_mutex> lock(cache_mutex_);

  auto iter = open_files_.find(fd);
  if (iter == open_files_.end()) {
    return -1;  // Invalid file descriptor
  }

  const int os_fd = iter->second;
  off_t current_pos = file_positions_[fd];
  size_t bytes_written_total = 0;

  while (bytes_written_total < size) {
    const int block_num = current_pos / KBlockSize;
    const size_t block_offset = current_pos % KBlockSize;
    const size_t bytes_to_write = std::min(KBlockSize - block_offset, size - bytes_written_total);

    // Create a unique block identifier, e.g., (fd << 32) | block_num
    const uint64_t block_id = (static_cast<uint64_t>(fd) << KFdOffset) | block_num;

    Block* block = GetBlock(block_id);
    if (block == nullptr) {
      AlignedVec block_data(KBlockSize, 0);
      const ssize_t bytes_read =
          pread(os_fd, block_data.data(), KBlockSize, block_num * KBlockSize);
      if (bytes_read == -1) {
        return -1;  // Read error
      }

      PutBlock(block_id, block_data.data(), KBlockSize);
      block = GetBlock(block_id);
      if (block == nullptr) {
        return -1;  // Failed to load block
      }
    }

    // Ensure block size is sufficient
    if (block->data.size() < block_offset + bytes_to_write) {
      return -1;  // Invalid block size
    }

    // Write data from buffer to block
    std::memcpy(block->data.data() + block_offset, buf + bytes_written_total, bytes_to_write);
    block->is_dirty = true;
    bytes_written_total += bytes_to_write;
    current_pos += bytes_to_write;
  }

  file_positions_[fd] = current_pos;
  return bytes_written_total;
}

off_t FIFOCache::LSeek(int fd, off_t offset, int whence) {
  const std::unique_lock<std::shared_mutex> lock(cache_mutex_);

  auto iter = open_files_.find(fd);
  if (iter == open_files_.end()) {
    return -1;  // Invalid file descriptor
  }

  off_t new_pos = 0;
  switch (whence) {
    case SEEK_SET:
      new_pos = offset;
      break;
    case SEEK_CUR:
      new_pos = file_positions_[fd] + offset;
      break;
    case SEEK_END: {
      struct stat stat_data = {};
      if (fstat(iter->second, &stat_data) == -1) {
        return -1;  // fstat failed
      }
      new_pos = stat_data.st_size + offset;
      break;
    }
    default:
      return -1;  // Invalid 'whence'
  }

  if (new_pos < 0) {
    return -1;  // Invalid position
  }

  file_positions_[fd] = new_pos;
  return new_pos;
}

int FIFOCache::SyncFile(int fd) {
  const std::unique_lock<std::shared_mutex> lock(cache_mutex_);

  auto iter = open_files_.find(fd);
  if (iter == open_files_.end()) {
    return -1;  // Invalid file descriptor
  }

  // Flush all dirty blocks related to this file
  for (auto& block : cache_list_) {
    if ((block.block_id >> KFdOffset) == fd && block.is_dirty) {
      if (WriteBlockToDisk(block) == -1) {
        return -1;  // Write error
      }
      block.is_dirty = false;
    }
  }

  // Sync the OS file descriptor
  if (fsync(iter->second) == -1) {
    return -1;  // fsync failed
  }

  return 0;  // Success
}

// Private Methods

Block* FIFOCache::GetBlock(uint64_t block_id) {
  auto map_it = map_.find(block_id);
  if (map_it == map_.end()) {
    return nullptr;
  }

  // В FIFO порядок доступа не изменяется.
  return &(*(map_it->second));
}

void FIFOCache::PutBlock(uint64_t block_id, const char* block_data, size_t data_size) {
  auto it = map_.find(block_id);
  if (it != map_.end()) {
    Block& block = *(it->second);
    block.data.assign(block_data, block_data + data_size);
    block.is_dirty = true;
    // В FIFO порядок не обновляем, поэтому не вызываем Touch.
  } else {
    // Block not in cache, need to add it
    if (cache_list_.size() >= capacity_) {
      EvictIfNeeded();
    }

    // Для FIFO вставляем новый блок в конец списка
    cache_list_.emplace_back(block_id, AlignedVec(data_size));
    auto new_it = std::prev(cache_list_.end());
    new_it->data.assign(block_data, block_data + data_size);
    new_it->is_dirty = true;
    map_[block_id] = new_it;
  }
}

Block* FIFOCache::LoadBlock(uint64_t block_id, const AlignedVec& data) {
  // Evict if cache is full
  EvictIfNeeded();

  // Для FIFO вставляем новый блок в конец списка
  cache_list_.emplace_back(block_id, data, false);
  auto new_it = std::prev(cache_list_.end());
  map_[block_id] = new_it;
  return &(*new_it);
}

void FIFOCache::EvictIfNeeded() {
  if (cache_list_.size() < capacity_) {
    return;
  }

  // Для FIFO эвиктируем самый старый блок (находящийся в начале списка)
  Block& block_to_evict = cache_list_.front();
  if (block_to_evict.is_dirty) {
    WriteBlockToDisk(block_to_evict);
  }

  map_.erase(block_to_evict.block_id);
  cache_list_.pop_front();
}

int FIFOCache::WriteBlockToDisk(Block& block) {
  const int fd = block.block_id >> KFdOffset;
  const int block_num = block.block_id & 0xFFFFFFFF;

  auto iter = open_files_.find(fd);
  if (iter == open_files_.end()) {
    return -1;  // Invalid file descriptor
  }

  const ssize_t bytes_written = pwrite(
      iter->second, block.data.data(), block.data.size(), static_cast<off_t>(block_num) * KBlockSize
  );
  if (bytes_written == -1) {
    return -1;  // Write error
  }

  return 0;  // Success
}

void FIFOCache::Touch(std::list<Block>::iterator /*it*/) {
  // В FIFO порядок доступа не изменяется, поэтому данная функция не выполняет никаких действий.
}

}  // namespace lab2
