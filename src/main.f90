program driver
  USE, INTRINSIC :: iso_c_binding

  implicit none

  integer :: ret

  interface
     function initialize_server__() bind(c, name="initialize_server__")
       use, intrinsic :: iso_c_binding
       integer(kind=c_int) :: initialize_server__
     end function initialize_server__

     function initialize_arrays__() bind(c, name="initialize_arrays__")
       use, intrinsic :: iso_c_binding
       integer(kind=c_int) :: initialize_arrays__
     end function initialize_arrays__

     function communicate__() bind(c, name="communicate__")
       use, intrinsic :: iso_c_binding
       integer(kind=c_int) :: communicate__
     end function communicate__

     function run_simulation__() bind(c, name="run_simulation__")
       use, intrinsic :: iso_c_binding
       integer(kind=c_int) :: run_simulation__
     end function run_simulation__
  end interface

  !initialize the server
!  ret = initialize_server__()

  !start the client
  !ret = initialize_arrays__()
  !call execute_command_line("/project/projectdirs/m1944/tabarnes/edison/qmmm/pipes/build/src_client/client", WAIT=.FALSE.)

  !start communicating with the client
  !ret = communicate__()

  !start running the simulation
  !call execute_command_line("srun -n 1 /project/projectdirs/m1944/tabarnes/edison/qmmm/lammps/mm_main/lib/qmmm/pwqmmm.x qmmm.inp > input.out", WAIT=.FALSE.)
!  call execute_command_line("cd mm_main", WAIT=.TRUE.)
!  call execute_command_line("srun -n 1 /project/projectdirs/m1944/tabarnes/edison/qmmm/lammps/mm_small/lib/qmmm/pwqmmm.x qmmm.inp > input.out", WAIT=.FALSE.)
!  call execute_command_line("cd ../", WAIT=.TRUE.)

!  call execute_command_line("pwd", WAIT=.TRUE.)
!  call execute_command_line("cd mm_main", WAIT=.TRUE.)
!  call execute_command_line("pwd", WAIT=.TRUE.)
!  call execute_command_line("srun -n 1 /project/projectdirs/m1944/tabarnes/edison/qmmm/lammps/mm_small/lib/qmmm/pwqmmm.x mm_main/qmmm.inp > input.out", WAIT=.FALSE.)
!  call execute_command_line("cd ../", WAIT=.TRUE.)
!  call execute_command_line("pwd", WAIT=.TRUE.)

  !call execute_command_line("echo ""Starting MASTER client""", WAIT=.TRUE.)
!  call execute_command_line("(cd ./mm_main; srun -n 1 /project/projectdirs/m1944/tabarnes/edison/qmmm/lammps/mm_small/lib/qmmm/pwqmmm.x qmmm.inp > input.out)", WAIT=.FALSE.)

  !call execute_command_line("echo ""Starting SLAVE client""", WAIT=.TRUE.)
!  call execute_command_line("(cd ./mm_subset; srun -n 1 /project/projectdirs/m1944/tabarnes/edison/qmmm/lammps/mm_small/lib/qmmm/pwqmmm.x qmmm.inp > input.out)", WAIT=.FALSE.)

!<<<<
!  ret = initialize_server__()

  !NOTE: NEED SOME WAY TO ENSURE THAT THE JOB IS CREATED BY THIS POINT

!  ret = run_simulation__()

  !sleep, to ensure that the LAMMPS call completes
!  CALL SLEEP(5)
!>>>
  call execute_command_line("hostname -i > hostname", WAIT=.TRUE.)
  ret = initialize_server__()
  call execute_command_line("(cd ./mm_main; srun -N 1 -n 1 /project/projectdirs/m1944/tabarnes/edison/qmmm/lammps/mm_small/lib/qmmm/pwqmmm.x qmmm.inp > input.out)", WAIT=.FALSE.)
  call execute_command_line("(cd ./mm_subset; srun -N 1 -n 1 /project/projectdirs/m1944/tabarnes/edison/qmmm/lammps/mm_small/lib/qmmm/pwqmmm.x qmmm.inp > input.out)", WAIT=.FALSE.)
  ret = run_simulation__()
  CALL SLEEP(5)

end program driver
