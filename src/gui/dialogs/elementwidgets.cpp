/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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

#include <QLayout>
#include <QTextEdit>
#include <QBuffer>

#include <KPushButton>
#include <KGlobalSettings>
#include <KLocale>

#include <fileimporterbibtex.h>
#include <fileexporterbibtex.h>
#include <file.h>
#include <entry.h>
#include <macro.h>
#include "elementwidgets.h"

void SourceWidget::createGUI()
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 0);
    layout->setColumnStretch(2, 0);
    layout->setRowStretch(0, 1);
    layout->setRowStretch(1, 0);

    sourceEdit = new QTextEdit(this);
    layout->addWidget(sourceEdit, 0, 0, 1, 3);
    sourceEdit->document()->setDefaultFont(KGlobalSettings::fixedFont());
    sourceEdit->setTabStopWidth(QFontMetrics(sourceEdit->font()).averageCharWidth() * 4);

    KPushButton *buttonCheck = new KPushButton(i18n("Validate"), this);
    layout->addWidget(buttonCheck, 1, 1, 1, 1);
    buttonCheck->setEnabled(false); // TODO: implement functionalty

    KPushButton *buttonRestore = new KPushButton(KIcon("edit-undo"), i18n("Restore"), this);
    layout->addWidget(buttonRestore, 1, 2, 1, 1);
    connect(buttonRestore, SIGNAL(clicked()), parent(), SLOT(reset()));
}

SourceWidget::SourceWidget(QWidget *parent)
        : ElementWidget(parent)
{
    createGUI();
}

bool SourceWidget::apply(Element *element) const
{
    QString text = sourceEdit->document()->toPlainText();
    FileImporterBibTeX importer;
    File *file = importer.fromString(text);
    if (file == NULL) return false;

    bool result = false;
    if (file->count() == 1) {
        Entry *entry = dynamic_cast<Entry*>(element);
        Entry *readEntry = dynamic_cast<Entry*>(file->first());
        if (readEntry != NULL && entry != NULL) {
            entry->operator =(*readEntry);
            result = true;
        } else {
            Macro *macro = dynamic_cast<Macro*>(element);
            Macro *readMacro = dynamic_cast<Macro*>(file->first());
            if (readMacro != NULL && macro != NULL) {
                macro->operator =(*readMacro);
                result = true;
            }
        }
    }

    delete file;
    return result;
}

bool SourceWidget::reset(const Element *element)
{
    FileExporterBibTeX exporter;
    QBuffer textBuffer;
    textBuffer.open(QIODevice::WriteOnly);
    bool result = exporter.save(&textBuffer, element, NULL);
    textBuffer.close();
    textBuffer.open(QIODevice::ReadOnly);
    QTextStream ts(&textBuffer);
    QString text = ts.readAll();
    sourceEdit->document()->setPlainText(text);
    return result;
}

void SourceWidget::setReadOnly(bool isReadOnly)
{
    Q_UNUSED(isReadOnly);
    // TODO
}
