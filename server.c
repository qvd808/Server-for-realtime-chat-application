#include <asm-generic/socket.h>
#include <dirent.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
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
  Room **room_head;

} HandleConnectionArg;

void send_msg_to_other_client(Client **head, int fd, const char *buffer) {
  Client *temp = *head;

  printf("The current client is %d, and there message is %s", fd, buffer);

  while (temp != NULL) {
    if (temp->fd != fd) {
      write(temp->fd, buffer, BUFFER_SIZE);
    }
    // printf("the current client is %d and address is %p\n", temp->fd, temp);
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

    // if (*function_arg.room_head != NULL) {
    //   Room *current_room = *(function_arg.room_head);
    //   if (current_room->head != NULL) {
    //     // printf("the point is %p\n", current_room->head);
    //     Client *head = *current_room->head;
    //
    //     while (head != NULL) {
    //       printf("Client id is %d\n", head->fd);
    //       head = head->next;
    //     }
    //   }
    // }

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

      strncpy(room->password, request_room.password, BUFFER_SIZE);
      printf("The created room password is %s", room->password);

      add_room(function_arg.room_head, room);

    } else if (request_type.type == RequestType_JOIN_ROOM) {

      RequestJoinRoom request_room = RequestJoinRoom_init_zero;
      bzero(request_room.password, BUFFER_SIZE);

      if (!pb_decode_delimited(&input, RequestJoinRoom_fields, &request_room)) {
        perror("Decoding join room request failed\n");
        break;
      }
      printf("Receive request to join a room with id %d\n",
             request_room.room_id);

      Room *temp = *function_arg.room_head;
      // printf("The address of temp is %p\n", temp);

      int send_response = 0;

      while (temp != NULL) {
        if (temp->room_id == request_room.room_id) {
          if (strncmp(request_room.password, temp->password,
                      strlen(request_room.password)) == 0) {
            {
              Client *new_client =
                  create_client(function_arg.current_client_fd);

              add_client((*function_arg.room_head)->head, new_client);

              ResponseJoinRoom response_room = ResponseJoinRoom_init_zero;
              response_room.status = ResponseStatus_SUCCESS;

              if (!pb_encode_delimited(&output, ResponseJoinRoom_fields,
                                       &response_room)) {
                perror("Failed to send response join room to client\n");
                break;
              }
            }
            send_response = 1;
            break;
          }
        }
        temp = temp->next;
      }

      if (!send_response) {
        ResponseJoinRoom response_room = ResponseJoinRoom_init_zero;
        response_room.status = ResponseStatus_FAILED;

        if (!pb_encode_delimited(&output, ResponseJoinRoom_fields,
                                 &response_room)) {
          perror("Failed to send response join room to client\n");
          break;
        }
      }

    } else if (request_type.type == RequestType_SEND_MESSAGE) {

      printf("Client request to send message\n");
      ChatMessage chat = ChatMessage_init_zero;
      bzero(chat.chat, BUFFER_SIZE);
      if (!pb_decode_delimited(&input, ChatMessage_fields, &chat)) {
        perror("Decode failed\n");
        break;
      }
      Room *temp = *function_arg.room_head;

      while (temp != NULL) {
        if (temp->room_id == chat.room_id) {

          if (strncmp(chat.chat, ">exit<", 6) == 0) {
            write(function_arg.current_client_fd, "Exit session for \n", 18);
            //
            Client *traverse = *temp->head;

            if (traverse == NULL) {

              break;
            } else if (traverse->fd == function_arg.current_client_fd) {

              Client *current_client = traverse;

              if (traverse->next != NULL) {
                // temp->head = &(traverse->next);
                traverse = traverse->next;
                *((*function_arg.room_head)->head) = traverse;

                printf("The current room head is %p\n",
                       (*function_arg.room_head)->head);
                printf("The current room pointer is %p\n",
                       (*function_arg.room_head));
                // current_client->next = NULL;
                printf("The client address is %p\n", current_client);

                free(current_client);
                current_client = NULL;

              } else {
                printf("The memory is current_client is %p\n", current_client);

                free(current_client);

                current_client = NULL;
                Room *temp_room = *function_arg.room_head;

                printf("The memory is temp_room->head is %p\n",
                       *(temp_room->head));
                printf("The memory is temp_room is %p\n", temp_room);

                free(temp_room->head);
                // temp_room->head = NULL;
                free(temp_room);
                // temp_room = NULL;
                *(function_arg.room_head) = NULL;
                // Room *new_room_head = NULL;
                // function_arg.room_head = &new_room_head;
              }

              // current_client->next = NULL;
              // }
            } else {

              while (traverse->next != NULL) {
                if (traverse->next->fd == function_arg.current_client_fd) {
                  Client *current_client = traverse->next;
                  traverse->next = current_client->next;
                  free(current_client);
                }
              }
            }

            break;

          } else {
            send_msg_to_other_client(temp->head, function_arg.current_client_fd,
                                     (const char *)chat.chat);
          }
          break;
        }
        temp = temp->next;
      }
    }
  }

  // Need a ways to free all the room allocated if the client cut of the
  // conenction midway

  close(fd);
  pthread_exit(NULL);
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

  Room *room_head = NULL;

  while (1) {
    connfd = accept(listenfd, NULL, NULL);

    if (connfd == -1) {
      printf("Have trouble accept connection\n");
    }

    pthread_t thread_id;
    HandleConnectionArg arg = {
        .current_client_fd = connfd,
        .thread_id = &thread_id,
        .room_head = &room_head,
    };

    printf("A new client has connected with id %d\n", connfd);

    int check = pthread_create(&thread_id, NULL, handle_connection, &arg);

    if (check != 0) {
      printf("Failed to handle connection\n");
    }
  }

  // pthread_join(thread_id, NULL);
  return 0;
}
