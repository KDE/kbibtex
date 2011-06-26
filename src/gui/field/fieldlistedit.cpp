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

#include <typeinfo>

#include <QScrollArea>
#include <QLayout>
#include <QSignalMapper>
#include <QCheckBox>

#include <KMessageBox>
#include <KLocale>
#include <KPushButton>
#include <KFileDialog>
#include <KInputDialog>

#include <file.h>
#include <entry.h>
#include <fileimporterbibtex.h>
#include <fileexporterbibtex.h>
#include <fieldlineedit.h>
#include "fieldlistedit.h"

class FieldListEdit::FieldListEditProtected
{
private:
    FieldListEdit *p;
    const int innerSpacing;
    QSignalMapper *smRemove, *smGoUp, *smGoDown;
    QVBoxLayout *layout;
    KBibTeX::TypeFlag preferredTypeFlag;
    KBibTeX::TypeFlags typeFlags;

public:
    QList<FieldLineEdit*> lineEditList;
    QWidget *pushButtonContainer;
    QBoxLayout *pushButtonContainerLayout;
    KPushButton *addLineButton;
    const File *file;
    QWidget *container;
    QScrollArea *scrollArea;
    bool m_isReadOnly;

    FieldListEditProtected(KBibTeX::TypeFlag ptf, KBibTeX::TypeFlags tf, FieldListEdit *parent)
            : p(parent), innerSpacing(4), preferredTypeFlag(ptf), typeFlags(tf), file(NULL), m_isReadOnly(false) {
        smRemove = new QSignalMapper(parent);
        smGoUp = new QSignalMapper(parent);
        smGoDown = new QSignalMapper(parent);
        setupGUI();
    }

    void setupGUI() {
        QBoxLayout *outerLayout = new QVBoxLayout(p);
        outerLayout->setMargin(0);
        outerLayout->setSpacing(0);
        scrollArea = new QScrollArea(p);
        outerLayout->addWidget(scrollArea);

        container = new QWidget(scrollArea->viewport());
        container->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        scrollArea->setWidget(container);
        layout = new QVBoxLayout(container);
        layout->setMargin(0);
        layout->setSpacing(innerSpacing);

        pushButtonContainer = new QWidget(container);
        pushButtonContainerLayout = new QHBoxLayout(pushButtonContainer);
        pushButtonContainerLayout->setMargin(0);
        layout->addWidget(pushButtonContainer);

        addLineButton = new KPushButton(KIcon("list-add"), i18n("Add"), pushButtonContainer);
        addLineButton->setObjectName(QLatin1String("addButton"));
        connect(addLineButton, SIGNAL(clicked()), p, SLOT(lineAdd()));
        connect(addLineButton, SIGNAL(clicked()), p, SIGNAL(modified()));
        pushButtonContainerLayout->addWidget(addLineButton);

        layout->addStretch(100);

        connect(smRemove, SIGNAL(mapped(QWidget*)), p, SLOT(lineRemove(QWidget*)));
        connect(smRemove, SIGNAL(mapped(QWidget*)), p, SIGNAL(modified()));
        connect(smGoDown, SIGNAL(mapped(QWidget*)), p, SLOT(lineGoDown(QWidget*)));
        connect(smGoDown, SIGNAL(mapped(QWidget*)), p, SIGNAL(modified()));
        connect(smGoUp, SIGNAL(mapped(QWidget*)), p, SLOT(lineGoUp(QWidget*)));
        connect(smGoDown, SIGNAL(mapped(QWidget*)), p, SIGNAL(modified()));

        scrollArea->setBackgroundRole(QPalette::Base);
        scrollArea->ensureWidgetVisible(container);
        scrollArea->setWidgetResizable(true);
    }

    void addButton(KPushButton *button) {
        button->setParent(pushButtonContainer);
        pushButtonContainerLayout->addWidget(button);
    }

    int recommendedHeight() {
        int heightHint = 0;

        for (QList<FieldLineEdit*>::ConstIterator it = lineEditList.constBegin();it != lineEditList.constEnd(); ++it)
            heightHint += (*it)->sizeHint().height();

        heightHint += lineEditList.count() * innerSpacing;
        heightHint += addLineButton->sizeHint().height();

        return heightHint;
    }

    FieldLineEdit *addFieldLineEdit() {
        FieldLineEdit *le = new FieldLineEdit(preferredTypeFlag, typeFlags, false, container);
        le->setFile(file);
        le->setReadOnly(m_isReadOnly);
        le->setInnerWidgetsTransparency(true);
        layout->insertWidget(layout->count() - 2, le);
        lineEditList.append(le);

        KPushButton *remove = new KPushButton(KIcon("list-remove"), QLatin1String(""), le);
        remove->setToolTip(i18n("Remove value"));
        le->appendWidget(remove);
        connect(remove, SIGNAL(clicked()), smRemove, SLOT(map()));
        smRemove->setMapping(remove, le);

        KPushButton *goDown = new KPushButton(KIcon("go-down"), QLatin1String(""), le);
        goDown->setToolTip(i18n("Move value down"));
        le->appendWidget(goDown);
        connect(goDown, SIGNAL(clicked()), smGoDown, SLOT(map()));
        smGoDown->setMapping(goDown, le);

        KPushButton *goUp = new KPushButton(KIcon("go-up"), QLatin1String(""), le);
        goUp->setToolTip(i18n("Move value up"));
        le->appendWidget(goUp);
        connect(goUp, SIGNAL(clicked()), smGoUp, SLOT(map()));
        smGoUp->setMapping(goUp, le);

        return le;
    }

    void removeAllFieldLineEdits() {
        while (!lineEditList.isEmpty()) {
            FieldLineEdit *fieldLineEdit = *lineEditList.begin();
            layout->removeWidget(fieldLineEdit);
            lineEditList.removeFirst();
            delete fieldLineEdit;
        }

        /// This fixes a layout problem where the container element
        /// does not shrink correctly once the line edits have been
        /// removed
        QSize pSize = container->size();
        pSize.setHeight(addLineButton->height());
        container->resize(pSize);
    }

    void removeFieldLineEdit(FieldLineEdit *fieldLineEdit) {
        lineEditList.removeOne(fieldLineEdit);
        layout->removeWidget(fieldLineEdit);
        delete fieldLineEdit;
    }

    void goDownFieldLineEdit(FieldLineEdit *fieldLineEdit) {
        int idx = lineEditList.indexOf(fieldLineEdit);
        if (idx < lineEditList.count() - lineEditList.size()) {
            layout->removeWidget(fieldLineEdit);
            lineEditList.removeOne(fieldLineEdit);
            lineEditList.insert(idx + 1, fieldLineEdit);
            layout->insertWidget(idx + 1, fieldLineEdit);
        }
    }

    void goUpFieldLineEdit(FieldLineEdit *fieldLineEdit) {
        int idx = lineEditList.indexOf(fieldLineEdit);
        if (idx > 0) {
            layout->removeWidget(fieldLineEdit);
            lineEditList.removeOne(fieldLineEdit);
            lineEditList.insert(idx - 1, fieldLineEdit);
            layout->insertWidget(idx - 1, fieldLineEdit);
        }
    }
};

FieldListEdit::FieldListEdit(KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, QWidget *parent)
        : QWidget(parent), d(new FieldListEditProtected(preferredTypeFlag, typeFlags, this))
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

bool FieldListEdit::reset(const Value& value)
{
    d->removeAllFieldLineEdits();
    for (Value::ConstIterator it = value.constBegin(); it != value.constEnd(); ++it) {
        Value v;
        v.append(*it);
        FieldLineEdit *fieldLineEdit = d->addFieldLineEdit();
        fieldLineEdit->setFile(d->file);
        fieldLineEdit->reset(v);
    }
    QSize size(d->container->width(), d->recommendedHeight());
    d->container->resize(size);

    return true;
}

bool FieldListEdit::apply(Value& value) const
{
    value.clear();

    for (QList<FieldLineEdit*>::ConstIterator it = d->lineEditList.constBegin(); it != d->lineEditList.constEnd(); ++it) {
        Value v;
        (*it)->apply(v);
        for (Value::ConstIterator itv = v.constBegin(); itv != v.constEnd(); ++itv)
            value.append(*itv);
    }

    return true;
}

void FieldListEdit::clear()
{
    d->removeAllFieldLineEdits();
}

void FieldListEdit::setReadOnly(bool isReadOnly)
{
    d->m_isReadOnly = isReadOnly;
    for (QList<FieldLineEdit*>::ConstIterator it = d->lineEditList.constBegin(); it != d->lineEditList.constEnd(); ++it)
        (*it)->setReadOnly(isReadOnly);
    d->addLineButton->setEnabled(!isReadOnly);
}

void FieldListEdit::setFile(const File *file)
{
    d->file = file;
}

void FieldListEdit::addButton(KPushButton *button)
{
    d->addButton(button);
}

void FieldListEdit::lineAdd(Value *value)
{
    FieldLineEdit *le = d->addFieldLineEdit();
    if (value != NULL)
        le->reset(*value);
}

void FieldListEdit::lineAdd()
{
    FieldLineEdit *newEdit = d->addFieldLineEdit();
    QSize size(d->container->width(), d->recommendedHeight());
    d->container->resize(size);
    newEdit->setFocus(Qt::ShortcutFocusReason);
}

void FieldListEdit::lineRemove(QWidget * widget)
{
    FieldLineEdit *fieldLineEdit = static_cast<FieldLineEdit*>(widget);
    d->removeFieldLineEdit(fieldLineEdit);
    QSize size(d->container->width(), d->recommendedHeight());
    d->container->resize(size);
}

void FieldListEdit::lineGoDown(QWidget * widget)
{
    FieldLineEdit *fieldLineEdit = static_cast<FieldLineEdit*>(widget);
    d->goDownFieldLineEdit(fieldLineEdit);
}

void FieldListEdit::lineGoUp(QWidget * widget)
{
    FieldLineEdit *fieldLineEdit = static_cast<FieldLineEdit*>(widget);
    d->goUpFieldLineEdit(fieldLineEdit);

}

PersonListEdit::PersonListEdit(KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, QWidget *parent)
        : FieldListEdit(preferredTypeFlag, typeFlags, parent)
{
    m_checkBoxOthers = new QCheckBox(i18n("... and others (et al.)"), this);
    QBoxLayout *boxLayout = static_cast<QBoxLayout *>(layout());
    boxLayout->addWidget(m_checkBoxOthers);
}

bool PersonListEdit::reset(const Value& value)
{
    Value internal = value;

    m_checkBoxOthers->setCheckState(Qt::Unchecked);
    if (!internal.isEmpty() && typeid(PlainText) == typeid(*internal.last())) {
        PlainText *pt = static_cast<PlainText*>(internal.last());
        if (pt->text() == QLatin1String("others")) {
            internal.removeLast();
            m_checkBoxOthers->setCheckState(Qt::Checked);
        }
    }

    return FieldListEdit::reset(internal);
}

bool PersonListEdit::apply(Value& value) const
{
    bool result = FieldListEdit::apply(value);

    if (result && m_checkBoxOthers->checkState() == Qt::Checked)
        value.append(new PlainText(QLatin1String("others")));

    return result;
}

void PersonListEdit::setReadOnly(bool isReadOnly)
{
    FieldListEdit::setReadOnly(isReadOnly);
    m_checkBoxOthers->setEnabled(!isReadOnly);
}


UrlListEdit::UrlListEdit(QWidget *parent)
        : FieldListEdit(KBibTeX::tfVerbatim, KBibTeX::tfVerbatim, parent)
{
    m_addLocalFile = new KPushButton(KIcon("document-new"), i18n("Add local file"), this);
    addButton(m_addLocalFile);
    connect(m_addLocalFile, SIGNAL(clicked()), this, SLOT(slotAddLocalFile()));
    connect(m_addLocalFile, SIGNAL(clicked()), this, SIGNAL(modified()));
}

void UrlListEdit::slotAddLocalFile()
{
    KUrl fileUrl(d->file != NULL ? d->file->property(File::Url, QVariant()).value<KUrl>() : KUrl());
    QFileInfo fileUrlInfo = fileUrl.isEmpty() ? QFileInfo() : QFileInfo(fileUrl.path());

    QString filename = KFileDialog::getOpenFileName(KUrl(fileUrlInfo.absolutePath()), QString(), this, i18n("Add Local File"));
    if (!filename.isEmpty()) {
        QFileInfo filenameInfo(filename);
        if (!fileUrl.isEmpty() && (filenameInfo.absolutePath() == fileUrlInfo.absolutePath() || filenameInfo.absolutePath().startsWith(fileUrlInfo.absolutePath() + QDir::separator()))) {
            QString relativePath = filenameInfo.absolutePath().mid(fileUrlInfo.absolutePath().length() + 1);
            QString relativeFilename = relativePath + (relativePath.isEmpty() ? QLatin1String("") : QString(QDir::separator())) + filenameInfo.fileName();
            if (KMessageBox::questionYesNo(this, i18n("<qt><p>Use a path relative to the bibliography file?</p><p>The relative path would be<br/><tt>%1</tt></p>", relativeFilename), i18n("Relative Path"), KGuiItem(i18n("Relative Path")), KGuiItem(i18n("Absolute Path"))) == KMessageBox::Yes)
                filename = relativeFilename;
        }

        Value *value = new Value();
        ValueItem *vi = new VerbatimText(filename);
        value->append(vi);
        lineAdd(value);
    }
}

void UrlListEdit::setReadOnly(bool isReadOnly)
{
    FieldListEdit::setReadOnly(isReadOnly);
    m_addLocalFile->setEnabled(!isReadOnly);
}


const QString KeywordListEdit::keyGlobalKeywordList = QLatin1String("globalKeywordList");

KeywordListEdit::KeywordListEdit(QWidget *parent)
        : FieldListEdit(KBibTeX::tfKeyword, KBibTeX::tfKeyword | KBibTeX::tfSource, parent), m_config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), m_configGroupName(QLatin1String("Global Keywords"))
{
    m_addKeyword = new KPushButton(KIcon("list-add"), i18n("Add Keyword"), this);
    addButton(m_addKeyword);
    connect(m_addKeyword, SIGNAL(clicked()), this, SLOT(slotAddKeyword()));
    connect(m_addKeyword, SIGNAL(clicked()), this, SIGNAL(modified()));
}

void KeywordListEdit::slotAddKeyword()
{
    KConfigGroup configGroup(m_config, m_configGroupName);
    QStringList keywordList = configGroup.readEntry(KeywordListEdit::keyGlobalKeywordList, QStringList());

    bool ok = false;
    QStringList newKeywordList = KInputDialog::getItemList(i18n("Keywords to Add"), i18n("Select keywords to add:"), keywordList, QStringList(), true, &ok, this);
    if (ok) {
        foreach(const QString &newKeywordText, newKeywordList) {
            Value *value = new Value();
            ValueItem *vi = new Keyword(newKeywordText);
            value->append(vi);
            lineAdd(value);
        }
    }
}

void KeywordListEdit::setReadOnly(bool isReadOnly)
{
    FieldListEdit::setReadOnly(isReadOnly);
    m_addKeyword->setEnabled(!isReadOnly);
}
