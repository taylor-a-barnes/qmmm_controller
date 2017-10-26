/* Server code for the QM/MM driver */
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <errno.h>
#include "messages.h"

int qm_socket, qm_socket_in;
struct sockaddr_un qm_server, qm_client;
char buffer[BUFFER_SIZE];

int natoms = 3;
int num_qm = 0;
int num_mm = 0;
int ntypes = 0;



void error(char *msg)
{
  perror(msg);
  exit(1);
}



int initialize_server()
{
  int ret;

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
  int ret;
  int i;
  int max_iterations = 10;

  //accept a connection
  qm_socket_in = accept(qm_socket, NULL, NULL);
  if (qm_socket_in < 0) {
    error("Could not accept connection");
  }

  //send information about number of atoms, etc. to the client
  send_initialization();

  for (i=1; i <= max_iterations+1; i++) {
    printf("\nIteration %i",i);
    printf("\n");

    //send a message through the socket
    if (i <= max_iterations) {
      send_coordinates();
    }
    else {
      send_exit();
    }


    //read message from client
    /*
    ret = read(qm_socket_in, buffer, BUFFER_SIZE);
    if (ret < 0) {
      error("Could not read message");
    }
    */
    read_label(buffer);
    
    printf(buffer);
    printf("\n");
    

  }

}



/* Send initialization information through the socket */
int send_initialization()
{
  int init[4];

  //label this message
  send_text("INIT");

  //send the nuclear coordinates
  init[0] = natoms;
  init[1] = num_qm;
  init[2] = num_mm;
  init[3] = ntypes;

  send_array(init, sizeof(init));
}



/* Send atomic positions through the socket */
int send_coordinates()
{
  int i;
  float coords[3*natoms];

  //label this message
  send_text("COORDS");

  //send the nuclear coordinates
  for (i=0; i < 3*natoms; i++) {
    coords[i] = 1.0;
    printf("coords: %i %f\n",i,coords[i]);
  }
  send_array(coords, sizeof(coords));
}



/* Send exit signal through the socket */
int send_exit()
{
  send_text("EXIT");
}



/* Send text through the socket */
int send_text(char *msg)
{
  int ret;

  strcpy(buffer, msg);
  ret = write(qm_socket_in, buffer, strlen(buffer) + 1);
  if (ret < 0) {
    error("Could not write to socket");
  }
}



/* Send an array through the socket */
int send_array(void *data, int size)
{
  int ret;

  printf("len: %i",sizeof(data));

  ret = write(qm_socket_in, data, size);
  if (ret < 0) {
    error("Could not write to socket");
  }
}



/* Read a label from the socket */
int read_label(char *buf)
{
  int ret;

  //read message from client
  ret = read(qm_socket_in, buf, BUFFER_SIZE);
  if (ret < 0) {
    error("Could not read message");
  }
}
