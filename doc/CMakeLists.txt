# Create and install documentation (handbook)

kde4_create_handbook(
    index.docbook
    INSTALL_DESTINATION ${HTML_INSTALL_DIR}/en
    SUBDIR kbibtex
)

# Create and install documentation (man page)

kde4_create_manpage(
    man-kbibtex.1.docbook 1
    INSTALL_DESTINATION ${MAN_INSTALL_DIR}
)
