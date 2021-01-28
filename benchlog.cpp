#include <cstdint>
#include <cmath>
#include <iostream>
#include <vector>
#include "string.h"
#include <random>

#include <benchmark/benchmark.h>

static int count = 0;
static uint64_t dummy64 = 0;
static double dummydouble = 0;

#if 0
static void BM_OurHistogram(benchmark::State& state) {
  constexpr double const base = 10.0;
  constexpr double const low = 0;
  constexpr double const high = 1000;
  constexpr size_t const n = 11;
  double nn = -1.0 * (n - 1);
  std::vector<double> delim;
  delim.resize(n-1);
  for (auto& i : delim) {
    i = (high - low) * std::pow(base, nn) + low;
    nn += 1.0;
  }
  double const div = (high - low) * std::pow(base, -10);
  double const lbase = std::log(base);

  uint64_t counts[n];
  for (size_t i = 0; i < n; ++i) {
    counts[i] = 0;
  }
  double v = 1.0;
  for (auto _ : state) {
    size_t x = static_cast<size_t>(1+std::floor(log((v - low)/div)/lbase));
    counts[x]++;
    v += 1.0;
    v = (v > 999.0) ? 1.0 : v;
  }
  dummydouble += v;
  for (size_t i = 0; i < 10; ++i) {
    dummy64 += counts[i];
  }
#if 0
  if (++count == 1) {
    std::cout << "Limit              Count\n";
    for (size_t i = 0; i < 10; ++i) {
      std::cout << delim[i] << "     " << counts[i] << "\n";
    }
  }
#endif
}
BENCHMARK(BM_OurHistogram);

static void BM_NewHistogram(benchmark::State& state) {
  constexpr double const base = 10.0;
  constexpr double const low = 0;
  constexpr double const high = 1000;
  constexpr size_t const n = 11;
  double nn = -1.0 * (n - 1);
  std::vector<double> delim;
  delim.resize(n-1);
  for (auto& i : delim) {
    i = (high - low) * std::pow(base, nn) + low;
    nn += 1.0;
  }
  double const div = (high - low) * std::pow(base, -10);
  double const lbase = std::log(base);

  uint64_t counts[n];
  for (size_t i = 0; i < n; ++i) {
    counts[i] = 0;
  }
  double v = 1.0;
  for (auto _ : state) {
    uint32_t x = ((uint32_t) (((uint64_t) v >> 52) & 0x7ff)) / 10;
    counts[x]++;
    v += 1.0;
    v = (v > 999.0) ? 1.0 : v;
  }
#if 0
  if (++count == 1) {
    std::cout << "Limit              Count\n";
    for (size_t i = 0; i < 10; ++i) {
      std::cout << delim[i] << "     " << counts[i] << "\n";
    }
  }
#endif
  dummydouble += v;
  for (size_t i = 0; i < 10; ++i) {
    dummy64 += counts[i];
  }
}
BENCHMARK(BM_NewHistogram);
#endif

static void BM_Log2Empty(benchmark::State& state) {
  double vtab[10];
  double v = 1.0;
  for (int i = 0; i < 10; ++i) {
    vtab[i] = v;
    v *= 10.0;
  }
  double r = 0;
  size_t i = 0;
  for (auto _ : state) {
    //r += vtab[i];
    benchmark::DoNotOptimize(i = (i >= 9) ? 0 : i+1);
  }
  dummydouble += r;
}
BENCHMARK(BM_Log2Empty);

static void BM_Log2(benchmark::State& state) {
  double vtab[10];
  double v = 1.0;
  for (int i = 0; i < 10; ++i) {
    vtab[i] = v;
    v *= 10.0;
  }
  double r = 0;
  size_t i = 0;
  for (auto _ : state) {
    r += std::log2(vtab[i]);
    i = (i >= 9) ? 0 : i+1;
  }
  dummydouble += r;
}
BENCHMARK(BM_Log2);

static void BM_Log(benchmark::State& state) {
  double vtab[10];
  double v = 1.0;
  for (int i = 0; i < 10; ++i) {
    vtab[i] = v;
    v *= 10.0;
  }
  double r = 0;
  size_t i = 0;
  for (auto _ : state) {
    r += std::log(vtab[i]);
    i = (i >= 9) ? 0 : i+1;
  }
  dummydouble += r;
}
BENCHMARK(BM_Log);

static inline uint32_t log2rough(double x) {
  uint64_t y;
  memcpy(&y, &x, 8);
  return ((uint32_t)(y >> 52) & 0x7ff) - 1023;
}

static void BM_Log2Rough(benchmark::State& state) {
  double vtab[10];
  double v = 1.0;
  for (int i = 0; i < 10; ++i) {
    vtab[i] = v;
    v *= 10.0;
  }
  uint32_t r = 0;
  size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(r += log2rough(vtab[i]) >> 3);
    i = (i >= 9) ? 0 : i+1;
  }
  dummydouble += r;
}
BENCHMARK(BM_Log2Rough);

void BM_log_random(benchmark::State& state) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dis(1, 10);
  auto s = dis(gen);
  double y;
  for (auto _ : state) {
    benchmark::DoNotOptimize(y = std::log(s));
  }
}
void BM_log2_random(benchmark::State& state) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dis(1, 10);
  auto s = dis(gen);
  double y;
  for (auto _ : state) {
    benchmark::DoNotOptimize(y = std::log2(s));
  }
  dummydouble += y;
}
void BM_log2r_random(benchmark::State& state) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dis(1, 10);
  auto s = dis(gen);
  uint32_t y = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(y *= log2rough(s) >> 3);
  }
  dummy64 += y;
}
BENCHMARK(BM_log_random);
BENCHMARK(BM_log2_random);
BENCHMARK(BM_log2r_random);

static double table[9] = {1.0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8 };

static inline size_t findBucket(double d) {
  for (size_t i = 0; i < 9; ++i) {
    if (d <= table[i]) {
      return i;
    }
  }
  return 9;
}

void BM_LinearSearch(benchmark::State& state) {
  double vtab[10];
  double v = 1.0;
  for (int i = 0; i < 10; ++i) {
    vtab[i] = v;
    v *= 10.0;
  }
  uint32_t r = 0;
  size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(r += findBucket(vtab[i]));
    i = (i >= 9) ? 0 : i+1;
  }
  dummydouble += r;
}
BENCHMARK(BM_LinearSearch);

BENCHMARK_MAIN();
