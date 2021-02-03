#include <cstdint>
#include <cmath>
#include <iostream>
#include <vector>
#include "string.h"
#include <random>
#include <omp.h>
#include <chrono>
#include <atomic>

#include <benchmark/benchmark.h>
#include "Metrics.h"


template<typename T>
static void BM_std_log_histogram(benchmark::State& state) {
  auto h = Histogram(log_scale_t<T>(2.0, 0., 10., 10), "", "");
  uint32_t y;
  std::random_device rd;
  std::mt19937 gen(rd());
  T s;
  for (auto _ : state) {
    state.PauseTiming();
    std::vector<T> data;
    for (int i = 0; i < state.range(1); ++i)
      if constexpr (std::is_integral<T>::value) {
        std::uniform_int_distribution<T> dis(-10,20);
        data.push_back(dis(gen));
      } else {
        std::uniform_real_distribution<T> dis(T(-10),T(20));
        data.push_back(dis(gen));
      }
    auto s = data.begin();
    state.ResumeTiming();
    for (int j = 0; j < state.range(1); ++j) {
      h.count(*s++);
    }
  }
  std::cout << h << std::endl;
}
BENCHMARK_TEMPLATE(BM_std_log_histogram, double)->Args({1, 128})->Args({1, 256})->Args({1, 1024})->Args({1, 2048});

BENCHMARK_MAIN();

