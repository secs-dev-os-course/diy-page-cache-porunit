#include <pthread.h>  // Include pthread library

#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>  // C++11 thread library for std::thread::hardware_concurrency()
#include <vector>

#include "lab1/IoLatReadCacheBench.hpp"
#include "lab1/IoLatReadBench.hpp"

namespace lab1::app {

namespace {

struct ThreadData {
  int iterations{};
  std::string benchmark;
};

void* RunBenchmark(void* arg) {
  auto* data = static_cast<ThreadData*>(arg);

  if (data->benchmark == "io") {
    IOLatencyReadBenchmark(data->iterations);
    std::cout << "IO latency read benchmark completed with " << data->iterations
              << " iterations.\n";
  } else if (data->benchmark == "cache") {
    IOLatencyReadCacheBenchmark(data->iterations);
    std::cout << "IO latency read cache benchmark completed with " << data->iterations
              << " iterations.\n";
  } else {
    std::cerr << "Invalid benchmark specified. Use 'binary' or 'io'.\n";
  }

  return nullptr;
}

void Main(int argc, const std::vector<std::string>& args) {
  if (argc < 3) {
    std::cerr << "Usage: " << args[0] << " cache <iterations>\n";
    return;
  }

  unsigned int num_threads = std::thread::hardware_concurrency();
  std::vector<pthread_t> threads(num_threads);
  std::vector<ThreadData> thread_data(num_threads);

  for (unsigned int i = 0; i < num_threads; ++i) {
    thread_data[i].iterations = std::stoi(args[2]);
    thread_data[i].benchmark = args[1];

    pthread_create(&threads[i], nullptr, RunBenchmark, &thread_data[i]);
  }

  for (unsigned int i = 0; i < num_threads; ++i) {
    pthread_join(threads[i], nullptr);
  }
}

}  // namespace

}  // namespace lab1::app

int main(int argc, char* argv[]) {
  std::vector<std::string> args(argv, argv + argc);
  lab1::app::Main(argc, args);
}
