/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef KBIBTEX_GUI_ELEMENTWIDGETS_H
#define KBIBTEX_GUI_ELEMENTWIDGETS_H

#include "kbibtexgui_export.h"

#include <QLabel>
#include <QWidget>

#include <QUrl>
#include <QIcon>

#include "elementeditor.h"
#include "entrylayout.h"

class QTreeWidget;
class QTreeWidgetItem;
class QGridLayout;

class KLineEdit;
class KComboBox;
class QPushButton;

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
    virtual bool validate(QWidget **widgetWithIssue, QString &message) const = 0;
    virtual void setReadOnly(bool isReadOnly) {
        this->isReadOnly = isReadOnly;
    }
    virtual void showReqOptWidgets(bool, const QString &) = 0;
    virtual QString label() = 0;
    virtual QIcon icon() = 0;
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
    Q_OBJECT

private:
    typedef struct {
        QLabel *label;
        FieldInput *fieldInput;
        bool isVerticallyMinimumExpaning;
    } LabeledFieldInput;
    LabeledFieldInput **listOfLabeledFieldInput;
    const int fieldInputCount, numCols;
    QGridLayout *gridLayout;

    const QSharedPointer<const EntryTabLayout> etl;
    QMap<QString, FieldInput *> bibtexKeyToWidget;

    void createGUI();
    void layoutGUI(bool forceVisible, const QString &entryType = QString());

public:
    EntryConfiguredWidget(const QSharedPointer<const EntryTabLayout> &entryTabLayout, QWidget *parent);
    ~EntryConfiguredWidget() override;

    bool apply(QSharedPointer<Element> element) const override;
    bool reset(QSharedPointer<const Element> element) override;
    bool validate(QWidget **widgetWithIssue, QString &message) const override;
    void setReadOnly(bool isReadOnly) override;
    void showReqOptWidgets(bool forceVisible, const QString &entryType) override;
    QString label() override;
    QIcon icon() override;

    void setFile(const File *file) override;

    bool canEdit(const Element *element) override;
};

class ReferenceWidget : public ElementWidget
{
    Q_OBJECT

private:
    KComboBox *entryType;
    KLineEdit *entryId;
    QPushButton *buttonSuggestId;

    void createGUI();

public:
    explicit ReferenceWidget(QWidget *parent);

    bool apply(QSharedPointer<Element> element) const override;
    bool reset(QSharedPointer<const Element> element) override;
    bool validate(QWidget **widgetWithIssue, QString &message) const override;
    void setReadOnly(bool isReadOnly) override;
    void showReqOptWidgets(bool, const QString &) override {}
    void setApplyElementInterface(ElementEditor::ApplyElementInterface *applyElement) {
        m_applyElement = applyElement;
    }

    void setOriginalElement(const QSharedPointer<Element> &orig);

    /**
     * Return the current value of the entry id/macro key editing widget.
     *
     * @return Current value of entry id/macro key if any, otherwise empty string
     */
    QString currentId() const;
    void setCurrentId(const QString &newId);

    QString label() override;
    QIcon icon() override;

    bool canEdit(const Element *element) override;

public slots:
    void setEntryIdByDefault();

private:
    ElementEditor::ApplyElementInterface *m_applyElement;
    bool m_entryIdManuallySet;
    QSharedPointer<Element> m_element;

    QString computeType() const;

private slots:
    void prepareSuggestionsMenu();
    void insertSuggestionFromAction();
    void entryIdManuallyChanged();

signals:
    void entryTypeChanged();
};

class FilesWidget : public ElementWidget
{
    Q_OBJECT

private:
    FieldInput *fileList;

public:
    explicit FilesWidget(QWidget *parent);

    bool apply(QSharedPointer<Element> element) const override;
    bool reset(QSharedPointer<const Element> element) override;
    bool validate(QWidget **widgetWithIssue, QString &message) const override;
    void setReadOnly(bool isReadOnly) override;
    void showReqOptWidgets(bool, const QString &) override {}

    QString label() override;
    QIcon icon() override;

    void setFile(const File *file) override;

    bool canEdit(const Element *element) override;

private:
    static const QStringList keyStart;
};

class OtherFieldsWidget : public ElementWidget
{
    Q_OBJECT

private:
    KLineEdit *fieldName;
    FieldInput *fieldContent;
    QTreeWidget *otherFieldsList;
    QPushButton *buttonDelete;
    QPushButton *buttonOpen;
    QPushButton *buttonAddApply;
    QUrl currentUrl;
    const QStringList blackListed;
    QSharedPointer<Entry> internalEntry;
    QStringList deletedKeys, modifiedKeys;
    bool m_isReadOnly;

    void createGUI();
    void updateList();

public:
    OtherFieldsWidget(const QStringList &blacklistedFields, QWidget *parent);
    ~OtherFieldsWidget() override;

    bool apply(QSharedPointer<Element> element) const override;
    bool reset(QSharedPointer<const Element> element) override;
    bool validate(QWidget **widgetWithIssue, QString &message) const override;
    void setReadOnly(bool isReadOnly) override;
    void showReqOptWidgets(bool, const QString &) override {}
    QString label() override;
    QIcon icon() override;

    bool canEdit(const Element *element) override;

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
    Q_OBJECT

private:
    FieldInput *fieldInputValue;

    void createGUI();

public:
    explicit MacroWidget(QWidget *parent);
    ~MacroWidget() override;

    bool apply(QSharedPointer<Element> element) const override;
    bool reset(QSharedPointer<const Element> element) override;
    bool validate(QWidget **widgetWithIssue, QString &message) const override;
    void setReadOnly(bool isReadOnly) override;
    void showReqOptWidgets(bool, const QString &) override {}
    QString label() override;
    QIcon icon() override;

    bool canEdit(const Element *element) override;
};

class PreambleWidget : public ElementWidget
{
    Q_OBJECT

private:
    FieldInput *fieldInputValue;

    void createGUI();

public:
    explicit PreambleWidget(QWidget *parent);

    bool apply(QSharedPointer<Element> element) const override;
    bool reset(QSharedPointer<const Element> element) override;
    bool validate(QWidget **widgetWithIssue, QString &message) const override;
    void setReadOnly(bool isReadOnly) override;
    void showReqOptWidgets(bool, const QString &) override {}
    QString label() override;
    QIcon icon() override;

    bool canEdit(const Element *element) override;
};

class SourceWidget : public ElementWidget
{
    Q_OBJECT

public:
    enum ElementClass { elementInvalid = -1, elementEntry = 0, elementMacro, elementPreamble };

private:
    class SourceWidgetTextEdit;
    SourceWidgetTextEdit *sourceEdit;
    QString originalText;
    ElementClass elementClass;

    void createGUI();

public:
    explicit SourceWidget(QWidget *parent);
    ~SourceWidget() override;

    void setElementClass(ElementClass elementClass);
    bool apply(QSharedPointer<Element> element) const override;
    bool reset(QSharedPointer<const Element> element) override;
    bool validate(QWidget **widgetWithIssue, QString &message) const override;
    void setReadOnly(bool isReadOnly) override;
    void showReqOptWidgets(bool, const QString &) override {}
    QString label() override;
    QIcon icon() override;

    bool canEdit(const Element *element) override;

private slots:
    void reset();

private:
    class Private;
    Private *const d;
};

#endif // KBIBTEX_GUI_ELEMENTWIDGETS_H
