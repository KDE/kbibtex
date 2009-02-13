include_directories(
  ../../libkbibtexio
  ../../gui
)

set( kbibtexpart_SRCS
	part.cpp
	browserextension.cpp
	partfactory.cpp
)

kde4_add_plugin( kbibtexpart  WITH_PREFIX ${kbibtexpart_SRCS})

target_link_libraries(
	kbibtexpart
	kbibtexio
	${KDE4_KPARTS_LIBS}
)

install( TARGETS kbibtexpart  DESTINATION ${PLUGIN_INSTALL_DIR})
install( FILES kbibtexpart.desktop  DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES kbibtexpartui.rc  DESTINATION ${DATA_INSTALL_DIR}/kbibtexpart )

