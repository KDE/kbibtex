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
#ifndef KBIBTEX_GUI_FIELDLISTEDIT_H
#define KBIBTEX_GUI_FIELDLISTEDIT_H

#include <QWidget>
#include <QSet>

#include <kbibtexnamespace.h>
#include <value.h>

class QCheckBox;

class KPushButton;

/**
@author Thomas Fischer
*/
class FieldListEdit : public QWidget
{
    Q_OBJECT

public:
    FieldListEdit(KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, QWidget *parent = NULL);

    virtual bool reset(const Value& value);
    virtual bool apply(Value& value) const;

    void clear();
    virtual void setReadOnly(bool isReadOnly);
    virtual void setFile(const File *file);
    virtual void setCompletionItems(const QStringList &items);

signals:
    void modified();

protected:
    void addButton(KPushButton *button);
    void lineAdd(Value *value);

private slots:
    void lineAdd();
    void lineRemove(QWidget * widget);
    void lineGoDown(QWidget * widget);
    void lineGoUp(QWidget * widget);

protected:
    class FieldListEditProtected;
    FieldListEditProtected *d;
};


/**
@author Thomas Fischer
*/
class PersonListEdit : public FieldListEdit
{
public:
    PersonListEdit(KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, QWidget *parent = NULL);

    virtual bool reset(const Value& value);
    virtual bool apply(Value& value) const;

    virtual void setReadOnly(bool isReadOnly);

private:
    QCheckBox *m_checkBoxOthers;
};


/**
@author Thomas Fischer
*/
class UrlListEdit : public FieldListEdit
{
    Q_OBJECT

public:
    UrlListEdit(QWidget *parent = NULL);

    virtual void setReadOnly(bool isReadOnly);

private slots:
    void slotAddLocalFile();

private:
    KPushButton *m_addLocalFile;
};


/**
@author Thomas Fischer
*/
class KeywordListEdit : public FieldListEdit
{
    Q_OBJECT

public:
    static const QString keyGlobalKeywordList;

    KeywordListEdit(QWidget *parent = NULL);

    virtual void setReadOnly(bool isReadOnly);
    virtual void setFile(const File *file);

private slots:
    void slotAddKeyword();

private:
    KSharedConfigPtr m_config;
    const QString m_configGroupName;
    KPushButton *m_addKeyword;
    QSet<QString> m_keywordsFromFile;
};

#endif // KBIBTEX_GUI_FIELDLISTEDIT_H
