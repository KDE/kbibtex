/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include <KBibTeX>
#include <Value>

class QCheckBox;
class QDropEvent;
class QDragEnterEvent;
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

protected:
    FieldLineEdit *addFieldLineEdit() override;

private slots:
    void slotAddReference();
    void slotAddReferenceFromClipboard();
    void downloadFinished(KJob *);

private:
    QPushButton *m_buttonAddFile;

    void addReference(const QUrl &url);
    void downloadAndSaveLocally(const QUrl &url);
    void textChanged(QPushButton *buttonSaveLocally, FieldLineEdit *fieldLineEdit);
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
