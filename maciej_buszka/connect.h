/*
  Maciej Buszka
  279129
*/

#ifndef CONNECT_H
#define CONNECT_H

#include "entry.h"

int subscribe();
int unsubscribe(int fd);
int receive_entry(entry_t *entry, int fd);
int send_entry(int fd, addr_t ip, entry_t *entry);

#endif
