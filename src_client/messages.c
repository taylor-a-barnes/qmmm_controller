#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <errno.h>
#include "messages.h"

int socket_to_driver;

int natoms;
int num_qm;
int num_mm;
int ntypes;

char buffer[BUFFER_SIZE];

void error(char *msg)
{
  perror(msg);
  exit(1);
}

int initialize_client()
{
  int ret;
  struct sockaddr_un driver_address;
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

  //read initialization information
  read_label(socket_to_driver, buffer);
  if( strcmp(buffer,"INIT") == 0 ) {
    receive_initialization();
  }
  else {
    error("Initial message from server is invalid");
  }

  for (;;) {

    //read message from client
    read_label(socket_to_driver, buffer);

    printf("Buffer: %s",buffer);
    printf("\n");

    if ( strcmp(buffer,"COORDS") == 0 ) {
      receive_coordinates();
    }
    else if ( strcmp(buffer,"EXIT") == 0 ) {
      printf("Client exiting\n");
      exit(0);
    }

    //send a message through the socket
    send_label(socket_to_driver, "FORCES");

  }

}



/* Read the initialization information from the socket */
int receive_initialization()
{
  int i;
  int ret;
  int remaining;
  int32_t init[4]; //uses int32_t to ensure that client and server both use the same sized int

  receive_array(socket_to_driver, init, sizeof(init));

  natoms = init[0];
  num_qm = init[1];
  num_mm = init[2];
  ntypes = init[3];
}



/* Read the coordinates from the socket */
int receive_coordinates()
{
  int i;
  int ret;
  double coords[3*natoms];

  receive_array(socket_to_driver, coords, sizeof(coords));

  for (i=0; i < 3*natoms; i++) {
    printf("received coords: %i %f\n",i,coords[i]);
  }

}
