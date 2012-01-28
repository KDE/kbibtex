install(
    FILES
    kbibtexrc
    DESTINATION
    ${CONFIG_INSTALL_DIR}
)
install(
    FILES
    bibtex.kbstyle
    biblatex.kbstyle
    DESTINATION
    ${DATA_INSTALL_DIR}/kbibtex
)

