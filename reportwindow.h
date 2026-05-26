#ifndef REPORTWINDOW_H
#define REPORTWINDOW_H

#include <QMainWindow>
#include <QDate>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QDateEdit;
class QPushButton;
class QTableWidget;
class QPrinter;
QT_END_NAMESPACE

class Database;

class ReportWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ReportWindow(Database *db, QWidget *parent = nullptr);
    ~ReportWindow();

private slots:
    void generateSummaryReport();
    void printReport();
    void exportToExcel();

private:
    void setupUi();
    void createMenuBar();

    Database *database;

    // Элементы интерфейса
    QDateEdit *startDateEdit;
    QDateEdit *endDateEdit;
    QPushButton *generateButton;
    QPushButton *printButton;
    QPushButton *exportButton;
    QTextEdit *reportTextEdit;
    QTableWidget *reportTable;
};

#endif // REPORTWINDOW_H
