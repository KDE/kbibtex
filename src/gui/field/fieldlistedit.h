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
#ifndef KBIBTEX_GUI_FIELDLISTEDIT_H
#define KBIBTEX_GUI_FIELDLISTEDIT_H

#include <QWidget>
#include <QSet>

#include <KSharedConfig>

#include "kbibtexnamespace.h"
#include "value.h"

class QCheckBox;
class QDropEvent;
class QDragEnterEvent;
class QSignalMapper;

class KPushButton;

class Element;
class FieldLineEdit;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class FieldListEdit : public QWidget
{
    Q_OBJECT

public:
    FieldListEdit(KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, QWidget *parent = NULL);
    ~FieldListEdit();

    virtual bool reset(const Value &value);
    virtual bool apply(Value &value) const;

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
    void addButton(KPushButton *button);
    void lineAdd(Value *value);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *);

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
    PersonListEdit(KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, QWidget *parent = NULL);

    virtual bool reset(const Value &value);
    virtual bool apply(Value &value) const;

    virtual void setReadOnly(bool isReadOnly);

private slots:
    void slotAddNamesFromClipboard();

private:
    QCheckBox *m_checkBoxOthers;
    KPushButton *m_buttonAddNamesFromClipboard;
};


/**
@author Thomas Fischer
 */
class UrlListEdit : public FieldListEdit
{
    Q_OBJECT

public:
    explicit UrlListEdit(QWidget *parent = NULL);
    ~UrlListEdit();

    virtual void setReadOnly(bool isReadOnly);

    static QString askRelativeOrStaticFilename(QWidget *parent, const QString &filename, const QUrl &baseUrl);

    /// Own function as QUrl's isLocalFile is not reliable
    static bool urlIsLocal(const QUrl &url);

protected:
    virtual FieldLineEdit *addFieldLineEdit();

private slots:
    void slotAddReference();
    void slotAddReferenceFromClipboard();
    /// Slot for events where the "save locally" button is triggered
    void slotSaveLocally(QWidget *widget);
    /// Catch events where the line edit's text change
    void textChanged(QWidget *widget);

private:
    KPushButton *m_buttonAddFile;
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

    explicit KeywordListEdit(QWidget *parent = NULL);

    virtual void setReadOnly(bool isReadOnly);
    virtual void setFile(const File *file);
    virtual void setCompletionItems(const QStringList &items);

private slots:
    void slotAddKeywordsFromList();
    void slotAddKeywordsFromClipboard();

private:
    KSharedConfigPtr m_config;
    const QString m_configGroupName;
    KPushButton *m_buttonAddKeywordsFromList, *m_buttonAddKeywordsFromClipboard;
    QSet<QString> m_keywordsFromFile;
};

#endif // KBIBTEX_GUI_FIELDLISTEDIT_H
