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
  //<<<
  int delay_value = 1;
  //>>>

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

  //<<<
  ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &delay_value, sizeof(int));
  if (ret < 0) {
    error("Could not turn off TCP delay");
  }
  //>>>

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
