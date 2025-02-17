#include <benchmark/benchmark.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>

#include "lab2/Api.hpp"

constexpr const char* KTestFileName = "testfile.bin";

namespace lab1 {

constexpr const size_t KBenchBlockSize = 2048;  // 2 KB

inline void GenerateTestFileCache() {
  // Generate a test file with random data
  const auto file = lab2_open(KTestFileName);
  if (file < 0) {
    perror("lab2_open");
    std::quick_exit(EXIT_FAILURE);
  }

  constexpr size_t KFileSize = 1024 * KBenchBlockSize;  // 2 MB file
  std::array<char, KBenchBlockSize> buffer{};
  std::memset(buffer.data(), 'a', KBenchBlockSize);

  for (size_t i = 0; i < KFileSize / KBenchBlockSize; ++i) {
    const auto bytes_written = lab2_write(file, buffer.data(), KBenchBlockSize);
    if (bytes_written != static_cast<ssize_t>(KBenchBlockSize)) {
      perror("lab2_write");
      lab2_close(file);
      std::quick_exit(EXIT_FAILURE);
    }
  }

  lab2_close(file);
}

inline void IOLatencyReadCacheBenchmark(int iterations) {
  static bool initialized = false;
  if (!initialized) {
    GenerateTestFileCache();
    initialized = true;
  }

  const auto file = lab2_open(KTestFileName);
  if (file < 0) {
    perror("lab2_open");
    std::quick_exit(EXIT_FAILURE);
  }

  // Get file size to calculate the number of blocks
  const auto file_size = lab2_lseek(file, 0, SEEK_END);
  if (file_size == static_cast<off_t>(-1)) {
    perror("lab2_lseek");
    lab2_close(file);
    std::quick_exit(EXIT_FAILURE);
  }
  const auto num_blocks = static_cast<size_t>(file_size) / KBenchBlockSize;

  // Seed random number generator
  std::mt19937 engine(std::random_device{}());
  std::uniform_int_distribution<size_t> dist(0, num_blocks - 1);

  std::array<char, KBenchBlockSize> buffer{};

  for (int i = 0; i < iterations; ++i) {
    // Random block
    const auto block_num = dist(engine);
    const auto offset = static_cast<off_t>(block_num * KBenchBlockSize);
    if (lab2_lseek(file, offset, SEEK_SET) == static_cast<off_t>(-1)) {
      perror("lab2_lseek");
      lab2_close(file);
      std::quick_exit(EXIT_FAILURE);
    }

    const auto bytes_read = lab2_read(file, buffer.data(), KBenchBlockSize);
    if (bytes_read != static_cast<ssize_t>(KBenchBlockSize)) {
      perror("lab2_read");
      lab2_close(file);
      std::quick_exit(EXIT_FAILURE);
    }

    // Prevent compiler optimization
    benchmark::DoNotOptimize(buffer);
  }

  lab2_close(file);
}

}  // namespace lab1
