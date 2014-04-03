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
    jabref_journalabbrevlist.txt
    DESTINATION
    ${DATA_INSTALL_DIR}/kbibtex
)

