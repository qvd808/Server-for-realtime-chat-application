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
  int current_client_fd;
  pthread_t *thread_id;
  Room *room_head;

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
  pb_ostream_t output = pb_ostream_from_socket(fd);

  while (1) {
    RequestHeader request_type = RequestHeader_init_zero;

    if (!pb_decode_delimited(&input, RequestHeader_fields, &request_type)) {
      perror("Decoding the request type failed!\n");
      break;
    }

    if (request_type.type == RequestType_CREATE_ROOM) {
      printf("Client request to create a room\n");

      RequestCreateRoom request_room = RequestHeader_init_zero;
      bzero(request_room.password, BUFFER_SIZE);

      if (!pb_decode_delimited(&input, RequestCreateRoom_fields,
                               &request_room)) {
        perror("Decoding room password failed\n");
        break;
      }

      Room *room = create_room();

      Client *new_client = create_client(fd);
      add_client(&room->head, new_client);
      strncpy(room->password, request_room.password, BUFFER_SIZE);
      room->num_of_clients += 1;
      room->room_id = 1;

      add_room(&function_arg.room_head, room);

      printf("Create a room sucessfully\n");

    } else if (request_type.type == RequestType_JOIN_ROOM) {

      RequestJoinRoom request_room = RequestJoinRoom_init_zero;
      bzero(request_room.password, BUFFER_SIZE);

      if (!pb_decode_delimited(&input, RequestJoinRoom_fields, &request_room)) {
        perror("Decoding join room request failed\n");
        break;
      }
      printf("Receive request to join a room with id %d\n",
             request_room.room_id);

      Room *temp = function_arg.room_head;

      // while (temp != NULL) {
      //   if (temp->room_id == request_room.room_id) {
      //     if (strcmp(temp->password, request_room.password) == 0) {
      //       printf("Provided the right password\n");
      //       break;
      //     } else {
      //       printf("Provided the wrong password\n");
      //       break;
      //     }
      //   }
      //   temp = temp->next;
      // }

      {
        ResponseJoinRoom response_room = ResponseJoinRoom_init_zero;
        response_room.status = ResponseStatus_FAILED;
        response_room.room_id = 1;
        printf("assigned status\n");

        if (!pb_encode_delimited(&output, ResponseJoinRoom_fields,
                                 &response_room)) {
          perror("Failed to send response join room to client\n");
          break;
        }
        printf("Finished encode\n");
      }
    }
  }

  // while (1) {
  //
  //   ChatMessage chat = ChatMessage_init_zero;
  //   bzero(chat.chat, BUFFER_SIZE);
  //   if (!pb_decode_delimited(&input, ChatMessage_fields, &chat)) {
  //     perror("Decode failed\n");
  //     break;
  //   }
  //
  //   send_msg_to_other_client(function_arg.head,
  //   function_arg.current_client_fd,
  //                            (const char *)chat.chat);
  // }
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

  while (1) {
    connfd = accept(listenfd, NULL, NULL);

    if (connfd == -1) {
      printf("Have trouble accept connection\n");
    }

    pthread_t thread_id;
    HandleConnectionArg arg = {
        .current_client_fd = connfd,
        .thread_id = &thread_id,
        .room_head = NULL,
    };

    printf("A new client has connected with id %d\n", connfd);

    int check = pthread_create(&thread_id, NULL, handle_connection, &arg);

    if (check != 0) {
      printf("Failed to handle connection\n");
    }
  }
  return 0;
}
