#ifndef SHARE_HPP
#define SHARE_HPP

#include <cstdint>
#include <stdio.h>
#include <random>

using namespace std;

std::random_device rd;
std::mt19937 rng{rd()};

int64_t randRange(int64_t start, int64_t end) {
  uniform_int_distribution<int64_t> distr(start, end - 1);
  return distr(rng);
}

template<typename T>
constexpr T DUMMY() {
  return std::numeric_limits<T>::max();
}

int64_t getPow2Lt(double n) {
  int64_t k = 1;
  int64_t N = (int64_t)n;
  while (k > 0 && k < N) k <<= 1;
  return k >> 1;
}

int64_t smallPowKLt(int64_t n, int k) {
  int64_t num = 1;
  while (num > 0 && num < n) num *= k;
  return num;
}

#endif // !SHARE_HPP

