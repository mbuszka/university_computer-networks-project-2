/*
  Maciej Buszka
  279129
*/

#include <arpa/inet.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include "util.h"
#include "entry.h"
#include "table.h"
#include "connect.h"

#define TURN 5
#define UNREACHABLE UINT32_MAX
#define DOUBT_PERIOD 3
#define PREVENT_COUNTING_TO_INF 20

static conn_table_t connections;
static conn_table_t direct;

void send_table(int stream) {
  for (int i=0; i<direct.entry_count; i++) {
    addr_t bcast;
    uint32_t mask = (1 << (32 - direct.c_tab[i].prefix_len)) - 1;
    bcast.s_addr  = direct.c_tab[i].network_addr.s_addr | htonl(mask);
    for (int j=0; j<connections.entry_count; j++) {
      entry_t entry = connections.c_tab[j];
      if (entry.reachable <= 0) entry.distance = UNREACHABLE;

      // send if only recently lost connection, or this is the target network
      if (entry.reachable > -3 || i == j) {
        if (0 > send_entry(stream, bcast, &entry)) {
          if (errno == ENOTCONN || errno == ENETUNREACH) {
            mark_unreachable(&direct, i);
            mark_unreachable(&connections, i);
          } else {
            fail(errno);
          }
        }
      }
    }
  }
}

void update_connections() {
  for (int i=0; i<direct.entry_count; i++) {
    direct.c_tab[i].reachable--;
    if (direct.c_tab[i].reachable <= 0) {
      mark_unreachable(&direct, i);
      if (connections.c_tab[i].connection_type == CONNECTION_DIRECT) {
        mark_unreachable(&connections, i);
      }
    }
  }

  for (int i=0; i<direct.entry_count; i++) {
    if (connections.c_tab[i].reachable <= 0)
      connections.c_tab[i] = direct.c_tab[i];
  }

  for (int i=direct.entry_count; i<connections.entry_count; i++) {
    if (connections.c_tab[i].reachable <= 0) connections.c_tab[i].reachable--;
    if (connections.c_tab[i].reachable < -3) rem_entry(&connections, i);
  }
}

void handle_incoming(entry_t *entry) {
  int idx = find_entry_network(&connections, entry->network_addr);
  int via = find_entry_network(&direct, entry->router_addr);
  assert(via >= 0);

  connections.c_tab[via].reachable = DOUBT_PERIOD;
  direct.c_tab[via].reachable = DOUBT_PERIOD;

  if (entry->distance == UNREACHABLE) {
    if (connections.c_tab[idx].router_addr.s_addr == entry->router_addr.s_addr) {
      mark_unreachable(&connections, idx);
    }
    return;
  }

  entry->reachable = DOUBT_PERIOD;
  if (idx == via) {
    entry->connection_type = CONNECTION_DIRECT;
  } else {
    entry->distance += connections.c_tab[via].distance;
  }
  if (entry->distance > PREVENT_COUNTING_TO_INF) return;
  if (idx < 0) {
    add_entry(&connections, entry);
  } else if (  entry->distance <= connections.c_tab[idx].distance
            || connections.c_tab[idx].reachable <= 0) {
      connections.c_tab[idx] = *entry;
  }
}

int main() {
  int     initial_count;
  int     stream = subscribe();
  entry_t entry;

  init_table(&connections);
  init_table(&direct);

  scanf("%d", &initial_count);
  for (int i=0; i<initial_count; i++) {
    read_entry(&entry);
    add_entry(&connections, &entry);
    add_entry(&direct, &entry);
  }

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
    update_connections();
    show_table(&connections);
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
  free_table(&connections);
  free_table(&direct);
  return 0;
}
