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

  new_room->head = malloc(sizeof(new_room->head));
  *(new_room->head) = NULL;
  new_room->next = NULL;
  new_room->num_of_clients = 1;

  return new_room;
}

void add_room(Room **head, Room *room) {
  if (*head == NULL) {
    *head = room;
    (*head)->next = NULL;
    (*head)->room_id = 1;
  } else {
    Room *temp = *head;

    int i = temp->room_id;
    while (temp->next != NULL) {
      temp = temp->next;
      i = temp->room_id;
    }

    temp->next = room;
    temp->next->room_id = i + 1;
  }
}
