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

#include "pushupcontroller.h"

PushUpController::PushUpController(QObject *parent)
    : QObject(parent)
    , m_watcher("org.jensge.Korva", QDBusConnection::sessionBus())
    , m_controller()
    , m_available(false)
{
    connect(&m_watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)), SLOT(onServiceOwnerChanged(QString,QString,QString)));
}

void PushUpController::onServiceOwnerChanged(const QString &service, const QString &oldOwner, const QString &newOwner)
{
    qDebug() << "Service owner changed:" << service << oldOwner << newOwner;
    Q_UNUSED(service)

    if (oldOwner.isEmpty() && not newOwner.isEmpty()) {
        m_available = true;
    } else if (not oldOwner.isEmpty() && newOwner.isEmpty()) {
        m_available = false;
    }

    emit availabilityChanged(m_available);
}
