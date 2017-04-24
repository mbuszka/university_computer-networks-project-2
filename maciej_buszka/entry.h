#ifndef ENTRY_H
#define ENTRY_H

#include <stdint.h>
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

void read_entry(entry_t *entry);
void show_entry(entry_t *entry);

#endif
