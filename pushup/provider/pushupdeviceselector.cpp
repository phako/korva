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

#include <QAction>
#include <QImage>
#include <QGraphicsLinearLayout>
#include <QUrl>

#include <QtNetwork/QNetworkConfigurationManager>
#include <QtNetwork/QNetworkSession>

#include <MAbstractCellCreator>
#include <MButton>
#include <MBasicSheetHeader>
#include <MBasicListItem>
#include <MImageWidget>
#include <MLabel>
#include <MList>
#include <MPannableViewport>
#include <MStylableWidget>

#include "pushupdeviceselector.h"
#include "pushupdevicemodel.h"
#include "pushupcontroller.h"

class ListCreator : public MAbstractCellCreator<MBasicListItem>
{
    virtual MWidget *createCell(const QModelIndex &index, MWidgetRecycler &recycler) const
    {
        Q_UNUSED(recycler);
        MWidget *widget = new MBasicListItem(MBasicListItem::IconWithTitle);
        updateCell(index, widget);
        return widget;
    }

    virtual void updateCell(const QModelIndex &index, MWidget *cell) const
    {
        MBasicListItem *item = qobject_cast<MBasicListItem *>(cell);
        if (item == 0) {
            return;
        }

        const QVariant data = index.data(Qt::DisplayRole);
        const QVariant imageUrl = index.data(Qt::DecorationRole);
        QImage image = QImage(imageUrl.toUrl().toLocalFile());
        MImageWidget *imageWidget = new MImageWidget(&image, cell);
        imageWidget->setAspectRatioMode(Qt::KeepAspectRatio);
        imageWidget->setStyleName("CommonMainIcon");

        item->setTitle(data.toString());
        item->setImageWidget(imageWidget);
    }
};

PushUpDeviceSelector::PushUpDeviceSelector(PushUpController *controller)
    : MSheet()
    , m_nextAction(0)
    , m_list(0)
{
    MBasicSheetHeader *header = new MBasicSheetHeader;
    QAction *cancelAction = new QAction("Cancel", header);
    header->setNegativeAction(cancelAction);
    connect(cancelAction, SIGNAL(triggered()), SLOT(onCancel()));

    m_nextAction = new QAction("Ok", header);
    m_nextAction->setParent(header);
    m_nextAction->setEnabled(false);
    header->setPositiveAction(m_nextAction);
    connect(m_nextAction, SIGNAL(triggered()), SLOT(onNext()));

    setHeaderWidget(header);

    QGraphicsLinearLayout *centralLayout = new QGraphicsLinearLayout(Qt::Vertical, centralWidget());
    centralLayout->setContentsMargins(0., 0., 0., 0.);
    centralLayout->setSpacing(0.);

    MStylableWidget *pannable = new MStylableWidget(centralWidget());
    QGraphicsLinearLayout *pannableLayout = new QGraphicsLinearLayout(Qt::Vertical, pannable);
    pannableLayout->setContentsMargins(0., 0., 0., 0.);
    pannableLayout->setSpacing(0.);

    MLabel *headerLabel = new MLabel(pannable);
    headerLabel->setStyleName("CommonHeader");
    headerLabel->setTextFormat(Qt::PlainText);
    headerLabel->setText("Select Device");
    pannableLayout->addItem(headerLabel);

    m_list = new MList(centralWidget());
    m_list->setSelectionMode(MList::SingleSelection);
    m_list->setItemModel(new PushUpDeviceModel (controller, this));
    m_list->setContentsMargins(0., 0., 0., 0.);
    m_list->setCellCreator(new ListCreator);
    pannableLayout->addItem(m_list);
    connect(m_list->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(onSelectionChanged(QItemSelection,QItemSelection)));
    connect(m_list->itemModel(), SIGNAL(modelReset()), SLOT(onModelReset()));

    MPannableViewport *viewport = new MPannableViewport(centralWidget());
    viewport->setWidget(pannable);

    centralLayout->addItem(viewport);

    QNetworkConfigurationManager manager;
    QNetworkSession *session = new QNetworkSession(manager.defaultConfiguration(), this);
    if (not manager.isOnline()) {
        session->open();
    }
}

void PushUpDeviceSelector::onSelectionChanged(const QItemSelection &selected, const QItemSelection &unselected)
{
    m_nextAction->setEnabled (!selected.isEmpty());
}

void PushUpDeviceSelector::onModelReset()
{
    m_nextAction->setEnabled(false);
}

void PushUpDeviceSelector::onNext()
{
    disappear();
    QString uri = m_list->selectionModel()->selectedIndexes().first().data(Qt::EditRole).toString();
    emit done(uri);
}

void PushUpDeviceSelector::onCancel()
{
    disappear();
    emit done(QString());
}
