/* MolSSI MM driver */
#include "qmmm.h"

int run_mm()
{
  int iteration;
  int i;
  int max_iterations = 11;
  
  printf("Running the simulation\n");

  //receive the number of MM atoms from the MM main process
  send_label(mm_socket, "<NAT");
  receive_array(mm_socket, &num_mm, 1*sizeof(int));
  natoms = num_mm;

  //receive the number of MM atom types from the MM main process
  send_label(mm_socket, "<NTYPES");
  receive_array(mm_socket, &ntypes, 1*sizeof(int));

  //initialize the arrays for atoms, forces, etc.
  initialize_arrays();

  //receive the MM types
  send_label(mm_socket, "<TYPES");
  receive_array(mm_socket, type, (natoms)*sizeof(int));

  //begin the main MD loop
  for (iteration=1; iteration <= max_iterations; iteration++) {

    printf("\nIteration %i\n",iteration);

    //receive the cell information from the MM main process
    send_label(mm_socket, "<CELL");
    receive_cell(mm_socket);
    
    //receive the MM coordinates
    send_label(mm_socket, "<COORD");
    receive_array(mm_socket, mm_coord_all, (3*num_mm)*sizeof(double));

    //receive the MM charges
    send_label(mm_socket, "<CHARGE");
    receive_array(mm_socket, mm_charge_all, (num_mm)*sizeof(double));

    //receive the MM types
    send_label(mm_socket, "<MASS");
    receive_array(mm_socket, mass, (ntypes+1)*sizeof(double));

    //zero the MM forces (for +FORCE command)
    for (i=0; i < 3*natoms; i++) {
      mm_force[i] = 0.0;
    }

    //send the updated forces to the MM main process
    send_label(mm_socket, "+FORCES");
    send_array(mm_socket, mm_force, (3*num_mm)*sizeof(double));

    //have the MM main process iterate
    if (iteration == 1) {
      send_label(mm_socket, "MD_INIT");
    }
    send_label(mm_socket, "TIMESTEP");

  }

  printf("Completed MM simulation \n");

  return 0;
}
