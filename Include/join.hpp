#ifndef JOIN_HPP
#define JOIN_HPP

#include "common.hpp"
#include "table.hpp"
#include "bitonic.hpp"

namespace Algorithm {
  using namespace eServer;

  int64_t write_block_sizes(int64_t n, int tableId) {
    int64_t output_size = 0;
    int64_t height = 0, width = 0, last_join_attr = INT64_MIN, secSize;
    int64_t boundary = ceil(1.0 * n / params.M);
    TableEntry *data = new TableEntry[params.M];
    // scan in forward direction to fill in height fields for table 1 entries
    for (int64_t i = 0; i < boundary; ++i) {
      secSize = (i == boundary - 1) ? n - i * params.M : params.M;
      ScanBlock<TableEntry>(i * params.M, data, secSize, tableId, 0);
      for (int64_t j = 0; j < secSize; ++j) {
        bool same_attr = data[j].join_attr == last_join_attr;
        if (data[j].table_id == 0 && !same_attr) {
          height = 1;
        } else if (data[j].table_id == 0 && same_attr) {
          height++;
        } else if (data[j].table_id == 1 && !same_attr) {
          height = 0;
          data[j].block_height = 0;
        } else if (data[j].table_id == 1 && same_attr) {
          data[j].block_height = height;
        }
        last_join_attr = data[j].join_attr;
      }
      ScanBlock<TableEntry>(i * params.M, data, secSize, tableId, 1);
    }
    // scan in backward direction to fill in width + height fields for table 0 entries
    height = 0; width = 0, last_join_attr = INT64_MIN;
    for (int i = boundary - 1; i >= 0; --i) {
      secSize = (i == boundary - 1) ? n - i * params.M : params.M;
      ScanBlock<TableEntry>(i * params.M, data, secSize, tableId, 0);
      for (int j = 0; j < secSize; ++j) {
        bool same_attr = data[j].join_attr == last_join_attr;
        if (data[j].table_id == 0 && !same_attr) {
          width = 0;
          data[j].block_width = 0;
          height = 0;
          data[j].block_height = 0;
        } else if (data[j].table_id == 0 && same_attr) {
          data[j].block_width = width;
          data[j].block_height = height;
        } else if (data[j].table_id == 1 && !same_attr) {
          width = 1;
          height = data[j].block_height;
        } else if (data[j].table_id == 1 && same_attr) {
          width++;
          height = data[j].block_height;
        }
        last_join_attr = data[j].join_attr;
      }
      ScanBlock<TableEntry>(i * params.M, data, secSize, tableId, 1);
    }
    // scan in forward direction to fill in width fields for table 1 entries
    height = 0; width = 0, last_join_attr = INT64_MIN;
    for (int i = 0; i < boundary; ++i) {
      secSize = (i == boundary - 1) ? n - i * params.M : params.M;
      ScanBlock<TableEntry>(i * params.M, data, secSize, tableId, 0);
      for (int j = 0; j < secSize; ++j) {
        bool same_attr = data[j].join_attr == last_join_attr;
        if (data[j].table_id == 0 && !same_attr) {
          width = data[j].block_width;
          output_size += data[j].block_width * data[j].block_height;
        } else if (data[j].table_id == 0 && same_attr) {
          width = data[j].block_width;
        } else if (data[j].table_id == 1 && !same_attr) {
          width = 0;
          data[j].block_width = 0;
        } else if (data[j].table_id == 1 && same_attr) {
          data[j].block_width = width;
        }
        last_join_attr = data[j].join_attr;
      }
      ScanBlock<TableEntry>(i * params.M, data, secSize, tableId, 1);
    }
    delete[] data;
    return output_size;
  }

  template <typename T, int (*ind_func)(T e)>
  bool ind_func_comp(T e1, T e2) {
    if (ind_func(e1) == -1) return false;
    if (ind_func(e1) == -1) return true;
    else return ind_func(e1) < ind_func(e2);
  }

  template <typename T, int (*ind_func)(T e)>
  void obli_distribute(int64_t size, int64_t m, int tableId) {
    callBiSort<T, ind_func_comp<T, ind_func>>(tableId, size);
    OcallResize(tableId, m, 0);
    int64_t j = getPow2Lt(m);
    int64_t boundary = ceil(1.0 * (m - j - 1) / params.B);
    T *block1 = new T[params.B];
    T *block2 = new T[params.B];
    for (; j >= 1; j /= 2) {
      for (int i = boundary - 1; i >= 0; i--) {
        int secSize = (i == boundary - 1) ? (m - j - 1) - i * params.B : params.B;
        ScanBlock<T>(i * params.B, block1, secSize, tableId, 0);
        ScanBlock<T>(i * params.B + j, block2, secSize, tableId, 0);
        for (int m = secSize - 1; m >= 0; m--) {
          int dest_i = ind_func(block1[m]);
          assert(dest_i < m);
          if (dest_i >= j + i * params.B + m) {
            assert(ind_func(block2[m]) == -1);
            TableEntry e = block1[m];
            block1[m] = block2[m];
            block2[m] = e;
          }
        }
        ScanBlock<T>(i * params.B, block1, secSize, tableId, 1);
        ScanBlock<T>(i * params.B + j, block2, secSize, tableId, 1);
      }
    }
  }

  template <int64_t (*weight_func)(TableEntry e)>
  static void obli_expand(int64_t size, int tableId) {
    int csum = 0;
    TableEntry *data = new TableEntry[params.M];
    if (size < params.M) {
      ScanBlock<TableEntry>(0, data, size, tableId, 0);
      for (int64_t i = 0; i < size; ++i) {
        int64_t weight = weight_func(data[i]);
        if (weight == 0) data[i].entry_type = EMPTY_ENTRY;
        else data[i].index = csum;
        csum += weight;
      }
      ScanBlock<TableEntry>(0, data, size, tableId, 1);
    } else {
      int64_t boundary = ceil(1.0 * size / params.M);
      int64_t secSize;
      for (int64_t i = 0; i < boundary; ++i) {
        secSize = (i == boundary - 1) ? size - i * params.M : params.M;
        ScanBlock<TableEntry>(i * params.M, data, secSize, tableId, 0);
        for (int64_t j = 0; j < secSize; ++j) {
          int64_t weight = weight_func(data[j]);
          if (weight == 0) data[j].entry_type = EMPTY_ENTRY;
          else data[j].index = csum;
          csum += weight;
        }
        ScanBlock<TableEntry>(i * params.M, data, secSize, tableId, 1);
      }
    }
    
    obli_distribute<TableEntry, TableEntry::entry_index>(size, csum, tableId);

    TableEntry last;
    int64_t dupl_off = 0, block_off = 0, secSize;
    int64_t boundary = ceil(1.0 * csum / params.M);
    for (int64_t i = 0; i < boundary; ++i) {
      secSize = (i == boundary - 1) ? csum - i * params.M : params.M;
      ScanBlock<TableEntry>(i * params.M, data, secSize, tableId, 0);
      for (int64_t j = 0; j < secSize; ++j) {
        if (data[j].entry_type != EMPTY_ENTRY) {
          if (i != 0 && data[j].join_attr != last.join_attr) block_off = 0;
          last = data[j];
          dupl_off = 0;
        } else {
          assert(i != 0);
          data[j] = last;
        }
        data[j].index += dupl_off;
        data[j].t1index = int64_t(block_off / data[j].block_height) + 
                          (block_off % data[j].block_height) * data[j].block_width;
        dupl_off ++;
        block_off ++;
      }
      ScanBlock<TableEntry>(i * params.M, data, secSize, tableId, 1);
    }
    delete[] data;
  }

  void join(int tableId, int tableId0, int tableId1, int64_t n, int64_t n0, int64_t n1) {
    // sort lexicographically by (join_attr, table_id)
    callBiSort<TableEntry, TableEntry::attr_comp>(tableId, n);
    // fill in block heights and widths after initial sort & get output_size
    int64_t output_size = write_block_sizes(n, tableId);
    // resort lexicographically by (table_id, join_attr, data_attr)
    callBiSort<TableEntry, TableEntry::tid_comp>(tableId, n);
    TableEntry *data = new TableEntry[params.M];
    int64_t boundary = ceil(1.0 * n0 / params.M);
    int64_t secSize;
    for (int64_t i = 0; i < boundary; i++) {
      secSize = (i == boundary - 1) ? n0 - i * params.M : params.M;
      ScanBlock<TableEntry>(i * params.M, data, secSize, tableId, 0);
      ScanBlock<TableEntry>(i * params.M, data, secSize, tableId0, 1);
    }
    boundary = ceil(1.0 * n1 / params.M);
    for (int64_t i = 0; i < boundary; i++) {
      secSize = (i == boundary - 1) ? n1 - i * params.M : params.M;
      ScanBlock<TableEntry>(n0 + i * params.M, data, secSize, tableId, 0);
      ScanBlock<TableEntry>(i * params.M, data, secSize, tableId1, 1);
    }
    obli_expand<TableEntry::entry_width>(n0, tableId0);
    obli_expand<TableEntry::entry_height>(n1, tableId1);
    // TODO: check tableId0 & tableId1 data size
    callBiSort<TableEntry, TableEntry::tid_comp>(tableId1, n1);
  }
}

#endif // !JOIN_HPP
