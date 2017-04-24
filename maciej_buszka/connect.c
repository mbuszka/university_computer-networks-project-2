/*
  Maciej Buszka
  279129
*/

#include "connect.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define PORT 54321

int subscribe() {
  int bcast_permission = 1;
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in server_address;
  bzero (&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);

  bind(fd, (struct sockaddr*) &server_address, sizeof(server_address));
  setsockopt( fd, SOL_SOCKET, SO_BROADCAST
            , &bcast_permission, sizeof(bcast_permission)
            );
  return fd;
}

int unsubscribe(int fd) {
  close(fd);
  return 0;
}

int send_entry(int fd, addr_t ip, entry_t *entry) {
  uint8_t  buffer[9];
  uint32_t distance = htonl(entry->distance);
  struct sockaddr_in recipient;
  bzero (&recipient, sizeof(recipient));

  recipient.sin_family = AF_INET;
  recipient.sin_port = htons(PORT);
  recipient.sin_addr.s_addr = ip.s_addr;

  memcpy(buffer, &entry->network_addr.s_addr, 4);
  memcpy(buffer + 4, &entry->prefix_len, 1);
  memcpy(buffer + 5, &distance, 4);

  return sendto( fd, buffer, sizeof(buffer), 0
        , (struct sockaddr*) &recipient, sizeof(recipient)
        );
}

int receive_entry(entry_t *entry, int fd) {
  struct sockaddr_in sender;
  socklen_t          sender_len = sizeof(sender);
  uint8_t            buffer[IP_MAXPACKET+1];

  ssize_t packet_len = recvfrom(
    fd,
    buffer,
    IP_MAXPACKET,
    MSG_DONTWAIT,
    (struct sockaddr*) &sender,
    &sender_len
  );

  if (packet_len < 0) return -errno;
  if (packet_len == 9) {
    uint32_t ndist;
    memcpy(&ndist, buffer + 5, 4);
    entry->connection_type = CONNECTION_VIA;
    entry->router_addr     = sender.sin_addr;
    memcpy(&entry->network_addr.s_addr, buffer, 4);
    entry->prefix_len      = buffer[4];
    entry->distance        = ntohl(ndist);
    return 0;
  }

  return 1;
}
