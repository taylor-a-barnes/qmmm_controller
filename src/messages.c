/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <errno.h>
#include "messages.h"

int qm_socket, qm_socket_in;
int ret;
struct sockaddr_un qm_server, qm_client;
char buffer[BUFFER_SIZE];

void error(char *msg)
{
  perror(msg);
  exit(1);
}

int initialize_server()
{

  printf("In C code\n");

  //unlink the socket, in case the program previously exited unexpectedly
  unlink(SOCKET_NAME);

  //create the socket
  qm_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (qm_socket < 0) {
    error("Could not create socket");
  }
  printf("Here is the socket: %i\n",qm_socket);

  //create the socket address
  memset(&qm_server, 0, sizeof(struct sockaddr_un));
  qm_server.sun_family = AF_UNIX;
  strncpy(qm_server.sun_path, SOCKET_NAME, sizeof(qm_server.sun_path) - 1);
  ret = bind(qm_socket, (const struct sockaddr *) &qm_server, sizeof(struct sockaddr_un));
  if (ret < 0) {
    error("Could not bind socket");
  }

  //start listening
  // the second argument is the backlog size
  ret = listen(qm_socket, 20);
  if (ret < 0) {
    error("Could not listen");
  }
  
}

int communicate()
{

  //accept a connection
  qm_socket_in = accept(qm_socket, NULL, NULL);
  if (qm_socket_in < 0) {
    error("Could not accept connection");
  }

  //read message from client
  ret = read(qm_socket_in, buffer, BUFFER_SIZE);
  if (ret < 0) {
    error("Could not read message");
  }

  printf(buffer);
  printf("\n");
}
