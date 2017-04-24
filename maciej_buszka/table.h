#include "entry.h"

typedef struct {
  entry_t *c_tab;
  int      max_size;
  int      entry_count;
} conn_table_t;

void init_table(conn_table_t *table);
void free_table(conn_table_t *table);
void add_entry(conn_table_t *table, entry_t *e);
int  find_entry_network(conn_table_t *table, addr_t network_ip);
void rem_entry(conn_table_t *table, int idx);
void show_table(conn_table_t *table);
void mark_unreachable(conn_table_t *table, int idx);
