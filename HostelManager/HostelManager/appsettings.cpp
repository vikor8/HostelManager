#include "appsettings.h"
#include <QSettings>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

// Конструктор
AppSettings::AppSettings()
{
    // Инициализация настроек
    QSettings settings("YourCompany", "HostelManager");

    qDebug() << "AppSettings инициализирован";
}

// Деструктор
AppSettings::~AppSettings()
{
    qDebug() << "AppSettings сохранен";
}

// Статический метод для получения экземпляра
AppSettings& AppSettings::instance()
{
    static AppSettings instance;
    return instance;
}

QString AppSettings::getDatabasePath() const
{
    QSettings settings("YourCompany", "HostelManager");
    QString path = settings.value("database/path", "BD_Kolcovo.sqlite").toString();

    // Если путь относительный, преобразуем в абсолютный относительно приложения
    if (QDir::isRelativePath(path)) {
        path = QCoreApplication::applicationDirPath() + "/" + path;
    }

    return path;
}

void AppSettings::setDatabasePath(const QString& path)
{
    QSettings settings("YourCompany", "HostelManager");

    // Сохраняем путь относительно папки приложения, если возможно
    QString appDir = QCoreApplication::applicationDirPath();
    QString relativePath = path;

    if (path.startsWith(appDir)) {
        relativePath = path.mid(appDir.length() + 1); // +1 для слеша
    }

    settings.setValue("database/path", relativePath);
    qDebug() << "Сохранен путь к БД по умолчанию:" << relativePath;
}

QString AppSettings::getLastDatabasePath() const
{
    QSettings settings("YourCompany", "HostelManager");
    QString path = settings.value("database/last_path").toString();

    // Проверяем, существует ли файл
    if (!path.isEmpty() && !QFile::exists(path)) {
        qDebug() << "Последняя БД не найдена по пути:" << path;
        return QString();
    }

    return path;
}

void AppSettings::setLastDatabasePath(const QString& path)
{
    QSettings settings("YourCompany", "HostelManager");
    settings.setValue("database/last_path", path);
    qDebug() << "Сохранен путь к последней БД:" << path;
}

QStringList AppSettings::getRecentDatabases() const
{
    QSettings settings("YourCompany", "HostelManager");
    return settings.value("database/recent_list").toStringList();
}

void AppSettings::addRecentDatabase(const QString& path)
{
    QSettings settings("YourCompany", "HostelManager");
    QStringList recent = settings.value("database/recent_list").toStringList();

    // Удаляем дубликат, если есть
    recent.removeAll(path);

    // Добавляем в начало
    recent.prepend(path);

    // Ограничиваем количество (максимум 5)
    while (recent.size() > 5) {
        recent.removeLast();
    }

    settings.setValue("database/recent_list", recent);
    qDebug() << "Добавлена в недавние БД:" << path;
}

void AppSettings::clearSettings()
{
    QSettings settings("YourCompany", "HostelManager");
    settings.clear();
    qDebug() << "Настройки очищены";
}
