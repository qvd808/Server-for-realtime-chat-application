#include <asm-generic/errno-base.h>
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
#include "message.pb.h"

void evaluate_cmd(char *arg, int fd);
void *listen_input(void *arg);
void *write_output(void *arg);

typedef struct ThreadWriteArg {
  int fd;
  int *running;
} ThreadWriteArg;

int main() {

  int sockfd;
  struct sockaddr_in server_addr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  server_addr.sin_port = htons(PORT);

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {

    perror("Failed to connect");
    return 1;
  }

  pthread_t thread_read, thread_write;
  pthread_create(&thread_read, NULL, listen_input, &sockfd);
  // pthread_create(&thread_write, NULL, write_output, &sockfd);

  pthread_join(thread_read, NULL);
  // pthread_join(thread_write, NULL);

  close(sockfd);

  return 0;
}

// Listen to user command or message coming from terminal/STDIN_FILENO
void *listen_input(void *arg) {

  int sockfd = *(int *)(arg);

  while (1) {

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    int byte_read = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1);

    evaluate_cmd((char *)(buffer), sockfd);
  }

  return NULL;
}

// Listen to chat message from server
void *write_output(void *arg) {

  ThreadWriteArg function_arg = *(ThreadWriteArg *)(arg);
  int sockfd = function_arg.fd;
  char buffer[BUFFER_SIZE];

  while (*(function_arg.running)) {
    bzero(buffer, BUFFER_SIZE);
    int byte = read(sockfd, buffer, BUFFER_SIZE - 1);
    if (byte < 0) {
      break;
    }
    printf("%s", buffer);
  }

  return NULL;
}

void join_room_session(int fd, int room_id) {

  // const char *CLEAR_SCREEN_ANSI = "\e[1;1H\e[2J";
  // write(STDOUT_FILENO, CLEAR_SCREEN_ANSI, 12);

  pb_ostream_t output = pb_ostream_from_socket(fd);

  int running = 1;
  ThreadWriteArg arg = {
      .fd = fd,
      .running = &running,
  };

  pthread_t thread_write;
  pthread_create(&thread_write, NULL, write_output, (void *)&arg);

  while (1) {

    {
      RequestHeader request = RequestHeader_init_zero;
      request.type = RequestType_SEND_MESSAGE;

      if (!pb_encode_delimited(&output, RequestHeader_fields, &request)) {
        perror("Encoidng the message failed!\n");
        return;
      }
    }

    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);

    int byte = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1);
    if (strncmp(buffer, ">exit<", 6) == 0) {
      ChatMessage chat;
      strcpy(chat.chat, buffer);
      chat.room_id = room_id;

      if (!pb_encode_delimited(&output, ChatMessage_fields, &chat)) {
        fprintf(stderr, "Encoding failed: %s\n", PB_GET_ERROR(&output));
      }
      running = 0;
      break;
    }

    ChatMessage chat;
    strcpy(chat.chat, buffer);
    chat.room_id = room_id;

    if (!pb_encode_delimited(&output, ChatMessage_fields, &chat)) {
      fprintf(stderr, "Encoding failed: %s\n", PB_GET_ERROR(&output));
    }
  }

  pthread_join(thread_write, NULL);
  printf("Exit the chat room\n");
}

void join(int argc, char *argv[], int fd) {

  if (argc != 3) {
    printf("Please provide the right format: join room_id\n");
    return;
  }

  if (argv[1] == NULL || strlen(argv[1]) == 0) {
    printf("Please provide an id for the room\n");
    return;
  }

  if (argv[2] == NULL || strlen(argv[1]) == 0) {
    printf("Please provide password for the room\n");
    return;
  }

  int room_id = strtol(argv[1], NULL, 10);
  if (room_id == 0) {
    printf("Failed to convert. Please provide a valid id\n");
    return;
  }

  {
    pb_ostream_t output = pb_ostream_from_socket(fd);
    RequestHeader request = RequestHeader_init_zero;
    request.type = RequestType_JOIN_ROOM;

    if (!pb_encode_delimited(&output, RequestHeader_fields, &request)) {
      perror("Encoidng the message failed!\n");
      return;
    }

    RequestJoinRoom request_room = RequestJoinRoom_init_zero;
    request_room.room_id = room_id;
    strcpy(request_room.password, argv[2]);
    printf("Request room password is %s\n", request_room.password);

    if (!pb_encode_delimited(&output, RequestJoinRoom_fields, &request_room)) {
      perror("Encoidng the message failed!\n");
      return;
    }
  }

  {
    pb_istream_t input = pb_istream_from_socket(fd);
    ResponseJoinRoom response = {};
    if (!pb_decode_delimited(&input, ResponseJoinRoom_fields, &response)) {
      perror("Encoidng the response failed!\n");
      return;
    }

    if (response.status == ResponseStatus_FAILED) {
      printf("Failed to join the room\n");
    } else {
      printf("Joining a room\n");
      join_room_session(fd, room_id);
    }
  }
}

void create(int argc, char *argv[], int fd) {

  if (argc != 1) {
    printf("Please provide the right format: create\n");
    return;
  }

  printf("Please provide password for your room:\n");
  char *buffer = malloc(BUFFER_SIZE);
  memset(buffer, 0, BUFFER_SIZE);
  int byte = read(STDIN_FILENO, buffer, BUFFER_SIZE);

  if (byte < 0) {
    printf("Error in creating a room\n");
    return;
  }

  {
    RequestHeader request = RequestHeader_init_zero;
    request.type = RequestType_CREATE_ROOM;
    pb_ostream_t output = pb_ostream_from_socket(fd);

    if (!pb_encode_delimited(&output, RequestHeader_fields, &request)) {
      perror("Encoidng the message failed!\n");
      return;
    }

    RequestCreateRoom request_room = RequestCreateRoom_init_zero;
    strcpy(request_room.password, buffer);
    printf("The password for the room is %s\n", request_room.password);

    if (!pb_encode_delimited(&output, RequestCreateRoom_fields,
                             &request_room)) {
      perror("Encoidng the message failed!\n");
      return;
    }
  }

  printf("Creating a room\n");
  free(buffer);
}

void exit_program(int argc, char *argv[], int fd) {
  pb_ostream_t output = pb_ostream_from_socket(fd);

  RequestHeader header = RequestHeader_init_zero;
  header.type = RequestType_EXIT_ROOM;

  if (!pb_encode_delimited(&output, RequestHeader_fields, &header)) {
    printf("Failed to encode the exit request\n");
  }

}
void help(int argc, char *argv[], int fd) {

  printf("join <room_id> to join a room\ncreate to create a room to chat with "
         "your friends\n");
}

typedef struct {
  char *command;
  void (*function)(int argc, char *argv[], int fd);
} DispatchEntry;

void execute_dispatch_command(int argc, char *argv[], int fd) {

  DispatchEntry dispatch_table[] = {
      {"create", create},
      {"join", join},
      {"help", help},
      {"exit", exit_program},
  };

  int run_command = 0;
  int numCommand = sizeof(dispatch_table) / sizeof(dispatch_table[0]);

  for (int i = 0; i < numCommand; i++) {
    if (strcmp(dispatch_table[i].command, argv[0]) == 0) {
      dispatch_table[i].function(argc, argv, fd);
      run_command = 1;
    }
  }

  // execute help if nothing can be found
  if (!run_command) {
    dispatch_table[1].function(argc, argv, fd);
  }
}

void evaluate_cmd(char *buffer, int fd) {

  char *argv[10];
  int argc = 0;
  char *token;

  token = strtok(buffer, " \n\0");

  while (token != NULL) {
    argv[argc] = token;
    argc += 1;
    if (argc == 10) {
      printf("Can't not handle more than 10 argument\n");
    }
    token = strtok(NULL, " \n\0");
  }

  execute_dispatch_command(argc, argv, fd);
}
