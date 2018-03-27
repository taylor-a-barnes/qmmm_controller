#include "main.h"

int main()
{
  printf("IN MAIN\n");

  //get the hostname of the driver
  system("hostname > hostname");
  ifstream hostfile;
  hostfile.open("hostname");
  if (!hostfile) {
    printf("Unable to open hostname\n");
    exit(1);
  }
  int x;
  string name;
  while (hostfile >> name) {
    printf("hostname: %s\n",name.c_str());
  }

  //initialize the driver so that it can accept connections
  initialize_driver_socket();

  //start the MM main process
  //   this is the process that will perform the MM calculation on the full system
  launch_server("(cd ./mm_main; mpirun -n 1 ~/qmmm/lammps/src/lmp_cori2 -in water.in > input.out)");
  accept_mm_connection();

  //start the MM subset process
  //   this is the process that will perform the MM calculation on the QM system
  launch_server("(cd ./mm_subset; mpirun -n 1 ~/qmmm/lammps/src/lmp_cori2 -in water_single.in > input.out)");
  accept_mm_subset_connection();

  //start the QM process
  string qm_command = "(cd ./qm; mpirun -n 32 ~/qmmm/qe//bin/pw.x -ipi " + name + ":8021 -in water.in > water.out)";
  printf("QM COMMAND: %s\n",qm_command.c_str());
  launch_server(qm_command.c_str());
  accept_qm_connection();

  //start running the simulation
  run_qmmm();

  return 0;
}
