#include "helper.h"
#include <stdio.h>
#include <stdlib.h>

Client *create_client(int fd) {
  Client *newClient = (Client *)malloc(sizeof(*newClient));
  if (newClient == NULL) {
    printf("Failed to allocate new client\n");
    return NULL;
  }

  newClient->fd = fd;
  newClient->next = NULL;

  return newClient;
}

void add_client(Client **head, Client *client) {
  if (*head == NULL) {
    *head = client;
    (*head)->next = NULL;

  } else {

    Client *temp = *head;

    while (temp->next != NULL) {
      temp = temp->next;
    }

    temp->next = client;
  }
}

Room *create_room() {
  Room *new_room = (Room *)malloc(sizeof(*new_room));

  if (new_room == NULL) {
    printf("Failed to allocate new room\n");
    return NULL;
  }

  return new_room;
}

void add_room(Room **head, Room *room) {
  if (*head == NULL) {
    *head = room;
    (*head)->next = NULL;
  } else {
    Room *temp = *head;

    while (temp->next != NULL) {
      temp = temp->next;
    }

    temp->next = room;
  }
}
