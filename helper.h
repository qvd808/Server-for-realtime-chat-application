#ifndef _HELPER_H_
#define _HELPER_H_

struct Client {
  int fd;
  struct Client *next;
};

typedef struct Client Client;

Client *create_client(int fd);
void add_client(Client **head, Client *client);

#endif
