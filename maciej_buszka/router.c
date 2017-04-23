#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "entry.h"
#include "connect.h"

#define panic(msg) do { \
  fprintf(stderr, "%s : %s/n", __func__, msg); \
  exit(1); \
} while(0)

#define fail(err_number) do {\
  fprintf(stderr, "%s : %s\n", __func__, strerror(err_number));\
  exit(1);\
} while (0)

#define TURN 5
#define UNREACHABLE UINT32_MAX

void send_table(int stream) {
  for (int i=0; i<direct_count; i++) {
    addr_t bcast;
    uint32_t mask = (1 << (32 - table[i].prefix_len)) - 1;
    bcast.s_addr  = table[i].network_addr.s_addr | htonl(mask);
    for (int j=0; j<entry_count; j++) {
      entry_t entry = table[j];
      if (entry.age <= 0) entry.distance = UNREACHABLE;
      if (entry.age > -3 || i == j)
        send_entry(stream, bcast, &entry);
    }
  }
}

void age_connections() {
  for (int i=0; i<entry_count; i++) {
    table[i].age--;
    if (table[i].age <= 0 && i >= direct_count)
      table[i].distance = UNREACHABLE;
    if (table[i].age < -2 && i >= direct_count)
      rem_entry(i);
  }
}

void handle_incoming(entry_t *entry) {
  entry->age = INIT_AGE;
  int idx = find_entry_network(entry->network_addr);
  int via = find_entry_network(entry->router_addr);
  assert(via >= 0);

  if (table[via].router_addr.s_addr == entry->router_addr.s_addr) return;
  table[via].age = INIT_AGE;

  if (entry->distance == UNREACHABLE) {
    if (table[idx].router_addr.s_addr == entry->router_addr.s_addr) {
      table[idx].distance = UNREACHABLE;
    }
    return;
  }

  entry->distance += table[via].distance;
  if (idx < 0) {
    add_entry(entry);
  } else {
    if (table[idx].connection_type == CONNECTION_DIRECT) {
      return;
    } else if (entry->distance <= table[idx].distance) {
      table[idx] = *entry;
    }
  }
}

int main() {
  int     initial_count;
  int     stream = subscribe();
  entry_t entry;

  init_table();

  scanf("%d", &initial_count);
  for (int i=0; i<initial_count; i++) {
    read_entry(&entry);
    add_entry(&entry);
  }
  direct_count = initial_count;

  for (;;) {
    time_t start;
    time(&start);
    time_t end;
    fd_set descriptors;
    FD_ZERO(&descriptors);
    FD_SET(stream, &descriptors);
    struct timeval tv;
    tv.tv_sec = TURN;
    tv.tv_usec = 0;
    age_connections();
    show_table();
    send_table(stream);
    printf("\n\n");
    while(select(stream + 1, &descriptors, NULL, NULL, &tv) > 0) {
      time(&end);
      while (-EWOULDBLOCK != receive_entry(&entry, stream)) {
        handle_incoming(&entry);
      }
      if (start + TURN - end <= 0) break;
      tv.tv_sec = start + TURN - end;
      tv.tv_usec = 0;
    }
  }

  unsubscribe(stream);
  free_table();
  return 0;
}
