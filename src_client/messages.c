#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <errno.h>
#include "messages.h"

int socket_to_driver;

int natoms = 3;

void error(char *msg)
{
  perror(msg);
  exit(1);
}

int initialize_client()
{
  int ret;
  struct sockaddr_un driver_address;
  char buffer[BUFFER_SIZE];
  int i;

  printf("In C code\n");

  //create the socket
  socket_to_driver = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socket_to_driver < 0) {
    error("Could not create socket");
  }
  printf("Here is the socket: %i\n",socket_to_driver);

  //create the socket address
  memset(&driver_address, 0, sizeof(struct sockaddr_un));
  driver_address.sun_family = AF_UNIX;
  strncpy(driver_address.sun_path, SOCKET_NAME, sizeof(driver_address.sun_path) - 1);
  ret = connect(socket_to_driver, (const struct sockaddr *) &driver_address, sizeof(struct sockaddr_un));
  if (ret < 0) {
    error("Could not connect to server");
  }

  for (;;) {

    //send a message through the socket
    strcpy(buffer, "FORCES");
    ret = write(socket_to_driver, buffer, strlen(buffer) + 1);
    if (ret < 0) {
      error("Could not write to socket");
    }

    //read message from client
    ret = read(socket_to_driver, buffer, BUFFER_SIZE);
    if (ret < 0) {
      error("Could not read message");
    }

    printf("Buffer: %s",buffer);
    printf("\n");

    if ( strcmp(buffer,"COORDS") == 0 ) {
      receive_coordinates();
    }
    else if ( strcmp(buffer,"EXIT") == 0 ) {
      printf("Client exiting\n");
      exit(0);
    }

  }

}

int receive_coordinates()
{
  int i;
  int ret;
  float coords[3*natoms];

  ret = read(socket_to_driver, coords, sizeof(coords));
  if (ret < 0) {
    error("Could not read message");
  }

  for (i=0; i < 3*natoms; i++) {
    printf("received coords: %i %f\n",i,coords[i]);
  }

}
