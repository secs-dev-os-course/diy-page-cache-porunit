#pragma once

#include <sys/types.h>

#include <cstddef>
#include <list>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "./Block.hpp"

namespace lab2 {

class FIFOCache {
public:
  // Constructor that initializes the cache with a maximum size in number of
  // blocks.
  explicit FIFOCache(size_t capacity);

  ~FIFOCache();

  // Non-copyable and non-movable.
  FIFOCache(const FIFOCache&) = delete;
  FIFOCache& operator=(const FIFOCache&) = delete;
  FIFOCache(FIFOCache&&) = delete;
  FIFOCache& operator=(FIFOCache&&) = delete;

  // API functions
  int OpenFile(const std::string& path);
  int CloseFile(int fd);
  ssize_t ReadFile(int fd, char* buf, size_t size);
  ssize_t WriteFile(int fd, const char* buf, size_t size);
  off_t LSeek(int fd, off_t offset, int whence);
  int SyncFile(int fd);

private:
  size_t capacity_;
  std::list<Block> cache_list_;
  std::unordered_map<uint64_t, std::list<Block>::iterator> map_;
  std::unordered_map<int, int> open_files_;        // Maps user_fd to OS fd
  std::unordered_map<int, off_t> file_positions_;  // Maps user_fd to current file position
  int next_fd_ = 3;  // Starting user-level fd (0,1,2 are standard fds)

  std::shared_mutex cache_mutex_;  // Mutex for synchronizing access to the cache

  // Moves a block to the front of the cache list, indicating it was recently
  // used. (В FIFO этот метод не выполняет никаких действий.)
  void Touch(std::list<Block>::iterator it);

  // Retrieves a block from the cache. Returns nullptr if not found.
  Block* GetBlock(uint64_t block_id);

  // Loads a block into the cache from disk
  Block* LoadBlock(uint64_t block_id, const AlignedVec& data);

  // Evicts the oldest block (FIFO) if the cache exceeds capacity
  void EvictIfNeeded();

  // Writes a dirty block back to disk
  int WriteBlockToDisk(Block& block);

  // Saves all modified blocks back to disk.
  void Flush();

  // Adds a block to the cache or updates an existing block.
  // If the block already exists, it updates the data and marks it as dirty.
  void PutBlock(uint64_t block_id, const char* block_data, size_t data_size);
};

}  // namespace lab2
