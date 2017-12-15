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



/* Initialize a socket */
int QMMMClient::initialize_socket(char *name)
{
  int ret;
  int sock;
  int socket_in;

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
  memset(&driver_server, 0, sizeof(struct sockaddr_un));
  driver_server.sun_family = AF_UNIX;
  strncpy(driver_server.sun_path, name, sizeof(driver_server.sun_path) - 1);
  ret = bind(sock, (const struct sockaddr *) &driver_server, sizeof(struct sockaddr_un));
  if (ret < 0) {
    error("Could not bind socket");
  }

  //start listening
  // the second argument is the backlog size
  ret = listen(sock, 20);
  if (ret < 0) {
    error("Could not listen");
  }

  socket_in = accept(sock, NULL, NULL);
  if (socket_in < 0) {
    error("Could not accept connection");
  }
  printf("Received connection from driver\n");

  return socket_in;
}



int QMMMClient::initialize_client()
{
  int ret;
  struct sockaddr_in driver_address;
  int i;
  //char *serv_host = "localhost";
  string readline;
  int port;
  struct hostent *host_ptr;
  ifstream hostfile("../hostname");
  
  port = 8021;

  printf("In C code TESTING\n");

  if (hostfile.is_open()) {
    //read the first line
    getline(hostfile,readline);
  }
  printf("%%% HOSTNAME: %s\n",readline);
  int hlen = readline.length();
  char serv_host[hlen+1];
  strcpy(serv_host, readline.c_str());
  printf("Driver hostname: %s\n",serv_host);





  //get the address of the host
  host_ptr = gethostbyname(serv_host);
  if (host_ptr == NULL) {
    error("Error in gethostbyname");
  }
  if (host_ptr->h_addrtype != AF_INET) {
    error("Unkown address type");
  }

  bzero((char *) &driver_address, sizeof(driver_address));
  driver_address.sin_family = AF_INET;
  driver_address.sin_addr.s_addr = 
    ((struct in_addr *)host_ptr->h_addr_list[0])->s_addr;
  driver_address.sin_port = htons(port);

  //create the socket
  socket_to_driver = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_to_driver < 0) {
    error("Could not create socket");
  }
  printf("Here is the socket: %i\n",socket_to_driver);

  //connect to the driver
  ret = connect(socket_to_driver, (const struct sockaddr *) &driver_address, sizeof(struct sockaddr_un));
  if (ret < 0) {
    error("Could not connect to the driver");
  }
}



int QMMMClient::initialize_client_unix()
{
  int ret;
  struct sockaddr_un driver_address;
  int i;

  printf("In C code TESTING\n");

  //create the socket
  socket_to_driver = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socket_to_driver < 0) {
    error("Could not create socket");
  }
  printf("Here is the socket: %i\n",socket_to_driver);

  //create the socket address
  //<<<
  /*
  memset(&driver_address, 0, sizeof(struct sockaddr_un));
  driver_address.sun_family = AF_UNIX;
  strncpy(driver_address.sun_path, SOCKET_NAME, sizeof(driver_address.sun_path) - 1);
  ret = connect(socket_to_driver, (const struct sockaddr *) &driver_address, sizeof(struct sockaddr_un));
  if (ret < 0) {
    error("Could not connect to server");
  }
  */
  printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
  ret = -1;
  do {
    //printf("   Trying\n");
    memset(&driver_address, 0, sizeof(struct sockaddr_un));
    driver_address.sun_family = AF_UNIX;
    strncpy(driver_address.sun_path, SOCKET_NAME, sizeof(driver_address.sun_path) - 1);
    ret = connect(socket_to_driver, (const struct sockaddr *) &driver_address, sizeof(struct sockaddr_un));
  }
  while ( ret < 0 );
  //>>>


  //send the initialization information
  //send_initialization(socket_to_driver);

  /*
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
  */

}



/* Receive the initialization information from the socket */
int QMMMClient::receive_initialization()
{
  int32_t init[4]; //uses int32_t to ensure that client and server both use the same sized int

  printf("Receiving initilizaiton: %i\n",socket_to_driver);

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

  printf("Received initilizaiton: %i\n",socket_to_driver);
}



/* Receive the cell dimensions */
int QMMMClient::receive_cell()
{
  double celldata[9];

  receive_array(socket_to_driver, celldata, sizeof(celldata));

  boxlo0 = celldata[0];
  boxlo1 = celldata[1];
  boxlo2 = celldata[2];
  boxhi0 = celldata[3];
  boxhi1 = celldata[4];
  boxhi2 = celldata[5];
  cellxy = celldata[6];
  cellxz = celldata[7];
  cellyz = celldata[8];
}



/* Receive the coordinates from the socket */
int QMMMClient::receive_coordinates(double* qm_coord__, double* qm_charge__, double* mm_charge_all__, double* mm_coord_all__, int* mm_mask_all__, int* type__, double* mass__)
{
  receive_array(socket_to_driver, qm_coord__, (3*num_qm)*sizeof(double));
  receive_array(socket_to_driver, qm_charge__, (num_qm)*sizeof(double));
  receive_array(socket_to_driver, mm_charge_all__, (natoms)*sizeof(double));
  receive_array(socket_to_driver, mm_coord_all__, (3*natoms)*sizeof(double));
  receive_array(socket_to_driver, mm_mask_all__, (natoms)*sizeof(int));
  receive_array(socket_to_driver, type__, (natoms)*sizeof(int));
  receive_array(socket_to_driver, mass__, (ntypes+1)*sizeof(double));
}



/* Send the coordinates from the socket */
int QMMMClient::send_coordinates(double* qm_coord__, double* qm_charge__, double* mm_charge_all__, double* mm_coord_all__, int* mm_mask_all__, int* type__, double* mass__)
{
  //label this message
  send_label(socket_to_driver, "COORDS");

  send_array(socket_to_driver, qm_coord__, (3*num_qm)*sizeof(double));
  send_array(socket_to_driver, qm_charge__, (num_qm)*sizeof(double));
  send_array(socket_to_driver, mm_charge_all__, (natoms)*sizeof(double));
  send_array(socket_to_driver, mm_coord_all__, (3*natoms)*sizeof(double));
  send_array(socket_to_driver, mm_mask_all__, (natoms)*sizeof(int));
  send_array(socket_to_driver, type__, (natoms)*sizeof(int));
  send_array(socket_to_driver, mass__, (ntypes+1)*sizeof(double));
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
  receive_array(socket_to_driver, mass, (ntypes+1)*sizeof(int));
}



int QMMMClient::send_forces()
{
  send_label(socket_to_driver, "FORCES");

  //send the arrays that contain the qm forces
  send_array(socket_to_driver, qm_force, (3*num_qm)*sizeof(double));
  send_array(socket_to_driver, mm_force_all, (3*natoms)*sizeof(double));
}



/* Receive forces from the socket */
int QMMMClient::receive_forces(double* qm_force__, double* mm_force_all__, double* mm_force_on_qm_atoms__)
{
  receive_array(socket_to_driver, qm_force__, (3*num_qm)*sizeof(double));
  receive_array(socket_to_driver, mm_force_all__, (3*natoms)*sizeof(double));
  receive_array(socket_to_driver, mm_force_on_qm_atoms__, (3*num_qm)*sizeof(double));
}



/* Receive QM coordinates */
int QMMMClient::receive_qm_coordinates(double* qm_coord__, int num_qm__)
{
  receive_array(socket_to_driver, qm_coord__, (3*num_qm__)*sizeof(double));
}



/* Send QM forces */
int QMMMClient::send_mm_force_on_qm_atoms(double* mm_force_on_qm_atoms__, int num_qm__)
{
  send_array(socket_to_driver, mm_force_on_qm_atoms__, (3*num_qm__)*sizeof(double));
}



/* Send initialization information through the socket */
int QMMMClient::send_initialization(int sock)
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



/* Send initialization information through the socket */
int QMMMClient::send_natoms(int natoms__, int num_qm__, int num_mm__, int ntypes__)
{
  int32_t init[4]; //uses int32_t to ensure that client and server both use the same sized int

  //label this message
  send_label(socket_to_driver, "INIT");

  //send the nuclear coordinates
  natoms = natoms__;
  num_qm = num_qm__;
  num_mm = num_mm__;
  ntypes = ntypes__;
  init[0] = natoms;
  init[1] = num_qm;
  init[2] = num_mm;
  init[3] = ntypes;

  send_array(socket_to_driver, init, sizeof(init));
}



/* Send cell dimensions */
int QMMMClient::send_cell()
{
  double celldata[9];

  //label this message
  send_label(socket_to_driver, "CELL");

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

  send_array(socket_to_driver, celldata, sizeof(celldata));
}



/* Receive the forces from the socket */
int QMMMClient::receive_forces()
{
  receive_array(socket_to_driver, qm_force, (3*num_qm)*sizeof(double));
  receive_array(socket_to_driver, mm_force_all, (3*natoms)*sizeof(double));
}



/* Send initialization information through the socket */
int QMMMClient::send_qm_information(int sock, int qmmm_mode__, int verbose__, int steps__)
{
  int32_t init[3]; //uses int32_t to ensure that client and server both use the same sized int

  //label this message
  send_label(sock, "QM_INFO");

  //send the nuclear coordinates
  init[0] = qmmm_mode__;
  //init[1] = qm_comm;
  init[1] = verbose__;
  init[2] = steps__;

  send_array(sock, init, sizeof(init));
}
