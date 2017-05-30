find_package(
    SharedMimeInfo
)

########### install files ###############
install(
    FILES
    bibliography.xml
    DESTINATION
    ${KDE_INSTALL_MIMEDIR}
)
update_xdg_mimetypes(
    ${KDE_INSTALL_MIMEDIR}
)
