#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdint>
#include <cstring>
#include <math.h>
#include <assert.h>
#include <algorithm>

#include "share.hpp"
#include "encDecUtils.hpp"

namespace eServer {
  struct Params {
    int64_t N, M;
    int B, sigma, nonEnc;
    double IOcost, IOtime;
    uint64_t start, end;

    Params() {}

    Params(int64_t N, int64_t M, int B, int sigma, int nonEnc = 0) : N(N), M(M), B(B), sigma(sigma), nonEnc(nonEnc), IOcost(0), IOtime(0) {}
  };

  Params params;

  // Settings for the common algorithm, call at the beginning of the program
  void setParams(int64_t N, int64_t M, int B, int sigma, int nonEnc = 0) {
    params = Params(N, M, B, sigma, nonEnc);
  }

  // Main algorithms
  double getIOcost() {
    return params.IOcost;
  }

  double getIOtime() {
    return params.IOtime;
  }

  // R&W data
  template <typename T>
  void ReadPage(int64_t startIdx, T* buffer, int pageSize, int tableId, int addrId = 0) {
    if (pageSize == 0) return ;
    OcallRead(startIdx, (int*)buffer, pageSize, tableId, addrId);
    if (!params.nonEnc) {
      for (int i = 0; i < pageSize; ++i) {
        gcm_decrypt((uint8_t*)(buffer + i), (uint8_t*)(buffer + i), sizeof(T));
      }
    }
  }

  template <typename T>
  void WritePage(int64_t startIdx, T* buffer, int pageSize, int tableId, int addrId = 0) {
    if (pageSize == 0) return ;
    if (!params.nonEnc) {
      for (int i = 0; i < pageSize; ++i) {
        gcm_encrypt((uint8_t*)(buffer + i), (uint8_t*)(buffer + i), sizeof(T));
      }
    }
    OcallWrite(startIdx, (int*)buffer, pageSize, tableId, addrId);
  }

  template <typename T>
  void ScanBlock(int64_t index, T* block, int64_t eleNum, int tableId, int write, int64_t dummyNum = 0, int addrId = 0) {
    assert(dummyNum >= 0 && "Dummy number should be non-negative");
    ocall_measure_time(&(params.start));
    if (eleNum + dummyNum == 0) return ;
    int64_t boundary = ceil(1.0 * eleNum / params.B);
    params.IOcost += boundary;
    int size;
    if (!write) {
      for (int64_t i = 0; i < boundary; ++i) {
        size = min((int64_t)params.B, eleNum - i * params.B);
        ReadPage(index + i * params.B, block + i * params.B, size, tableId, addrId);
      }
    } else {
      for (int64_t i = 0; i < boundary; ++i) {
        size = min((int64_t)params.B, eleNum - i * params.B);
        WritePage(index + i * params.B, block + i * params.B, size, tableId, addrId);
      }
      if (dummyNum > 0) {
        T *junk = new T[dummyNum];
        memset(junk, DUMMY<int>(), sizeof(T) * dummyNum);
        int64_t dumBoundary = ceil(1.0 * dummyNum / params.B);
        params.IOcost += dumBoundary;
        for (int64_t j = 0; j < dumBoundary; ++j) {
          size = min((int64_t)params.B, dummyNum - j * params.B);
          WritePage(index + eleNum + j * params.B, junk + j * params.B, size, tableId, addrId);
        }
        delete[] junk;
      }
    }
    ocall_measure_time(&(params.end));
    params.IOtime += params.end - params.start;
  }
}

#endif // !COMMON_HPP


