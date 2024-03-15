

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

int main() {

  int sockfd;
  struct sockaddr_in server_addr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  server_addr.sin_port = htons(1234);

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {

    perror("Failed to connect");
    return 1;
  }

  SimpleMessage message = {};
  message.lucky_number = 13;
  pb_ostream_t output = pb_ostream_from_socket(sockfd);

  if (!pb_encode_delimited(&output, SimpleMessage_fields, &message)) {
    fprintf(stderr, "Encoding failed: %s\n", PB_GET_ERROR(&output));
  }

  close(sockfd);

  return 0;
}
