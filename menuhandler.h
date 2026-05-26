#ifndef MENUHANDLER_H
#define MENUHANDLER_H

#include <QObject>

// Forward declaration
class HostelManager;

class MenuHandler : public QObject
{
    Q_OBJECT

public:
    explicit MenuHandler(QObject *parent = nullptr);
    void setMainWindow(HostelManager *window) { mainWindow = window; }

public slots:
    // Файл
    void onFileNew();
    void onFileOpen();
    void onFileSave();
    void onFileExport();
    void onFilePrint();
    void onFileExit();

    // Клиенты
    void onClientAdd();
    void onClientView();
    void onClientEdit();
    void onClientDelete();

    // Бронирование
    void onBookingNew();
    void onBookingView();
    void onBookingEdit();
    void onBookingCancel();
    void onBookingCheckIn();
    void onBookingCheckOut();

    // Доп. услуги
    void onServiceAdd();
    void onServiceView();
    void onServiceAssign();

    // Статистика
    void onStatsOccupancy();
    void onStatsRevenue();
    void onStatsClients();

    // О программе
    void onHelpAbout();
    void onHelpAboutQt();
    void onHelpContents();

private:
    HostelManager *mainWindow;
};

#endif // MENUHANDLER_H
