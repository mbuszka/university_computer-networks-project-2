#include "util.h"
#include "table.h"


void init_table(conn_table_t *table) {
  table->c_tab = malloc(8 * sizeof(entry_t));
  table->max_size = 8;
  table->entry_count = 0;
}

void free_table(conn_table_t *table) {
  free(table->c_tab);
}

void add_entry(conn_table_t *table, entry_t *e) {
  if (table->entry_count >= table->max_size) {
    table->max_size *= 2;
    table->c_tab = realloc(table->c_tab, table->max_size * sizeof(entry_t));
    if (table->c_tab == NULL) fail(errno);
  }
  table->c_tab[table->entry_count++] = *e;
}

int is_in_network(conn_table_t *table, addr_t ip, int idx) {
  int32_t mask = 0x80000000;
  mask >>= table->c_tab[idx].prefix_len - 1;
  mask = htonl(mask);
  return (table->c_tab[idx].network_addr.s_addr == (ip.s_addr & mask));
}

int find_entry_network(conn_table_t *table, addr_t ip) {
  int idx = 0;
  while (idx < table->entry_count) {
    if (is_in_network(table, ip, idx)) break;
    idx++;
  }

  if (idx == table->entry_count) return -1;
  return idx;
}

void rem_entry(conn_table_t *table, int idx) {
  if (idx == table->entry_count -1) {
    table->entry_count--;
  } else {
    table->c_tab[idx] = table->c_tab[--table->entry_count];
  }
}

void mark_unreachable(conn_table_t *table, int idx) {
  if (table->c_tab[idx].reachable > 0) {
    table->c_tab[idx].reachable = 0;
  }
  for (int i=0; i<table->entry_count; i++) {
    if (  is_in_network(table, table->c_tab[i].router_addr, idx)
       && table->c_tab[i].reachable > 0) {
      table->c_tab[i].reachable = 0;
    }
  }
}

void show_table(conn_table_t *table) {
  for (int i=0; i<table->entry_count; i++) {
    show_entry(&table->c_tab[i]);
  }
}
