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
#ifndef KBIBTEX_GUI_FIELDLISTEDIT_H
#define KBIBTEX_GUI_FIELDLISTEDIT_H

#include <QWidget>
#include <QSet>

#include <KSharedConfig>

#include "kbibtex.h"
#include "value.h"

class QCheckBox;
class QDropEvent;
class QDragEnterEvent;
class QSignalMapper;
class QPushButton;

class KJob;
class KJob;

class Element;
class FieldLineEdit;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class FieldListEdit : public QWidget
{
    Q_OBJECT

public:
    FieldListEdit(KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, QWidget *parent = nullptr);
    ~FieldListEdit() override;

    virtual bool reset(const Value &value);
    virtual bool apply(Value &value) const;
    virtual bool validate(QWidget **widgetWithIssue, QString &message) const;

    void clear();
    virtual void setReadOnly(bool isReadOnly);
    virtual void setFile(const File *file);
    virtual void setElement(const Element *element);
    virtual void setFieldKey(const QString &fieldKey);
    virtual void setCompletionItems(const QStringList &items);

signals:
    void modified();

protected:
    /// Add a new field line edit to this list
    /// Allows to get overwritten by descentants of this class
    virtual FieldLineEdit *addFieldLineEdit();
    void addButton(QPushButton *button);
    void lineAdd(Value *value);
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *) override;

    const Element *m_element;

private slots:
    void lineAdd();
    void lineRemove(QWidget *widget);
    void lineGoDown(QWidget *widget);
    void lineGoUp(QWidget *widget);

protected:
    class FieldListEditProtected;
    FieldListEditProtected *d;
};


/**
@author Thomas Fischer
 */
class PersonListEdit : public FieldListEdit
{
    Q_OBJECT

public:
    PersonListEdit(KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, QWidget *parent = nullptr);

    bool reset(const Value &value) override;
    bool apply(Value &value) const override;

    void setReadOnly(bool isReadOnly) override;

private slots:
    void slotAddNamesFromClipboard();

private:
    QCheckBox *m_checkBoxOthers;
    QPushButton *m_buttonAddNamesFromClipboard;
};


/**
@author Thomas Fischer
 */
class UrlListEdit : public FieldListEdit
{
    Q_OBJECT

public:
    explicit UrlListEdit(QWidget *parent = nullptr);

    void setReadOnly(bool isReadOnly) override;

    static QString askRelativeOrStaticFilename(QWidget *parent, const QString &filename, const QUrl &baseUrl);

    /// Own function as QUrl's isLocalFile is not reliable
    static bool urlIsLocal(const QUrl &url);

protected:
    FieldLineEdit *addFieldLineEdit() override;

private slots:
    void slotAddReference();
    void slotAddReferenceFromClipboard();
    /// Slot for events where the "save locally" button is triggered
    void slotSaveLocally(QWidget *widget);
    void downloadFinished(KJob *);
    /// Catch events where the line edit's text change
    void textChanged(QWidget *widget);

private:
    QPushButton *m_buttonAddFile;
    QSignalMapper *m_signalMapperSaveLocallyButtonClicked;
    QSignalMapper *m_signalMapperFieldLineEditTextChanged;

    void addReference(const QUrl &url);
};


/**
@author Thomas Fischer
 */
class KeywordListEdit : public FieldListEdit
{
    Q_OBJECT

public:
    static const QString keyGlobalKeywordList;

    explicit KeywordListEdit(QWidget *parent = nullptr);

    void setReadOnly(bool isReadOnly) override;
    void setFile(const File *file) override;
    void setCompletionItems(const QStringList &items) override;

private slots:
    void slotAddKeywordsFromList();
    void slotAddKeywordsFromClipboard();

private:
    KSharedConfigPtr m_config;
    const QString m_configGroupName;
    QPushButton *m_buttonAddKeywordsFromList, *m_buttonAddKeywordsFromClipboard;
    QSet<QString> m_keywordsFromFile;
};

#endif // KBIBTEX_GUI_FIELDLISTEDIT_H
