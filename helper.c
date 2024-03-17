#include "helper.h"
#include <stdio.h>
#include <stdlib.h>

Client *create_client(int fd) {
  Client *newClient = (Client *)malloc(sizeof(Client));
  if (newClient == NULL) {
    printf("Failed to allocate new client\n");
  }

  newClient->fd = fd;
  newClient->next = NULL;

  return newClient;
}

void add_client(Client **head, Client *client) {
  if (*head == NULL) {
    *head = client;
    (*head)->next = client;
  }

  Client *temp = *head;
  while (temp->next != *head) {
    temp = temp->next;
  }

  temp->next = client;
  client->next = *head;
}
