#ifndef _HELPER_H_
#define _HELPER_H_

struct Client {
  int fd;
  struct Client *next;
};

typedef struct Client Client;

Client *create_client(int fd);

#endif
