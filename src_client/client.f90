program client
  USE, INTRINSIC :: iso_c_binding

  IMPLICIT NONE

  integer :: ret

  interface
     function initialize_client() bind(c, name="initialize_client")
       use, intrinsic :: iso_c_binding
       integer(kind=c_int) :: initialize_client
     end function initialize_client
  end interface

  write(6,*) ""
  write(6,*) "I am the client"

  !call initialize()
  ret = initialize_client()

end program client
