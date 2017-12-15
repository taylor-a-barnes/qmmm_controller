#include "messages.h"

using namespace QMMM_CLIENT;

/*
void error(char *msg)
{
  perror(msg);
  exit(1);
}
*/

QMMMClient::QMMMClient()
{
  
}

int QMMMClient::initialize_client()
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

  //read information about cell dimensions
  read_label(socket_to_driver, buffer);
  if ( strcmp(buffer,"CELL") == 0 ) {
    receive_cell();
  }
  else {
    error("Expected cell information from driver");
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
    else {
      error("Label from driver not recognized");
    }

    //send a message through the socket
    send_forces();

  }

}



/* Receive the initialization information from the socket */
int QMMMClient::receive_initialization()
{
  int32_t init[4]; //uses int32_t to ensure that client and server both use the same sized int

  receive_array(socket_to_driver, init, sizeof(init));

  natoms = init[0];
  num_qm = init[1];
  num_mm = init[2];
  ntypes = init[3];

  //initialize arrays for QM communication
  qm_coord = ( double* )malloc( 3*num_qm * sizeof(double) );
  qm_charge = ( double* )malloc( num_qm * sizeof(double) );
  mm_charge_all = ( double* )malloc( natoms * sizeof(double) );
  mm_coord_all = ( double* )malloc( 3*natoms * sizeof(double) );
  mm_mask_all = ( int* )malloc( natoms * sizeof(int) );
  type = ( int* )malloc( natoms * sizeof(int) );
  mass = ( double* )malloc( ntypes+1 * sizeof(double) );
  qm_force = ( double* )malloc( 3*num_qm * sizeof(double) );
  mm_force_all = ( double* )malloc( 3*natoms * sizeof(double) );

}



/* Receive the cell dimensions */
int QMMMClient::receive_cell()
{
  double celldata[9];

  receive_array(socket_to_driver, celldata, sizeof(celldata));

  boxlo0 = celldata[0];
  boxlo1 = celldata[1];
  boxlo3 = celldata[2];
  boxhi0 = celldata[3];
  boxhi1 = celldata[4];
  boxhi2 = celldata[5];
  cellxy = celldata[6];
  cellxz = celldata[7];
  cellyz = celldata[8];
}



/* Read the coordinates from the socket */
int QMMMClient::receive_coordinates()
{
  int i;
  double coords[3*natoms];

  receive_array(socket_to_driver, coords, sizeof(coords));

  for (i=0; i < 3*natoms; i++) {
    printf("received coords: %i %f\n",i,coords[i]);
  }

  printf("c - size of qm_coords: %i\n",(3*num_qm)*sizeof(double));
  receive_array(socket_to_driver, qm_coord, (3*num_qm)*sizeof(double));
  receive_array(socket_to_driver, qm_charge, (num_qm)*sizeof(double));
  receive_array(socket_to_driver, mm_charge_all, (natoms)*sizeof(double));
  receive_array(socket_to_driver, mm_coord_all, (3*natoms)*sizeof(double));
  receive_array(socket_to_driver, mm_mask_all, (natoms)*sizeof(int));
  receive_array(socket_to_driver, type, (natoms)*sizeof(int));
  receive_array(socket_to_driver, mass, (ntypes+1)*sizeof(double));
}



int QMMMClient::send_forces()
{
  send_label(socket_to_driver, "FORCES");

  //send the arrays that contain the qm forces
  send_array(socket_to_driver, qm_force, (3*num_qm)*sizeof(double));
  send_array(socket_to_driver, mm_force_all, (3*natoms)*sizeof(double));
}
