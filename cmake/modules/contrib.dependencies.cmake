set(FEELPP_HAS_GFLAGS 1)
set(FEELPP_HAS_GLOG 1)
set(FEELPP_HAS_GINAC 1)

if ( FEELPP_HAS_GFLAGS )
  set(GFLAGS_IS_SUBPROJECT TRUE)
  set(GFLAGS_NAMESPACE "google;gflags")
  #add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gflags)
  list(INSERT FEELPP_LIBRARIES 0 feelpp_gflags)

  #add_dependencies(contrib feelpp_gflags feelpp_gflags_shared feelpp_gflags_nothreads_shared)
  #target_include_directories(feelpp_gflags_nothreads_shared BEFORE PUBLIC ${FEELPP_BINARY_DIR}/gflags/include ${FEELPP_BINARY_DIR}/gflags/include/gflags)

  include_directories(${FEELPP_BINARY_DIR}/contrib/gflags/include )#${FEELPP_BINARY_DIR}/gflags/include/gflags)

  set(FEELPP_ENABLED_OPTIONS "${FEELPP_ENABLED_OPTIONS} GFlags/Contrib" )
  
  # for GLog
  set(gflags_LIBRARIES feelpp_gflags)
  set(gflags_FOUND 1)
endif()

if ( FEELPP_HAS_GLOG )
    #     INCLUDE_DIRECTORIES(${FEELPP_BINARY_DIR}/glog/ ${CMAKE_CURRENT_SOURCE_DIR}/glog/src)
  #add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glog)
  list(INSERT FEELPP_LIBRARIES 0 feelpp_glog)
  #add_dependencies(contrib feelpp_glog)
  set(FEELPP_ENABLED_OPTIONS "${FEELPP_ENABLED_OPTIONS} GLog/Contrib" )
endif()


if ( FEELPP_HAS_GINAC )
  #
  # cln and ginac
  #
  find_package(CLN)

  add_definitions(-DIN_GINAC -DHAVE_LIBDL)

  #link_directories( ${CMAKE_BINARY_DIR}/contrib/ginac/ginac)

  include_directories( BEFORE ${CLN_INCLUDE_DIR} ${FEELPP_SOURCE_DIR}/contrib/ginac/ ${FEELPP_BUILD_DIR}/ginac/ ${CMAKE_SOURCE_DIR}/contrib/ginac/ginac ${FEELPP_BUILD_DIR}/contrib/ginac/ginac)
  set(DL_LIBS ${CMAKE_DL_LIBS})


  #add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ginac)
  #add_dependencies(contrib feelpp_ginac ginsh)
  list(INSERT FEELPP_LIBRARIES 0 feelpp_ginac)
endif()




OPTION( FEELPP_ENABLE_NT2 "Enable the numerical toolkit tmplate library" OFF )
if ( FEELPP_ENABLE_NT2 )
  #set(NT2_SOURCE_ROOT ${FEELPP_ROOT}/nt2)
  #set(NT2_WITH_TESTS OFF)
  option(NT2_WITH_TESTS "Enable benchmarks and unit tests" OFF)
  #add_subdirectory(nt2)
  set(CMAKE_CXX_FLAGS "${NT2_SIMD_FLAGS} ${CMAKE_CXX_FLAGS}")
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/nt2/include/)

  foreach(module ${NT2_FOUND_COMPONENTS})
    string(TOUPPER ${module} module_U)
    if(NT2_${module_U}_ROOT)
      INCLUDE_DIRECTORIES(${NT2_${module_U}_ROOT}/include)
      message(status "[feelpp/nt2] adding ${NT2_${module_U}_ROOT}/include" )
    endif()
  endforeach()

  set(NT2_FOUND 1)
  set(FEELPP_HAS_NT2 1)
  SET(FEELPP_LIBRARIES nt2  ${FEELPP_LIBRARIES}  )
  SET(FEELPP_ENABLED_OPTIONS "${FEELPP_ENABLED_OPTIONS} NT2" )
  message(STATUS "[feelpp] nt2 is enabled" )

endif( FEELPP_ENABLE_NT2 )

#
# Eigen
#
if ( FEELPP_ENABLE_SYSTEM_EIGEN3 )
  FIND_PACKAGE(Eigen3)
  MESSAGE(STATUS "Eigen3 system found:")
  MESSAGE("EIGEN_INCLUDE_DIR=${EIGEN_INCLUDE_DIR}")
  MESSAGE("EIGEN3_INCLUDE_DIR=${EIGEN3_INCLUDE_DIR}")
  MESSAGE(STATUS "Adding unsupported headers to EIGEN3_INCLUDE_DIR:")
  set( EIGEN3_INCLUDE_DIR ${EIGEN3_INCLUDE_DIR} ${EIGEN3_INCLUDE_DIR}/unsupported)
  MESSAGE("EIGEN3_INCLUDE_DIR=${EIGEN3_INCLUDE_DIR}")
endif()
if (NOT EIGEN3_FOUND AND EXISTS ${CMAKE_SOURCE_DIR}/feel AND EXISTS ${CMAKE_SOURCE_DIR}/contrib )
  option(EIGEN_BUILD_PKGCONFIG "Build pkg-config .pc file for Eigen" OFF)
  
  set( EIGEN3_INCLUDE_DIR ${FEELPP_SOURCE_DIR}/contrib/eigen ${FEELPP_SOURCE_DIR}/contrib/eigen/unsupported )

  SET(FEELPP_ENABLED_OPTIONS "${FEELPP_ENABLED_OPTIONS} Eigen3/Contrib" )
elseif( EIGEN3_FOUND )
  SET(FEELPP_ENABLED_OPTIONS "${FEELPP_ENABLED_OPTIONS} Eigen3/System" )
else()
  find_path(EIGEN3_INCLUDE_DIR NAMES signature_of_eigen3_matrix_library
    PATHS
    $ENV{FEELPP_DIR}/include/feel
    NO_DEFAULT_PATH
    )
endif()
INCLUDE_DIRECTORIES( ${EIGEN3_INCLUDE_DIR} )
SET(FEELPP_HAS_EIGEN3 1)
if ( FEELPP_HAS_EIGEN3 )
  #add_subdirectory(${FEELPP_SOURCE_DIR}/contrib/eigen)
  unset(INCLUDE_INSTALL_DIR CACHE)
  unset(CMAKEPACKAGE_INSTALL_DIR CACHE)
  unset(PKGCONFIG_INSTALL_DIR CACHE)
endif()
message(STATUS "[feelpp] eigen3 headers: ${EIGEN3_INCLUDE_DIR}" )



#FIND_PACKAGE(Eigen2 REQUIRED)
#INCLUDE_DIRECTORIES( ${Eigen2_INCLUDE_DIR} )
#add_subdirectory(contrib/eigen)
#INCLUDE_DIRECTORIES( ${FEELPP_SOURCE_DIR}/contrib/eigen )
#add_definitions( -DEIGEN_NO_STATIC_ASSERT )


option( FEELPP_ENABLE_METIS "Enable Metis Support" ${FEELPP_ENABLE_PACKAGE_DEFAULT_OPTION} )
if(FEELPP_ENABLE_METIS)
  include(feelpp.module.metis)
endif()

if ( NOT FEELPP_HAS_PARMETIS )
  option( FEELPP_ENABLE_PARMETIS "Enable Parmetis Support" OFF )
  if(FEELPP_ENABLE_PARMETIS)
    FIND_LIBRARY(PARMETIS_LIBRARY
      NAMES
      parmetis
      PATHS
      $ENV{PETSC_DIR}/lib
      $ENV{PETSC_DIR}/$ENV{PETSC_ARCH}/lib
      )
    IF( PARMETIS_LIBRARY )
      message(STATUS "[feelpp] Parmetis: ${PARMETIS_LIBRARY}" )
      SET(FEELPP_LIBRARIES ${PARMETIS_LIBRARY} ${FEELPP_LIBRARIES})
      set(FEELPP_HAS_PARMETIS 1)
    ENDIF()
  endif()
endif()

# include pybind11 after python cmake macros to avoid detecting different python versions
include(feelpp.module.pybind11)
include(feelpp.module.mongocxx)
include(feelpp.module.hpddm)
include(feelpp.module.nlopt)
include(feelpp.module.ipopt)
#include(feelpp.module.cereal)
#include(feelpp.module.paralution)
include(feelpp.module.jsonlab)

# Add an info message to be displayed at the end of the cmake process.
if( FEELPP_CONTRIB_SUBMODULE_UPDATED )
  list( APPEND FEELPP_MESSAGE_INFO_END "Feel++ submodules already initialized!\nPlease make sure submodules are up to date (run `git submodule update --init --recursive` in source directory)" )
  set( FEELPP_MESSAGE_INFO_END ${FEELPP_MESSAGE_INFO_END} )
endif()
