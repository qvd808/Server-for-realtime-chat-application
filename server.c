#include <asm-generic/socket.h>
#include <dirent.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <pb_decode.h>
#include <pb_encode.h>

#include "common.h"
#include "message.pb.h"

void *handle_connection(void *arg) {

  // Reader the Header to determine the size of message to read

  int fd = *(int *)(arg);
  ChatMessage chat = {};

  pb_istream_t input = pb_istream_from_socket(fd);
  if (!pb_decode_delimited(&input, ChatMessage_fields, &chat)) {
    perror("Decode failed\n");
  }

  printf("%s", chat.chat);

  // Close the connection once done
  close(fd);

  return NULL;
}

int main() {
  int listenfd, connfd;
  struct sockaddr_in server_addr;
  int reuse = 1;

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  server_addr.sin_port = htons(1234);

  if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {
    perror("Bind failed!\n");
    return 1;
  }

  if (listen(listenfd, 5) != 0) {
    perror("Listen failed!\n");
    return 1;
  }

  while (1) {
    connfd = accept(listenfd, NULL, NULL);

    if (connfd == -1) {
      printf("Have trouble accept connection\n");
    }

    printf("A new client has connected\n");
    pthread_t thread_id;

    int check = pthread_create(&thread_id, NULL, handle_connection, &connfd);

    if (check != 0) {
      printf("Failed to handle connection\n");
    }
  }
  return 0;
}
