# **************************************************************************
# * Copyright(c) 1998-2014, ALICE Experiment at CERN, All rights reserved. *
# *                                                                        *
# * Author: The ALICE Off-line Project.                                    *
# * Contributors are mentioned in the code where appropriate.              *
# *                                                                        *
# * Permission to use, copy, modify and distribute this software and its   *
# * documentation strictly for non-commercial purposes is hereby granted   *
# * without fee, provided that the above copyright notice appears in all   *
# * copies and that both the copyright notice and this permission notice   *
# * appear in the supporting documentation. The authors make no claims     *
# * about the suitability of this software for any purpose. It is          *
# * provided "as is" without express or implied warranty.                  *
# **************************************************************************

# Module
set(MODULE DRGEN)
add_definitions(-D_MODULE_="${MODULE}")

# Module include folder
include_directories(${AliRoot_SOURCE_DIR}/${MODULE}
                   )

# Additional include folders in alphabetical order except ROOT
include_directories(SYSTEM ${ROOT_INCLUDE_DIR})
include_directories(${AliRoot_SOURCE_DIR}/EVGEN
                    ${AliRoot_SOURCE_DIR}/STEER/STEER
                   )

# Sources
set(SRCS
    TDRgen.cxx
    AliGenDRgen.cxx
)

set(FSRCS
    PP_gen.f
)

# Headers from sources
string(REPLACE ".cxx" ".h" HDRS "${SRCS}")

# Generate the dictionary
# It will create G_ARG1.cxx and G_ARG1.h / ARG1 = function first argument
get_directory_property(incdirs INCLUDE_DIRECTORIES)
generate_dictionary("${MODULE}" "${MODULE}LinkDef.h" "${HDRS}" "${incdirs}")

# Generate the ROOT map
# Dependecies
#set(LIBDEPS EVGEN STEER AliPythia6 microcern)
set(LIBDEPS EVGEN)
generate_rootmap("${MODULE}" "${LIBDEPS}" "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}LinkDef.h")

# Add a library to the project using the specified source files
add_library_tested(${MODULE} SHARED ${SRCS} G__${MODULE}.cxx ${FSRCS})

# Additional compilation flags
# Additional compilation flags
if(${CMAKE_Fortran_COMPILER} MATCHES gfortran)
set( CMAKE_Fortran_FLAGS "-fno-automatic -fno-backslash -ffixed-line-length-230 -fPIC -fno-second-underscore -fno-range-check  ")
endif(${CMAKE_Fortran_COMPILER} MATCHES gfortran)
if(${CMAKE_Fortran_COMPILER} MATCHES f95)
set( CMAKE_Fortran_FLAGS "-fno-automatic -fno-backslash -ffixed-line-length-230 -fPIC -fno-second-underscore -fno-range-check  ")
endif(${CMAKE_Fortran_COMPILER} MATCHES f95)
if(${CMAKE_Fortran_COMPILER} MATCHES ifort)
set ( CMAKE_Fortran_FLAGS "-extend-source")
endif(${CMAKE_Fortran_COMPILER} MATCHES ifort)
set_target_properties(${MODULE} PROPERTIES COMPILE_FLAGS "")

# Linking the library
target_link_libraries(${MODULE} ${LIBDEPS})

# System dependent: Modify the way the library is build
if(${CMAKE_SYSTEM} MATCHES Darwin)
    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(${CMAKE_SYSTEM} MATCHES Darwin)

# Installation
install(TARGETS ${MODULE}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)
install(FILES ${HDRS} DESTINATION include)

message(STATUS "${MODULE} enabled")
