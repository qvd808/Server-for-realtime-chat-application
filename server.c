#include <asm-generic/socket.h>
#include <dirent.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <pb_decode.h>
#include <pb_encode.h>

#include "common.h"
#include "message.pb.h"

void handle_connection(int fd) {

  printf("Starting handle socket connection\n");

  SimpleMessage message = {};
  pb_istream_t input = pb_istream_from_socket(fd);
  if (!pb_decode_delimited(&input, SimpleMessage_fields, &message)) {
    perror("Decode failed\n");
  } else {
    printf("The luckey number is %d\n", (int)message.lucky_number);
  }

  printf("Finished handle socket connection\n");
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
    printf("Got connection\n");

    handle_connection(connfd);

    printf("Close connection\n");

    close(connfd);
  }
  return 0;
}
