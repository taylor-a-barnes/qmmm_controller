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

  return;

  if (domain->box_exist == 0)
    error->all(FLERR,"Run command before simulation box is defined");

  // ignore run command, if walltime limit was already reached

  if (timer->is_timeout()) return;

  bigint nsteps_input = force->bnumeric(FLERR,arg[0]);

  // parse optional args

  int uptoflag = 0;
  int startflag = 0;
  int stopflag = 0;
  bigint start,stop;
  int preflag = 1;
  int postflag = 1;
  int nevery = 0;
  int ncommands = 0;
  int first,last;

  int iarg = 1;
  while (iarg < narg) {
    if (strcmp(arg[iarg],"upto") == 0) {
      if (iarg+1 > narg) error->all(FLERR,"Illegal run command");
      uptoflag = 1;
      iarg += 1;
    } else if (strcmp(arg[iarg],"start") == 0) {
      if (iarg+2 > narg) error->all(FLERR,"Illegal run command");
      startflag = 1;
      start = force->bnumeric(FLERR,arg[iarg+1]);
      iarg += 2;
    } else if (strcmp(arg[iarg],"stop") == 0) {
      if (iarg+2 > narg) error->all(FLERR,"Illegal run command");
      stopflag = 1;
      stop = force->bnumeric(FLERR,arg[iarg+1]);
      iarg += 2;
    } else if (strcmp(arg[iarg],"pre") == 0) {
      if (iarg+2 > narg) error->all(FLERR,"Illegal run command");
      if (strcmp(arg[iarg+1],"no") == 0) preflag = 0;
      else if (strcmp(arg[iarg+1],"yes") == 0) preflag = 1;
      else error->all(FLERR,"Illegal run command");
      iarg += 2;
    } else if (strcmp(arg[iarg],"post") == 0) {
      if (iarg+2 > narg) error->all(FLERR,"Illegal run command");
      if (strcmp(arg[iarg+1],"no") == 0) postflag = 0;
      else if (strcmp(arg[iarg+1],"yes") == 0) postflag = 1;
      else error->all(FLERR,"Illegal run command");
      iarg += 2;

      // all remaining args are commands
      // first,last = arg index of first/last commands
      // set ncommands = 0 if single command and it is NULL

    } else if (strcmp(arg[iarg],"every") == 0) {
      if (iarg+3 > narg) error->all(FLERR,"Illegal run command");
      nevery = force->inumeric(FLERR,arg[iarg+1]);
      if (nevery <= 0) error->all(FLERR,"Illegal run command");
      first = iarg+2;
      last = narg-1;
      ncommands = last-first + 1;
      if (ncommands == 1 && strcmp(arg[first],"NULL") == 0) ncommands = 0;
      iarg = narg;
    } else error->all(FLERR,"Illegal run command");
  }

  // set nsteps as integer, using upto value if specified

  int nsteps;
  if (!uptoflag) {
    if (nsteps_input < 0 || nsteps_input > MAXSMALLINT)
      error->all(FLERR,"Invalid run command N value");
    nsteps = static_cast<int> (nsteps_input);
  } else {
    bigint delta = nsteps_input - update->ntimestep;
    if (delta < 0 || delta > MAXSMALLINT)
      error->all(FLERR,"Invalid run command upto value");
    nsteps = static_cast<int> (delta);
  }

  // error check

  if (startflag) {
    if (start < 0)
      error->all(FLERR,"Invalid run command start/stop value");
    if (start > update->ntimestep)
      error->all(FLERR,"Run command start value is after start of run");
  }
  if (stopflag) {
    if (stop < 0)
      error->all(FLERR,"Invalid run command start/stop value");
    if (stop < update->ntimestep + nsteps)
      error->all(FLERR,"Run command stop value is before end of run");
  }

  if (!preflag && strstr(update->integrate_style,"respa"))
    error->all(FLERR,"Run flag 'pre no' not compatible with r-RESPA");

  // if nevery, make copies of arg strings that are commands
  // required because re-parsing commands via input->one() will wipe out args

  char **commands = NULL;
  if (nevery && ncommands > 0) {
    commands = new char*[ncommands];
    ncommands = 0;
    for (int i = first; i <= last; i++) {
      int n = strlen(arg[i]) + 1;
      commands[ncommands] = new char[n];
      strcpy(commands[ncommands],arg[i]);
      ncommands++;
    }
  }

  // perform a single run
  // use start/stop to set begin/end step
  // if pre or 1st run, do System init/setup,
  //   else just init timer and setup output
  // if post, do full Finish, else just print time

  update->whichflag = 1;
  timer->init_timeout();

  if (nevery == 0) {
    update->nsteps = nsteps;
    update->firststep = update->ntimestep;
    update->laststep = update->ntimestep + nsteps;
    if (update->laststep < 0 || update->laststep < update->firststep)
      error->all(FLERR,"Too many timesteps");

    if (startflag) update->beginstep = start;
    else update->beginstep = update->firststep;
    if (stopflag) update->endstep = stop;
    else update->endstep = update->laststep;

    if (preflag || update->first_update == 0) {
      lmp->init();
      update->integrate->setup();
    } else output->setup(0);

    timer->init();
    timer->barrier_start();
    update->integrate->run(nsteps);
    timer->barrier_stop();

    update->integrate->cleanup();

    Finish finish(lmp);
    finish.end(postflag);

  // perform multiple runs optionally interleaved with invocation command(s)
  // use start/stop to set begin/end step
  // if pre or 1st iteration of multiple runs, do System init/setup,
  //   else just init timer and setup output
  // if post or last iteration, do full Finish, else just print time

  } else {
    int iter = 0;
    int nleft = nsteps;
    while (nleft > 0 || iter == 0) {
      if (timer->is_timeout()) break;
      timer->init_timeout();

      nsteps = MIN(nleft,nevery);

      update->nsteps = nsteps;
      update->firststep = update->ntimestep;
      update->laststep = update->ntimestep + nsteps;
      if (update->laststep < 0 || update->laststep < update->firststep)
        error->all(FLERR,"Too many timesteps");

      if (startflag) update->beginstep = start;
      else update->beginstep = update->firststep;
      if (stopflag) update->endstep = stop;
      else update->endstep = update->laststep;

      if (preflag || iter == 0) {
        lmp->init();
        update->integrate->setup();
      } else output->setup(0);

      timer->init();
      timer->barrier_start();
      update->integrate->run(nsteps);
      timer->barrier_stop();

      update->integrate->cleanup();

      Finish finish(lmp);
      if (postflag || nleft <= nsteps) finish.end(1);
      else finish.end(0);

      // wrap command invocation with clearstep/addstep
      // since a command may invoke computes via variables

      if (ncommands) {
        modify->clearstep_compute();
        for (int i = 0; i < ncommands; i++) input->one(commands[i]);
        modify->addstep_compute(update->ntimestep + nevery);
      }

      nleft -= nsteps;
      iter++;
    }
  }

  update->whichflag = 0;
  update->firststep = update->laststep = 0;
  update->beginstep = update->endstep = 0;

  if (commands) {
    for (int i = 0; i < ncommands; i++) delete [] commands[i];
    delete [] commands;
  }
}
