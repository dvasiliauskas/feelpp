set_directory_properties(PROPERTIES LABEL testmesh )
foreach(THETEST entity mesh regiontree mesh_codim1 kdtree P1mesh meshmarker updatemarker partitioner_metis elementswithmarkedfaces meshmover)

  if(THETEST MATCHES partitioner_metis)
      if(FEELPP_HAS_METIS)
          feelpp_add_test( ${THETEST} )
      endif()
  else()
      feelpp_add_test( ${THETEST} )
  endif()

endforeach()

feelpp_add_test( mesh_filters_operations SRCS test_mesh_filters_operations.cpp CFG test_mesh_filters_operations.cfg GEO test_mesh_filters_operations.geo )
feelpp_add_test( submesh1d SRCS test_submesh1d.cpp CFG test_submesh1d.cfg GEO test_submesh1d.geo )

# add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS ${TESTS} )
