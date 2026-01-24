#ifndef HOSTELMANAGER_H
#define HOSTELMANAGER_H

#include <QMainWindow>
#include <QDate>

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
    void on_dateEdit_dateChanged(const QDate &date);
    void initializeTable();
    void updateTableHeaders();
    void updateTableColors();

private:
    Ui::HostelManager *ui;
    QDate currentStartDate;
    static const int DAYS_COUNT = 31;
};

#endif // HOSTELMANAGER_H
