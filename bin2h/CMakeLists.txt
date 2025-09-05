# CMakeList.txt : CMake project for bin2h, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (bin2h "main.c" )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET bin2h PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
