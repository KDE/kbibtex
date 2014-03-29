function get_files
{
    echo bibliography.xml
}

function po_for_file
{
    case "$1" in
       bibliography.xml)
           echo kbibtex_xml_mimetypes.po
       ;;
    esac
}

function tags_for_file
{
    case "$1" in
       bibliography.xml)
           echo comment
       ;;
    esac
}
