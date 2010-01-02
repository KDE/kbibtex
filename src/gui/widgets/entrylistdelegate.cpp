#include "entrylistdelegate.h"
#include "fieldlineedit.h"

using namespace KBibTeX::GUI::Widgets;

EntryListDelegate::EntryListDelegate(QObject *parent)
        : QItemDelegate(parent)
{
    // nothing
}

QWidget *EntryListDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    FieldLineEdit *editor = new FieldLineEdit(FieldLineEdit::Source | FieldLineEdit::Text, parent);

    return editor;
}

void EntryListDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    KBibTeX::IO::Value *value;
    QVariant v =  index.model()->data(index, Qt::EditRole);
    if (v.canConvert<void*>())
        value = (KBibTeX::IO::Value*)v.value<void*>();

    FieldLineEdit *fieldLineEdit = static_cast<FieldLineEdit*>(editor);
    fieldLineEdit->setValue(*value);
}

void EntryListDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    FieldLineEdit *fieldLineEdit = static_cast<FieldLineEdit*>(editor);

    //model->setData(index, value, Qt::EditRole);
}

void EntryListDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}
