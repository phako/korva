/*
 * Copyright (C) 2012 Tuomas Kulve <tuomas@kulve.fi>
 * Copyright (C) 2010-2011 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */




#ifndef _SHARE_UI_CMD_METHOD_H_
#define _SHARE_UI_CMD_METHOD_H_

#include <QObject>
#include <ShareUI/MethodBase>

#include "pushupcontroller.h"

/*!
   \class PushUpMethod
   \brief Class providing command line method to ShareUI
   \author Tuomas Kulve <tuomas@kulve.fi>
  */
class PushUpMethod : public ShareUI::MethodBase {

Q_OBJECT

public:

    PushUpMethod (QObject * parent = 0);
    virtual ~PushUpMethod ();

    /*!
      \brief See MethodBase
     */
    virtual QString title();

    /*!
      \brief See MethodBase
     */
    virtual QString icon();

    /*!
      \brief See MethodBase::id
     */
    virtual QString id ();

public Q_SLOTS:

    /*!
      \brief See ShareUI::MethodBase
     */
    void currentItems (const ShareUI::ItemContainer * items);

    /*!
      \brief See ShareUI::MethodBase
     */
    void selected (const ShareUI::ItemContainer * items);

private slots:
    void onSelectorDone(const QString& uid);
    void onPushDone(QDBusPendingCallWatcher *watcher);

private:
    PushUpController m_controller;
    const ShareUI::ItemContainer *m_items;

    /*!
      \brief If given content is Ok for command line sharing
      \return true if content is accepted, false if not
     */
    bool acceptContent (const ShareUI::ItemContainer * items);

};

#endif
