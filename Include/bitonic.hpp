#ifndef BITONIC_HPP
#define BITONIC_HPP

#include "common.hpp"
#include "table.hpp"
#include "share.hpp"

namespace Algorithm {
  using namespace eServer;

  template <typename T>
  struct Rows {
    T *row1;
    T *row2;

    Rows() {
      row1 = new T[params.B];
      row2 = new T[params.B];
    }

    ~Rows() {
      delete [] row1;
      delete [] row2;
    }
  };

  template <typename T, bool (*comp_func)(T e1, T e2)>
  void smallBitonicMerge(T *table, int64_t start, int64_t size, bool flipped) {
    if (size > 1) {
      int64_t mid = getPow2Lt((double)size);
      T num1, num2;
      int swap = 0, nswap;
      for (int64_t i = 0; i < size - mid; ++i) {
        num1 = table[start + i];
        num2 = table[start + i + mid];
        swap = comp_func(num1, num2);
        swap = swap ^ flipped;
        nswap = !swap;
        table[start + i] = swap * num2 + nswap * num1;
        table[start + i + mid] = swap * num1 + nswap * num2;
      }
      smallBitonicMerge<T, comp_func>(table, start, mid, flipped);
      smallBitonicMerge<T, comp_func>(table, start + mid, size - mid, flipped);
    }
  }

  template <typename T, bool (*comp_func)(T e1, T e2)>
  void smallBitonicSort(T *table, int64_t start, int64_t size, bool flipped) {
    if (size > 1) {
      int64_t mid = getPow2Lt(size);
      smallBitonicSort<T, comp_func>(table, start, mid, 1);
      smallBitonicSort<T, comp_func>(table, start + mid, size - mid, 0);
      smallBitonicMerge<T, comp_func>(table, start, size, flipped);
    }
  }

  template <typename T, bool (*comp_func)(T e1, T e2)>
  void bitonicMerge(int tableId, int64_t start, int64_t size, int flipped, Rows<T> *row) {
    if (size < 1) return;
    else if (size < params.M / params.B) {
      T *trsutedM = new T[size * params.B];
      for (int64_t i = 0; i < size; ++i) {
        ScanBlock<T>((start + i) * params.B, trsutedM + i * params.B, params.B, tableId, 0);
      }
      smallBitonicMerge<T, comp_func>(trsutedM, 0, size * params.B, flipped);
      for (int64_t i = 0; i < size; ++i) {
        ScanBlock<T>((start + i) * params.B, trsutedM + i * params.B, params.B, tableId, 1);
      }
      delete [] trsutedM;
    } else {
      int swap = 0, nswap;
      int64_t mid = getPow2Lt((double)size);
      for (int64_t i = 0; i < size - mid; ++i) {
        ScanBlock<T>((start + i) * params.B, row->row1, params.B, tableId, 0);
        ScanBlock<T>((start + i + mid) * params.B, row->row2, params.B, tableId, 0);
        T num1 = row->row1[0], num2 = row->row2[0];
        swap = comp_func(num1, num2);
        swap = swap ^ flipped;
        nswap = !swap;
        for (int j = 0; j < params.B; ++j) {
          num1 = row->row1[j];
          num2 = row->row2[j];
          row->row1[j] = swap * num2 + nswap * num1;
          row->row2[j] = swap * num1 + nswap * num2;
        }
        ScanBlock<T>((start + i) * params.B, row->row1, params.B, tableId, 1);
        ScanBlock<T>((start + i + mid) * params.B, row->row2, params.B, tableId, 1);
      }
      bitonicMerge<T, comp_func>(tableId, start, mid, flipped, row);
      bitonicMerge<T, comp_func>(tableId, start + mid, size - mid, flipped, row);
    }
  }

  template <typename T, bool (*comp_func)(T e1, T e2)>
  void bitonicSort(int tableId, int64_t start, int64_t size, int flipped, Rows<T> *row) {
    if (size < 1) {
      return;
    } else if (size < params.M / params.B) {
      T *trsutedM = new T[size * params.B];
      for (int64_t i = 0; i < size; ++i) {
        ScanBlock<T>((start + i) * params.B, trsutedM + i * params.B, params.B, tableId, 0);
      }
      smallBitonicSort<T, comp_func>(trsutedM, 0, size * params.B, flipped);
      for (int64_t i = 0; i < size; ++i) {
        ScanBlock<T>((start + i) * params.B, trsutedM + i * params.B, params.B, tableId, 1);
      }
      delete [] trsutedM;
    } else {
      int64_t mid = getPow2Lt((double)size);
      bitonicSort<T, comp_func>(tableId, start, mid, 1, row);
      bitonicSort<T, comp_func>(tableId, start + mid, size - mid, 0, row);
      bitonicMerge<T, comp_func>(tableId, start, size, flipped, row);
    }
  }

  template <typename T, bool (*comp_func)(T e1, T e2)>
  void callBiSort(int tableId, int64_t size) {
    assert(size % params.B == 0 && "size must be a multiple of B");
    Rows<T> row;
    bitonicSort<T, comp_func>(tableId, 0, size / params.B, 0, &row);
  }
}

#endif // !BITONIC_HPP


