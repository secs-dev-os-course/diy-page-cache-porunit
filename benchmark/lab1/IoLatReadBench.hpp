#include <benchmark/benchmark.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>

constexpr const char* TestFileName = "testfile.bin";
namespace lab1 {

constexpr const size_t KBlockSize = 2048;  // 2 KB

inline void GenerateTestFile() {
  // Generate a test file with random data
  const auto file = open(TestFileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (file < 0) {
    perror("open");
    std::quick_exit(EXIT_FAILURE);
  }

  constexpr size_t KFileSize = 1024 * KBlockSize;  // 2 MB file
  std::array<char, KBlockSize> buffer{};
  std::memset(buffer.data(), 'a', KBlockSize);

  for (size_t i = 0; i < KFileSize / KBlockSize; ++i) {
    const auto bytes_written = write(file, buffer.data(), KBlockSize);
    if (bytes_written != static_cast<ssize_t>(KBlockSize)) {
      perror("write");
      close(file);
      std::quick_exit(EXIT_FAILURE);
    }
  }

  close(file);
}

inline void IOLatencyReadBenchmark(int iterations) {
  static bool initialized = false;
  if (!initialized) {
    GenerateTestFile();
    initialized = true;
  }

  const auto file = open(TestFileName, O_RDONLY);
  if (file < 0) {
    perror("open");
    std::quick_exit(EXIT_FAILURE);
  }

  // Get file size to calculate the number of blocks
  const auto file_size = lseek(file, 0, SEEK_END);
  if (file_size == static_cast<off_t>(-1)) {
    perror("lseek");
    close(file);
    std::quick_exit(EXIT_FAILURE);
  }
  const auto num_blocks = static_cast<size_t>(file_size) / KBlockSize;

  // Seed random number generator
  std::mt19937 engine(std::random_device{}());
  std::uniform_int_distribution<size_t> dist(0, num_blocks - 1);

  std::array<char, KBlockSize> buffer{};

  for (int i = 0; i < iterations; ++i) {
    // Random block
    const auto block_num = dist(engine);
    const auto offset = static_cast<off_t>(block_num * KBlockSize);
    if (lseek(file, offset, SEEK_SET) == static_cast<off_t>(-1)) {
      perror("lseek");
      close(file);
      std::quick_exit(EXIT_FAILURE);
    }

    const auto bytes_read = read(file, buffer.data(), KBlockSize);
    if (bytes_read != static_cast<ssize_t>(KBlockSize)) {
      perror("read");
      close(file);
      std::quick_exit(EXIT_FAILURE);
    }

    // Prevent compiler optimization
    benchmark::DoNotOptimize(buffer);
  }

  close(file);
}

}  // namespace lab1
