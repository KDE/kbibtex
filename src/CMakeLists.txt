include(
    CheckIncludeFiles
)

include(
    KDE4Defaults
)

##SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
##SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
##SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -pg")
##SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
##SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
##SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -pg")
##SET(CMAKE_CXX_FLAGS_DEBUGFULL "${CMAKE_CXX_FLAGS_DEBUGFULL} -pg")
##SET(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -pg")
##SET(CMAKE_CXX_FLAGS_DISTRIBUTION "${CMAKE_CXX_FLAGS_DISTRIBUTION} -pg")

# check if QtWebKit is available, which seems to be not available
# on e.g. RedHat Enterprise Linux 6 or Scientific Linux 6
if(
    QT_QTWEBKIT_FOUND
)
    message(
        STATUS
        "Found QtWebKit, enabling WebKit support"
    )
    add_definitions(
        -DHAVE_QTWEBKIT
    )
else(
    QT_QTWEBKIT_FOUND
)
    message(
        STATUS
        "QtWebKit not found, disabling WebKit support"
    )
endif(
    QT_QTWEBKIT_FOUND
)

add_definitions(
    ${QT_DEFINITIONS}
    ${KDE4_DEFINITIONS}
)
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${KDE4_INCLUDES}
)

add_subdirectory(
    config
)
add_subdirectory(
    data
)
add_subdirectory(
    io
)
add_subdirectory(
    processing
)
add_subdirectory(
    networking
)
add_subdirectory(
    gui
)
add_subdirectory(
    program
)
add_subdirectory(
    parts
)
add_subdirectory(
    test
)

install(
    FILES
    kbibtexnamespace.h
    DESTINATION
    ${INCLUDE_INSTALL_DIR}/kbibtex
    COMPONENT
    Devel
)
