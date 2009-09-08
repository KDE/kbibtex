project( kbibtexprogram )

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../gui
)

set( kbibtex_SRCS
	main.cpp
	program.cpp
	about.cpp
	mainwindow.cpp
	documentlist.cpp
	mdiwidget.cpp
	referencepreview.cpp
)

# TODO kde4_add_app_icon( okteta_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/hi*-app-kbibtex.png" )

kde4_add_executable( kbibtex  ${kbibtex_SRCS} )

target_link_libraries( kbibtex
  ${QT_QTWEBKIT_LIBRARIES}
  ${KDE4_KIO_LIBS}
  ${KDE4_KPARTS_LIBS}
  kbibtexio
)

install( TARGETS kbibtex  ${INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES kbibtex.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES kbibtexui.rc  DESTINATION ${DATA_INSTALL_DIR}/kbibtex )

# TODO kde4_install_icons( ${ICON_INSTALL_DIR} )

