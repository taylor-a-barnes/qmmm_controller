#ifndef QMMM_MESSAGES
#define QMMM_MESSAGES

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../lib/sockets.h"
#include "../lib/qmmm_aux.c"

//class Messages {

//public:
int initialize_driver_socket();
int initialize_socket();
int initialize_arrays();
int accept_mm_connection();
int accept_mm_subset_connection();
int accept_qm_connection();
int run_simulation();
int send_cell(int);
int receive_cell(int);

int driver_socket;
int qm_socket, qm_socket_in;
int mm_socket, mm_socket_in;
int mm_subset_socket, mm_subset_socket_in;
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

const float angstrom_to_bohr = 1.889725989;

double *qm_coord; //in bohr
double *qm_charge;
double *mm_charge_all;
double *mm_coord_all; //in bohr
int *mm_mask_all;
int *type;
double *mass;

double *qm_force;
double *mm_force_all;
double *mm_force_on_qm_atoms;

double *mm_force;

double *qm_ec_force;
double *aradii;

int qm_mode;
int qm_verbose;
int qm_steps;

int qm_start;
int qm_end;

//}

/* Include C wrapper interfaces to some of the C++ routines */
extern "C" {
  int initialize_driver_socket__() {
    return initialize_driver_socket();
  }
  int accept_mm_connection__() {
    return accept_mm_connection();
  }
  int accept_mm_subset_connection__() {
    return accept_mm_subset_connection();
  }
  int accept_qm_connection__() {
    return accept_qm_connection();
  }
  int run_simulation__() {
    return run_simulation();
  }
}

#endif
