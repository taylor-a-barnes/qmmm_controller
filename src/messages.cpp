/* Server code for the QM/MM driver */
#include "messages.h"




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




/* Initialize everything necessary for the driver to act as a server */
int initialize_driver_socket()
{
  driver_socket = initialize_socket();

  return 0;
}



/* Launch a server by first creating a fork */
int launch_server(const char *line)
{
  //create a fork
  int pid = fork();

  if (pid == 0) {
    //child process
    system(line);
    exit(0);
  }

  return 0;
}



/* Initialize a socket */
int initialize_socket()
{
  int ret;
  int sockfd;
  struct sockaddr_in serv_addr;
  int port;
  int reuse_value = 1;

  port = 8021;

  //create the socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    error("Could not create socket");
  }

  //create the socket address
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  //enable reuse of the socket
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

  //unlink the socket, in case the program previously exited unexpectedly
  unlink(name);

  //create the socket
  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    error("Could not create socket");
  }

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
  int i;

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

  mm_force = ( double* )malloc( 3*natoms * sizeof(double) );

  for (i=0; i < natoms; i++) {
    mm_mask_all[i] = -1;
  }

  return 0;
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
  int j;
  int max_iterations = 11;
  double qm_energy;
  
  printf("Running the simulation\n");

  //NOTE: SHOULD READ THESE FROM QMMM.INP!!!
  qm_mode = 2;
  qm_verbose = 1;
  qm_steps = 11;
  qm_start = 25; //this is the first atom that is part of the QM system
  qm_end = 27; //this is the last atom that is part of the QM system

  printf("qm_mode:    %i\n",qm_mode);
  printf("qm_verbose: %i\n",qm_verbose);
  printf("qm_steps:   %i\n",qm_steps);

  //receive the number of MM atoms from the MM main process
  send_label(mm_socket, "<NAT");
  receive_array(mm_socket, &num_mm, 1*sizeof(int));
  natoms = num_mm;

  //receive the number of MM atom types from the MM main process
  send_label(mm_socket, "<NTYPES");
  receive_array(mm_socket, &ntypes, 1*sizeof(int));

  //receive the number of QM atoms from the MM subset process
  send_label(mm_subset_socket, "<NAT");
  receive_array(mm_subset_socket, &num_qm, 1*sizeof(int));

  //initialize the arrays for atoms, forces, etc.
  initialize_arrays();

  //receive the MM types
  send_label(mm_socket, "<TYPES");
  receive_array(mm_socket, type, (natoms)*sizeof(int));

  //set the mm_mask
  // this is -1 for non-QM atoms, and 1 for QM atoms
  for (i=qm_start-1; i <= qm_end-1; i++) {
    mm_mask_all[i] = type[i];
  }
  printf("Mask:\n");
  for (i=0; i<natoms; i++) {
    printf("   %i %i\n",i+1,mm_mask_all[i]);
  }

  //send the QMMM mode to QE
  send_label(qm_socket, ">QMMM_MODE");
  send_array(qm_socket, &qm_mode, 1*sizeof(int));

  //begin the main MD loop
  for (iteration=1; iteration <= max_iterations; iteration++) {

    printf("\nIteration %i\n",iteration);

    //receive the cell information from the MM main process
    send_label(mm_socket, "<CELL");
    receive_cell(mm_socket);
    
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

    //set the QM coordinates
    j = 0;
    for (i=0; i < natoms; i++) {
      if (mm_mask_all[i] != -1) {
	qm_coord[3*j+0] = mm_coord_all[3*i+0];
	qm_coord[3*j+1] = mm_coord_all[3*i+1];
	qm_coord[3*j+2] = mm_coord_all[3*i+2];
	j++;
      }
    }
    printf("Coordinates:\n");
    for (i=0; i<num_qm; i++) {
      printf("   %i %f %f %f\n",i,qm_coord[3*i+0],qm_coord[3*i+1],qm_coord[3*i+2]);
    }

    //receive the MM charges
    send_label(mm_socket, "<CHARGE");
    receive_array(mm_socket, mm_charge_all, (num_mm)*sizeof(double));

    printf("Charges:\n");
    for (i=0; i<natoms; i++) {
      printf("   %i %f\n",i,mm_charge_all[i]);
    }

    //set the QM charges
    j = 0;
    for (i=0; i < natoms; i++) {
      if (mm_mask_all[i] != -1) {
	qm_charge[j] = mm_charge_all[i];
	printf("   SETTING   %i %f\n",i,mm_charge_all[i]);
	j++;
      }
    }
    printf("Charges:\n");
    for (i=0; i<num_qm; i++) {
      printf("   %i %f\n",i,qm_charge[i]);
    }

    //receive the MM types
    send_label(mm_socket, "<MASS");
    receive_array(mm_socket, mass, (ntypes+1)*sizeof(double));

    //send the MM mask, which describes which atoms are part of the QM subsystem
    send_label(qm_socket, ">MM_MASK");
    send_array(qm_socket, mm_mask_all, num_mm*sizeof(int));

    //send the MM coordinates to QE
    send_label(qm_socket, ">MM_TYPE");
    send_array(qm_socket, type, num_mm*sizeof(int));

    //send the MM charges to QE
    send_label(qm_socket, ">MM_CHARGE");
    send_array(qm_socket, mm_charge_all, num_mm*sizeof(double));

    //send the MM masses to QE
    send_label(qm_socket, ">MM_MASS");
    send_array(qm_socket, mass, (ntypes+1)*sizeof(double));

    //send the MM coordinates to QE
    send_label(qm_socket, ">MM_COORD");
    send_array(qm_socket, mm_coord_all, (3*num_mm)*sizeof(double));

    printf("MM coordinates:\n");
    for (i=0; i<num_mm; i++) {
      printf("   %i %f %f %f\n",i+1,mm_coord_all[3*i+0],mm_coord_all[3*i+1],mm_coord_all[3*i+2]);
    }

    //get the aradii
    ec_fill_radii(aradii,&num_mm,mass,type,&ntypes);

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

      printf("EC Forces:\n");
      for (i=0; i<num_mm; i++) {
	printf("   %i %f %f %f\n",i+1,mm_force_all[3*i+0],mm_force_all[3*i+1],mm_force_all[3*i+2]);
      }
    }

    //send the coordinates to the MM subset process
    send_label(mm_subset_socket, ">COORD");
    send_array(mm_subset_socket, qm_coord, (3*num_qm)*sizeof(double));

    //have the MM subset process send the forces
    send_label(mm_subset_socket, "<FORCES");
    receive_array(mm_subset_socket, mm_force_on_qm_atoms, (3*num_qm)*sizeof(double));

    //zero the MM forces (for +FORCE command)
    for (i=0; i < 3*natoms; i++) {
      mm_force[i] = 0.0;
    }

    //add the QM forces to the MM forces
    j = 0;
    for (i=0; i < natoms; i++) {
      if (mm_mask_all[i] != -1) {
	mm_force[3*i+0] += qm_force[3*j+0] - mm_force_on_qm_atoms[3*j+0];
	mm_force[3*i+1] += qm_force[3*j+1] - mm_force_on_qm_atoms[3*j+1];
	mm_force[3*i+2] += qm_force[3*j+2] - mm_force_on_qm_atoms[3*j+2];
	j++;
      }
      else {
	mm_force[3*i+0] += mm_force_all[3*i+0];
	mm_force[3*i+1] += mm_force_all[3*i+1];
	mm_force[3*i+2] += mm_force_all[3*i+2];
      }
    }

    //send the updated forces to the MM main process
    printf("qm_force:\n");
    for (int i=0; i<num_qm; i++) {
      printf("   %i %f %f %f\n",i+1,qm_force[3*i+0],qm_force[3*i+1],qm_force[3*i+2]);
    }
    printf("mm_force_on_qm_atoms:\n");
    for (int i=0; i<num_qm; i++) {
      printf("   %i %f %f %f\n",i+1,mm_force_on_qm_atoms[3*i+0],mm_force_on_qm_atoms[3*i+1],mm_force_on_qm_atoms[3*i+2]);
    }
    printf("Sending MM Forces:\n");
    for (int i=0; i<num_mm; i++) {
      printf("   %i %f %f %f\n",i+1,mm_force[3*i+0],mm_force[3*i+1],mm_force[3*i+2]);
    }
    send_label(mm_socket, "+FORCES");
    send_array(mm_socket, mm_force, (3*num_mm)*sizeof(double));

    //have the MM main process iterate
    if (iteration == 1) {
      send_label(mm_socket, "MD_INIT");
    }
    send_label(mm_socket, "TIMESTEP");

  }

  //instruct QE to exit
  send_label(qm_socket, "EXIT        ");

  printf("Completed QM/MM simulation \n");

  return 0;
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
  
  return 0;
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

  return 0;
}
