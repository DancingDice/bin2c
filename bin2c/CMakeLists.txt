# CMakeList.txt : CMake project for bin2c, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (bin2c "main.c" )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET bin2c PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
