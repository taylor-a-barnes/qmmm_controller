program driver
  USE, INTRINSIC :: iso_c_binding

  implicit none

  integer :: ret
  character(len=32) :: hostname

  interface
     function initialize_driver_socket__() bind(c, name="initialize_driver_socket__")
       use, intrinsic :: iso_c_binding
       integer(kind=c_int) :: initialize_driver_socket__
     end function initialize_driver_socket__

     function accept_mm_connection__() bind(c, name="accept_mm_connection__")
       use, intrinsic :: iso_c_binding
       integer(kind=c_int) :: accept_mm_connection__
     end function accept_mm_connection__

     function accept_mm_subset_connection__() bind(c, name="accept_mm_subset_connection__")
       use, intrinsic :: iso_c_binding
       integer(kind=c_int) :: accept_mm_subset_connection__
     end function accept_mm_subset_connection__

     function accept_qm_connection__() bind(c, name="accept_qm_connection__")
       use, intrinsic :: iso_c_binding
       integer(kind=c_int) :: accept_qm_connection__
     end function accept_qm_connection__

     function run_simulation__() bind(c, name="run_simulation__")
       use, intrinsic :: iso_c_binding
       integer(kind=c_int) :: run_simulation__
     end function run_simulation__
  end interface

  ! get the server hostname
  call execute_command_line("hostname > hostname", WAIT=.TRUE.)
  open(unit=27, file="./hostname")
  READ(27,'(A)')hostname
  close(27)

  ! initialize the driver so that it can accept connections
  ret = initialize_driver_socket__()

  ! start the MM main process
  ! this is the process that will perform the MM calculation on the full system
  call execute_command_line("(cd ./mm_main; mpirun -n 1 ~/qmmm/lammps/&
       &src/lmp_cori2 -in water.in > input.out)", WAIT=.FALSE.)
  ret = accept_mm_connection__()

  ! start the MM subset process
  ! this is the process that will perform the MM calculation on the QM system
  call execute_command_line("(cd ./mm_subset; mpirun -n 1 ~/qmmm/lammps/&
       &src/lmp_cori2 -in water_single.in > input.out)", WAIT=.FALSE.)
  ret = accept_mm_subset_connection__()

  ! start the QM process
  call execute_command_line("(cd ./qm; mpirun -n 32 ~/qmmm/qe/&
       &/bin/pw.x -ipi """ // trim(hostname) // """:8021 -in water.in > water.out)", WAIT=.FALSE.)
  ret = accept_qm_connection__()

  ! start running the simulation
  ret = run_simulation__()

  CALL SLEEP(5)

end program driver
