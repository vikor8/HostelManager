#ifndef HOSTELMANAGER_H
#define HOSTELMANAGER_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class HostelManager; }
QT_END_NAMESPACE

class HostelManager : public QMainWindow
{
    Q_OBJECT

public:
    HostelManager(QWidget *parent = nullptr);
    ~HostelManager();

private:
    Ui::HostelManager *ui;
};
#endif // HOSTELMANAGER_H
