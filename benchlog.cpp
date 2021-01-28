#include <cstdint>
#include <cmath>
#include <iostream>
#include <vector>
#include "string.h"
#include <random>
#include <chrono>

#include <benchmark/benchmark.h>

static int count = 0;
static uint64_t dummy64 = 0;
static float dummyfloat = 0;
static double dummydouble = 0;

#if 0
static void BM_OurHistogram(benchmark::State& state) {
  constexpr float const base = 10.0;
  constexpr float const low = 0;
  constexpr float const high = 1000;
  constexpr size_t const n = 11;
  float nn = -1.0 * (n - 1);
  std::vector<float> delim;
  delim.resize(n-1);
  for (auto& i : delim) {
    i = (high - low) * std::pow(base, nn) + low;
    nn += 1.0;
  }
  float const div = (high - low) * std::pow(base, -10);
  float const lbase = std::log(base);

  uint64_t counts[n];
  for (size_t i = 0; i < n; ++i) {
    counts[i] = 0;
  }
  float v = 1.0;
  for (auto _ : state) {
    size_t x = static_cast<size_t>(1+std::floor(log((v - low)/div)/lbase));
    counts[x]++;
    v += 1.0;
    v = (v > 999.0) ? 1.0 : v;
  }
  dummyfloat += v;
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
  constexpr float const base = 10.0;
  constexpr float const low = 0;
  constexpr float const high = 1000;
  constexpr size_t const n = 11;
  float nn = -1.0 * (n - 1);
  std::vector<float> delim;
  delim.resize(n-1);
  for (auto& i : delim) {
    i = (high - low) * std::pow(base, nn) + low;
    nn += 1.0;
  }
  float const div = (high - low) * std::pow(base, -10);
  float const lbase = std::log(base);

  uint64_t counts[n];
  for (size_t i = 0; i < n; ++i) {
    counts[i] = 0;
  }
  float v = 1.0;
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
  dummyfloat += v;
  for (size_t i = 0; i < 10; ++i) {
    dummy64 += counts[i];
  }
}
BENCHMARK(BM_NewHistogram);
#endif

template<typename T>
void BM_Log2Empty(benchmark::State& state) {
  T vtab[10];
  T v = 1.0;
  for (int i = 0; i < 10; ++i) {
    vtab[i] = v;
    v *= 10.0;
  }
  T r = 0;
  size_t i = 0;
  while (state.KeepRunning()) {
    //r += vtab[i];
    benchmark::DoNotOptimize(i = (i >= 9) ? 0 : i+1);
  }
}
BENCHMARK_TEMPLATE(BM_Log2Empty, float);
BENCHMARK_TEMPLATE(BM_Log2Empty, double);

template<typename T>
void BM_Log2(benchmark::State& state) {
  T vtab[10];
  T v = 1.0;
  for (int i = 0; i < 10; ++i) {
    vtab[i] = v;
    v *= 10.0;
  }  T r = 0;
  size_t i = 0;
  while (state.KeepRunning()) {
    r += std::log2(vtab[i]);
    i = (i >= 9) ? 0 : i+1;
  }
  dummydouble += r;
}
BENCHMARK_TEMPLATE(BM_Log2, float);
BENCHMARK_TEMPLATE(BM_Log2, double);

template<typename T>
void BM_Log(benchmark::State& state) {
  T vtab[10];
  T v = 1.0;
  for (int i = 0; i < 10; ++i) {
    vtab[i] = v;
    v *= 10.0;
  }
  T r = 0;
  size_t i = 0;
  while (state.KeepRunning()) {
    r += std::log(vtab[i]);
    i = (i >= 9) ? 0 : i+1;
  }
  dummydouble += r;
}
BENCHMARK_TEMPLATE(BM_Log, float);
BENCHMARK_TEMPLATE(BM_Log, double);

static inline uint32_t log2rough(double x) {
  uint64_t y;
  memcpy(&y, &x, 8);
  return ((uint32_t)(y >> 52) & 0x7ff) - 1023;
}

static inline uint32_t log2rough(float x) {
  uint32_t y;
  memcpy(&y, &x, 4);
  return ((y >> 23) & 0xff) - 127;
}

const int tab64[64] = {
    63,  0, 58,  1, 59, 47, 53,  2,
    60, 39, 48, 27, 54, 33, 42,  3,
    61, 51, 37, 40, 49, 18, 28, 20,
    55, 30, 34, 11, 43, 14, 22,  4,
    62, 57, 46, 52, 38, 26, 32, 41,
    50, 36, 17, 19, 29, 10, 13, 21,
    56, 45, 25, 31, 35, 16,  9, 12,
    44, 24, 15,  8, 23,  7,  6,  5};
static inline int log2rough (uint64_t value) {
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return tab64[((uint64_t)((value - (value >> 1))*0x07EDD5E59A4E28C2)) >> 58];
}

const int tab32[32] = {
     0,  9,  1, 10, 13, 21,  2, 29,
    11, 14, 16, 18, 22, 25,  3, 30,
     8, 12, 20, 28, 15, 17, 24,  7,
    19, 27, 23,  6, 26,  5,  4, 31};
int log2rough (uint32_t value)
{
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return tab32[(uint32_t)(value*0x07C4ACDD) >> 27];
}

template<typename T>
static void BM_Log2Rough(benchmark::State& state) {
  T vtab[10];
  T v = 1.0;
  for (int i = 0; i < 10; ++i) {
    vtab[i] = v;
    v *= 10.0;
  }
  uint32_t r = 0;
  size_t i = 0;
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(r += log2rough(vtab[i]));
    i = (i >= 9) ? 0 : i+1;
  }
  dummydouble += r;
}
BENCHMARK_TEMPLATE(BM_Log2Rough, float);
BENCHMARK_TEMPLATE(BM_Log2Rough, double);
BENCHMARK_TEMPLATE(BM_Log2Rough, uint64_t);
BENCHMARK_TEMPLATE(BM_Log2Rough, uint32_t);

template<typename T>
void BM_log(benchmark::State& state) {
  std::random_device rd;
  std::mt19937 gen(rd());
  T s, y;
  if constexpr (std::is_integral<T>::value) {
    std::uniform_int_distribution<T> dis(1,100000);
    s = dis(gen);
  } else {
    std::uniform_real_distribution<T> dis(1,100000);
    s = dis(gen);
  }
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(y = std::log(s));
  }
  dummydouble += y;
}
template<typename T>
void BM_log2(benchmark::State& state) {
  std::random_device rd;
  std::mt19937 gen(rd());
  T s, y;
  if constexpr (std::is_integral<T>::value) {
    std::uniform_int_distribution<T> dis(1,100000);
    s = dis(gen);
  } else {
    std::uniform_real_distribution<T> dis(1,100000);
    s = dis(gen);
  }
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(y = std::log2(s));
  }
  dummydouble += y;
}
template<typename T>
void BM_log2r(benchmark::State& state) {
  std::random_device rd;
  std::mt19937 gen(rd());
  T s;
  if constexpr (std::is_integral<T>::value) {
    std::uniform_int_distribution<T> dis(1,100000);
    s = dis(gen);
  } else {
    std::uniform_real_distribution<T> dis(1,100000);
    s = dis(gen);
  }
  uint32_t y;
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(y = log2rough(s));
  }
  dummydouble += y;
}
BENCHMARK_TEMPLATE(BM_log, float);
BENCHMARK_TEMPLATE(BM_log, double);
BENCHMARK_TEMPLATE(BM_log2, float);
BENCHMARK_TEMPLATE(BM_log2, double);
BENCHMARK_TEMPLATE(BM_log2r, float);
BENCHMARK_TEMPLATE(BM_log2r, double);
BENCHMARK_TEMPLATE(BM_log2r, uint64_t);
BENCHMARK_TEMPLATE(BM_log2r, uint32_t);

static double table[9] = {1.0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8 };

static inline size_t findBucket(double d) {
  for (size_t i = 0; i < 9; ++i) {
    if (d <= table[i]) {
      return i;
    }
  }
  return 9;
}

static inline size_t findBucket2(double d) {
  size_t r = 0;
  r = (d <= table[0]) ? 0 : r;
  r = (d <= table[1]) ? 1 : r;
  r = (d <= table[2]) ? 2 : r;
  r = (d <= table[3]) ? 3 : r;
  r = (d <= table[4]) ? 4 : r;
  r = (d <= table[5]) ? 5 : r;
  r = (d <= table[6]) ? 6 : r;
  r = (d <= table[7]) ? 7 : r;
  r = (d <= table[8]) ? 8 : r;
  return r;
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
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(r += findBucket(vtab[i]));
    i = (i >= 9) ? 0 : i+1;
  }
  dummydouble += r;
}
BENCHMARK(BM_LinearSearch);

void BM_LinearSearch2(benchmark::State& state) {
  double vtab[10];
  double v = 1.0;
  for (int i = 0; i < 10; ++i) {
    vtab[i] = v;
    v *= 10.0;
  }
  uint32_t r = 0;
  size_t i = 0;
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(r += findBucket2(vtab[i]));
    i = (i >= 9) ? 0 : i+1;
  }
  dummydouble += r;
}
BENCHMARK(BM_LinearSearch2);

void BM_LogCast(benchmark::State& state) {
  uint64_t vtab[10];
  uint64_t v = 1;
  for (int i = 0; i < 10; ++i) {
    vtab[i] = v;
    v *= 10;
  }
  uint32_t r = 0;
  size_t i = 0;
  while (state.KeepRunning()) {
    r += log2rough((double) vtab[i]);
    i = (i >= 9) ? 0 : i+1;
  }
  dummydouble += r;
}
BENCHMARK(BM_LogCast);

void BM_steadyclock(benchmark::State& state) {
  auto start = std::chrono::steady_clock::now();
  auto end = std::chrono::steady_clock::now();
  while (state.KeepRunning()) {
    //benchmark::DoNotOptimize(x += std::chrono::duration(std::chrono::steady_clock::now() - start).count());
    std::cout << "Start";
    benchmark::DoNotOptimize(end = std::chrono::steady_clock::now());
    std::cout << "End";
  }
  dummy64 += std::chrono::duration(end - start).count();
}
BENCHMARK(BM_steadyclock);
BENCHMARK_MAIN();
