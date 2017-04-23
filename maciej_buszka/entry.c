#define ENTRY_INTERNAL
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "entry.h"

#define panic(msg) do { \
  fprintf(stderr, "%s : %s/n", __func__, msg); \
  exit(1); \
} while(0)

#define fail(err_number) do {\
  fprintf(stderr, "%s : %s\n", __func__, strerror(err_number));\
  exit(1);\
} while (0)

#define min(_a, _b) ((_a) < (_b) ? (_a) : (_b))


entry_t *table;
static int      table_size   = 0;
int      entry_count  = 0;
int      direct_count = 0;

void init_table() {
  table = malloc(8 * sizeof(entry_t));
  table_size = 8;
}

void free_table() {
  free(table);
}

void add_entry(entry_t *e) {
  if (entry_count >= table_size) {
    table_size *= 2;
    table = realloc(table, table_size * sizeof(entry_t));
    if (table == NULL) fail(errno);
  }
  table[entry_count++] = *e;
}

int is_in_network(addr_t ip, int idx) {
  int32_t mask = 0x80000000;
  mask >>= table[idx].prefix_len - 1;
  mask = htonl(mask);
  return (table[idx].network_addr.s_addr == (ip.s_addr & mask));
}

int find_entry_network(addr_t ip) {
  int idx = 0;
  while (idx < entry_count) {
    if (is_in_network(ip, idx)) break;
    idx++;
  }

  if (idx == entry_count) return -1;
  return idx;
}

void rem_entry(int idx) {
  if (idx == entry_count -1) {
    entry_count--;
  } else {
    table[idx] = table[--entry_count];
  }
}

void mark_unreachable(int idx) {
  if (table[idx].reachable > 0) {
    table[idx].reachable = 0;
  }
  if (idx < direct_count) {
    for (int i=direct_count; i<entry_count; i++)
      if (is_in_network(table[i].router_addr, idx))
        mark_unreachable(i);
  }
}

void read_entry(entry_t *entry) {
  char addr_str[30];
  uint32_t len;

  scanf("%s distance %u", addr_str, &entry->distance);

  char *prefix_len_str = strchr(addr_str, '/');
  *prefix_len_str = '\0';
  sscanf(prefix_len_str + 1, "%u", &len);
  entry->prefix_len = len;
  inet_pton(AF_INET, addr_str, &entry->router_addr);
  int32_t  mask = entry->prefix_len ? INT32_MIN >> (entry->prefix_len - 1) : 0;
  uint32_t addr = ntohl(entry->router_addr.s_addr);
  entry->network_addr.s_addr = htonl(addr & mask);
  entry->connection_type = CONNECTION_DIRECT;
  entry->reachable = 0;
}

void show_entry(entry_t *entry) {
  char addr[30];
  inet_ntop(AF_INET, &entry->network_addr, addr, 30);
  printf("%s/%d ", addr, entry->prefix_len);
  if (entry->reachable <= 0) {
    printf("unreachable ");
  } else {
    printf("distance %u ", entry->distance);
  }
  if (entry->connection_type == CONNECTION_DIRECT) {
    printf("connected directly\n");
  } else {
    inet_ntop(AF_INET, &entry->router_addr, addr, 30);
    printf("via %s\n", addr);
  }
}

void show_table() {
  for (int i=0; i<entry_count; i++) {
    show_entry(&table[i]);
  }
}
