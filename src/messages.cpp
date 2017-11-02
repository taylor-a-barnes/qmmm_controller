/* Server code for the QM/MM driver */
#include "messages.h"


/*
void error(char *msg)
{
  perror(msg);
  exit(1);
}
*/


/* Initialize everything necessary for the driver to act as a server */
int initialize_server()
{
  qm_socket = initialize_socket(SOCKET_NAME);
  
  mm_socket = initialize_socket("./mm_main.socket");

  //initialize_arrays();
}



/* Initialize a socket */
int initialize_socket(char *name)
{
  int ret;
  int sock;

  printf("In C code\n");

  //unlink the socket, in case the program previously exited unexpectedly
  unlink(name);

  //create the socket
  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    error("Could not create socket");
  }
  printf("Here is the socket: %i\n",sock);

  //create the socket address
  memset(&qm_server, 0, sizeof(struct sockaddr_un));
  qm_server.sun_family = AF_UNIX;
  strncpy(qm_server.sun_path, name, sizeof(qm_server.sun_path) - 1);
  ret = bind(sock, (const struct sockaddr *) &qm_server, sizeof(struct sockaddr_un));
  if (ret < 0) {
    error("Could not bind socket");
  }

  //start listening
  // the second argument is the backlog size
  ret = listen(sock, 20);
  if (ret < 0) {
    error("Could not listen");
  }

  return sock;
}



int initialize_arrays()
{
  //initialize arrays for QM communication
  qm_coord = ( double* )malloc( 3*num_qm );
  qm_charge = ( double* )malloc( num_qm );
  mm_charge_all = ( double* )malloc( natoms );
  mm_coord_all = ( double* )malloc( 3*natoms );
  mm_mask_all = ( int* )malloc( natoms );
  type = ( int* )malloc( natoms );
  mass = ( int* )malloc( ntypes+1 );
  qm_force = ( double* )malloc( 3*num_qm );
  mm_force_all = ( double* )malloc( 3*natoms );
  
}



int run_simulation()
{
  printf("Running the simulation\n");

  //accept a connection
  mm_socket_in = accept(mm_socket, NULL, NULL);
  if (mm_socket_in < 0) {
    error("Could not accept connection");
  }

  //read initialization information
  read_label(mm_socket_in, buffer);
  if( strcmp(buffer,"INIT") == 0 ) {
    receive_initialization();
  }
  else {
    error("Initial message from LAMMPS is invalid");
  }

  printf("Number of atoms: %i",num_qm);
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
  send_initialization(qm_socket_in);

  //send information about the cell dimensions
  send_cell();

  for (i=1; i <= max_iterations; i++) {

    printf("\nIteration %i",i);
    printf("\n");

    //send a message through the socket
    send_coordinates();

    //read message from client
    read_label(qm_socket_in,buffer);
    
    if ( strcmp(buffer,"FORCES") == 0 ) {
      receive_forces();
    }
    else {
      error("Label from client not recognized");
    }

    printf(buffer);
    printf("\n");

  }

  //tell the client to exit
  send_exit();

}



/* Send initialization information through the socket */
int send_initialization(int sock)
{
  int32_t init[4]; //uses int32_t to ensure that client and server both use the same sized int

  //label this message
  send_label(sock, "INIT");

  //send the nuclear coordinates
  init[0] = natoms;
  init[1] = num_qm;
  init[2] = num_mm;
  init[3] = ntypes;

  send_array(sock, init, sizeof(init));
}



/* Receive initialization information through the socket */
int receive_initialization()
{
  int32_t init[4]; //uses int32_t to ensure that client and server both use the same sized int

  receive_array(mm_socket_in, init, sizeof(init));

  natoms = init[0];
  num_qm = init[1];
  num_mm = init[2];
  ntypes = init[3];

  printf("natoms: %i\n",natoms);
  printf("num_qm: %i\n",num_qm);
  printf("num_mm: %i\n",num_mm);
  printf("ntypes: %i\n",ntypes);

  //initialize arrays for QM communication
  initialize_arrays();
}



/* Send cell dimensions */
int send_cell()
{
  double celldata[9];

  //label this message
  send_label(qm_socket_in, "CELL");

  //send the cell data
  celldata[0] = boxlo0;
  celldata[1] = boxlo1;
  celldata[2] = boxlo2;
  celldata[3] = boxhi0;
  celldata[4] = boxhi1;
  celldata[5] = boxhi2;
  celldata[6] = cellxy;
  celldata[7] = cellxz;
  celldata[8] = cellyz;

  send_array(qm_socket_in, celldata, sizeof(celldata));
}



/* Send atomic positions through the socket */
int send_coordinates()
{
  int i;
  double coords[3*natoms];

  //label this message
  send_label(qm_socket_in, "COORDS");

  //send the nuclear coordinates
  for (i=0; i < 3*natoms; i++) {
    coords[i] = 1.0;
    printf("coords: %i %f\n",i,coords[i]);
  }
  printf("size of coords: %i\n",sizeof(coords));
  send_array(qm_socket_in, coords, sizeof(coords));

  printf("size of qm_coords: %i\n",(3*num_qm)*sizeof(double));
  send_array(qm_socket_in, qm_coord, (3*num_qm)*sizeof(double));
  send_array(qm_socket_in, qm_charge, (num_qm)*sizeof(double));
  send_array(qm_socket_in, mm_charge_all, (natoms)*sizeof(double));
  send_array(qm_socket_in, mm_coord_all, (3*natoms)*sizeof(double));
  send_array(qm_socket_in, mm_mask_all, (natoms)*sizeof(int));
  send_array(qm_socket_in, type, (natoms)*sizeof(int));
  send_array(qm_socket_in, mass, (ntypes+1)*sizeof(int));
}



/* Send exit signal through the socket */
int send_exit()
{
  send_label(qm_socket_in, "EXIT");
}



/* Receive the forces from the socket */
int receive_forces()
{
  receive_array(qm_socket_in, qm_force, (3*num_qm)*sizeof(double));
  receive_array(qm_socket_in, mm_force_all, (3*natoms)*sizeof(double));
}
