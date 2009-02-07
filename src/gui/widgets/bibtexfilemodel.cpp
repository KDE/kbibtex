#include <QFile>
#include <QString>

#include <element.h>
#include <entry.h>

#include "bibtexfilemodel.h"

using namespace KBibTeX::GUI::Widgets;

BibTeXFileModel::BibTeXFileModel(QObject * parent)
        : QAbstractItemModel(parent), m_bibtexFile(NULL)
{
    initFieldNames();
// TODO
}

BibTeXFileModel::~BibTeXFileModel()
{
    if (m_bibtexFile != NULL) delete m_bibtexFile;
// TODO
}

void BibTeXFileModel::setBibTeXFile(KBibTeX::IO::File *bibtexFile)
{
    m_bibtexFile = bibtexFile;
}

QModelIndex BibTeXFileModel::index(int row, int column, const QModelIndex & parent) const
{
    return createIndex(row, column, (void*)NULL); // parent == QModelIndex() ? createIndex(row, column, (void*)NULL) : QModelIndex();
}

QModelIndex BibTeXFileModel::parent(const QModelIndex & index) const
{
    return QModelIndex();
}

bool BibTeXFileModel::hasChildren(const QModelIndex & parent) const
{
    return parent == QModelIndex();
}

int BibTeXFileModel::rowCount(const QModelIndex & parent) const
{
    return m_bibtexFile != NULL ? m_bibtexFile->count() : 0;
}

int BibTeXFileModel::columnCount(const QModelIndex & parent) const
{
    return 3; //parent==QModelIndex()?0:1;
}

QVariant BibTeXFileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (m_bibtexFile == NULL || index.row() >= m_bibtexFile->count())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    if (index.row() < m_bibtexFile->count() && index.column() < m_fieldNames.count()) {
        KBibTeX::IO::Element* element = (*m_bibtexFile)[index.row()];
        KBibTeX::IO::Entry* entry = dynamic_cast<KBibTeX::IO::Entry*>(element);
        if (entry != NULL) {
            QString raw = m_fieldNames[index.column()].raw;
            if (raw == "^id")
                return QVariant(entry->id());
            else {
                KBibTeX::IO::EntryField *field = NULL;
                KBibTeX::IO::Value *value = NULL;
                if ((field = entry->getField(raw)) && (value = field->value()))
                    return QVariant(value->text());
                else
                    return QVariant(index.column());
            }
        } else
            return QVariant(index.column());
    } else
        return QVariant(index.column());
}

QVariant BibTeXFileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        if (section < m_fieldNames.count())
            return m_fieldNames[section].i18nzed;
        else
            return QString("Column %1").arg(section);
    } else
        return QString("Row %1").arg(section);
}

void BibTeXFileModel::initFieldNames()
{
    ColumnDescr cdescr;
    cdescr.raw = QString("^id"); // FIXME QLatin1String
    cdescr.i18nzed = QString("Identifier"); // FIXME i18n
    m_fieldNames << cdescr;

    // TODO Create configuration file which contains all fields
    cdescr.raw = QString("title"); // FIXME QLatin1String
    cdescr.i18nzed = QString("Title"); // FIXME i18n
    m_fieldNames << cdescr;

    cdescr.raw = QString("author"); // FIXME QLatin1String
    cdescr.i18nzed = QString("Author"); // FIXME i18n
    m_fieldNames << cdescr;
}
