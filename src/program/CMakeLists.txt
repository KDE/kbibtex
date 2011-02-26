project( kbibtexprogram )

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../gui
	${CMAKE_CURRENT_SOURCE_DIR}/../gui/config
	${CMAKE_CURRENT_SOURCE_DIR}/../gui/bibtex
	${CMAKE_CURRENT_SOURCE_DIR}/../gui/element
	${CMAKE_CURRENT_SOURCE_DIR}/../gui/widgets
	${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio
	${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/config
	${CMAKE_CURRENT_SOURCE_DIR}/../websearch
	${CMAKE_CURRENT_SOURCE_DIR}/../../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}/../
)

set( kbibtex_SRCS
	program.cpp
	mainwindow.cpp
	documentlist.cpp
	mdiwidget.cpp
	referencepreview.cpp
	urlpreview.cpp
	valuelist.cpp
	searchform.cpp
	searchresults.cpp
	elementform.cpp
	openfileinfo.cpp
)

# debug area for KBibTeX's IO library
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=101014)

kde4_add_app_icon( kbibtex_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../../icons/hi*-app-kbibtex.png" )

kde4_add_executable( kbibtex${BINARY_POSTFIX}  ${kbibtex_SRCS} )

target_link_libraries( kbibtex${BINARY_POSTFIX}
  ${QT_QTWEBKIT_LIBRARIES}
  ${KDE4_KIO_LIBS}
  ${KDE4_KPARTS_LIBS}
  ${KDE4_KFILE_LIBS}
  kbibtexio
  kbibtexgui
  kbibtexws
)

install( TARGETS kbibtex${BINARY_POSTFIX}  ${INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES kbibtex.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES kbibtexui.rc  DESTINATION ${DATA_INSTALL_DIR}/kbibtex )
