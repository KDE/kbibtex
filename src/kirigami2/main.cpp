/***************************************************************************
 *   Copyright (C) 2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de>      *
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

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>

#include "bibliographymodel.h"

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

    QQmlApplicationEngine engine;

    qmlRegisterType<SortedBibliographyModel>("kirigami2.kbibtex", 1, 0, "SortedBibliographyModel");

    engine.load(QUrl(QStringLiteral("qrc:///KBibTeX.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
