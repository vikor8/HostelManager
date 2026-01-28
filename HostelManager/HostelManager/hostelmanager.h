#ifndef HOSTELMANAGER_H
#define HOSTELMANAGER_H

#include <QMainWindow>
#include <QDate>
#include <QInputDialog>
#include <QMap>
#include <QColor>

// Forward declarations вместо include
class Database;
class AddClientDialog;
class AddBookingDialog;

// Forward declarations для QTableWidget
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

    // Метод для перевода месяца на русский
    QString monthToRussian(const QString& month) const;

    // Метод для получения цвета категории
    QColor getCategoryColor(const QString& category) const;

private slots:
    void on_btnToday_clicked();
    void on_btnRefresh_clicked();
    void on_dateEdit_dateChanged(const QDate &date);
    void initializeTable();
    void updateTableHeaders();

    // Слоты для управления комнатами
    void onAddRoom();
    void onEditRoom();
    void onDeleteRoom();

    // Слоты для управления категориями
    void onManageCategories();
    void loadCategories(); // Загрузка категорий из базы данных

    // Слоты для управления клиентами
    void onViewClients();
    void onAddClient();
    void onEditClient();
    void onDeleteClient();

    // Слоты для бронирования
    void onAddBooking();

    //Слот ля двойного клика
    void onTableDoubleClicked(const QModelIndex &index);

private:
    Ui::HostelManager *ui;
    QDate currentStartDate;
    Database *database;
    static const int DAYS_COUNT = 31;

    QMap<QString, int> roomIdMap; // Карта для хранения ID комнат
    QMap<QString, QColor> categoryColors; // Карта цветов категорий

    void createMenuBar();
    void initializeDatabase();
    void updateRoomIdMap(); // Обновление карты ID комнат
};

#endif // HOSTELMANAGER_H
