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
