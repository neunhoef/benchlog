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

uint64_t dummy;

template<typename T>
static void BM_std_log_histogram(benchmark::State& state) {
  auto h = Histogram(log_scale_t<T>(2.0, 0., 100000000., 10), "", "");
  for (auto _ : state) {
    state.PauseTiming();
    uint32_t y;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::vector<T> data;

    typename std::conditional<
      std::is_integral<T>::value,
      std::uniform_int_distribution<T>,
      std::uniform_real_distribution<T>>::type dis(0.,1000000000.);

    for (int i = 0; i < state.range(1); ++i) {
      data.push_back(dis(gen));
    }
    auto s = data.begin();
    state.ResumeTiming();
    for (int j = 0; j < state.range(1); ++j) {
      h.count(*s++);
    }
  }
  std::string hs;
  h.toPrometheus(hs);
}
BENCHMARK_TEMPLATE(BM_std_log_histogram, uint64_t)->Args({1, 128})->Args({1, 256})->Args({1, 512})->Args({1, 1024})->Args({1, 2048});
BENCHMARK_TEMPLATE(BM_std_log_histogram, double)->Args({1, 128})->Args({1, 256})->Args({1, 512})->Args({1, 1024})->Args({1, 2048});
BENCHMARK_TEMPLATE(BM_std_log_histogram, float)->Args({1, 128})->Args({1, 256})->Args({1, 512})->Args({1, 1024})->Args({1, 2048});

template<typename T>
static void BM_gauge_add(benchmark::State& state) {
  auto g = Gauge<T>(T(0.), "", "");
  for (auto _ : state) {
    state.PauseTiming();
    uint32_t y;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::vector<T> data;

    typename std::conditional<
      std::is_integral<T>::value,
      std::uniform_int_distribution<T>,
      std::uniform_real_distribution<T>>::type dis(-1.,1.);

    for (int i = 0; i < state.range(1); ++i) {
      data.push_back(dis(gen));
    }
    auto s = data.begin();
    state.ResumeTiming();
    for (int j = 0; j < state.range(1); ++j) {
      g += *s++;
    }
  }
  std::string hs;
  g.toPrometheus(hs);
}
BENCHMARK_TEMPLATE(BM_gauge_add, uint64_t)->Args({1, 128})->Args({1, 256})->Args({1, 512})->Args({1, 1024})->Args({1, 2048});
BENCHMARK_TEMPLATE(BM_gauge_add, double)->Args({1, 128})->Args({1, 256})->Args({1, 512})->Args({1, 1024})->Args({1, 2048});
BENCHMARK_TEMPLATE(BM_gauge_add, float)->Args({1, 128})->Args({1, 256})->Args({1, 512})->Args({1, 1024})->Args({1, 2048});


BENCHMARK_MAIN();

