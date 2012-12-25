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

#ifndef PUSHUPDEVICEMODEL_H
#define PUSHUPDEVICEMODEL_H

#include <QAbstractListModel>
#include <QtDBus>

class PushUpController;
class PushUpDeviceModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit PushUpDeviceModel(PushUpController *controller = 0,
                               QObject *parent = 0);

    // virtual functions from QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
signals:

public slots:
private slots:
    void onInitialRequestFinished(QDBusPendingCallWatcher *watcher);
    void onDeviceAvailable(const QVariantMap& device);
    void onDeviceUnavailable(const QString& uid);
    void onAvailabilityChanged(bool available);
private:
    PushUpController *m_controller;
    QList<QVariantMap> m_devices;

    void syncInitialList();
};

#endif // PUSHUPDEVICEMODEL_H
