#include "database.h"

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

    // Таблица комнат
    query.exec("CREATE TABLE IF NOT EXISTS rooms ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "room_number TEXT UNIQUE NOT NULL,"
               "category TEXT NOT NULL,"
               "beds_count INTEGER DEFAULT 4,"
               "description TEXT,"
               "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
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

    // Таблица клиентов
    query.exec("CREATE TABLE IF NOT EXISTS clients ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "first_name TEXT NOT NULL,"
               "last_name TEXT NOT NULL,"
               "passport_number TEXT UNIQUE,"
               "phone_number TEXT,"
               "email TEXT,"
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
               "status TEXT DEFAULT 'active',"
               "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
               "FOREIGN KEY (bed_id) REFERENCES beds(id),"
               "FOREIGN KEY (client_id) REFERENCES clients(id),"
               "CHECK (check_out_date > check_in_date)"
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

    // Триггер для автоматического создания кроватей при добавлении комнаты
    query.exec("CREATE TRIGGER IF NOT EXISTS create_beds_after_room_insert "
               "AFTER INSERT ON rooms "
               "BEGIN "
               "   INSERT INTO beds (room_id, bed_number, price_per_day) "
               "   SELECT NEW.id, 1, 500.00 "
               "   UNION ALL SELECT NEW.id, 2, 500.00 "
               "   UNION ALL SELECT NEW.id, 3, 500.00 "
               "   UNION ALL SELECT NEW.id, 4, 500.00; "
               "END");

    // Триггер для логирования изменений в бронированиях
    query.exec("CREATE TRIGGER IF NOT EXISTS log_booking_changes "
               "AFTER INSERT ON bookings "
               "BEGIN "
               "   INSERT INTO change_history (table_name, record_id, action) "
               "   VALUES ('bookings', NEW.id, 'INSERT'); "
               "END");

    // Триггер для проверки пересечений дат бронирований
    query.exec("CREATE TRIGGER IF NOT EXISTS check_booking_overlap "
               "BEFORE INSERT ON bookings "
               "BEGIN "
               "   SELECT RAISE(ABORT, 'Кровать уже забронирована на эти даты') "
               "   WHERE EXISTS ("
               "       SELECT 1 FROM bookings "
               "       WHERE bed_id = NEW.bed_id "
               "       AND status = 'active' "
               "       AND NOT (NEW.check_out_date <= check_in_date OR NEW.check_in_date >= check_out_date)"
               "   ); "
               "END");
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

    // Добавляем тестовые комнаты
    QStringList rooms = {"101", "102", "103", "104", "105", "201", "202", "203", "204", "205"};
    QMap<QString, QString> categories = {
        {"101", "Эконом"}, {"102", "Эконом"}, {"103", "Стандарт"},
        {"104", "Стандарт"}, {"105", "Комфорт"}, {"201", "Эконом"},
        {"202", "Эконом"}, {"203", "Стандарт"}, {"204", "Комфорт"},
        {"205", "Люкс"}
    };

    for (const QString& room : rooms) {
        query.prepare("INSERT INTO rooms (room_number, category) VALUES (?, ?)");
        query.addBindValue(room);
        query.addBindValue(categories.value(room, "Стандарт"));
        query.exec();
    }

    // Обновляем цены кроватей в зависимости от категории
    query.exec("UPDATE beds SET price_per_day = "
               "CASE WHEN room_id IN (SELECT id FROM rooms WHERE category = 'Люкс') THEN 1500.00 "
               "     WHEN room_id IN (SELECT id FROM rooms WHERE category = 'Комфорт') THEN 1000.00 "
               "     WHEN room_id IN (SELECT id FROM rooms WHERE category = 'Стандарт') THEN 750.00 "
               "     ELSE 500.00 END");

    // Добавляем тестовых клиентов
    QStringList firstNames = {"Иван", "Петр", "Анна", "Мария", "Сергей", "Ольга", "Дмитрий", "Елена"};
    QStringList lastNames = {"Иванов", "Петров", "Сидорова", "Смирнов", "Кузнецов", "Попова", "Васильев", "Павлова"};

    for (int i = 0; i < 8; i++) {
        query.prepare("INSERT INTO clients (first_name, last_name, passport_number, phone_number, email) "
                     "VALUES (?, ?, ?, ?, ?)");
        query.addBindValue(firstNames[i]);
        query.addBindValue(lastNames[i]);
        query.addBindValue("AB" + QString::number(1000000 + i));
        query.addBindValue("+7 999 111 22 " + QString::number(33 + i));
        query.addBindValue(QString(firstNames[i]).toLower() + "." +
                          QString(lastNames[i]).toLower() + "@example.com");
        query.exec();
    }

    // Добавляем тестовые бронирования
    QDate today = QDate::currentDate();
    for (int i = 1; i <= 10; i++) {
        int bedId = i;
        int clientId = (i % 8) + 1;
        QDate checkIn = today.addDays(i * 2);
        QDate checkOut = checkIn.addDays(3 + (i % 4));

        query.prepare("INSERT INTO bookings (bed_id, client_id, check_in_date, check_out_date, total_price) "
                     "VALUES (?, ?, ?, ?, ?)");
        query.addBindValue(bedId);
        query.addBindValue(clientId);
        query.addBindValue(checkIn.toString("yyyy-MM-dd"));
        query.addBindValue(checkOut.toString("yyyy-MM-dd"));
        query.addBindValue((checkIn.daysTo(checkOut)) * 500.00);
        query.exec();
    }

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
                         const QDate& checkOutDate, double totalPrice)
{
    QSqlQuery query;
    query.prepare("INSERT INTO bookings (bed_id, client_id, check_in_date, check_out_date, total_price) "
                 "VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(bedId);
    query.addBindValue(clientId);
    query.addBindValue(checkInDate.toString("yyyy-MM-dd"));
    query.addBindValue(checkOutDate.toString("yyyy-MM-dd"));
    query.addBindValue(totalPrice);

    return query.exec();
}

bool Database::isBedAvailable(int bedId, const QDate& checkInDate, const QDate& checkOutDate)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM bookings "
                 "WHERE bed_id = ? "
                 "AND status = 'active' "
                 "AND NOT (? <= check_in_date OR ? >= check_out_date)");
    query.addBindValue(bedId);
    query.addBindValue(checkOutDate);
    query.addBindValue(checkInDate);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() == 0;
    }

    return false;
}

QList<QString> Database::getAllRooms()
{
    QList<QString> rooms;
    QSqlQuery query("SELECT room_number FROM rooms ORDER BY room_number");

    while (query.next()) {
        rooms.append(query.value(0).toString());
    }

    return rooms;
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

// Остальные методы реализуются аналогично...
