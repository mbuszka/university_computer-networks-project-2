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
#define DOUBT_PERIOD 3
#define PREVENT_COUNTING_TO_INF 20

void send_table(int stream) {
  for (int i=0; i<direct_count; i++) {
    addr_t bcast;
    uint32_t mask = (1 << (32 - table[i].prefix_len)) - 1;
    bcast.s_addr  = table[i].network_addr.s_addr | htonl(mask);
    for (int j=0; j<entry_count; j++) {
      entry_t entry = table[j];
      if (entry.reachable <= 0) entry.distance = UNREACHABLE;

      // send if only recently lost connection, or this is the target network
      if (entry.reachable > -3 || i == j) {
        if (0 > send_entry(stream, bcast, &entry)) {
          if (errno == ENOTCONN || errno == ENETUNREACH) mark_unreachable(i);
          else { printf("%d\n", errno); fail(errno); }
        }
      }
    }
  }
}

void age_connections() {
  for (int i=0; i<direct_count; i++) {
    table[i].reachable--;
    if (table[i].reachable <= 0) mark_unreachable(i);
  }

  for (int i=direct_count; i<entry_count; i++) {
    if (table[i].reachable <= 0) table[i].reachable--;
    if (table[i].reachable < -3) rem_entry(i);
  }
}

void handle_incoming(entry_t *entry) {
  int idx = find_entry_network(entry->network_addr);
  int via = find_entry_network(entry->router_addr);
  assert(via >= 0);

  if (table[via].router_addr.s_addr == entry->router_addr.s_addr) return;

  table[via].reachable = DOUBT_PERIOD;

  if (entry->distance == UNREACHABLE) {
    if (table[idx].router_addr.s_addr == entry->router_addr.s_addr) {
      mark_unreachable(idx);
    }
    return;
  }

  entry->reachable = 1;
  entry->distance += table[via].distance;
  if (entry->distance > PREVENT_COUNTING_TO_INF) return;
  if (idx < 0) {
    add_entry(entry);
  } else {
    if (table[idx].connection_type == CONNECTION_DIRECT) {
      return;
    } else if (  entry->distance <= table[idx].distance
              || table[idx].reachable <= 0) {
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
