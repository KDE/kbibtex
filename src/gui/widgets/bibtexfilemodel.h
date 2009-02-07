#include <QAbstractItemModel>
#include <QLatin1String>
#include <QList>

#include <file.h>

class KConfig;

namespace KBibTeX
{
namespace GUI {
namespace Widgets {

/**
@author Thomas Fischer
*/
class BibTeXFileModel : public QAbstractItemModel
{
public:
    BibTeXFileModel(QObject * parent = 0);
    virtual ~BibTeXFileModel();

    void setBibTeXFile(KBibTeX::IO::File *bibtexFile);

    virtual QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex & index) const;
    virtual bool hasChildren(const QModelIndex & parent = QModelIndex()) const;
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private:
    typedef struct {
        QString raw;
        QString i18nzed;
    } ColumnDescr;
    QList <ColumnDescr> m_fieldNames;

    KBibTeX::IO::File *m_bibtexFile;
    KConfig *m_config;

    void initFieldNames();
};

}
}
}
