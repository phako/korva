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

#include <QUrl>
#include <QImage>

#include "korva-controller1.h"
#include "pushupcontroller.h"
#include "pushupdevicemodel.h"

PushUpDeviceModel::PushUpDeviceModel(PushUpController *controller, QObject *parent)
    : QAbstractListModel(parent)
    , m_controller(controller == 0 ? new PushUpController(this) : controller)
    , m_devices()
{
    const KorvaController1 *interface = m_controller->getInterface();

    connect(m_controller, SIGNAL(availabilityChanged(bool)), SLOT(onAvailabilityChanged(bool)));

    connect(interface, SIGNAL(DeviceAvailable(QVariantMap)), SLOT(onDeviceAvailable(QVariantMap)));
    connect(interface, SIGNAL(DeviceUnavailable(QString)), SLOT(onDeviceUnavailable(QString)));

    syncInitialList();
}

// virtual functions from QAbstractListModel
int PushUpDeviceModel::rowCount(const QModelIndex &parent) const
{
    return m_devices.size();
}

QVariant PushUpDeviceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (!index.row() > m_devices.size()) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
        return m_devices.at(index.row())["DisplayName"];
    case Qt::DecorationRole:
        return m_devices.at(index.row())["IconURI"];
    case Qt::EditRole:
        return m_devices.at(index.row())["UID"];
    default:
        return QVariant();
    }
}

void PushUpDeviceModel::syncInitialList()
{
    QDBusPendingCall reply = m_controller->getInterface()->GetDevices();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(onInitialRequestFinished(QDBusPendingCallWatcher*)));
}

void PushUpDeviceModel::onInitialRequestFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QList<QVariantMap> > reply = *watcher;

    if (reply.isError()) {
        qWarning() << reply.error();
        return;
    }

    QList<QVariantMap> result = reply.argumentAt<0>();
    if (result.length() > 1 || result.at(0).size() > 0) {
        beginInsertRows(QModelIndex(), 0, result.size() - 1);
        m_devices.append(result);
        endInsertRows();
    }

    watcher->deleteLater();
}

void PushUpDeviceModel::onDeviceAvailable(const QVariantMap &device)
{
    beginInsertRows(QModelIndex(), m_devices.size(), m_devices.size());
    m_devices.append(device);
    endInsertRows();
}

void PushUpDeviceModel::onDeviceUnavailable(const QString &uid)
{
    int i;
    for (i = 0; i < m_devices.size(); ++i) {
        if (m_devices.at(i)["UID"] == uid) {
            break;
        }
    }

    if (i >= m_devices.size()) {
        // ignore
        return;
    }

    beginRemoveRows(QModelIndex(), i, i);
    m_devices.removeAt(i);
    endRemoveRows();
}

void PushUpDeviceModel::onAvailabilityChanged(bool available)
{
    if (not available) {
        beginResetModel();
        m_devices.clear();
        endResetModel();
    }
}
