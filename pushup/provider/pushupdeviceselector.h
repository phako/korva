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

#ifndef __PUSHUP_DEVICE_SELECTOR_H__
#define __PUSHUP_DEVICE_SELECTOR_H__

#include <QItemSelection>

#include <MSheet>
#include <MList>

#include <QAction>

class PushUpController;

class PushUpDeviceSelector : public MSheet
{
    Q_OBJECT
public:
    PushUpDeviceSelector (PushUpController *controller);

signals:
    void done(const QString& uid);

private slots:
    void onSelectionChanged(const QItemSelection& , const QItemSelection& );
    void onModelReset();
    void onNext();
    void onCancel();
private:
    QAction *m_nextAction;
    MList   *m_list;
};

#endif //__PUSHUP_DEVICE_SELECTOR_H__
