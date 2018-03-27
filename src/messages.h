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
#include "../lib/qmmm_aux.h"

//class Messages {

//public:
int initialize_driver_socket();
int launch_server(const char*);
int initialize_socket();
int initialize_arrays();
int accept_mm_connection();
int accept_mm_subset_connection();
int accept_qm_connection();
int send_cell(int);
int receive_cell(int);

extern int driver_socket;
extern int qm_socket, qm_socket_in;
extern int mm_socket, mm_socket_in;
extern int mm_subset_socket, mm_subset_socket_in;
extern struct sockaddr_un qm_server, qm_client;
extern char buffer[BUFFER_SIZE];

extern int natoms;
extern int num_qm;
extern int num_mm;
extern int ntypes;

extern float boxlo0;
extern float boxlo1;
extern float boxlo2;
extern float boxhi0;
extern float boxhi1;
extern float boxhi2;
extern float cellxy;
extern float cellxz;
extern float cellyz;

extern const float angstrom_to_bohr;

extern double *qm_coord; //in bohr
extern double *qm_charge;
extern double *mm_charge_all;
extern double *mm_coord_all; //in bohr
extern int *mm_mask_all;
extern int *type;
extern double *mass;

extern double *qm_force;
extern double *mm_force_all;
extern double *mm_force_on_qm_atoms;

extern double *mm_force;

extern double *qm_ec_force;
extern double *aradii;

extern int qm_mode;
extern int qm_verbose;
extern int qm_steps;

extern int qm_start;
extern int qm_end;

//}

/* Include C wrapper interfaces to some of the C++ routines */
/*
extern "C" {
  int initialize_driver_socket__() {
    return initialize_driver_socket();
  }
  int launch_server__() {
    return launch_server();
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
*/

#endif
