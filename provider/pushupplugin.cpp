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

#include "pushupplugin.h"
#include "pushupmethod.h"

PushUpPlugin::PushUpPlugin (QObject * parent) :
    ShareUI::PluginBase (parent) {

}

PushUpPlugin::~PushUpPlugin () {
}

QList<ShareUI::MethodBase *> PushUpPlugin::methods (QObject * parent) {

    QList<ShareUI::MethodBase *> list;

    list.append (new PushUpMethod(parent));

    return list;
}

Q_EXPORT_PLUGIN2(pushup, PushUpPlugin)
