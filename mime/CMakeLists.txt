find_package(
    SharedMimeInfo
)

########### install files ###############
install(
    FILES
    bibliography.xml
    DESTINATION
    ${XDG_MIME_INSTALL_DIR}
)
update_xdg_mimetypes(
    ${XDG_MIME_INSTALL_DIR}
)
