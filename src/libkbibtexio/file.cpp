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
#include <QTextStream>
#include <QIODevice>
#include <QStringList>
#include <QRegExp>

#include <file.h>
#include <entry.h>
#include <element.h>
#include <macro.h>
#include <comment.h>

using namespace KBibTeX::IO;

File::File()
        : QList<KBibTeX::IO::Element*>(), fileName(QString::null)
{
    // nothing
}

File::~File()
{
    // nothing
}

void File::append(KBibTeX::IO::Element* element)
{
    QList<KBibTeX::IO::Element*>::append(element);
}

void File::append(File* /*other*/)
{
    // TODO
}

void File::erase(Element* element)
{
    bool found = false;

    for (Iterator it = begin(); !found && it != end(); ++it)
        if ((found = (*it == element)))
            QList<KBibTeX::IO::Element*>::erase(it);

    if (!found)
        qDebug("BibTeX::File got told to delete an element which is not in this file.");
}

// void File::deleteElement(Element *element)
// {
//     bool found = false;
//     for (ElementList::Iterator it = elements.begin(); it != elements.end(); it++)
//         if (found = (*it == element)) {
//             elements.erase(it);
//             delete element;
//             break;
//         }
//
//     if (!found)
//         qDebug("BibTeX::File got told to delete an element which is not in this file.");
// }

// Element* File::cloneElement(Element *element)
// {
//     Entry * entry = dynamic_cast<Entry*>(element);
//     if (entry)
//         return new Entry(entry);
//     else {
//         Macro *macro = dynamic_cast<Macro*>(element);
//         if (macro)
//             return new Macro(macro);
//         else {
//             Comment *comment = dynamic_cast<Comment*>(element);
//             if (comment)
//                 return new Comment(comment);
//             else
//                 return NULL;
//         }
//     }
// }

// Element *File::containsKey(const QString &key)
// {
//     for (ElementList::iterator it = elements.begin(); it != elements.end(); it++) {
//         Entry* entry = dynamic_cast<Entry*>(*it);
//         if (entry != NULL) {
//             if (entry->id() == key)
//                 return entry;
//         } else {
//             Macro* macro = dynamic_cast<Macro*>(*it);
//             if (macro != NULL) {
//                 if (macro->key() == key)
//                     return macro;
//             }
//         }
//     }
//
//     return NULL;
// }

const Element *File::containsKey(const QString &key) const
{
    for (ConstIterator it = begin(); it != end(); ++it) {
        const Entry* entry = dynamic_cast<const Entry*>(*it);
        if (entry != NULL) {
            if (entry->id() == key)
                return entry;
        } else {
            const Macro* macro = dynamic_cast<const Macro*>(*it);
            if (macro != NULL) {
                if (macro->key() == key)
                    return macro;
            }
        }
    }

    return NULL;
}

QStringList File::allKeys() const
{
    QStringList result;

    for (ConstIterator it = begin(); it != end(); ++it) {
        const Entry* entry = dynamic_cast<const Entry*>(*it);
        if (entry != NULL)
            result.append(entry->id());
        else {
            const Macro* macro = dynamic_cast<const Macro*>(*it);
            if (macro != NULL)
                result.append(macro->key());
        }
    }

    return result;
}

QString File::text() const
{
    QString result;

    for (ConstIterator it = begin(); it != end(); ++it) {
        result.append((*it)->text());
        result.append("\n");
    }

    return result;
}

QStringList File::getAllValuesAsStringList(const EntryField::FieldType fieldType) const
{
    QStringList result;
    for (ConstIterator eit = constBegin(); eit != constEnd(); ++eit) {
        const Entry* entry = dynamic_cast<const Entry*>(*eit);
        EntryField * field = NULL;
        if (entry != NULL && (field = entry->getField(fieldType)) != NULL) {
            QLinkedList<ValueItem*> valueItems = field->value()->items;
            for (QLinkedList<ValueItem*>::ConstIterator vit = valueItems.begin(); vit != valueItems.end(); ++vit) {
                switch (fieldType) {
                case EntryField::ftKeywords : {
                    KeywordContainer *container = dynamic_cast<KeywordContainer*>(*vit);
                    if (container != NULL)
                        for (QLinkedList<Keyword*>::ConstIterator kit = container->keywords.constBegin(); kit != container->keywords.constEnd(); ++kit) {
                            QString text = (*kit)->text();
                            if (!result.contains(text))
                                result.append(text);
                        }
                }
                break;
                case EntryField::ftEditor :
                case EntryField::ftAuthor : {
                    PersonContainer *container = dynamic_cast<PersonContainer*>(*vit);
                    if (container != NULL)
                        for (QLinkedList<Person*>::ConstIterator pit = container->persons.constBegin(); pit != container->persons.constEnd(); ++pit) {
                            QString text = (*pit)->text();
                            if (!result.contains(text))
                                result.append(text);
                        }
                }
                break;
                default: {
                    QString text = (*vit)->text();
                    if (!result.contains(text))
                        result.append(text);
                }
                }
            }
        }
    }

    result.sort();
    return result;
}

QMap<QString, int> File::getAllValuesAsStringListWithCount(const EntryField::FieldType fieldType) const
{
    QMap<QString, int> result;
    for (ConstIterator eit = begin(); eit != end(); ++eit) {
        const Entry* entry = dynamic_cast<const Entry*>(*eit);
        EntryField * field = NULL;
        if (entry != NULL && (field = entry->getField(fieldType)) != NULL) {
            QLinkedList<ValueItem*> valueItems = field->value()->items;
            for (QLinkedList<ValueItem*>::ConstIterator vit = valueItems.begin(); vit != valueItems.end(); ++vit) {
                switch (fieldType) {
                case EntryField::ftKeywords : {
                    KeywordContainer *container = dynamic_cast<KeywordContainer*>(*vit);
                    if (container != NULL)
                        for (QLinkedList<Keyword*>::ConstIterator kit = container->keywords.constBegin(); kit != container->keywords.constEnd(); ++kit) {
                            QString text = (*kit)->text();
                            if (!result.contains(text))
                                result[text] = 1;
                            else
                                result[text] += 1;
                        }
                }
                break;
                case EntryField::ftEditor :
                case EntryField::ftAuthor : {
                    PersonContainer *container = dynamic_cast<PersonContainer*>(*vit);
                    if (container != NULL)
                        for (QLinkedList<Person*>::ConstIterator pit = container->persons.constBegin(); pit != container->persons.constEnd(); ++pit) {
                            QString text = (*pit)->text();
                            if (!result.contains(text))
                                result[text] = 1;
                            else
                                result[text] += 1;
                        }
                }
                break;
                default: {
                    QString text = (*vit)->text();
                    if (!result.contains(text))
                        result[text] = 1;
                    else
                        result[text] += 1;
                }
                }
            }
        }
    }

    return result;
}

void File::replaceValue(const QString& oldText, const QString& newText, const EntryField::FieldType fieldType)
{
// FIXME
    /*    for (ConstIterator it = begin(); it != end(); ++it) {
            const Entry* entry = dynamic_cast<const Entry*>(*it);
            if (entry != NULL) {
                if (fieldType != EntryField::ftUnknown) {
                    EntryField * field = entry->getField(fieldType);
                    if (field != NULL)
                        field->value() ->replace(oldText, newText);
                }
            }
        }*/
}

// Entry *File::completeReferencedFieldsConst(const Entry *entry) const
// {
//     Entry *myEntry = new Entry(entry);
//     completeReferencedFields(myEntry);
//     return myEntry;
// }

// void File::completeReferencedFields(Entry *entry) const
// {
//     EntryField *crossRefField = entry->getField(EntryField::ftCrossRef);
//     const Entry *parent = NULL;
//     if (crossRefField != NULL && (parent = dynamic_cast<const Entry*>(containsKeyConst(crossRefField->value()->text()))) != NULL) {
//         for (int ef = (int)EntryField::ftAbstract; ef <= (int)EntryField::ftYear; ++ef) {
//             EntryField *entryField = entry->getField((EntryField::FieldType) ef);
//             if (entryField == NULL) {
//                 EntryField *parentEntryField = parent->getField((EntryField::FieldType) ef);
//                 if (parentEntryField != NULL) {
//                     entryField = new EntryField((EntryField::FieldType)ef);
//                     entryField->setValue(parentEntryField->value());
//                     entry->addField(entryField);
//                 }
//             }
//         }
//
//         EntryField *entryField = entry->getField(EntryField::ftBookTitle);
//         EntryField *parentEntryField = parent->getField(EntryField::ftTitle);
//         if ((entry->entryType() == Entry::etInProceedings || entry->entryType() == Entry::etInBook) && entryField == NULL && parentEntryField != NULL) {
//             entryField = new EntryField(EntryField::ftBookTitle);
//             entryField->setValue(parentEntryField->value());
//             entry->addField(entryField);
//         }
//     }
//
//     for (int ef = (int)EntryField::ftAbstract; ef <= (int)EntryField::ftYear; ++ef) {
//         EntryField *entryField = entry->getField((EntryField::FieldType) ef);
//         if (entryField != NULL && entryField->value() != NULL && !entryField->value()->items.isEmpty()) {
//             MacroKey *macroKey = dynamic_cast<MacroKey*>(entryField->value()->items.first());
//             const Macro *macro = NULL;
//             if (macroKey != NULL && (macro = dynamic_cast<const Macro*>(containsKeyConst(macroKey->text()))) != NULL)
//                 entryField->setValue(macro->value());
//         }
//     }
// }
