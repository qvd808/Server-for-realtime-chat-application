#include "helper.h"
#include <stdio.h>
#include <stdlib.h>

Client* create_client(int fd) {
  Client* newClient = (Client *)malloc(sizeof(Client));
  if (newClient == NULL) {
    printf("Failed to allocate new client\n");
  }

  newClient->fd = fd;
  newClient->next = NULL;

  return newClient;
}
