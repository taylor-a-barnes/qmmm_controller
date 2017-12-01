/* Server code for the QM/MM driver */
#include "messages.h"



int initialize_client()
{
  printf("In C code TESTING\n");
  /*
  int ret;
  struct sockaddr_in driver_address;
  int i;
  //char *serv_host = "localhost";
  string readline;
  int port;
  struct hostent *host_ptr;
  ifstream hostfile("../../hostname");
  
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
  */
}
