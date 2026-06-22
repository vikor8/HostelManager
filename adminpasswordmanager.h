#ifndef ADMINPASSWORDMANAGER_H
#define ADMINPASSWORDMANAGER_H

#include <QString>
#include <QSettings>
#include <QCryptographicHash>

class AdminPasswordManager
{
public:
    static AdminPasswordManager* instance();

    bool checkPassword(const QString& password) const;
    void setPassword(const QString& newPassword);
    bool isDefaultPassword() const;

private:
    AdminPasswordManager();
    QString hashPassword(const QString& password) const;

    QSettings settings;
    QString m_hashedPassword;
};

#endif // ADMINPASSWORDMANAGER_H