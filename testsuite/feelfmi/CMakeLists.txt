if ( FEELPP_HAS_OMC AND FEELPP_HAS_FMILIB )
  feelpp_add_fmu( fmi2cs CLASS BouncingBall SRCS BouncingBall.mo TYPE cs VERS 2.0 )
  feelpp_add_omc( bouncingball CLASS BouncingBall SRCS BouncingBall.mo )
  feelpp_add_test( fmi DEPS feelpp_add_fmu_fmi2cs CFG test_fmi.cfg )
endif()
