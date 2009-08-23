/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
#include <QFile>
#include <QString>

#include <element.h>
#include <entry.h>
#include <macro.h>
#include <comment.h>

#include "bibtexfilemodel.h"

using namespace KBibTeX::GUI::Widgets;

const QRegExp BibTeXFileModel::whiteSpace = QRegExp("(\\s\\n\\r\\t)+");

BibTeXFileModel::BibTeXFileModel(QObject * parent)
        : QAbstractItemModel(parent), m_bibtexFile(NULL)
{
    m_bibtexFields = KBibTeX::GUI::Config::BibTeXFields::self();
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

QModelIndex BibTeXFileModel::index(int row, int column, const QModelIndex & /*parent*/) const
{
    return createIndex(row, column, (void*)NULL); // parent == QModelIndex() ? createIndex(row, column, (void*)NULL) : QModelIndex();
}

QModelIndex BibTeXFileModel::parent(const QModelIndex & /*index*/) const
{
    return QModelIndex();
}

bool BibTeXFileModel::hasChildren(const QModelIndex & parent) const
{
    return parent == QModelIndex();
}

int BibTeXFileModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_bibtexFile != NULL ? m_bibtexFile->count() : 0;
}

int BibTeXFileModel::columnCount(const QModelIndex & /*parent*/) const
{
    return m_bibtexFields->count();
}

QVariant BibTeXFileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (m_bibtexFile == NULL || index.row() >= m_bibtexFile->count())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    if (index.row() < m_bibtexFile->count() && index.column() < m_bibtexFields->count()) {
        QString raw = m_bibtexFields->at(index.column()).raw;
        KBibTeX::IO::Element* element = (*m_bibtexFile)[index.row()];
        KBibTeX::IO::Entry* entry = dynamic_cast<KBibTeX::IO::Entry*>(element);
        if (entry != NULL) {
            if (raw == "^id")
                return QVariant(entry->id());
            else if (raw == "^type")
                return QVariant(KBibTeX::IO::Entry::entryTypeToString(entry->entryType()));
            else {
                KBibTeX::IO::Field *field = NULL;
                if ((field = entry->getField(raw)) != NULL) {
                    QString text = KBibTeX::IO::PlainTextValue::text(field->value(), m_bibtexFile);
                    text = text.replace(whiteSpace, " ");
                    return QVariant(text);
                } else
                    return QVariant(index.column());
            }
        } else {
            KBibTeX::IO::Macro* macro = dynamic_cast<KBibTeX::IO::Macro*>(element);
            if (macro != NULL) {
                if (raw == "^id")
                    return QVariant(macro->key());
                else if (raw == "^type")
                    return QVariant("Macro"); // TODO: i18n
                else if (raw == "title") {
                    QString text = KBibTeX::IO::PlainTextValue::text(macro->value(), m_bibtexFile);
                    text = text.replace(whiteSpace, " ");
                    return QVariant(text);
                } else
                    return QVariant();
            } else {
                KBibTeX::IO::Comment* comment = dynamic_cast<KBibTeX::IO::Comment*>(element);
                if (comment != NULL) {
                    if (raw == "^type")
                        return QVariant("Comment"); // TODO: i18n
                    else if (raw == "title") {
                        QString text = comment->text().replace(QRegExp("[\\s\\n\\r\\t]+"), " ");
                        return QVariant(text);
                    } else
                        return QVariant();
                } else
                    return ("?");
            }
        }
    } else
        return QVariant("?");
}

QVariant BibTeXFileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        if (section < m_bibtexFields->count())
            return m_bibtexFields->at(section).label;
        else
            return QString("Column %1").arg(section);
    } else
        return QString("Row %1").arg(section);
}
