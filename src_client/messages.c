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
  read_label(buffer);
  if( strcmp(buffer,"INIT") == 0 ) {
    receive_initialization();
  }
  else {
    error("Initial message from server is invalid");
  }

  for (;;) {

    //read message from client
    read_label(buffer);

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
    send_label("FORCES");

  }

}



/* Read the initialization information from the socket */
int receive_initialization()
{
  int i;
  int ret;
  int remaining;
  int32_t init[4]; //uses int32_t to ensure that client and server both use the same sized int

  receive_array(init, sizeof(init));

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

  receive_array(coords, sizeof(coords));

  for (i=0; i < 3*natoms; i++) {
    printf("received coords: %i %f\n",i,coords[i]);
  }

}



/* Read an array from the socket */
int receive_array(void *data, int size)
{
  int ret;
  int remaining;

  remaining = size;
  do {
    ret = read(socket_to_driver, data, remaining);
    printf("Read return: %i %s\n",ret,data);
    if (ret < 0) {
      error("Could not read message");
    }
    else if (ret == 0) {
      error("Read message of size zero");
    }
    else {
      data += ret;
      remaining -= ret;
    }
  }
  while (remaining > 0);

}



/* Send text through the socket */
int send_label(char *msg)
{
  int ret;
  int remaining;

  strcpy(buffer, msg);

  char *buf = (char*)&buffer;
  remaining = sizeof(buffer);
  do {
    ret = write(socket_to_driver, buf, remaining);
    if (ret < 0) {
      error("Could not write to socket");
    }
    else if (ret == 0) {
      error("Read message of size zero");
    }
    else {
      buf += ret;
      remaining -= ret;
    }
    
  }
  while (remaining > 0);

}



/* Read a label from the socket */
int read_label(char *buf)
{
  int ret;
  int remaining;

  remaining = BUFFER_SIZE;
  do {
    ret = read(socket_to_driver, buf, remaining);
    printf("C - Read return: %i %s\n",ret,buf);
    if (ret < 0) {
      error("Could not read message");
    }
    else if (ret == 0) {
      error("Read message of size zero");
    }
    else {
      buf += ret;
      remaining -= ret;
    }
  }
  while (remaining > 0);

  printf("READ LABEL: %i\n",ret);
}
