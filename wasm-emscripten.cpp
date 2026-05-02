/**
 *  @file wasm-emscripten.cpp
 *  @author Jay
 *  @date 5 November 2024 && 2 May 2026
 *  @brief This program benchmarks various sorting
 *  and searching algorithms, Bubble Sort, Selection Sort,
 *  Insertion Sort, Sequential Search, Binary Search, and compares them
 *  and as of may 2026, i went ahead and transformed this simple cpp script into web assembly.
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

// ---------- Algorithms ----------
template <class RandomIt>
void bubble_sort(RandomIt first, RandomIt last) {
  bool swapped;
  for (auto i = first; i != last; ++i) {
    swapped = false;
    for (auto j = first; j < last - (i - first) - 1; ++j) {
      if (*(j + 1) < *j) {
        std::iter_swap(j, j + 1);
        swapped = true;
      }
    }
    if (!swapped) break;
  }
}

template <class RandomIt>
void selection_sort(RandomIt first, RandomIt last) {
  for (auto i = first; i != last; ++i) {
    auto min = i;
    for (auto j = i + 1; j != last; ++j) {
      if (*j < *min) min = j;
    }
    std::iter_swap(i, min);
  }
}

template <class RandomIt>
void insertion_sort(RandomIt first, RandomIt last) {
  for (auto i = first + 1; i != last; ++i) {
    auto key = *i;
    auto j = i;
    while (j != first && *(j - 1) > key) {
      *j = *(j - 1);
      --j;
    }
    *j = key;
  }
}

template <class ForwardIt, class T>
ForwardIt seq_search(ForwardIt first, ForwardIt last, const T& target) {
  for (auto it = first; it != last; ++it) {
    if (*it == target) return it;
  }
  return last;
}

template <class RandomIt, class T>
RandomIt bin_search(RandomIt first, RandomIt last, const T& target) {
  auto left = first;
  auto right = last;
  while (left < right) {
    auto mid = left + (right - left) / 2;
    if (*mid == target) return mid;
    if (*mid < target)
      left = mid + 1;
    else
      right = mid;
  }
  return last;
}

static inline double now_ms() {
#ifdef __EMSCRIPTEN__
  return emscripten_get_now();
#else
  using namespace std::chrono;
  return duration<double, std::milli>(
             high_resolution_clock::now().time_since_epoch())
      .count();
#endif
}

template <class Func>
double benchmark_ms(Func&& fn) {
  double start = now_ms();
  fn();
  return now_ms() - start;
}

// ---------- Data generation ----------
static std::vector<int> generate_random_data(size_t size) {
  std::mt19937 gen(12345);  // fixed seed -> reproducible benchmarks
  std::uniform_int_distribution<int> distrib(1, 1000000);
  std::vector<int> vec(size);
  for (auto& v : vec) v = distrib(gen);
  return vec;
}

extern "C" EMSCRIPTEN_KEEPALIVE
const char* run_benchmark(int size) {
  static std::string out; 

  auto base = generate_random_data(static_cast<size_t>(size));
  const int not_present = -1;

  auto time_sort = [&](auto sorter) {
    auto v = base;
    return benchmark_ms([&] { sorter(v.begin(), v.end()); });
  };

  double bubble = time_sort([](auto a, auto b) { bubble_sort(a, b); });
  double selection = time_sort([](auto a, auto b) { selection_sort(a, b); });
  double insertion = time_sort([](auto a, auto b) { insertion_sort(a, b); });
  double stdsort = time_sort([](auto a, auto b) { std::sort(a, b); });

  auto sorted = base;
  std::sort(sorted.begin(), sorted.end());

  double seq = benchmark_ms(
      [&] { seq_search(sorted.begin(), sorted.end(), not_present); });
  double bin = benchmark_ms(
      [&] { bin_search(sorted.begin(), sorted.end(), not_present); });

  std::ostringstream ss;
  ss << std::fixed << std::setprecision(4);
  ss << "{\"size\":" << size
     << ",\"bubble_ms\":" << bubble
     << ",\"selection_ms\":" << selection
     << ",\"insertion_ms\":" << insertion
     << ",\"std_sort_ms\":" << stdsort
     << ",\"seq_search_ms\":" << seq
     << ",\"bin_search_ms\":" << bin << "}";
  out = ss.str();
  return out.c_str();
}

// Native fallback
#ifndef __EMSCRIPTEN__
int main() {
  for (int s : {2000, 4000, 8000, 16000, 32000}) {
    std::cout << run_benchmark(s) << "\n";
  }
  return 0;
}
#endif