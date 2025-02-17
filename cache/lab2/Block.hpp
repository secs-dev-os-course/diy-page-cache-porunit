#pragma once

#include <sys/types.h>

#include <cstddef>
#include <list>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

template <typename T, size_t Alignment>
class aligned_allocator : public std::allocator<T> {
public:
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;

  template <typename U>
  struct rebind {
    typedef aligned_allocator<U, Alignment> other;
  };

  pointer allocate(size_type n, const void* hint = 0) {
    (void)hint;
    void* ptr = nullptr;
    if (posix_memalign(&ptr, Alignment, n * sizeof(T)) != 0)
      throw std::bad_alloc();
    return static_cast<pointer>(ptr);
  }

  void deallocate(pointer p, size_type /*n*/) {
    free(p);
  }
};

static constexpr size_t KBlockSize = 4096;
static constexpr size_t KFdOffset = 32;

using AlignedVec = std::vector<char, aligned_allocator<char, KBlockSize>>;

struct Block {
  // Unique identifier for the block (e.g., (fd << 32) |
  // block_num)
  uint64_t block_id;
  // Data contained in the block
  AlignedVec data;
  // Flag indicating if the block has been modified
  bool is_dirty;

  Block(uint64_t id, const AlignedVec& data_vec, bool dirty = false)
      : block_id(id), data(data_vec.begin(), data_vec.end()), is_dirty(dirty) {
  }
};