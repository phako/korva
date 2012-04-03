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
private slots:
    void onServiceOwnerChanged(const QString& service, const QString& oldOwner, const QString& newOwner);
private:
    QDBusServiceWatcher m_watcher;
    KorvaController1    m_controller;
    bool                m_available;
};

#endif // PUSHUPCONTROLLER_H
