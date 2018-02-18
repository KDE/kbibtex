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

#ifndef KBIBTEX_GUI_COLORLABELWIDGET_H
#define KBIBTEX_GUI_COLORLABELWIDGET_H

#include <KComboBox>

#include "kbibtexgui_export.h"

#include "value.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT ColorLabelWidget : public KComboBox
{
    Q_OBJECT

public:
    explicit ColorLabelWidget(QWidget *parent = nullptr);
    ~ColorLabelWidget() override;

    void clear();
    bool reset(const Value &value);
    bool apply(Value &value) const;
    bool validate(QWidget **widgetWithIssue, QString &message) const;
    void setReadOnly(bool);

    static QPixmap createSolidIcon(const QColor &color);

signals:
    void modified();

private slots:
    void slotCurrentIndexChanged(int);

private:
    class ColorLabelWidgetPrivate;
    ColorLabelWidget::ColorLabelWidgetPrivate *d;
};

#endif // KBIBTEX_GUI_COLORLABELWIDGET_H
