#ifndef QMMM_MESSAGES
#define QMMM_MESSAGES

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include "../lib/sockets.h"

#define SOCKET_NAME "./9Lq7BNBnBycd6nxy.socket"
//#define BUFFER_SIZE 12

//class Messages {

//public:
int initialize_server();
int initialize_socket(char*);
int initialize_arrays();
int run_simulation();
int communicate();
int send_initialization();
int send_cell();
int send_coordinates();
int send_exit();
int receive_forces();

int qm_socket, qm_socket_in;
int mm_socket, mm_socket_in;
struct sockaddr_un qm_server, qm_client;
char buffer[BUFFER_SIZE];

int natoms = 3;
int num_qm = 2;
int num_mm = 2;
int ntypes = 2;

float boxlo0 = 0.0;
float boxlo1 = 0.0;
float boxlo2 = 0.0;
float boxhi0 = 0.0;
float boxhi1 = 0.0;
float boxhi2 = 0.0;
float cellxy = 0.0;
float cellxz = 0.0;
float cellyz = 0.0;

double *qm_coord;
double *qm_charge;
double *mm_charge_all;
double *mm_coord_all;
int *mm_mask_all;
int *type;
int *mass;

double *qm_force;
double *mm_force_all;

//}

/* Include C wrapper interfaces to some of the C++ routines */
extern "C" {
  int initialize_server__() {
    return initialize_server();
  }
  int communicate__() {
    return communicate();
  }
  int run_simulation__() {
    return run_simulation();
  }
}

#endif
