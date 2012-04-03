/*
    This file is part of Korva.

    Copyright (C) 2012 Openismus GmbH.
    Author: Jens Georg <jensg@openismus.com>

    Korva is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Korva is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Korva.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "pushupmethod.h"
#include "pushupdeviceselector.h"

#include <QDebug>
#include <QProcess>
#include <QFile>
#include <ShareUI/ItemContainer>
#include <ShareUI/DataUriItem>
#include <ShareUI/FileItem>

PushUpMethod::PushUpMethod(QObject * parent)
    : ShareUI::MethodBase(parent)
    , m_controller()
{
}

PushUpMethod::~PushUpMethod()
{
}

QString PushUpMethod::title()
{
    QString name = qtTrId ("DLNA-compatible +PU+/+UP+ sharing");
    return name;
}

QString PushUpMethod::icon()
{
    return QString::fromLatin1 ("icon-pushup");
}

QString PushUpMethod::id()
{
    static const QString id = QLatin1String("org.jensge.korva");

    return id;
}

void PushUpMethod::selected(const ShareUI::ItemContainer *items)
{

    QStringList filePaths;
    QString body;
    QString subject;
    QString cmd;
    ShareUI::SharedItem item;

    PushUpDeviceSelector *selector = new PushUpDeviceSelector (&m_controller);
    m_items = items;
    connect (selector, SIGNAL(done(QString)), SLOT(onSelectorDone(QString)));
    selector->appearSystemwide(MSceneWindow::KeepWhenDone);
/*
    ShareUI::ItemIterator itemsIter = items->itemIterator();
    while (itemsIter.hasNext()) {

        item = itemsIter.next();

        //	GConfItem* gi = new GConfItem("/apps/ControlPanel/PushUpShare/cmd", this);
        //	cmd = gi->value().toString();

        ShareUI::FileItem * fileItem = qobject_cast<ShareUI::FileItem*> (item.data());

        if (fileItem != 0) {
            filePaths << fileItem->filePath();
        }
    }

    // Add files
    QStringList files;
    for (int i = 0; i < filePaths.count(); ++i) {
        files << filePaths.at (i);
    }

    // Replace %s with the first file
    cmd = cmd.replace("%s", files.at(0));

    if (1) { // Debug
        QFile file("/tmp/out.txt");
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file);
        out << "files:" << files.join(" ") << "\n";
        out << "cmd:" << cmd << "\n";
        file.close();
    }

    // Launch the command
    QProcess process;
    if (!QProcess::startDetached(cmd)) {

        // Failed to launch the command
        QString err = "Failed to launch: " + cmd;
        qCritical(err.toLatin1());
        emit(selectedFailed(err));
    } else {
        // Command successfully launched (still running)
        emit(done());
    } */
}

void PushUpMethod::currentItems (const ShareUI::ItemContainer * items)
{
    Q_EMIT visible(true);
}

bool PushUpMethod::acceptContent (const ShareUI::ItemContainer * items)
{

    if (items == 0 || items->count() == 0) {
        return false;
    }

    //  GConfItem* gi = new GConfItem("/apps/ControlPanel/PushUpShare/enabled", this);
    //  bool enabled = gi->value().toBool();
    bool enabled = false;

    if (!enabled) {
        return false;
    }

    ShareUI::ItemIterator itemsIter = items->itemIterator();

    // Limit to files
    while (itemsIter.hasNext()) {

        ShareUI::SharedItem item = itemsIter.next();
        ShareUI::FileItem * fileItem = ShareUI::FileItem::toFileItem (item);

        if (fileItem == 0) {
            return false;
        }
    }

    return true;
}

void PushUpMethod::onSelectorDone(const QString& uid)
{
    if (!uid.isEmpty()) {
        ShareUI::ItemIterator itemsIterator = m_items->itemIterator();
        while (itemsIterator.hasNext()) {
            ShareUI::SharedItem item = itemsIterator.next();
            ShareUI::FileItem *fileItem = qobject_cast<ShareUI::FileItem*>(item.data());
            if (fileItem != 0) {
                QVariantMap source;
                source["URI"] = fileItem->fileUri();
                m_controller.getInterface()->Push(source, uid);

                break;
            }
        }
    }

    emit done();
}
