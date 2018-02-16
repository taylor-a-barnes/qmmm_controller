/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>
#include "driver.h"
#include "atom.h"
#include "domain.h"
#include "update.h"
#include "force.h"
#include "integrate.h"
#include "modify.h"
#include "output.h"
#include "finish.h"
#include "input.h"
#include "timer.h"
#include "error.h"
#include "comm.h"
#include "irregular.h"

#include "verlet.h"
#include "neighbor.h"

using namespace LAMMPS_NS;

// socket interface
#ifndef _WIN32
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#endif

#define MSGLEN 12
#define MAXLINE 2048

/*<<<<<<

// asks for evaluation of PE at first step
modify->compute[modify->find_compute("thermo_pe")]->invoked_scalar = -1;
modify->addstep_compute_all(update->ntimestep + 1);

kspace_flag = (force->kspace) ? 1 : 0;

// when changing nuclear coordinates:

// makes sure that neighbor lists are re-built at each step (cannot make assumptions when cycling over beads!)
neighbor->delay = 0;
neighbor->every = 1;
/*>>>>>>



/** hash table top level data structure */
typedef struct taginthash_t {
  struct taginthash_node_t **bucket;     /* array of hash nodes */
  tagint size;                           /* size of the array */
  tagint entries;                        /* number of entries in table */
  tagint downshift;                      /* shift cound, used in hash function */
  tagint mask;                           /* used to select bits for hashing */
} taginthash_t;

#define HASH_FAIL  -1
#define HASH_LIMIT  0.5




/* Utility functions to simplify the interface with POSIX sockets */

static void open_socket(int &sockfd, int inet, int port, char* host,
                        Error *error)
/* Opens a socket.

   Args:
   sockfd: The id of the socket that will be created.
   inet: An integer that determines whether the socket will be an inet or unix
   domain socket. Gives unix if 0, inet otherwise.
   port: The port number for the socket to be created. Low numbers are often
   reserved for important channels, so use of numbers of 4 or more digits is
   recommended.
   host: The name of the host server.
   error: pointer to a LAMMPS Error object
*/
{
  int ai_err;

#ifdef _WIN32
  error->one(FLERR,"i-PI socket implementation requires UNIX environment");
#else
  if (inet>0) {  // creates an internet socket

    /*
    // fetches information on the host
    struct addrinfo hints, *res;
    char service[256];

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;

    sprintf(service,"%d",port); // convert the port number to a string
    ai_err = getaddrinfo(host, service, &hints, &res);
    if (ai_err!=0)
      error->one(FLERR,"Error fetching host data. Wrong host name?");

    // creates socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0)
      error->one(FLERR,"Error opening socket");

    // makes connection
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0)
      error->one(FLERR,"Error opening INET socket: wrong port or server unreachable");
    freeaddrinfo(res);
    */

  int ret;
  struct sockaddr_in driver_address;
  int i;
  //char *serv_host = "localhost";
  //string readline;
  int port;
  struct hostent *host_ptr;
  //ifstream hostfile("../hostname");
  FILE *hostfile;
  char buff[255];

  port = 8021;

  printf("In C code TESTING\n");



  hostfile = fopen("../hostname","r");
  fgets(buff, 255, (FILE*)hostfile);

  printf("Driver hostname: %s\n",buff);

  int hlen = 10;
  /*
  for (i=0; i < hlen; i++) {
    serv_host[i] = buff[i];
  }
  */
  hlen = strlen(buff);
  printf("hostname length: %i\n",hlen);

  char serv_host[hlen];
  for (i=0; i < hlen; i++) {
    serv_host[i] = buff[i];
  }
  serv_host[hlen-1] = '\0';

  /*
  if (hostfile.is_open()) {
    //read the first line
    getline(hostfile,readline);
  }
  printf("%%% HOSTNAME: %s\n",readline);
  int hlen = readline.length();
  char serv_host[hlen+1];
  strcpy(serv_host, readline.c_str());
  */
  printf("Driver hostname: %s\n",serv_host);
  printf("Driver hostname: %s\n",serv_host);





  //get the address of the host
  host_ptr = gethostbyname(serv_host);
  if (host_ptr == NULL) {
    error->one(FLERR,"Error in gethostbyname");
  }
  if (host_ptr->h_addrtype != AF_INET) {
    error->one(FLERR,"Unkown address type");
  }

  bzero((char *) &driver_address, sizeof(driver_address));
  driver_address.sin_family = AF_INET;
  driver_address.sin_addr.s_addr = 
    ((struct in_addr *)host_ptr->h_addr_list[0])->s_addr;
  driver_address.sin_port = htons(port);

  //create the socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    error->one(FLERR,"Could not create socket");
  }
  printf("Here is the socket: %i\n",sockfd);

  //connect to the driver
  ret = connect(sockfd, (const struct sockaddr *) &driver_address, sizeof(struct sockaddr_un));
  if (ret < 0) {
    error->one(FLERR,"Could not connect to the driver");
  }



  } else {  // creates a unix socket
    struct sockaddr_un serv_addr;

    // fills up details of the socket addres
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, "/tmp/ipi_");
    strcpy(serv_addr.sun_path+9, host);

    // creates the socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

    // connects
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
      error->one(FLERR,"Error opening UNIX socket: server may not be running "
                 "or the path to the socket unavailable");
  }
#endif
}

static void writebuffer(int sockfd, const char *data, int len, Error* error)
/* Writes to a socket.

   Args:
   sockfd: The id of the socket that will be written to.
   data: The data to be written to the socket.
   len: The length of the data in bytes.
*/
{
  int n;

  n = write(sockfd,data,len);
  if (n < 0)
    error->one(FLERR,"Error writing to socket: broken connection");
}



static void readbuffer(int sockfd, char *data, int len, Error* error)
/* Reads from a socket.

   Args:
   sockfd: The id of the socket that will be read from.
   data: The storage array for data read from the socket.
   len: The length of the data in bytes.
*/
{
  int n, nr;

  n = nr = read(sockfd,data,len);

  while (nr>0 && n<len ) {
    nr=read(sockfd,&data[n],len-n);
    n+=nr;
  }

  if (n == 0)
    error->one(FLERR,"Error reading from socket: broken connection");
}


/* ---------------------------------------------------------------------- */

Driver::Driver(LAMMPS *lmp) : Pointers(lmp) {}

/* ---------------------------------------------------------------------- */

void Driver::command(int narg, char **arg)
{
  /* format for driver command:
   * driver hostname port [unix]
   */
  if (narg < 2) error->all(FLERR,"Illegal driver command");

  if (atom->tag_enable == 0)
    error->all(FLERR,"Cannot use driver command without atom IDs");

  if (atom->tag_consecutive() == 0)
    error->all(FLERR,"Driver command requires consecutive atom IDs");

  // obtain host information from the command arguments
  host = strdup(arg[0]);
  port = force->inumeric(FLERR,arg[1]);
  inet   = ((narg > 2) && (strcmp(arg[2],"unix") == 0) ) ? 0 : 1;

  master = (comm->me==0) ? 1 : 0;

  // open the socket
  if (master) {
    open_socket(driver_socket, inet, port, host, error);
  } else driver_socket=0;

  // create instance of Irregular class
  irregular = new Irregular(lmp);

  /* ----------------------------------------------------------------- */
  // Answer commands from the driver
  /* ----------------------------------------------------------------- */
  char header[MSGLEN+1];

  while (true) {

    if (master) { 
      // read the next command from the driver
      readbuffer(driver_socket, header, MSGLEN, error);
      header[MSGLEN]=0;
    }
    // broadcast the command to the other tasks
    MPI_Bcast(header,12,MPI_CHAR,0,world);
    
    if (screen)
      fprintf(screen,"Read label from driver: %s\n",header);
    if (logfile)
      fprintf(logfile,"Read label from driver: %s\n",header);

    if (strcmp(header,"STATUS      ") == 0 ) {
      if (master) {
	writebuffer(driver_socket,"READY       ",MSGLEN, error);
      }
    }
    else if (strcmp(header,">NAT        ") == 0 ) {
      if (master) {
	readbuffer(driver_socket, (char*) &atom->natoms, 4, error);
      }
      MPI_Bcast(&atom->natoms,1,MPI_INTEGER,0,world);
    }
    else if (strcmp(header,"<NAT        ") == 0 ) {
      if (master) {
	writebuffer(driver_socket, (char*) &atom->natoms, 4, error);
      }
    }
    else if (strcmp(header,"<NTYPES     ") == 0 ) {
      if (master) {
	writebuffer(driver_socket, (char*) &atom->ntypes, 4, error);
      }
    }
    else if (strcmp(header,">COORD      ") == 0 ) {
      // receive the coordinate information
      read_coordinates(error);
    }
    else if (strcmp(header,"<FORCES     ") == 0 ) {
      write_forces(error);
    }
    else {
      error->all(FLERR,"Unknown command from driver");
    }

  }

}


void Driver::read_coordinates(Error* error)
/* Writes to a socket.

   Args:
   sockfd: The id of the socket that will be written to.
   data: The data to be written to the socket.
   len: The length of the data in bytes.
*/
{
  double posconv;
  posconv=0.52917721*force->angstrom;

  // create a buffer to hold the coordinates
  double *buffer;
  buffer = new double[3*atom->natoms];

  readbuffer(driver_socket, (char*) buffer, 8*(3*atom->natoms), error);
  MPI_Bcast(buffer,3*atom->natoms,MPI_DOUBLE,0,world);

  // pick local atoms from the buffer
  double **x = atom->x;
  int *mask = atom->mask;
  int nlocal = atom->nlocal;
  //if (igroup == atom->firstgroup) nlocal = atom->nfirst;
  for (int i = 0; i < nlocal; i++) {
    //if (mask[i] & groupbit) {
      x[i][0]=buffer[3*(atom->tag[i]-1)+0]*posconv;
      x[i][1]=buffer[3*(atom->tag[i]-1)+1]*posconv;
      x[i][2]=buffer[3*(atom->tag[i]-1)+2]*posconv;
      //}
  }

  // ensure atoms are in current box & update box via shrink-wrap
  // has to be be done before invoking Irregular::migrate_atoms() 
  //   since it requires atoms be inside simulation box
  if (domain->triclinic) domain->x2lamda(atom->nlocal);
  domain->pbc();
  domain->reset_box();
  if (domain->triclinic) domain->lamda2x(atom->nlocal);

  // move atoms to new processors via irregular()
  // only needed if migrate_check() says an atom moves to far
  if (domain->triclinic) domain->x2lamda(atom->nlocal);
  if (irregular->migrate_check()) irregular->migrate_atoms();
  if (domain->triclinic) domain->lamda2x(atom->nlocal);
}


void Driver::write_forces(Error* error)
/* Writes to a socket.

   Args:
   sockfd: The id of the socket that will be written to.
   data: The data to be written to the socket.
   len: The length of the data in bytes.
*/
{
  double potconv, posconv, forceconv;
  potconv=3.1668152e-06/force->boltz;
  posconv=0.52917721*force->angstrom;
  forceconv=potconv*posconv;

  double *forces;
  double *forces_reduced;

  forces = new double[3*atom->natoms];
  forces_reduced = new double[3*atom->natoms];

  // calculate the forces
  update->whichflag = 1;
  //timer->init_timeout();
  update->nsteps = 1;
  //update->firststep = update->ntimestep;
  //update->laststep = update->ntimestep + update->nsteps;
  //update->beginstep = update->firststep;
  //update->endstep = update->laststep;
  lmp->init();
  update->integrate->setup();
  

  // pick local atoms from the buffer
  double **f = atom->f;
  int *mask = atom->mask;
  int nlocal = atom->nlocal;
  //if (igroup == atom->firstgroup) nlocal = atom->nfirst;
  for (int i = 0; i < nlocal; i++) {
    //if (mask[i] & groupbit) {

    if (screen)
      fprintf(screen,"f: %i %f %f %f\n",i+1,f[i][0],f[i][1],f[i][2]);
    if (logfile)
      fprintf(logfile,"f: %i %f %f %f\n",i+1,f[i][0],f[i][1],f[i][2]);

      forces[3*(atom->tag[i]-1)+0] = f[i][0]*forceconv;
      forces[3*(atom->tag[i]-1)+1] = f[i][1]*forceconv;
      forces[3*(atom->tag[i]-1)+2] = f[i][2]*forceconv;

    //}
  }

  MPI_Reduce(forces, forces_reduced, 3*atom->natoms, MPI_DOUBLE, MPI_SUM, 0, world);

  if (master) { 
    writebuffer(driver_socket, (char*) forces_reduced, 8*(3*atom->natoms), error);
  }
}


