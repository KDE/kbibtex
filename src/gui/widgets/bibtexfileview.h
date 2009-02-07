#include <QTreeView>

namespace KBibTeX
{
namespace GUI {
namespace Widgets {

/**
@author Thomas Fischer
*/
class BibTeXFileView : public QTreeView
{
public:
    BibTeXFileView(QWidget * parent = 0);
    virtual ~BibTeXFileView();
};


}
}
}
