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
  //qm_socket = initialize_socket(SOCKET_NAME);  
  //mm_socket = initialize_socket("./mm_main/driver.socket");
  //mm_subset_socket = initialize_socket("./mm_subset/driver.socket");

  driver_socket = initialize_socket("./mm_main/driver.socket");

  //mm_socket = initialize_client("./mm_main/driver.socket");
  //mm_subset_socket = initialize_client("./mm_subset/driver.socket");
}



/* Initialize a socket */
int initialize_socket(char *name)
{
  int ret;
  int sockfd;
  struct sockaddr_in serv_addr;
  int port;

  port = 8021;

  printf("In C code\n");

  //unlink the socket, in case the program previously exited unexpectedly
  //unlink(name);

  //create the socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    error("Could not create socket");
  }
  printf("Here is the socket: %i\n",sockfd);

  //create the socket address
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  //enable reuse of the socket
  ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
  if (ret < 0) {
    error("Could not reuse socket");
  }

  //bind the socket
  ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
  if (ret < 0) {
    error("Could not bind socket");
  }

  //start listening
  // the second argument is the backlog size
  ret = listen(sockfd, 20);
  if (ret < 0) {
    error("Could not listen");
  }

  return sockfd;
}



/* Initialize a socket */
int initialize_socket_unix(char *name)
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



int initialize_client(char *name)
{
  int ret;
  struct sockaddr_un server_address;
  int i;
  int sock;

  printf("In initialize_client\n");

  //create the socket
  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    error("Could not create socket");
  }
  printf("Here is the socket: %i\n",sock);

  printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
  printf("Location: %s\n",name);
  memset(&server_address, 0, sizeof(struct sockaddr_un));
  server_address.sun_family = AF_UNIX;
  strncpy(server_address.sun_path, name, sizeof(server_address.sun_path) - 1);
  printf("Path: %s\n",server_address.sun_path);


  ret = connect(sock, (const struct sockaddr *) &server_address, sizeof(struct sockaddr_un));
  if (ret < 0) {
    error("Could not connect");
  }
  return sock;


  ret = -10;
  do {
    //printf("   Trying %i\n",ret);
    ret = connect(sock, (const struct sockaddr *) &server_address, sizeof(struct sockaddr_un));
  }
  while ( ret < 0 );

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
  mass = ( double* )malloc( ntypes+1 );

  qm_force = ( double* )malloc( 3*num_qm );
  mm_force_all = ( double* )malloc( 3*natoms );
  mm_force_on_qm_atoms = ( double* )malloc( 3*num_qm );
}



int accept_mm_connection()
{  
  printf("Accepting connection from main MM process\n");

  //accept a connection
  mm_socket = accept(driver_socket, NULL, NULL);
  if (mm_socket < 0) {
    error("Could not accept connection");
  }
  printf("Received connection from LAMMPS master\n");

  //send information about the role of this process
  send_label(mm_socket, "MASTER");

  //receive QM information
  read_label(mm_socket, buffer);
  if( strcmp(buffer,"QM_INFO") == 0 ) {
    printf("Reading QM information from LAMMPS master\n");
    receive_qm_information(mm_socket);
  }
  else {
    error("Expected QM information");
  }


  return 0;
}



int accept_mm_subset_connection()
{
  printf("Accepting connection from subset MM process\n");

  //accept a connection
  mm_subset_socket = accept(driver_socket, NULL, NULL);
  if (mm_subset_socket < 0) {
    error("Could not accept connection");
  }
  printf("Received connection from LAMMPS slave\n");

  //send information about the role of this process
  send_label(mm_subset_socket, "SLAVE");

  return 0;
}



int accept_qm_connection()
{
  int id;
  int plen;
  
  printf("Accepting connection from QM process\n");

  //accept a connection
  qm_socket = accept(driver_socket, NULL, NULL);
  if (qm_socket < 0) {
    error("Could not accept connection");
  }
  printf("Received connection from Quantum ESPRESSO\n");

  //check the status of QE
  send_label(qm_socket, "STATUS      ");
  read_label(qm_socket, buffer);
  printf("Read label from QE: %s\n",buffer);

  //initialize QE
  send_label(qm_socket, ">RID        ");
  //writebuffer(qm_socket, 1, 1*sizeof(int));
  //writebuffer(qm_socket, 12, 1*sizeof(int));
  //writebuffer(qm_socket, "            ", 1*sizeof(char));
  id = 12355;
  send_array(qm_socket, &id, 1*sizeof(int));

  //check the status of QE
  send_label(qm_socket, "STATUS      ");
  read_label(qm_socket, buffer);
  printf("Read label from QE: %s\n",buffer);

  return 0;
}



int run_simulation()
{
  int iteration;
  int i;
  int max_iterations = 101;
  
  printf("Running the simulation\n");

  /*
  //accept a connection
  mm_socket = accept(driver_socket, NULL, NULL);
  if (mm_socket < 0) {
    error("Could not accept connection");
  }
  printf("Received connection from LAMMPS master\n");

  //accept a connection
  mm_subset_socket = accept(driver_socket, NULL, NULL);
  if (mm_subset_socket < 0) {
    error("Could not accept connection");
  }
  printf("Received connection from LAMMPS slave\n");

  //send information about the role of this process
  send_label(mm_socket, "MASTER");

  //send information about the role of this process
  send_label(mm_subset_socket, "SLAVE");
  */

  //read initialization information
  read_label(mm_socket, buffer);
  if( strcmp(buffer,"INIT") == 0 ) {
    printf("Reading initialization information from LAMMPS master");
    receive_initialization(mm_socket);
  }
  else {
    error("Initial message from LAMMPS is invalid");
  }

  printf("Number of atoms: %i",num_qm);

  //begin the main MD loop
  for (iteration=1; iteration <= max_iterations; iteration++) {

    printf("\nIteration %i",iteration);
    printf("\n");
    
    //read the label - should be the cell information
    read_label(mm_socket, buffer);
    printf("Read new label: %s\n",buffer);
    if( strcmp(buffer,"CELL") == 0 ) {
      receive_cell(mm_socket);
    }
    else {
      error("Unexpected label");
    }

    //read the label - should be the coordinate information
    read_label(mm_socket, buffer);
    printf("Read new label: %s\n",buffer);
    if( strcmp(buffer,"COORDS") == 0 ) {
      receive_coordinates(mm_socket);
    }
    else {
      error("Unexpected label");
    }

    //send the forces to the MM subset process
    send_array(mm_subset_socket, qm_coord, (3*num_qm)*sizeof(double));

    //wait for message response
    //read_label(mm_subset_socket, buffer);
    //printf("Read response label: %s\n",buffer);

    //zero the forces (SHOULD BE GETTING QM FORCES INSTEAD)
    for (i=0; i<3*num_qm; i++) { qm_force[i] = 0.0; }
    for (i=0; i<3*natoms; i++) { mm_force_all[i] = 0.0; }
    //for (i=0; i<3*num_qm; i++) { mm_force_on_qm_atoms[i] = 0.0; }
    receive_array(mm_subset_socket, mm_force_on_qm_atoms, (3*num_qm)*sizeof(double));

    //send the forces information
    send_forces(mm_socket);

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
int receive_initialization(int sock)
{
  int32_t init[4]; //uses int32_t to ensure that client and server both use the same sized int

  receive_array(sock, init, sizeof(init));

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



/* Receive cell dimensions */
int receive_cell(int sock)
{
  double celldata[9];

  //receive the cell data
  receive_array(sock, celldata, sizeof(celldata));

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
  send_array(qm_socket_in, mass, (ntypes+1)*sizeof(double));
}



/* Receive atomic positions through the socket */
int receive_coordinates(int sock)
{
  receive_array(sock, qm_coord, (3*num_qm)*sizeof(double));
  receive_array(sock, qm_charge, (num_qm)*sizeof(double));
  receive_array(sock, mm_charge_all, (natoms)*sizeof(double));
  receive_array(sock, mm_coord_all, (3*natoms)*sizeof(double));
  receive_array(sock, mm_mask_all, (natoms)*sizeof(int));
  receive_array(sock, type, (natoms)*sizeof(int));
  receive_array(sock, mass, (ntypes+1)*sizeof(double));
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



/* Send the forces through the socket */
int send_forces(int sock)
{
  send_array(sock, qm_force, (3*num_qm)*sizeof(double));
  send_array(sock, mm_force_all, (3*natoms)*sizeof(double));
  send_array(sock, mm_force_on_qm_atoms, (3*num_qm)*sizeof(double));
}



/* Receive initialization information through the socket */
int receive_qm_information(int sock)
{
  int32_t init[3]; //uses int32_t to ensure that client and server both use the same sized int

  receive_array(sock, init, sizeof(init));

  qm_mode = init[0];
  qm_verbose = init[1];
  qm_steps = init[2];

  printf("qm_mode:    %i\n",qm_mode);
  printf("qm_verbose: %i\n",qm_verbose);
  printf("qm_steps:   %i\n",qm_steps);
}
