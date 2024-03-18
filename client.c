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

typedef struct MessageChunk {
  char buffer[BUFFER_SIZE];
} MessageChunk;

void evaluate_cmd(char *arg);
void *listen_input(void *arg);
void *write_output(void *arg);

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
  pthread_create(&thread_write, NULL, write_output, &sockfd);

  pthread_join(thread_read, NULL);
  pthread_join(thread_write, NULL);

  close(sockfd);

  return 0;
}

void *listen_input(void *arg) {

  int sockfd = *(int *)(arg);
  pb_ostream_t output = pb_ostream_from_socket(sockfd);

  while (1) {

    MessageChunk chunk;
    memset(chunk.buffer, 0, BUFFER_SIZE);

    int byte_read = read(STDIN_FILENO, chunk.buffer, BUFFER_SIZE - 1);

    evaluate_cmd((char *)(chunk.buffer));
    // ChatMessage chat = {};
    // strcpy(chat.chat, chunk.buffer);
    //
    // if (!pb_encode_delimited(&output, ChatMessage_fields, &chat)) {
    //   fprintf(stderr, "Encoding failed: %s\n", PB_GET_ERROR(&output));
    // }
  }

  return NULL;
}

void *write_output(void *arg) {

  int sockfd = *(int *)(arg);
  char buffer[BUFFER_SIZE];

  while (1) {
    bzero(buffer, BUFFER_SIZE);
    int byte = read(sockfd, buffer, BUFFER_SIZE - 1);
    if (byte < 0) {
      break;
    }
    printf("%s", buffer);
  }

  return NULL;
}

void join(int argc, char *argv[]) {

  if (argv[1] == NULL || strlen(argv[1]) == 0) {
    printf("Please provide an id for the room\n");
    return;
  }

  printf("argv 1 is %s\n", argv[1]);
  printf("Joining a room\n");
}
void create(int argc, char *argv[]) {

  if (argv[1] == NULL || strlen(argv[1]) == 0) {
    printf("Please provide an id for the room\n");
    return;
  }

  printf("Creating a romm\n");
}
void help(int argc, char *argv[]) {

  if (argv[1] == NULL || strlen(argv[1]) == 0) {
    printf("Please provide an id for the room\n");
    return;
  }

  printf("Helpping\n");
}

typedef struct {
  char *command;
  void (*function)(int argc, char *argv[]);
} DispatchEntry;

void evaluate_cmd(char *buffer) {

  char *argv[10];
  int argc = 0;
  char *token;

  // char *newStr[11];
  // memcpy(newStr, argv, 88);
  //
  // if (newStr[0] != NULL)
  //   printf("The new string is %s and %s\n", newStr[0], newStr[0] + 7);

  token = strtok(buffer, " \n\0");

  DispatchEntry dispatch_table[] = {
      {"create", create},
      {"join", join},
      {"help", help},
  };

  while (token != NULL) {
    argv[argc] = token;
    argc += 1;
    if (argc == 10) {
      printf("Can't not handle more than 10 argument\n");
    }
    token = strtok(NULL, " \n\0");
  }

  int numCommand = sizeof(dispatch_table) / sizeof(dispatch_table[0]);
  for (int i = 0; i < numCommand; i++) {
    if (strcmp(dispatch_table[i].command, argv[0]) == 0) {
      dispatch_table[i].function(argc, argv);
      break;
    }
  }

  for (int i = 0; i < argc; i++) {
    memset(argv[i], 0, strlen(argv[i]));
  }

  // create an enum to pass in

  //   printf("Argument at index %d is %s\n", i, argv[i]);
  // }
}
