set(kbibtex_kirigami2_SRCS
    main.cpp
    bibliographymodel.cpp
    searchenginelist.cpp
)

ecm_qt_declare_logging_category(kbibtex_kirigami2_SRCS
    HEADER logging_kirigami2.h
    IDENTIFIER LOG_KBIBTEX_KIRIGAMI2
    CATEGORY_NAME kbibtex.kirigami2
)

qt5_add_resources(RESOURCES resources.qrc)

add_executable(kbibtex_kirigami2
    ${kbibtex_kirigami2_SRCS}
    ${RESOURCES}
)

target_include_directories(kbibtex_kirigami2
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(kbibtex_kirigami2
    PRIVATE
        Qt5::Core
        Qt5::Widgets
        Qt5::Qml
        Qt5::Quick
        Qt5::XmlPatterns
        KF5::I18n
        KF5::Kirigami2
        KBibTeX::Data
        KBibTeX::Networking
)

install(
    TARGETS
    kbibtex_kirigami2
    ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)
