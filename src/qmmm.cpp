/* MolSSI QM/MM driver */
#include "qmmm.h"

int run_qmmm()
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
