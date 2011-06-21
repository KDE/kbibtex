include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio
  ${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/config
  ${CMAKE_CURRENT_SOURCE_DIR}/../gui
  ${CMAKE_CURRENT_SOURCE_DIR}/../gui/config
  ${CMAKE_CURRENT_SOURCE_DIR}/../gui/widgets
  ${CMAKE_CURRENT_SOURCE_DIR}/../gui/bibtex
  ${CMAKE_CURRENT_SOURCE_DIR}/../processing/
)

# debug area for KBibTeX's IO library
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=101013)


set( kbibtexpart_SRCS
	part.cpp
	browserextension.cpp
	partfactory.cpp
)

kde4_add_plugin( kbibtexpart  ${kbibtexpart_SRCS})

target_link_libraries( kbibtexpart
	${KDE4_KPARTS_LIBS}
	kbibtexio
	kbibtexgui
	kbibtexproc
)

install( TARGETS kbibtexpart  DESTINATION ${PLUGIN_INSTALL_DIR})
install( FILES kbibtexpart.desktop  DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES kbibtexpartui.rc  DESTINATION ${DATA_INSTALL_DIR}/kbibtexpart )

