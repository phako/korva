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

#include <MMessageBox>

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
    QString name = qtTrId ("Play to DLNA");
    return name;
}

QString PushUpMethod::icon()
{
    return QString::fromLatin1 ("icon-l-pushup_play");
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
}

void PushUpMethod::currentItems (const ShareUI::ItemContainer * items)
{
    if (items == 0 || items->count() == 0 || items->count() > 1) {
        Q_EMIT visible (false);

        return;
    }

    ShareUI::ItemIterator itemsIter = items->itemIterator();

    // Limit to files
    while (itemsIter.hasNext()) {

        ShareUI::SharedItem item = itemsIter.next();
        ShareUI::FileItem * fileItem = ShareUI::FileItem::toFileItem (item);

        if (fileItem == 0) {
            Q_EMIT visible (false);

            return;
        }
    }

    Q_EMIT visible (true);
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
                QDBusPendingCall reply = m_controller.getInterface()->Push(source, uid);
                QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
                connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(onPushDone(QDBusPendingCallWatcher*)));

                break;
            }
        }
    }
}

void PushUpMethod::onPushDone(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QString> reply = *watcher;

    if (reply.isError()) {
        MMessageBox *dialog = new MMessageBox(reply.error().message());
        dialog->appear(MSceneWindow::DestroyWhenDone);

        return;
    }

    watcher->deleteLater();
    emit done();
}
