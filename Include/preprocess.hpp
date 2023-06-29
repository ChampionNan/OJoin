#ifndef PREPROCESS_HPP
#define PREPROCESS_HPP

#include <cstdint>

#include "table.hpp"
#include "share.hpp"

using namespace std;

// TODO: rewrite after change DS
void parseTables(Table<TableEntry> **tables, int64_t n1, int64_t n2) {
  Table<TableEntry> *t = tables[0], *t0 = tables[1], *t1 = tables[2];
  t = new Table<TableEntry>(n1 + n2);
  t0 = new Table<TableEntry>(n1);
  t1 = new Table<TableEntry>(n2);
  for (int64_t i = 0; i < n1 + n2; ++i) {
    TableEntry e;
    e.entry_type = REG_ENTRY;
    e.table_id = i < n1 ? 0 : 1;
    e.join_attr = randRange(0, 10); // TODO: Adjust this for later join
    e.data_attr = randRange(numeric_limits<int>::min(), numeric_limits<int>::max()); // max for dummy
    e.random_key = randRange(numeric_limits<int>::min(), numeric_limits<int>::max()); // [min, max-1]
    t->write(i, e);
  }
}

#endif // !PREPROCESS_HPP

