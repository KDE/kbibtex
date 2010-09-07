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

#ifndef KBIBTEX_GUI_DIALOGS_ELEMENTSWIDGETS_H
#define KBIBTEX_GUI_DIALOGS_ELEMENTSWIDGETS_H

#include <kbibtexgui_export.h>

#include <QWidget>

#include <KUrl>
#include <KIcon>

#include <entrylayout.h>

class QTextEdit;
class QTreeWidget;
class QTreeWidgetItem;

class KLineEdit;
class KComboBox;
class KPushButton;

class Element;
class FieldInput;

class ElementWidget : public QWidget
{
public:
    ElementWidget(QWidget *parent): QWidget(parent) {};
    virtual bool apply(Element *element) const = 0;
    virtual bool reset(const Element *element) = 0;
    virtual void setReadOnly(bool isReadOnly) = 0;
    virtual QString label() = 0;
    virtual KIcon icon() = 0;

    static bool canEdit(const Element *element) {
        Q_UNUSED(element)
        return false;
    };
};

class EntryConfiguredWidget : public ElementWidget
{
private:
    EntryTabLayout &etl;
    QMap<QString, FieldInput*> bibtexKeyToWidget;
    void createGUI();

public:
    EntryConfiguredWidget(EntryTabLayout &entryTabLayout, QWidget *parent);

    bool apply(Element *element) const;
    bool reset(const Element *element);
    void setReadOnly(bool isReadOnly);
    QString label();
    KIcon icon();

    static bool canEdit(const Element *element);
};

class ReferenceWidget : public ElementWidget
{
private:
    KComboBox *entryType;
    KLineEdit *entryId;
    void createGUI();

public:
    ReferenceWidget(QWidget *parent);

    bool apply(Element *element) const;
    bool reset(const Element *element);
    void setReadOnly(bool isReadOnly);
    QString label();
    KIcon icon();

    static bool canEdit(const Element *element);
};

class OtherFieldsWidget : public ElementWidget
{
    Q_OBJECT

private:
    KLineEdit *otherFieldsName;
    FieldInput *otherFieldsContent;
    QTreeWidget *otherFieldsList;
    KPushButton *buttonDelete;
    KPushButton *buttonOpen;
    KPushButton *buttonAddApply;
    KUrl currentUrl;
    QStringList blackListed;
    Entry *internalEntry;
    QStringList deletedKeys;

    void createGUI();
    void updateList();

public:
    OtherFieldsWidget(QWidget *parent);
    ~OtherFieldsWidget();

    bool apply(Element *element) const;
    bool reset(const Element *element);
    void setReadOnly(bool isReadOnly);
    QString label();
    KIcon icon();

    static bool canEdit(const Element *element);

private slots:
    void listElementExecuted(QTreeWidgetItem *item, int column);
    void listCurrentChanged(QTreeWidgetItem *item, QTreeWidgetItem *previous);
    void actionAddApply();
    void actionDelete();
    void actionOpen();
    void updateGUI();
};

class MacroWidget : public ElementWidget
{
private:
    FieldInput *fieldInputValue;

    void createGUI();

public:
    MacroWidget(QWidget *parent);

    bool apply(Element *element) const;
    bool reset(const Element *element);
    void setReadOnly(bool isReadOnly);
    QString label();
    KIcon icon();

    static bool canEdit(const Element *element);
};

class PreambleWidget : public ElementWidget
{
private:
    FieldInput *fieldInputValue;

    void createGUI();

public:
    PreambleWidget(QWidget *parent);

    bool apply(Element *element) const;
    bool reset(const Element *element);
    void setReadOnly(bool isReadOnly);
    QString label();
    KIcon icon();

    static bool canEdit(const Element *element);
};

class SourceWidget : public ElementWidget
{
    Q_OBJECT

private:
    QTextEdit *sourceEdit;
    QString originalText;

    void createGUI();

public:
    SourceWidget(QWidget *parent);

    bool apply(Element *element) const;
    bool reset(const Element *element);
    void setReadOnly(bool isReadOnly);
    QString label();
    KIcon icon();

    static bool canEdit(const Element *element);

private slots:
    void reset();
};

#endif // KBIBTEX_GUI_DIALOGS_ELEMENTSWIDGETS_H
