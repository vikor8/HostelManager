#include "database.h"
#include <QColor>

Database::Database(QObject *parent) : QObject(parent)
{
    databasePath = "BD_Kolcovo.sqlite";
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(databasePath);
}

Database::~Database()
{
    if (db.isOpen()) {
        db.close();
    }
}

bool Database::initializeDatabase()
{
    try {
        // Проверяем существование файла базы данных
        bool dbExists = QFile::exists(databasePath);

        // Открываем базу данных
        if (!db.open()) {
            qDebug() << "Ошибка открытия базы данных:" << db.lastError().text();
            return false;
        }

        // Если база данных не существовала, создаем таблицы
        if (!dbExists) {
            qDebug() << "Создание новой базы данных...";
            createTables();
            createTriggers();
            createViews();
            seedTestData();
            qDebug() << "База данных успешно создана и заполнена тестовыми данными";
        } else {
            qDebug() << "База данных успешно открыта";
        }

        return true;

    } catch (const std::exception& e) {
        qDebug() << "Исключение при инициализации базы данных:" << e.what();
        return false;
    }
}

void Database::createTables()
{
    QSqlQuery query;

    // Таблица категорий (новая таблица)
    query.exec("CREATE TABLE IF NOT EXISTS room_categories ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "category_name TEXT UNIQUE NOT NULL,"
               "color TEXT DEFAULT '#FFFFFF',"
               "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
               ")");

    // Таблица комнат (обновляем внешний ключ)
    query.exec("CREATE TABLE IF NOT EXISTS rooms ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "room_number TEXT UNIQUE NOT NULL,"
               "category TEXT NOT NULL,"
               "beds_count INTEGER NOT NULL,"
               "description TEXT,"
               "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
               "FOREIGN KEY (category) REFERENCES room_categories(category_name)"
               ")");

    // Таблица кроватей
    query.exec("CREATE TABLE IF NOT EXISTS beds ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "room_id INTEGER NOT NULL,"
               "bed_number INTEGER NOT NULL,"
               "price_per_day REAL NOT NULL,"
               "is_active INTEGER DEFAULT 1,"
               "FOREIGN KEY (room_id) REFERENCES rooms(id) ON DELETE CASCADE,"
               "UNIQUE(room_id, bed_number)"
               ")");

    // Таблица клиентов (ОБНОВЛЕНА)
    query.exec("CREATE TABLE IF NOT EXISTS clients ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "first_name TEXT NOT NULL,"
               "last_name TEXT NOT NULL,"
               "middle_name TEXT,"
               "passport_number TEXT UNIQUE NOT NULL,"
               "phone_number TEXT NOT NULL,"
               "birth_date DATE NOT NULL,"
               "country TEXT,"
               "registration_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
               "notes TEXT"
               ")");

    // Таблица бронирований
    query.exec("CREATE TABLE IF NOT EXISTS bookings ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "bed_id INTEGER NOT NULL,"
               "client_id INTEGER NOT NULL,"
               "check_in_date DATE NOT NULL,"
               "check_out_date DATE NOT NULL,"
               "total_price REAL NOT NULL,"
               "paid_amount REAL DEFAULT 0,"
               "payment_method TEXT DEFAULT 'Наличные',"
               "status TEXT DEFAULT 'active',"
               "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
               "cancelled_at TIMESTAMP,"
               "FOREIGN KEY (bed_id) REFERENCES beds(id),"
               "FOREIGN KEY (client_id) REFERENCES clients(id)"
               ")");

    // Таблица дополнительных услуг
    query.exec("CREATE TABLE IF NOT EXISTS services ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "service_name TEXT NOT NULL,"
               "description TEXT,"
               "price REAL NOT NULL"
               ")");

    // Таблица связей бронирований и услуг
    query.exec("CREATE TABLE IF NOT EXISTS booking_services ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "booking_id INTEGER NOT NULL,"
               "service_id INTEGER NOT NULL,"
               "quantity INTEGER DEFAULT 1,"
               "total_price REAL NOT NULL,"
               "FOREIGN KEY (booking_id) REFERENCES bookings(id),"
               "FOREIGN KEY (service_id) REFERENCES services(id)"
               ")");

    // Таблица истории изменений
    query.exec("CREATE TABLE IF NOT EXISTS change_history ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "table_name TEXT NOT NULL,"
               "record_id INTEGER NOT NULL,"
               "action TEXT NOT NULL,"
               "changed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
               "user_info TEXT"
               ")");
}

void Database::createTriggers()
{
    QSqlQuery query;

//    // Триггер для автоматического создания кроватей при добавлении комнаты
//    query.exec("CREATE TRIGGER IF NOT EXISTS create_beds_after_room_insert "
//               "AFTER INSERT ON rooms "
//               "BEGIN "
//               "   INSERT INTO beds (room_id, bed_number, price_per_day) "
//               "   SELECT NEW.id, 1, 500.00 "
//               "   UNION ALL SELECT NEW.id, 2, 500.00 "
//               "   UNION ALL SELECT NEW.id, 3, 500.00 "
//               "   UNION ALL SELECT NEW.id, 4, 500.00; "
//               "END");

    // Триггер для логирования изменений в бронированиях
    query.exec("CREATE TRIGGER IF NOT EXISTS log_booking_changes "
               "AFTER INSERT ON bookings "
               "BEGIN "
               "   INSERT INTO change_history (table_name, record_id, action) "
               "   VALUES ('bookings', NEW.id, 'INSERT'); "
               "END");

    // Триггер для проверки пересечений дат бронирований
//    query.exec("CREATE TRIGGER IF NOT EXISTS check_booking_overlap "
//               "BEFORE INSERT ON bookings "
//               "BEGIN "
//               "   SELECT RAISE(ABORT, 'Кровать уже забронирована на эти даты') "
//               "   WHERE EXISTS ("
//               "       SELECT 1 FROM bookings "
//               "       WHERE bed_id = NEW.bed_id "
//               "       AND status = 'active' "
//               "       AND NOT (NEW.check_out_date <= check_in_date OR NEW.check_in_date >= check_out_date)"
//               "   ); "
//               "END");
}

void Database::createViews()
{
    QSqlQuery query;

    // Представление для отображения текущих бронирований
    query.exec("CREATE VIEW IF NOT EXISTS current_bookings AS "
               "SELECT "
               "   b.id as booking_id, "
               "   r.room_number, "
               "   bd.bed_number, "
               "   c.first_name || ' ' || c.last_name as client_name, "
               "   bk.check_in_date, "
               "   bk.check_out_date, "
               "   bk.total_price, "
               "   bk.status "
               "FROM bookings bk "
               "JOIN beds bd ON bk.bed_id = bd.id "
               "JOIN rooms r ON bd.room_id = r.id "
               "JOIN clients c ON bk.client_id = c.id "
               "WHERE bk.status = 'active' "
               "ORDER BY bk.check_in_date");

    // Представление для статистики занятости
    query.exec("CREATE VIEW IF NOT EXISTS occupancy_stats AS "
               "SELECT "
               "   r.room_number, "
               "   COUNT(bd.id) as total_beds, "
               "   SUM(CASE WHEN EXISTS ("
               "       SELECT 1 FROM bookings bk "
               "       WHERE bk.bed_id = bd.id "
               "       AND bk.status = 'active' "
               "       AND date('now') BETWEEN bk.check_in_date AND bk.check_out_date"
               "   ) THEN 1 ELSE 0 END) as occupied_beds "
               "FROM rooms r "
               "LEFT JOIN beds bd ON r.id = bd.room_id "
               "GROUP BY r.id");
}

void Database::seedTestData()
{
    QSqlQuery query;

    // Добавляем стандартные категории
    QStringList defaultCategories = {"Эконом", "Стандарт", "Комфорт", "Люкс"};
    QMap<QString, QString> defaultColors = {
        {"Эконом", "#E6F3FF"},    // Светло-голубой
        {"Стандарт", "#E6FFE6"},  // Светло-зеленый
        {"Комфорт", "#FFF9E6"},   // Светло-желтый
        {"Люкс", "#FFE6E6"}       // Светло-красный
    };

    for (const QString& category : defaultCategories) {
        query.prepare("INSERT OR IGNORE INTO room_categories (category_name, color) VALUES (?, ?)");
        query.addBindValue(category);
        query.addBindValue(defaultColors.value(category, "#FFFFFF"));
        query.exec();
    }

//    // Добавляем тестовые комнаты
//    QStringList rooms = {"101", "102", "103", "104", "105", "201", "202", "203", "204", "205"};
//    QMap<QString, QString> categories = {
//        {"101", "Эконом"}, {"102", "Эконом"}, {"103", "Стандарт"},
//        {"104", "Стандарт"}, {"105", "Комфорт"}, {"201", "Эконом"},
//        {"202", "Эконом"}, {"203", "Стандарт"}, {"204", "Комфорт"},
//        {"205", "Люкс"}
//    };

//    for (const QString& room : rooms) {
//        query.prepare("INSERT INTO rooms (room_number, category) VALUES (?, ?)");
//        query.addBindValue(room);
//        query.addBindValue(categories.value(room, "Стандарт"));
//        query.exec();
//    }

    // Обновляем цены кроватей в зависимости от категории
    query.exec("UPDATE beds SET price_per_day = "
               "CASE WHEN room_id IN (SELECT id FROM rooms WHERE category = 'Люкс') THEN 1500.00 "
               "     WHEN room_id IN (SELECT id FROM rooms WHERE category = 'Комфорт') THEN 1000.00 "
               "     WHEN room_id IN (SELECT id FROM rooms WHERE category = 'Стандарт') THEN 750.00 "
               "     ELSE 500.00 END");

    // Добавляем тестовых клиентов (ОБНОВЛЕНО)
    QStringList firstNames = {"Иван", "Петр", "Анна", "Мария", "Сергей", "Ольга", "Дмитрий", "Елена"};
    QStringList lastNames = {"Иванов", "Петров", "Сидорова", "Смирнов", "Кузнецов", "Попова", "Васильев", "Павлова"};
    QStringList middleNames = {"Иванович", "Петрович", "Сергеевна", "Алексеевна", "Николаевич", "Дмитриевна", "Владимирович", "Андреевна"};
    QStringList countries = {"Россия", "Беларусь", "Казахстан", "Украина", "", "Россия", "Казахстан", ""};

    for (int i = 0; i < 8; i++) {
        query.prepare("INSERT INTO clients (first_name, last_name, middle_name, "
                     "passport_number, phone_number, birth_date, country) "
                     "VALUES (?, ?, ?, ?, ?, ?, ?)");
        query.addBindValue(firstNames[i]);
        query.addBindValue(lastNames[i]);
        query.addBindValue(middleNames[i]);
        query.addBindValue("AB" + QString::number(1000000 + i));
        query.addBindValue("+7 999 111 22 " + QString::number(33 + i));

        // Генерируем случайную дату рождения (от 20 до 60 лет)
        QDate birthDate = QDate::currentDate().addYears(-(20 + rand() % 40));
        query.addBindValue(birthDate.toString("yyyy-MM-dd"));

        query.addBindValue(countries[i]);
        query.exec();
    }

//    // Добавляем тестовые бронирования
//    QDate today = QDate::currentDate();
//    for (int i = 1; i <= 10; i++) {
//        int bedId = i;
//        int clientId = (i % 8) + 1;
//        QDate checkIn = today.addDays(i * 2);
//        QDate checkOut = checkIn.addDays(3 + (i % 4));

//        query.prepare("INSERT INTO bookings (bed_id, client_id, check_in_date, check_out_date, total_price) "
//                     "VALUES (?, ?, ?, ?, ?)");
//        query.addBindValue(bedId);
//        query.addBindValue(clientId);
//        query.addBindValue(checkIn.toString("yyyy-MM-dd"));
//        query.addBindValue(checkOut.toString("yyyy-MM-dd"));
//        query.addBindValue((checkIn.daysTo(checkOut)) * 500.00);
//        query.exec();
//    }

    // Добавляем дополнительные услуги
    QList<QPair<QString, double>> services = {
        {"Завтрак", 300.00},
        {"Уборка номера", 200.00},
        {"Стирка", 150.00},
        {"Трансфер", 500.00},
        {"Хранение багажа", 100.00}
    };

    for (const auto& service : services) {
        query.prepare("INSERT INTO services (service_name, price) VALUES (?, ?)");
        query.addBindValue(service.first);
        query.addBindValue(service.second);
        query.exec();
    }
}

bool Database::addRoom(const QString& roomNumber, const QString& category, int bedsCount)
{
    QSqlQuery query;
    query.prepare("INSERT INTO rooms (room_number, category, beds_count) VALUES (?, ?, ?)");
    query.addBindValue(roomNumber);
    query.addBindValue(category);
    query.addBindValue(bedsCount);

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Ошибка добавления комнаты:" << query.lastError().text();
        return false;
    }
}

bool Database::addBooking(int bedId, int clientId, const QDate& checkInDate,
                         const QDate& checkOutDate, double totalPrice,
                         double paidAmount, const QString& paymentMethod)
{
    // Проверяем доступность койки перед добавлением
    if (!isBedAvailable(bedId, checkInDate, checkOutDate)) {
        qDebug() << "Кровать" << bedId << "занята на период"
                 << checkInDate.toString("dd.MM.yyyy") << "-"
                 << checkOutDate.toString("dd.MM.yyyy");
        return false;
    }

    QSqlQuery query;
    query.prepare("INSERT INTO bookings (bed_id, client_id, check_in_date, check_out_date, "
                 "total_price, paid_amount, payment_method, status) "
                 "VALUES (?, ?, ?, ?, ?, ?, ?, 'active')");
    query.addBindValue(bedId);
    query.addBindValue(clientId);
    query.addBindValue(checkInDate.toString("yyyy-MM-dd"));
    query.addBindValue(checkOutDate.toString("yyyy-MM-dd"));
    query.addBindValue(totalPrice);
    query.addBindValue(paidAmount);
    query.addBindValue(paymentMethod);

    if (query.exec()) {
        qDebug() << "Бронирование успешно создано для койки" << bedId
                 << "с" << checkInDate.toString("dd.MM.yyyy")
                 << "по" << checkOutDate.toString("dd.MM.yyyy");
        return true;
    } else {
        qDebug() << "Ошибка добавления бронирования:" << query.lastError().text();
        return false;
    }
}

// Метод для проверки доступности койки
bool Database::isBedAvailable(int bedId, const QDate& checkInDate, const QDate& checkOutDate)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM bookings "
                 "WHERE bed_id = ? "
                 "AND status = 'active' "
                 "AND check_in_date < ? "  // Проверка что check_in_date < checkOutDate (заезд до выезда)
                 "AND check_out_date > ? " // Проверка что check_out_date > checkInDate (выезд после заезда)
                 "AND ? < ?");             // Дополнительная проверка дат
    query.addBindValue(bedId);
    query.addBindValue(checkOutDate);
    query.addBindValue(checkInDate);
    query.addBindValue(checkInDate);
    query.addBindValue(checkOutDate);

    if (query.exec() && query.next()) {
        int count = query.value(0).toInt();
        qDebug() << "Проверка доступности койки" << bedId
                 << "за период" << checkInDate.toString("dd.MM.yyyy")
                 << "-" << checkOutDate.toString("dd.MM.yyyy")
                 << "результат:" << (count == 0 ? "свободно" : "занято");
        return count == 0;
    } else {
        qDebug() << "Ошибка проверки доступности:" << query.lastError().text();
        return false;
    }
}
// Метод для добавления категории
bool Database::addCategory(const QString& categoryName, const QString& color)
{
    QSqlQuery query;
    query.prepare("INSERT INTO room_categories (category_name, color) VALUES (?, ?)");
    query.addBindValue(categoryName);
    query.addBindValue(color);

    return query.exec();
}

// Метод для удаления категории
bool Database::removeCategory(const QString& categoryName)
{
    // Проверяем, используется ли категория в комнатах
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM rooms WHERE category = ?");
    checkQuery.addBindValue(categoryName);

    if (checkQuery.exec() && checkQuery.next()) {
        int usageCount = checkQuery.value(0).toInt();
        if (usageCount > 0) {
            qDebug() << "Категория" << categoryName << "используется в" << usageCount << "комнатах";
            return false;
        }
    }

    QSqlQuery query;
    query.prepare("DELETE FROM room_categories WHERE category_name = ?");
    query.addBindValue(categoryName);

    return query.exec();
}

// Метод для обновления цвета категории
bool Database::updateCategoryColor(const QString& categoryName, const QString& color)
{
    QSqlQuery query;
    query.prepare("UPDATE room_categories SET color = ? WHERE category_name = ?");
    query.addBindValue(color);
    query.addBindValue(categoryName);

    return query.exec();
}

// Метод для получения всех категорий
QList<QPair<QString, QString>> Database::getAllCategories()
{
    QList<QPair<QString, QString>> categories;
    QSqlQuery query("SELECT category_name, color FROM room_categories ORDER BY category_name");

    while (query.next()) {
        QString name = query.value(0).toString();
        QString color = query.value(1).toString();
        categories.append(qMakePair(name, color));
    }

    return categories;
}

// Метод для получения цвета категории
QString Database::getCategoryColor(const QString& categoryName)
{
    QSqlQuery query;
    query.prepare("SELECT color FROM room_categories WHERE category_name = ?");
    query.addBindValue(categoryName);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }

    return "#FFFFFF"; // Белый по умолчанию
}

bool Database::addClient(const QString& firstName, const QString& lastName,
                        const QString& passport, const QString& phone,
                        const QString& email)
{
    QSqlQuery query;
    query.prepare("INSERT INTO clients (first_name, last_name, passport_number, phone_number, email) "
                 "VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(firstName);
    query.addBindValue(lastName);
    query.addBindValue(passport);
    query.addBindValue(phone);
    query.addBindValue(email);

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Ошибка добавления клиента:" << query.lastError().text();
        return false;
    }
}

// НОВЫЙ метод для добавления клиента
bool Database::addClient(const QString& firstName, const QString& lastName,
                        const QString& middleName, const QString& passport,
                        const QString& phone, const QDate& birthDate,
                        const QString& country)
{
    QSqlQuery query;
    query.prepare("INSERT INTO clients (first_name, last_name, middle_name, "
                 "passport_number, phone_number, birth_date, country) "
                 "VALUES (?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(firstName);
    query.addBindValue(lastName);
    query.addBindValue(middleName.isEmpty() ? QVariant() : middleName);
    query.addBindValue(passport);
    query.addBindValue(phone);
    query.addBindValue(birthDate.toString("yyyy-MM-dd"));
    query.addBindValue(country.isEmpty() ? QVariant() : country);

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Ошибка добавления клиента:" << query.lastError().text();
        return false;
    }
}

// Метод для обновления клиента
bool Database::updateClient(int clientId, const QString& firstName, const QString& lastName,
                           const QString& middleName, const QString& passport,
                           const QString& phone, const QDate& birthDate,
                           const QString& country)
{
    QSqlQuery query;
    query.prepare("UPDATE clients SET "
                 "first_name = ?, "
                 "last_name = ?, "
                 "middle_name = ?, "
                 "passport_number = ?, "
                 "phone_number = ?, "
                 "birth_date = ?, "
                 "country = ? "
                 "WHERE id = ?");
    query.addBindValue(firstName);
    query.addBindValue(lastName);
    query.addBindValue(middleName.isEmpty() ? QVariant() : middleName);
    query.addBindValue(passport);
    query.addBindValue(phone);
    query.addBindValue(birthDate.toString("yyyy-MM-dd"));
    query.addBindValue(country.isEmpty() ? QVariant() : country);
    query.addBindValue(clientId);

    return query.exec();
}

// Метод для удаления клиента
bool Database::deleteClient(int clientId)
{
    // Проверяем, есть ли активные бронирования у клиента
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM bookings WHERE client_id = ? AND status = 'active'");
    checkQuery.addBindValue(clientId);

    if (checkQuery.exec() && checkQuery.next()) {
        int activeBookings = checkQuery.value(0).toInt();
        if (activeBookings > 0) {
            qDebug() << "Нельзя удалить клиента с активными бронированиями";
            return false;
        }
    }

    QSqlQuery query;
    query.prepare("DELETE FROM clients WHERE id = ?");
    query.addBindValue(clientId);

    return query.exec();
}

// Метод для получения всех клиентов
QList<QVariantMap> Database::getAllClients()
{
    QList<QVariantMap> clients;
    QSqlQuery query("SELECT id, first_name, last_name, middle_name, "
                   "passport_number, phone_number, birth_date, country, "
                   "registration_date, notes "
                   "FROM clients ORDER BY last_name, first_name");

    while (query.next()) {
        QVariantMap client;
        client["id"] = query.value(0);
        client["first_name"] = query.value(1);
        client["last_name"] = query.value(2);
        client["middle_name"] = query.value(3);
        client["passport_number"] = query.value(4);
        client["phone_number"] = query.value(5);
        client["birth_date"] = query.value(6);
        client["country"] = query.value(7);
        client["registration_date"] = query.value(8);
        client["notes"] = query.value(9);

        clients.append(client);
    }

    return clients;
}

// Метод для получения клиента по ID
QVariantMap Database::getClientById(int clientId)
{
    QSqlQuery query;
    query.prepare("SELECT id, first_name, last_name, middle_name, "
                 "passport_number, phone_number, birth_date, country, "
                 "registration_date, notes "
                 "FROM clients WHERE id = ?");
    query.addBindValue(clientId);

    if (query.exec() && query.next()) {
        QVariantMap client;
        client["id"] = query.value(0);
        client["first_name"] = query.value(1);
        client["last_name"] = query.value(2);
        client["middle_name"] = query.value(3);
        client["passport_number"] = query.value(4);
        client["phone_number"] = query.value(5);
        client["birth_date"] = query.value(6);
        client["country"] = query.value(7);
        client["registration_date"] = query.value(8);
        client["notes"] = query.value(9);

        return client;
    }

    return QVariantMap();
}

// Метод для проверки существования клиента по паспорту
bool Database::clientExists(const QString& passport)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM clients WHERE passport_number = ?");
    query.addBindValue(passport);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

double Database::calculateRevenue(const QDate& startDate, const QDate& endDate)
{
    QSqlQuery query;
    query.prepare("SELECT SUM(total_price) FROM bookings "
                 "WHERE status = 'active' "
                 "AND check_in_date BETWEEN ? AND ?");
    query.addBindValue(startDate.toString("yyyy-MM-dd"));
    query.addBindValue(endDate.toString("yyyy-MM-dd"));

    if (query.exec() && query.next()) {
        return query.value(0).toDouble();
    }

    return 0.0;
}

QVariantMap Database::getBookingInfo(const QString& roomNumber, int bedNumber, const QDate& date)
{
    QVariantMap info;

    QSqlQuery query;
    query.prepare("SELECT bk.id, bk.total_price, bk.paid_amount, "
                 "c.first_name || ' ' || c.last_name as client_name "
                 "FROM bookings bk "
                 "JOIN beds b ON bk.bed_id = b.id "
                 "JOIN rooms r ON b.room_id = r.id "
                 "JOIN clients c ON bk.client_id = c.id "
                 "WHERE r.room_number = ? "
                 "AND b.bed_number = ? "
                 "AND ? BETWEEN bk.check_in_date AND bk.check_out_date "
                 "AND bk.status = 'active'");
    query.addBindValue(roomNumber);  // Теперь roomNumber - QString
    query.addBindValue(bedNumber);
    query.addBindValue(date.toString("yyyy-MM-dd"));

    if (query.exec() && query.next()) {
        info["booking_id"] = query.value(0);
        info["total_price"] = query.value(1);
        info["paid_amount"] = query.value(2);
        info["client_name"] = query.value(3);

        // Рассчитываем процент оплаты
        double total = info["total_price"].toDouble();
        double paid = info["paid_amount"].toDouble();
        if (total > 0) {
            double percentage = (paid / total) * 100;
            info["paid_percentage"] = qRound(percentage);
        } else {
            info["paid_percentage"] = 0;
        }

        info["balance"] = total - paid;
    }

    return info;
}

//Методы для работы с оплатой
QVariantMap Database::getBookingInfoByDate(const QString& roomNumber, int bedNumber, const QDate& date)
{
    QVariantMap info;

    QSqlQuery query;
    query.prepare("SELECT bk.id, bk.total_price, bk.paid_amount, bk.payment_method, "
                 "c.first_name || ' ' || c.last_name as client_name, "
                 "bk.check_in_date, bk.check_out_date "
                 "FROM bookings bk "
                 "JOIN beds b ON bk.bed_id = b.id "
                 "JOIN rooms r ON b.room_id = r.id "
                 "JOIN clients c ON bk.client_id = c.id "
                 "WHERE r.room_number = ? "
                 "AND b.bed_number = ? "
                 "AND ? BETWEEN bk.check_in_date AND bk.check_out_date "
                 "AND bk.status = 'active'");
    query.addBindValue(roomNumber);
    query.addBindValue(bedNumber);
    query.addBindValue(date.toString("yyyy-MM-dd"));

    if (query.exec() && query.next()) {
        info["booking_id"] = query.value(0);
        info["total_price"] = query.value(1);
        info["paid_amount"] = query.value(2);
        info["payment_method"] = query.value(3);
        info["client_name"] = query.value(4);
        info["check_in_date"] = query.value(5);
        info["check_out_date"] = query.value(6);
    }

    return info;
}

bool Database::updatePayment(int bookingId, double paidAmount, const QString& paymentMethod)
{
    QSqlQuery query;

    if (paymentMethod.isEmpty()) {
        query.prepare("UPDATE bookings SET paid_amount = ? WHERE id = ?");
        query.addBindValue(paidAmount);
        query.addBindValue(bookingId);
    } else {
        query.prepare("UPDATE bookings SET paid_amount = ?, payment_method = ? WHERE id = ?");
        query.addBindValue(paidAmount);
        query.addBindValue(paymentMethod);
        query.addBindValue(bookingId);
    }

    if (query.exec()) {
        qDebug() << "Оплата обновлена для бронирования" << bookingId
                 << "на сумму" << paidAmount << "руб.";
        return true;
    } else {
        qDebug() << "Ошибка обновления оплаты:" << query.lastError().text();
        return false;
    }
}

QList<QVariantMap> Database::getPaymentReport(const QDate& startDate, const QDate& endDate)
{
    QList<QVariantMap> report;

    QSqlQuery query;
    query.prepare("SELECT "
                  "bk.check_in_date, "
                  "bk.check_out_date, "
                  "r.room_number, "
                  "b.bed_number, "
                  "c.last_name || ' ' || c.first_name as client_name, "
                  "bk.total_price, "
                  "bk.paid_amount, "
                  "bk.payment_method, "
                  "bk.status "
                  "FROM bookings bk "
                  "JOIN beds b ON bk.bed_id = b.id "
                  "JOIN rooms r ON b.room_id = r.id "
                  "JOIN clients c ON bk.client_id = c.id "
                  "WHERE bk.status = 'active' "
                  "AND bk.check_in_date <= ? "
                  "AND bk.check_out_date >= ? "
                  "ORDER BY bk.check_in_date, r.room_number, b.bed_number");
    query.addBindValue(endDate.toString("yyyy-MM-dd"));
    query.addBindValue(startDate.toString("yyyy-MM-dd"));

    if (query.exec()) {
        while (query.next()) {
            QVariantMap item;
            item["check_in_date"] = query.value(0);
            item["check_out_date"] = query.value(1);
            item["room_number"] = query.value(2);
            item["bed_number"] = query.value(3);
            item["client_name"] = query.value(4);
            item["total_price"] = query.value(5);
            item["paid_amount"] = query.value(6);
            item["payment_method"] = query.value(7);
            item["status"] = query.value(8);

            // Рассчитываем остаток
            double total = item["total_price"].toDouble();
            double paid = item["paid_amount"].toDouble();
            item["balance"] = total - paid;

            report.append(item);
        }
    }

    return report;
}

// Отмена бронирования
bool Database::cancelBooking(int bookingId)
{
    QSqlQuery query;

    // Проверяем существование бронирования
    query.prepare("SELECT id, status FROM bookings WHERE id = ?");
    query.addBindValue(bookingId);

    if (!query.exec() || !query.next()) {
        qDebug() << "Бронирование с ID" << bookingId << "не найдено";
        return false;
    }

    QString currentStatus = query.value(1).toString();
    if (currentStatus == "cancelled") {
        qDebug() << "Бронирование" << bookingId << "уже отменено";
        return true; // Уже отменено
    }

    // Обновляем статус бронирования на "cancelled"
    // ВАЖНО: в таблице bookings должна быть колонка cancelled_at
    // Если ее нет, нужно добавить ее в запрос CREATE TABLE
    query.prepare("UPDATE bookings SET status = 'cancelled', "
                 "cancelled_at = CURRENT_TIMESTAMP WHERE id = ?");
    query.addBindValue(bookingId);

    if (query.exec()) {
        qDebug() << "Бронирование" << bookingId << "успешно отменено";

        // Логируем отмену в истории изменений
        query.prepare("INSERT INTO change_history (table_name, record_id, action) "
                     "VALUES ('bookings', ?, 'CANCEL')");
        query.addBindValue(bookingId);
        query.exec(); // Это второй параметр, все верно

        return true;
    } else {
        qDebug() << "Ошибка отмены бронирования:" << query.lastError().text();
        return false;
    }
}

bool Database::removeBooking(int bookingId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM bookings WHERE id = ?");
    query.addBindValue(bookingId);

    if (query.exec()) {
        qDebug() << "Бронирование" << bookingId << "успешно удалено из базы данных";
        return true;
    } else {
        qDebug() << "Ошибка удаления бронирования:" << query.lastError().text();
        return false;
    }
}
