#ifndef _HELPER_H_
#define _HELPER_H_

#include "common.h"

struct Client {
  int fd;
  struct Client *next;
};

typedef struct Client Client;

typedef struct Room {
  Client **head;
  char password[BUFFER_SIZE];
  int num_of_clients;
  int room_id;
  struct Room *next;
} Room;

Client *create_client(int fd);
void add_client(Client **head, Client *client);

Room *create_room();
void add_room(Room **head, Room *room);

#endif
