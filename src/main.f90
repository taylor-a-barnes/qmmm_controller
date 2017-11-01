program driver
  USE, INTRINSIC :: iso_c_binding

  implicit none

  integer :: ret

  interface
     function initialize_server() bind(c, name="initialize_server")
       use, intrinsic :: iso_c_binding
       integer(kind=c_int) :: initialize_server
     end function initialize_server

     function communicate() bind(c, name="communicate")
       use, intrinsic :: iso_c_binding
       integer(kind=c_int) :: communicate
     end function communicate

     function run_simulation() bind(c, name="run_simulation")
       use, intrinsic :: iso_c_binding
       integer(kind=c_int) :: run_simulation
     end function run_simulation
  end interface

  !initialize the server
  ret = initialize_server()

  !start the client
  call execute_command_line("/project/projectdirs/m1944/tabarnes/edison/qmmm/pipes/build/src_client/client", WAIT=.FALSE.)

  !start communicating with the client
  ret = communicate()

  !start running the simulation
  !call execute_command_line("srun -n 1 /project/projectdirs/m1944/tabarnes/edison/qmmm/lammps/mm_main/lib/qmmm/pwqmmm.x qmmm.inp > input.out", WAIT=.FALSE.)
  !ret = run_simulation()

  !sleep, to ensure that the LAMMPS call completes
  !CALL SLEEP(20)

end program driver
