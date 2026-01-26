#ifndef STATISTICS_H
#define STATISTICS_H

#include <QObject>

class Statistics : public QObject
{
    Q_OBJECT

public:
    explicit Statistics(QObject *parent = nullptr);

public slots:
    // Доп. услуги
    void onServiceAdd();
    void onServiceView();
    void onServiceAssign();

    // Статистика
    void onStatsOccupancy();
    void onStatsRevenue();
    void onStatsClients();

private:
    void showNotImplemented(const QString &actionName);
};

#endif // STATISTICS_H
