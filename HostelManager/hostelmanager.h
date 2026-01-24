#ifndef HOSTELMANAGER_H
#define HOSTELMANAGER_H

#include <QMainWindow>
#include <QDate>

// Forward declarations
class QTableWidget;

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

    // Методы для доступа из других классов
    QTableWidget* getTableWidget();
    QDate getCurrentStartDate() const { return currentStartDate; }
    void updateTableColors();

private slots:
    void on_btnToday_clicked();
    void on_btnRefresh_clicked();
    void on_dateEdit_dateChanged(const QDate &date);
    void initializeTable();
    void updateTableHeaders();

private:
    Ui::HostelManager *ui;
    QDate currentStartDate;
    static const int DAYS_COUNT = 31;

    void createMenuBar();
};

#endif // HOSTELMANAGER_H
