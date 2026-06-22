#include "adminpasswordmanager.h"
#include <QCoreApplication>
#include <QDebug>

AdminPasswordManager* AdminPasswordManager::instance()
{
    static AdminPasswordManager inst;
    return &inst;
}

AdminPasswordManager::AdminPasswordManager()
    : settings("YourCompany", "HostelManager")
{
    m_hashedPassword = settings.value("Admin/PasswordHash").toString();
    if (m_hashedPassword.isEmpty()) {
        // Пароль по умолчанию "0000"
        setPassword("0000");
    }
}

bool AdminPasswordManager::checkPassword(const QString& password) const
{
    QString hashed = hashPassword(password);
    return hashed == m_hashedPassword;
}

void AdminPasswordManager::setPassword(const QString& newPassword)
{
    m_hashedPassword = hashPassword(newPassword);
    settings.setValue("Admin/PasswordHash", m_hashedPassword);
}

bool AdminPasswordManager::isDefaultPassword() const
{
    return checkPassword("0000");
}

QString AdminPasswordManager::hashPassword(const QString& password) const
{
    return QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}