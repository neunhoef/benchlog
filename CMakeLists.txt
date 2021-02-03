cmake_minimum_required(VERSION 3.15)
project(benchlog)

set(CMAKE_CXX_STANDARD 17)

# required for clang completion in editors - must be set after creating project
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_package(benchmark REQUIRED)

add_executable(benchlog
  benchlog.cpp
)
add_executable(benchmetrics
  benchmetrics.cpp Metrics.cpp
)

target_link_libraries(benchlog
  benchmark::benchmark
  ${CMAKE_THREAD_LIBS_INIT}
)
target_link_libraries(benchmetrics
  benchmark::benchmark
  ${CMAKE_THREAD_LIBS_INIT}
)


add_executable(correct
  correct.cpp
)
