#ifndef TABLE_HPP
#define TABLE_HPP

// Include Table id, tuple index, tuple data type use EncrytedOneDataBlock
#include <random>
#include <cstring>
#include "share.hpp"

#define EMPTY_ENTRY 0
#define REG_ENTRY 1
#define PAYLOAD 4 // NOTE: Change for payload size
#define MEM_ADDRESS 5

/* In enclave, use TableEntry & tableId to process*/
struct TableEntry {
  // flag whether Entry valid
  int entry_type = EMPTY_ENTRY;
  // input data value
  int table_id;
  int join_attr;
  int data_attr;  // used for set dummy
  int payload[PAYLOAD];
  int random_key; // tie_breaker
  // aucillary data
  int64_t block_height;
  int64_t block_width;
  int64_t index;
  int64_t t1index;
  
  // constructor
  TableEntry(int _entry_type = EMPTY_ENTRY) : entry_type(_entry_type) {}

  friend TableEntry operator*(const bool &flag, const TableEntry &y) {
    TableEntry res;
    res.entry_type = flag * y.entry_type;
    res.table_id = flag * y.table_id;
    res.join_attr = flag * y.join_attr;
    res.data_attr = flag * y.data_attr;
    res.random_key = flag * y.random_key;
    res.block_height = flag * y.block_height;
    res.block_width = flag * y.block_width;
    res.index = flag * y.index;
    res.t1index = flag * y.t1index;
    for (int i = 0; i < PAYLOAD; ++i) {
      res.payload[i] = flag * y.payload[i];
    }
    return res;
  }

  friend TableEntry operator^(const TableEntry &x, const TableEntry &y) {
    TableEntry res;
    res.entry_type = x.entry_type ^ y.entry_type;
    res.table_id = x.table_id ^ y.table_id;
    res.join_attr = x.join_attr ^ y.join_attr;
    res.data_attr = x.data_attr ^ y.data_attr;
    res.random_key = x.random_key ^ y.random_key;
    res.block_height = x.block_height ^ y.block_height;
    res.block_width = x.block_width ^ y.block_width;
    res.index = x.index ^ y.index;
    res.t1index = x.t1index ^ y.t1index;
    for (int i = 0; i < PAYLOAD; ++i) {
      res.payload[i] = x.payload[i] ^ y.payload[i];
    }
    return res;
  }

  friend TableEntry operator&(const TableEntry &x, const TableEntry &y) {
    TableEntry res;
    res.entry_type = x.entry_type & y.entry_type;
    res.table_id = x.table_id & y.table_id;
    res.join_attr = x.join_attr & y.join_attr;
    res.data_attr = x.data_attr & y.data_attr;
    res.random_key = x.random_key & y.random_key;
    res.block_height = x.block_height & y.block_height;
    res.block_width = x.block_width & y.block_width;
    res.index = x.index & y.index;
    res.t1index = x.t1index & y.t1index;
    for (int i = 0; i < PAYLOAD; ++i) {
      res.payload[i] = x.payload[i] & y.payload[i];
    }
    return res;
  }

  friend TableEntry operator+(const TableEntry &x, const TableEntry &y) {
    TableEntry res;
    res.entry_type = x.entry_type + y.entry_type;
    res.table_id = x.table_id + y.table_id;
    res.join_attr = x.join_attr + y.join_attr;
    res.data_attr = x.data_attr + y.data_attr;
    res.random_key = x.random_key + y.random_key;
    res.block_height = x.block_height + y.block_height;
    res.block_width = x.block_width + y.block_width;
    res.index = x.index + y.index;
    res.t1index = x.t1index + y.t1index;
    for (int i = 0; i < PAYLOAD; ++i) {
      res.payload[i] = x.payload[i] + y.payload[i];
    }
    return res;
  }

  bool operator=(const TableEntry &a) {
    memcpy(this, &a, sizeof(TableEntry));
    return true;
  }

  // Index function
  static int entry_index(TableEntry e) {
    if (e.entry_type == EMPTY_ENTRY) return -1;
    return e.index;
  }

  // width/height function
  static int64_t entry_width(TableEntry e) {
    if (e.entry_type == EMPTY_ENTRY) return 0;
    return e.block_width;
  }

  static int64_t entry_height(TableEntry e) {
    if (e.entry_type == EMPTY_ENTRY) return 0;
    return e.block_height;
  }

  // comparison functions
  static bool attr_comp(TableEntry e1, TableEntry e2) {
    if (e1.join_attr == e2.join_attr)
      return e1.table_id < e2.table_id;
    return e1.join_attr < e2.join_attr;
  }
  
  static bool tid_comp(TableEntry e1, TableEntry e2) {
    if (e1.table_id == e2.table_id) {
      if (e1.join_attr == e2.join_attr)
        return e1.data_attr < e2.data_attr;
      return e1.join_attr < e2.join_attr;
    }
    return e1.table_id < e2.table_id;
  }

  static bool t1_comp(TableEntry e1, TableEntry e2) {
    if (e1.join_attr == e2.join_attr) {
      return e1.t1index < e2.t1index;
    }
    return e1.join_attr < e2.join_attr;
  }

  // basic compare function
  static bool data_comp(TableEntry e1, TableEntry e2) {
    if (e1.data_attr == e2.data_attr) {
      if (e1.join_attr == e2.join_attr) 
        return e1.random_key < e2.random_key;
      return e1.join_attr < e2.join_attr;
    }
    return e1.data_attr < e2.data_attr;
  }

  static void setDummy(TableEntry *e, int64_t size) {
    if (size <= 0) return ;
    std::random_device rd;
    std::mt19937 rng{rd()};
    std::uniform_int_distribution<int> dist{std::numeric_limits<int>::min(), std::numeric_limits<int>::max()};
    for (int64_t i = 0; i < size; ++i) {
      e[i].data_attr = DUMMY<int>();
      e[i].random_key = dist(rng);
    }
  }

  static void swapRow(TableEntry *e1, TableEntry *e2) {
    TableEntry tmp = *e1;
    *e1 = *e2;
    *e2 = tmp;
  }

  static void oswap(TableEntry *e1, TableEntry *e2, bool cond) {
    int mask = ~((int)cond - 1);
    *e1 = *e1 ^ *e2;
    *e2 = *e2 ^ (*e1 & mask);
    *e1 = *e1 ^ *e2;
  }

};

/* Manege the data stored in memory */
template <typename T>
struct Table {
  T *data[MEM_ADDRESS] = {nullptr};
  int64_t data_size[MEM_ADDRESS] = {0};

  // Initialization, 
  // TODO: Dummy here is 0
  Table(int64_t size, int addrId = 0) {
    data_size[addrId] = size;
    data[addrId] = (T *)calloc(size, sizeof(T));
  }

  void resize(int64_t new_size, int addrId = 0) {
    data[addrId] = (T *)realloc(data[addrId], new_size * sizeof(T));
    if (new_size > data_size[addrId]) {
      memset(data[addrId] + data_size[addrId], 0, (new_size - data_size[addrId]) * sizeof(T));
    }
    data_size[addrId] = new_size;
  }

  T read(int64_t idx, int addrId = 0) {
    assert(idx >= 0 && idx < data_size[addrId]);
    return data[addrId][idx];
  }

  void read(int64_t start, int64_t size, T *buffer, int addrId = 0) {
    assert(start >= 0 && start + size <= data_size[addrId]);
    memcpy(buffer, data[addrId] + start, size * sizeof(T));
  }

  void write(int64_t idx, T val, int addrId = 0) {
    assert(idx >= 0 && idx < data_size[addrId]);
    data[addrId][idx] = val;
  }

  void write(int64_t start, int64_t size, T *buffer, int addrId = 0) {
    assert(start >= 0 && start + size <= data_size[addrId]);
    memcpy(data[addrId] + start, buffer, size * sizeof(T));
  }
};

#endif // !TABLE_HPP

