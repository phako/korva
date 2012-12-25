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

#ifndef PUSHUPCONTROLLER_H
#define PUSHUPCONTROLLER_H

#include <QObject>
#include <QtDBus/QDBusServiceWatcher>

#include "korva-controller1.h"

class PushUpController : public QObject
{
    Q_OBJECT
public:
    explicit PushUpController(QObject *parent = 0);

    KorvaController1 *getInterface() { return &m_controller; }
    bool available() const { return m_available; }

signals:
    void availabilityChanged(bool available);

public slots:
    void push(const QString &uri, const QString &uuid);
private slots:
    void onServiceOwnerChanged(const QString& service, const QString& oldOwner, const QString& newOwner);
private:
    QDBusServiceWatcher m_watcher;
    KorvaController1    m_controller;
    bool                m_available;
};

#endif // PUSHUPCONTROLLER_H
