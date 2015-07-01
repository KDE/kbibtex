/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#ifndef KBIBTEX_GUI_ELEMENTWIDGETS_H
#define KBIBTEX_GUI_ELEMENTWIDGETS_H

#include "kbibtexgui_export.h"

#include <QLabel>
#include <QWidget>

#include <KUrl>
#include <KIcon>

#include "elementeditor.h"
#include "entrylayout.h"

class QTreeWidget;
class QTreeWidgetItem;
class QGridLayout;

class KLineEdit;
class KComboBox;
class KPushButton;

class File;
class Entry;
class Element;
class FieldInput;

class ElementWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ElementWidget(QWidget *parent);
    virtual bool apply(QSharedPointer<Element> element) const = 0;
    virtual bool reset(QSharedPointer<const Element> element) = 0;
    virtual void setReadOnly(bool isReadOnly) {
        this->isReadOnly = isReadOnly;
    }
    virtual void showReqOptWidgets(bool, const QString &) = 0;
    virtual QString label() = 0;
    virtual KIcon icon() = 0;
    bool isModified() const;
    void setModified(bool);

    virtual void setFile(const File *file) {
        m_file = file;
    }

    virtual bool canEdit(const Element *) = 0;

protected:
    bool isReadOnly;
    const File *m_file;

protected slots:
    void gotModified();

private:
    bool m_isModified;

signals:
    void modified(bool);
};

class EntryConfiguredWidget : public ElementWidget
{
private:
    typedef struct {
        QLabel *label;
        FieldInput *fieldInput;
        bool isVerticallyMinimumExpaning;
    } LabeledFieldInput;
    LabeledFieldInput **listOfLabeledFieldInput;
    int fieldInputCount, numCols;
    QGridLayout *gridLayout;

    QSharedPointer<EntryTabLayout> etl;
    QMap<QString, FieldInput *> bibtexKeyToWidget;

    void createGUI();
    void layoutGUI(bool forceVisible, const QString &entryType = QString());

public:
    EntryConfiguredWidget(QSharedPointer<EntryTabLayout> &entryTabLayout, QWidget *parent);
    virtual ~EntryConfiguredWidget();

    bool apply(QSharedPointer<Element> element) const;
    bool reset(QSharedPointer<const Element> element);
    void setReadOnly(bool isReadOnly);
    void showReqOptWidgets(bool forceVisible, const QString &entryType);
    QString label();
    KIcon icon();

    virtual void setFile(const File *file);

    bool canEdit(const Element *element);
};

class ReferenceWidget : public ElementWidget
{
    Q_OBJECT

private:
    KComboBox *entryType;
    KLineEdit *entryId;
    KPushButton *buttonSuggestId;

    void createGUI();

public:
    explicit ReferenceWidget(QWidget *parent);

    bool apply(QSharedPointer<Element> element) const;
    bool reset(QSharedPointer<const Element> element);
    void setReadOnly(bool isReadOnly);
    void showReqOptWidgets(bool, const QString &) {}
    void setApplyElementInterface(ElementEditor::ApplyElementInterface *applyElement) {
        m_applyElement = applyElement;
    }

    void setOriginalElement(const QSharedPointer<Element> &orig);
    bool isDuplicateId() const;

    QString label();
    KIcon icon();

    bool canEdit(const Element *element);

public slots:
    void setEntryIdByDefault();

private:
    ElementEditor::ApplyElementInterface *m_applyElement;
    bool m_entryIdManuallySet;
    QSharedPointer<Element> m_element;

private slots:
    void prepareSuggestionsMenu();
    void insertSuggestionFromAction();
    void entryIdManuallyChanged();

signals:
    void entryTypeChanged();
};

class FilesWidget : public ElementWidget
{
private:
    FieldInput *fileList;

public:
    explicit FilesWidget(QWidget *parent);

    bool apply(QSharedPointer<Element> element) const;
    bool reset(QSharedPointer<const Element> element);
    void setReadOnly(bool isReadOnly);
    void showReqOptWidgets(bool, const QString &) {}

    QString label();
    KIcon icon();

    virtual void setFile(const File *file);

    bool canEdit(const Element *element);
};

class OtherFieldsWidget : public ElementWidget
{
    Q_OBJECT

private:
    KLineEdit *fieldName;
    FieldInput *fieldContent;
    QTreeWidget *otherFieldsList;
    KPushButton *buttonDelete;
    KPushButton *buttonOpen;
    KPushButton *buttonAddApply;
    KUrl currentUrl;
    const QStringList blackListed;
    QSharedPointer<Entry> internalEntry;
    QStringList deletedKeys, modifiedKeys;
    bool m_isReadOnly;

    void createGUI();
    void updateList();

public:
    OtherFieldsWidget(const QStringList &blacklistedFields, QWidget *parent);
    ~OtherFieldsWidget();

    bool apply(QSharedPointer<Element> element) const;
    bool reset(QSharedPointer<const Element> element);
    void setReadOnly(bool isReadOnly);
    void showReqOptWidgets(bool, const QString &) {}
    QString label();
    KIcon icon();

    bool canEdit(const Element *element);

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
    explicit MacroWidget(QWidget *parent);
    ~MacroWidget();

    bool apply(QSharedPointer<Element> element) const;
    bool reset(QSharedPointer<const Element> element);
    void setReadOnly(bool isReadOnly);
    void showReqOptWidgets(bool, const QString &) {}
    QString label();
    KIcon icon();

    bool canEdit(const Element *element);
};

class PreambleWidget : public ElementWidget
{
private:
    FieldInput *fieldInputValue;

    void createGUI();

public:
    explicit PreambleWidget(QWidget *parent);

    bool apply(QSharedPointer<Element> element) const;
    bool reset(QSharedPointer<const Element> element);
    void setReadOnly(bool isReadOnly);
    void showReqOptWidgets(bool, const QString &) {}
    QString label();
    KIcon icon();

    bool canEdit(const Element *element);
};

class SourceWidget : public ElementWidget
{
    Q_OBJECT

private:
    class SourceWidgetTextEdit;
    SourceWidgetTextEdit *sourceEdit;
    QString originalText;

    void createGUI();

public:
    explicit SourceWidget(QWidget *parent);
    ~SourceWidget();

    bool apply(QSharedPointer<Element> element) const;
    bool reset(QSharedPointer<const Element> element);
    void setReadOnly(bool isReadOnly);
    void showReqOptWidgets(bool, const QString &) {}
    QString label();
    KIcon icon();

    bool canEdit(const Element *element);

private slots:
    void reset();

private:
    KPushButton *m_buttonRestore;
};

#endif // KBIBTEX_GUI_ELEMENTWIDGETS_H
