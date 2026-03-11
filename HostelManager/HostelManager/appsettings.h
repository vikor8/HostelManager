#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QString>
#include <QStringList>

class AppSettings
{
public:
    static AppSettings& instance();

    QString getDatabasePath() const;
    void setDatabasePath(const QString& path);

    QString getLastDatabasePath() const;
    void setLastDatabasePath(const QString& path);

    QStringList getRecentDatabases() const;
    void addRecentDatabase(const QString& path);

    void clearSettings();

private:
    AppSettings();
    ~AppSettings();
    AppSettings(const AppSettings&) = delete;
    AppSettings& operator=(const AppSettings&) = delete;
};

#endif // APPSETTINGS_H
