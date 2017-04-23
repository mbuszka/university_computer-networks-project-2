#ifndef ENTRY_H
#define ENTRY_H

#include <arpa/inet.h>

typedef struct in_addr addr_t;

#define CONNECTION_DIRECT 0
#define CONNECTION_VIA    1

typedef struct {
  int      connection_type;
  int      reachable; // yes if > 0
  addr_t   router_addr;
  addr_t   network_addr;
  uint8_t  prefix_len;
  uint32_t distance;
} entry_t;

#ifndef ENTRY_INTERNAL
extern entry_t*     table;
extern volatile int entry_count;
extern volatile int direct_count;
#endif

void init_table();
void free_table();
void add_entry(entry_t *e);
int  find_entry_network(addr_t network_ip);
void rem_entry(int idx);
void read_entry(entry_t *entry);
void show_entry(entry_t *entry);
void show_table();
void mark_unreachable(int idx);

#endif
