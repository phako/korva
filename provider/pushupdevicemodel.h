#ifndef PUSHUPDEVICEMODEL_H
#define PUSHUPDEVICEMODEL_H

#include <QAbstractListModel>

#include "korva-controller1.h"

class PushUpController;

class PushUpDeviceModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit PushUpDeviceModel(PushUpController *controller, QObject *parent = 0);

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
