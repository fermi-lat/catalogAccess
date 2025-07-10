##### Library ######
add_library(
  catalogAccess STATIC
  src/catalog.cxx
  src/catalog_io.cxx
  src/catalog_ioText.cxx
  src/catalog_sel.cxx
  src/quantity.cxx
)

target_link_libraries(catalogAccess PUBLIC st_facilities tip)

target_include_directories(
  catalogAccess PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:>
)

###### Tests ######
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_executable(main_test src/test/main_test.cxx)
add_executable(test_catalogAccess src/test/main_test.cxx)
target_link_libraries(main_test PRIVATE catalogAccess)
target_link_libraries(test_catalogAccess PRIVATE catalogAccess)

###############################################################
# Installation
###############################################################
install(DIRECTORY catalogAccess DESTINATION ${FERMI_INSTALL_INCLUDEDIR})
install(DIRECTORY data/ DESTINATION ${FERMI_INSTALL_REFDATADIR}/catalogAccess)

install(
  TARGETS catalogAccess main_test test_catalogAccess
  # EXPORT fermiTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
