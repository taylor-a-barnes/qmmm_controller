#ifndef CLIENT_MESSAGES
#define CLIENT_MESSAGES

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <errno.h>
#include "sockets.h"

//#define SOCKET_NAME "~/mm_main.socket"
#define SOCKET_NAME "/project/projectdirs/m1944/tabarnes/edison/qmmm/pipes/t3/mm_main.socket"

namespace QMMM_CLIENT {

class QMMMClient {

 public:
  QMMMClient();
  int initialize_client();
  int receive_initialization();
  int receive_cell();
  int receive_coordinates();
  int send_forces();
  int receive_forces(double*,double*,double*);
  int send_initialization(int);
  int send_natoms(int, int, int, int);
  int send_cell();
  int send_coordinates(double*,double*,double*,double*,int*,int*,double*);
  int receive_coordinates(double*,double*,double*,double*,int*,int*,double*);
  int receive_forces();

  int socket_to_driver;

  int natoms;
  int num_qm;
  int num_mm;
  int ntypes;

  double boxlo0;
  double boxlo1;
  double boxlo2;
  double boxhi0;
  double boxhi1;
  double boxhi2;
  double cellxy;
  double cellxz;
  double cellyz;

  double *qm_coord;
  double *qm_charge;
  double *mm_charge_all;
  double *mm_coord_all;
  int *mm_mask_all;
  int *type;
  int *mass;

  double *qm_force;
  double *mm_force_all;

  char buffer[BUFFER_SIZE];

};

/* Include C wrapper interfaces to some of the C++ routines */
/*
extern "C" {
  int initialize_client__() {

    QMMMClient client;
    
    return client.initialize_client();
  }

}
*/

}

#endif
