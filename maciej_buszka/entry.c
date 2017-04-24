#include <arpa/inet.h>

#include "util.h"
#include "entry.h"

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
