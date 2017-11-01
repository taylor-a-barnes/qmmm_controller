program client
  USE, INTRINSIC :: iso_c_binding

  IMPLICIT NONE

  integer :: ret

  interface
     function initialize_client__() bind(c, name="initialize_client__")
       use, intrinsic :: iso_c_binding
       integer(kind=c_int) :: initialize_client__
     end function initialize_client__
  end interface

  write(6,*) ""
  write(6,*) "I am the client"

  ret = initialize_client__()

end program client
