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
  int reuse_value = 1;

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
  //ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
  ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_value, sizeof(int));
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
  printf("$$$ NATOMS: %i\n",natoms);
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
  mm_force_on_qm_atoms = ( double* )malloc( 3*num_qm * sizeof(double) );

  qm_ec_force = ( double* )malloc( 3*num_qm * sizeof(double) );
  aradii = ( double* )malloc( num_mm * sizeof(double) );
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

  //request a status update
  send_label(mm_socket, "STATUS");

  read_label(mm_socket, buffer);
  printf("Read label from LAMMPS main: %s\n",buffer);

  /*
  //send information about the role of this process
  send_label(mm_socket, "MASTER");

  //receive QM information
  printf("$$$\n");
  read_label(mm_socket, buffer);
  if( strcmp(buffer,"QM_INFO") == 0 ) {
    printf("Reading QM information from LAMMPS master\n");
    receive_qm_information(mm_socket);
  }
  else {
    error("Expected QM information");
  }
  */

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
  printf("Received connection from LAMMPS subset\n");

  //request a status update
  send_label(mm_subset_socket, "STATUS");

  read_label(mm_subset_socket, buffer);
  printf("Read label from LAMMPS subset: %s\n",buffer);

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
  send_label(qm_socket, "STATUS");
  read_label(qm_socket, buffer);
  printf("Read label from QE: %s\n",buffer);

  //initialize the QE replica ID
  send_label(qm_socket, ">RID        ");
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
  int max_iterations = 11;
  double qm_energy;
  
  printf("Running the simulation\n");

  //return 0;
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
  /*
  read_label(mm_socket, buffer);
  if( strcmp(buffer,"INIT") == 0 ) {
    printf("Reading initialization information from LAMMPS master\n");
    receive_initialization(mm_socket);
  }
  else {
    error("Initial message from LAMMPS is invalid");
  }
  */

  //receive the number of MM atoms from the MM main process
  send_label(mm_subset_socket, "<NAT");
  receive_array(mm_subset_socket, &num_mm, 1*sizeof(int));
  natoms = num_mm;

  //receive the number of MM atom types from the MM subset process
  send_label(mm_subset_socket, "<NTYPES");
  receive_array(mm_subset_socket, &ntypes, 1*sizeof(int));

  //receive the number of MM atoms from the MM subset process
  send_label(mm_subset_socket, "<NAT");
  receive_array(mm_subset_socket, &num_qm, 1*sizeof(int));

  //initialize the arrays for atoms, forces, etc.
  initialize_arrays();

  //send the QMMM mode to QE
  send_label(qm_socket, ">QMMM_MODE");
  send_array(qm_socket, &qm_mode, 1*sizeof(int));

  printf("Number of atoms: %i",num_qm);

  //begin the main MD loop
  for (iteration=1; iteration <= max_iterations; iteration++) {

    printf("\nIteration %i",iteration);
    printf("\n");

    //receive the cell information from the MM main process
    send_label(mm_socket, "<CELL");
    receive_cell(mm_socket);
    
    /*
    //read the label - should be the cell information
    read_label(mm_socket, buffer);
    printf("Read new label: %s\n",buffer);
    if( strcmp(buffer,"CELL") == 0 ) {
      receive_cell(mm_socket);
    }
    else {
      error("Unexpected label");
    }
    */

    //send the cell information to QE
    //send_label(qm_socket, ">CELL");
    //send_cell(qm_socket);

    //send the number of qm atoms to QE
    send_label(qm_socket, ">NAT");
    send_array(qm_socket, &num_qm, 1*sizeof(int));

    //send the number of mm atoms to QE
    send_label(qm_socket, ">MM_NAT");
    send_array(qm_socket, &num_mm, 1*sizeof(int));

    //send the number of atom types to QE
    send_label(qm_socket, ">NTYPES");
    send_array(qm_socket, &ntypes, 1*sizeof(int));

    //send the MM cell information to QE
    send_label(qm_socket, ">MM_CELL");
    send_cell(qm_socket);

    //receive the MM coordinates
    send_label(mm_socket, "<COORD");
    receive_array(mm_socket, mm_coord_all, (3*num_mm)*sizeof(double));

    return 0;

    //read the label - should be the coordinate information
    read_label(mm_socket, buffer);
    printf("Read new label: %s\n",buffer);
    if( strcmp(buffer,"COORDS") == 0 ) {
      receive_coordinates(mm_socket);
    }
    else {
      error("Unexpected label");
    }

    //send the MM mask, which describes which atoms are part of the QM subsystem
    send_label(qm_socket, ">MM_MASK");
    send_array(qm_socket, mm_mask_all, num_mm*sizeof(int));

    //send the MM coordinates to QE
    send_label(qm_socket, ">MM_COORD");
    send_array(qm_socket, mm_coord_all, (3*num_mm)*sizeof(double));

    //send the MM coordinates to QE
    send_label(qm_socket, ">MM_TYPE");
    send_array(qm_socket, type, num_mm*sizeof(int));

    //send the MM charges to QE
    send_label(qm_socket, ">MM_CHARGE");
    send_array(qm_socket, mm_charge_all, num_mm*sizeof(double));

    //send the MM masses to QE
    send_label(qm_socket, ">MM_MASS");
    send_array(qm_socket, mass, (ntypes+1)*sizeof(double));

    //get the aradii
    ec_fill_radii(aradii,&num_mm,mass,type,&ntypes);
    /*
    for (i=0; i<num_mm; i++) {
      printf("aradii: %i %f\n",i+1,aradii[i]);
    }
    */

    //send the coordinates to the QM process
    send_label(qm_socket, ">COORD");
    printf("   O: %f %f %f\n",qm_coord[0],qm_coord[1],qm_coord[2]);
    printf("   H: %f %f %f\n",qm_coord[3],qm_coord[4],qm_coord[5]);
    printf("   H: %f %f %f\n",qm_coord[6],qm_coord[7],qm_coord[8]);
    send_array(qm_socket, qm_coord, (3*num_qm)*sizeof(double));

    //have the QM process recenter the coodinates (THE DRIVER SHOULD PROBABLY DO THIS INSTEAD)
    //CURRENTLY, >MM_MASS DOES THIS
    send_label(qm_socket, "RECENTER");

    //have the QM process run an SCF calculation
    send_label(qm_socket, "SCF");

    //get the QM energy
    send_label(qm_socket, "<ENERGY");
    receive_array(qm_socket, &qm_energy, 1*sizeof(double));
    printf("Read energy from QE: %f\n",qm_energy);

    //get the QM forces
    send_label(qm_socket, "<FORCE");
    receive_array(qm_socket, qm_force, (3*num_qm)*sizeof(double));
    printf("QM Forces:\n");
    for (i=0; i<num_qm; i++) {
      printf("   %i %f %f %f\n",i+1,qm_force[3*i+0],qm_force[3*i+1],qm_force[3*i+2]);
    }

    if ( qm_mode == 2 ) {
      
      //get the EC forces on the QM atoms
      send_label(qm_socket, "<EC_FORCE");
      receive_array(qm_socket, qm_ec_force, (3*num_qm)*sizeof(double));
      
      for (i=0; i<3*num_qm; i++) {
      	qm_force[i] += qm_ec_force[i];
      }

      //get the EC forces on the MM atoms
      printf("Requesting MM_FORCE from QE: %i\n",qm_mode);
      send_label(qm_socket, "<MM_FORCE");
      receive_array(qm_socket, mm_force_all, (3*num_mm)*sizeof(double));
    }


    //send the number of atoms to the MM subset process
    //send_label(mm_subset_socket, ">NAT");
    //send_array(mm_subset_socket, &num_qm, 1*sizeof(int));

    //send the coordinates to the MM subset process
    send_label(mm_subset_socket, ">COORD");
    send_array(mm_subset_socket, qm_coord, (3*num_qm)*sizeof(double));

    //have the MM subset process send the forces
    send_label(mm_subset_socket, "<FORCES");
    receive_array(mm_subset_socket, mm_force_on_qm_atoms, (3*num_qm)*sizeof(double));

    //send the coordinates to the MM subset process
    //send_array(mm_subset_socket, qm_coord, (3*num_qm)*sizeof(double));

    //wait for message response
    //read_label(mm_subset_socket, buffer);
    //printf("Read response label: %s\n",buffer);

    //zero the forces (SHOULD BE GETTING QM FORCES INSTEAD)
    //for (i=0; i<3*num_qm; i++) { qm_force[i] = 0.0; }
    //for (i=0; i<3*natoms; i++) { mm_force_all[i] = 0.0; }
    //for (i=0; i<3*num_qm; i++) { mm_force_on_qm_atoms[i] = 0.0; }
    //receive_array(mm_subset_socket, mm_force_on_qm_atoms, (3*num_qm)*sizeof(double));

    //send the forces information
    send_forces(mm_socket);

  }

  //instruct QE to exit
  send_label(qm_socket, "EXIT        ");

  printf("Completed QM/MM simulation \n");
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
  send_cell(qm_socket_in);

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
int send_cell(int sock)
{
  double celldata[9];

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

  send_array(sock, celldata, sizeof(celldata));
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

  /*
  printf("boxlo0: %f\n",celldata[0]);
  printf("boxlo1: %f\n",celldata[1]);
  printf("boxlo2: %f\n",celldata[2]);
  printf("boxhi0: %f\n",celldata[3]);
  printf("boxhi1: %f\n",celldata[4]);
  printf("boxhi2: %f\n",celldata[5]);
  printf("cellxy: %f\n",celldata[6]);
  printf("cellxz: %f\n",celldata[7]);
  printf("cellyz: %f\n",celldata[8]);
  */
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
  int i;


  receive_array(sock, qm_coord, (3*num_qm)*sizeof(double));
  receive_array(sock, qm_charge, (num_qm)*sizeof(double));
  receive_array(sock, mm_charge_all, (natoms)*sizeof(double));
  receive_array(sock, mm_coord_all, (3*natoms)*sizeof(double));
  receive_array(sock, mm_mask_all, (natoms)*sizeof(int));
  receive_array(sock, type, (natoms)*sizeof(int));
  receive_array(sock, mass, (ntypes+1)*sizeof(double));

  //convert coordinates to a.u.
  for (i=0; i < 3*num_qm; i++) {
    qm_coord[i] = qm_coord[i]*angstrom_to_bohr;
  }
  for (i=0; i < 3*natoms; i++) {
    mm_coord_all[i] = mm_coord_all[i]*angstrom_to_bohr;
  }

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
  printf("Sending Forces:\n");
  for (int i=0; i<num_qm; i++) {
    printf("   %i %f %f %f\n",i+1,qm_force[3*i+0],qm_force[3*i+1],qm_force[3*i+2]);
  }
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
