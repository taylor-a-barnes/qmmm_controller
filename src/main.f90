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
  end interface

  !initialize the server
  ret = initialize_server()

  !start the client
  call execute_command_line("/project/projectdirs/m1944/tabarnes/edison/qmmm/pipes/build/src_client/client")

  !start communicating with the client
  ret = communicate()

end program driver
