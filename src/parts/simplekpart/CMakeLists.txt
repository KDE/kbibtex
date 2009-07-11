include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../libkbibtexio
  ${CMAKE_CURRENT_SOURCE_DIR}/../../gui
  ${CMAKE_CURRENT_SOURCE_DIR}/../../gui/config
  ${CMAKE_CURRENT_SOURCE_DIR}/../../gui/widgets
)


set( simplekbibtexpart_SRCS
	part.cpp
	browserextension.cpp
	partfactory.cpp
)

kde4_add_plugin( simplekbibtexpart  WITH_PREFIX ${simplekbibtexpart_SRCS})

target_link_libraries( simplekbibtexpart
	kbibtexio
	kbibtexgui
	${KDE4_KPARTS_LIBS}
)

install( TARGETS simplekbibtexpart  DESTINATION ${PLUGIN_INSTALL_DIR})
install( FILES simplekbibtexpart.desktop  DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES simplekbibtexpartui.rc  DESTINATION ${DATA_INSTALL_DIR}/kbibtexpart )

