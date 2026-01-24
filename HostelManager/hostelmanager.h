#ifndef HOSTELMANAGER_H
#define HOSTELMANAGER_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
    class HostelManager;
}
QT_END_NAMESPACE

class HostelManager : public QMainWindow
{
    Q_OBJECT

public:
    HostelManager(QWidget *parent = nullptr);
    ~HostelManager();

private slots:
    void on_btnToday_clicked();
    void on_btnRefresh_clicked();
    void initializeTable();
    void updateTableColors();

private:
    Ui::HostelManager *ui;
};

#endif // HOSTELMANAGER_H
