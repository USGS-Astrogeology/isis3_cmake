# CMake module for find_package(Qwt)
# Finds include directory and all applicable libraries
#
# Sets the following:
#   QWT_INCLUDE_DIR
#   QWT_LIBRARY

FIND_PATH(QWT_INCLUDE_DIR
  NAMES qwt.h
  PATH_SUFFIXES qwt-qt5 qwt qwt6
)

find_library(QWT_LIBRARY
  NAMES qwt
)

get_filename_component(QWT_ROOT_INCLUDE_DIR "${QWT_INCLUDE_DIR}" DIRECTORY)