#include <asm-generic/socket.h>
#include <dirent.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <pb_decode.h>
#include <pb_encode.h>

#include "common.h"
#include "helper.h"
#include "message.pb.h"

typedef struct HandleConnectionArg {
  Client **head;
  int current_client_fd;
  pthread_t *thread_id;
} HandleConnectionArg;

void send_msg_to_other_client(Client **head, int fd, const char *buffer) {
  Client *temp = *head;

  // printf("The current client is %d, and there message is %s", fd, buffer);

  while (temp != NULL) {
    if (temp->fd != fd) {
      write(temp->fd, buffer, BUFFER_SIZE);
    }
    temp = temp->next;
  }
}

void *handle_connection(void *arg) {

  // Reader the Header to determine the size of message to read

  HandleConnectionArg function_arg = *(HandleConnectionArg *)(arg);

  pthread_detach(*function_arg.thread_id);
  int fd = function_arg.current_client_fd;
  pb_istream_t input = pb_istream_from_socket(fd);

  while (1) {

    ChatMessage chat = ChatMessage_init_zero;
    bzero(chat.chat, BUFFER_SIZE);
    if (!pb_decode_delimited(&input, ChatMessage_fields, &chat)) {
      perror("Decode failed\n");
      break;
    }

    send_msg_to_other_client(function_arg.head, function_arg.current_client_fd,
                             (const char *)chat.chat);
  }
  // Close the connection once done
  close(fd);

  pthread_exit(0);
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
  server_addr.sin_port = htons(PORT);

  if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {
    perror("Bind failed!\n");
    return 1;
  }

  if (listen(listenfd, 5) != 0) {
    perror("Listen failed!\n");
    return 1;
  }

  Client *head = NULL;

  while (1) {
    connfd = accept(listenfd, NULL, NULL);

    if (connfd == -1) {
      printf("Have trouble accept connection\n");
    }

    Client *new_client = create_client(connfd);
    pthread_t thread_id;
    add_client(&head, new_client);
    HandleConnectionArg arg = {
        .head = &head,
        .current_client_fd = connfd,
        .thread_id = &thread_id,
    };

    printf("A new client has connected with id %d\n", connfd);

    int check = pthread_create(&thread_id, NULL, handle_connection, &arg);

    if (check != 0) {
      printf("Failed to handle connection\n");
    }
  }
  return 0;
}
