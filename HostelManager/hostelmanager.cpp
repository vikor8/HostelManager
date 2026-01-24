#include "hostelmanager.h"
#include "ui_hostelmanager.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDate>
#include <QHeaderView>
#include <QBrush>
#include <QColor>
#include <QRandomGenerator>
#include <QTime>
#include <QDebug>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>

HostelManager::HostelManager(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::HostelManager)
    , currentStartDate(QDate::currentDate())
    , database(new Database(this))
{
    ui->setupUi(this);

    qDebug() << "Конструктор HostelManager начал работу";

    // Инициализируем базу данных
    initializeDatabase();

    // Создаем меню бар
    createMenuBar();

    // Устанавливаем текущую дату
    ui->dateEdit->setDate(currentStartDate);

    // Подключаем сигналы и слоты
    connect(ui->btnToday, &QPushButton::clicked, this, &HostelManager::on_btnToday_clicked);
    connect(ui->btnRefresh, &QPushButton::clicked, this, &HostelManager::on_btnRefresh_clicked);
    connect(ui->dateEdit, &QDateEdit::dateChanged, this, &HostelManager::on_dateEdit_dateChanged);

    // Инициализируем таблицу
    initializeTable();

    qDebug() << "Конструктор HostelManager завершил работу";
}

HostelManager::~HostelManager()
{
    delete ui;
    if (database) {
        delete database;
    }
}

// Реализация метода getTableWidget
QTableWidget* HostelManager::getTableWidget()
{
    return ui->tableWidget;
}

void HostelManager::initializeDatabase()
{
    if (database->initializeDatabase()) {
        qDebug() << "База данных успешно инициализирована";
        ui->lblStatus->setText("База данных: подключена");
    } else {
        qDebug() << "Ошибка инициализации базы данных";
        ui->lblStatus->setText("База данных: не подключена (демо-режим)");
        QMessageBox::warning(this, "Ошибка базы данных",
                            "Не удалось подключиться к базе данных. "
                            "Приложение будет работать в демонстрационном режиме.");
    }
}

void HostelManager::createMenuBar()
{
    // Создаем меню бар
    QMenuBar *menuBar = new QMenuBar(this);
    this->setMenuBar(menuBar);

    // Создаем меню "База данных"
    QMenu *databaseMenu = menuBar->addMenu("&База данных");

    QAction *dbConnectAction = databaseMenu->addAction("&Подключить/переподключить");
    dbConnectAction->setShortcut(Qt::CTRL | Qt::Key_D);
    connect(dbConnectAction, &QAction::triggered, this, [this](){
        if (database->initializeDatabase()) {
            QMessageBox::information(this, "База данных", "База данных успешно подключена");
            ui->lblStatus->setText("База данных: подключена");
            initializeTable(); // Перезагружаем таблицу с новыми данными
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось подключиться к базе данных");
            ui->lblStatus->setText("База данных: не подключена");
        }
    });

    QAction *dbBackupAction = databaseMenu->addAction("&Создать резервную копию");
    dbBackupAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_B);
    connect(dbBackupAction, &QAction::triggered, this, [this](){
        QString fileName = QFileDialog::getSaveFileName(this, "Создать резервную копию",
            "BD_Kolcovo_backup_" + QDate::currentDate().toString("yyyy-MM-dd") + ".sqlite",
            "SQLite Database (*.sqlite)");
        if (!fileName.isEmpty()) {
            if (QFile::copy("BD_Kolcovo.sqlite", fileName)) {
                QMessageBox::information(this, "Резервная копия",
                                       "Резервная копия базы данных создана:\n" + fileName);
            } else {
                QMessageBox::warning(this, "Ошибка", "Не удалось создать резервную копию");
            }
        }
    });

    QAction *dbRestoreAction = databaseMenu->addAction("&Восстановить из копии");
    dbRestoreAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_R);
    connect(dbRestoreAction, &QAction::triggered, this, [this](){
        QString fileName = QFileDialog::getOpenFileName(this, "Восстановить из резервной копии",
            "", "SQLite Database (*.sqlite)");
        if (!fileName.isEmpty()) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, "Восстановление",
                "Вы уверены, что хотите восстановить базу данных из резервной копии?\n"
                "Текущие данные будут потеряны!",
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                // Закрываем текущее соединение
                if (database->isDatabaseConnected()) {
                    database->getDatabase().close();
                }

                // Удаляем текущую базу и копируем резервную
                QFile::remove("BD_Kolcovo.sqlite");
                if (QFile::copy(fileName, "BD_Kolcovo.sqlite")) {
                    QMessageBox::information(this, "Восстановление",
                                           "База данных восстановлена из резервной копии");
                    // Переподключаемся
                    if (database->initializeDatabase()) {
                        ui->lblStatus->setText("База данных: подключена");
                        initializeTable();
                    }
                } else {
                    QMessageBox::warning(this, "Ошибка", "Не удалось восстановить базу данных");
                }
            }
        }
    });

    QAction *dbResetAction = databaseMenu->addAction("&Создать новую базу данных");
    connect(dbResetAction, &QAction::triggered, this, [this](){
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Новая база данных",
            "Вы уверены, что хотите создать новую базу данных?\n"
            "Все текущие данные будут удалены без возможности восстановления!",
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            // Закрываем текущее соединение
            if (database->isDatabaseConnected()) {
                database->getDatabase().close();
            }

            // Удаляем текущую базу
            QFile::remove("BD_Kolcovo.sqlite");

            // Создаем новую
            if (database->initializeDatabase()) {
                QMessageBox::information(this, "Новая база данных",
                                       "Новая база данных успешно создана");
                ui->lblStatus->setText("База данных: подключена (новая)");
                initializeTable();
            }
        }
    });

    databaseMenu->addSeparator();

    QAction *dbStatsAction = databaseMenu->addAction("&Статистика базы");
    dbStatsAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_S);
    connect(dbStatsAction, &QAction::triggered, this, [this](){
        if (database->isDatabaseConnected()) {
            QSqlQuery query(database->getDatabase());
            QString stats = "<html><body><h3>Статистика базы данных</h3><table width='100%'>";

            query.exec("SELECT COUNT(*) FROM rooms");
            if (query.next()) stats += "<tr><td>Комнат:</td><td><b>" + query.value(0).toString() + "</b></td></tr>";

            query.exec("SELECT COUNT(*) FROM beds");
            if (query.next()) stats += "<tr><td>Кроватей:</td><td><b>" + query.value(0).toString() + "</b></td></tr>";

            query.exec("SELECT COUNT(*) FROM clients");
            if (query.next()) stats += "<tr><td>Клиентов:</td><td><b>" + query.value(0).toString() + "</b></td></tr>";

            query.exec("SELECT COUNT(*) FROM bookings WHERE status = 'active'");
            if (query.next()) stats += "<tr><td>Активных бронирований:</td><td><b>" + query.value(0).toString() + "</b></td></tr>";

            query.exec("SELECT COUNT(*) FROM bookings WHERE status = 'completed'");
            if (query.next()) stats += "<tr><td>Завершенных бронирований:</td><td><b>" + query.value(0).toString() + "</b></td></tr>";

            query.exec("SELECT SUM(total_price) FROM bookings WHERE status = 'active' "
                      "AND check_in_date >= date('now', '-30 days')");
            if (query.next()) {
                double revenue = query.value(0).toDouble();
                stats += "<tr><td>Доход за 30 дней:</td><td><b>" + QString::number(revenue, 'f', 2) + " руб.</b></td></tr>";
            }

            query.exec("SELECT COUNT(*) FROM services");
            if (query.next()) stats += "<tr><td>Дополнительных услуг:</td><td><b>" + query.value(0).toString() + "</b></td></tr>";

            stats += "</table></body></html>";

            QMessageBox::information(this, "Статистика базы данных", stats);
        } else {
            QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        }
    });

    // Создаем меню "Комнаты"
    QMenu *roomsMenu = menuBar->addMenu("&Комнаты");

    QAction *newRoomAction = roomsMenu->addAction("&Новая комната");
    newRoomAction->setShortcut(QKeySequence::New);
    connect(newRoomAction, &QAction::triggered, this, [this](){
        bool ok;
        QString roomNumber = QInputDialog::getText(this, "Новая комната",
                                                  "Введите номер комнаты:",
                                                  QLineEdit::Normal, "", &ok);
        if (ok && !roomNumber.isEmpty()) {
            QString category = QInputDialog::getItem(this, "Категория комнаты",
                                                    "Выберите категорию:",
                                                    QStringList() << "Эконом" << "Стандарт" << "Комфорт" << "Люкс",
                                                    1, false, &ok);
            if (ok && !category.isEmpty()) {
                if (database->isDatabaseConnected()) {
                    if (database->addRoom(roomNumber, category, 4)) {
                        QMessageBox::information(this, "Новая комната",
                                               "Комната " + roomNumber + " успешно добавлена");
                        initializeTable(); // Обновляем таблицу
                    } else {
                        QMessageBox::warning(this, "Ошибка", "Не удалось добавить комнату");
                    }
                } else {
                    QMessageBox::warning(this, "Ошибка", "База данных не подключена");
                }
            }
        }
    });

    QAction *editRoomAction = roomsMenu->addAction("&Редактировать комнату");
    editRoomAction->setShortcut(Qt::CTRL | Qt::Key_E);
    connect(editRoomAction, &QAction::triggered, this, [this](){
        QMessageBox::information(this, "Редактировать комнату", "Функция в разработке...");
    });

    QAction *deleteRoomAction = roomsMenu->addAction("&Удалить комнату");
    deleteRoomAction->setShortcut(Qt::CTRL | Qt::Key_D);
    connect(deleteRoomAction, &QAction::triggered, this, [this](){
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Удалить комнату",
            "Вы уверены, что хотите удалить комнату?\nЭто действие невозможно отменить.",
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            QMessageBox::information(this, "Удалить комнату", "Функция в разработке...");
        }
    });

    roomsMenu->addSeparator();

    QAction *exportRoomsAction = roomsMenu->addAction("&Экспорт списка комнат");
    exportRoomsAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_E);
    connect(exportRoomsAction, &QAction::triggered, this, [this](){
        QString fileName = QFileDialog::getSaveFileName(this, "Экспорт списка комнат",
            "rooms_export.xlsx", "Excel Files (*.xlsx);;CSV Files (*.csv)");
        if (!fileName.isEmpty()) {
            QMessageBox::information(this, "Экспорт", "Экспорт списка комнат: " + fileName);
            qDebug() << "Меню: Комнаты -> Экспорт списка комнат:" << fileName;
        }
    });

    QAction *printRoomsAction = roomsMenu->addAction("&Печать списка комнат");
    printRoomsAction->setShortcut(QKeySequence::Print);
    connect(printRoomsAction, &QAction::triggered, this, [this](){
        QMessageBox::information(this, "Печать", "Печать списка комнат...");
        qDebug() << "Меню: Комнаты -> Печать списка комнат";
    });

    roomsMenu->addSeparator();

    QAction *exitAction = roomsMenu->addAction("&Выход");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, [](){
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr, "Выход", "Вы уверены, что хотите выйти?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            qDebug() << "Меню: Комнаты -> Выход";
            QApplication::quit();
        }
    });

    // Создаем меню "Клиенты"
    QMenu *clientsMenu = menuBar->addMenu("&Клиенты");

    QAction *addClientAction = clientsMenu->addAction("&Добавить клиента");
    connect(addClientAction, &QAction::triggered, this, [this](){
        QMessageBox::information(this, "Добавить клиента", "Функция в разработке...");
    });

    QAction *viewClientsAction = clientsMenu->addAction("&Просмотр клиентов");
    connect(viewClientsAction, &QAction::triggered, this, [this](){
        if (database->isDatabaseConnected()) {
            QSqlQuery query(database->getDatabase());
            query.exec("SELECT first_name, last_name, phone_number, email FROM clients ORDER BY last_name");

            QString clientsList = "<html><body><h3>Список клиентов</h3><table border='1' width='100%'>"
                                 "<tr><th>Имя</th><th>Фамилия</th><th>Телефон</th><th>Email</th></tr>";

            while (query.next()) {
                clientsList += "<tr>";
                for (int i = 0; i < 4; ++i) {
                    clientsList += "<td>" + query.value(i).toString() + "</td>";
                }
                clientsList += "</tr>";
            }
            clientsList += "</table></body></html>";

            QMessageBox::information(this, "Просмотр клиентов", clientsList);
        } else {
            QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        }
    });

    QAction *editClientAction = clientsMenu->addAction("&Редактировать клиента");
    connect(editClientAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Редактировать клиента", "Редактирование данных клиента...");
        qDebug() << "Меню: Клиенты -> Редактировать клиента";
    });

    QAction *deleteClientAction = clientsMenu->addAction("&Удалить клиента");
    connect(deleteClientAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Удалить клиента", "Удаление клиента...");
        qDebug() << "Меню: Клиенты -> Удалить клиента";
    });

    // Создаем меню "Бронирование"
    QMenu *bookingMenu = menuBar->addMenu("&Бронирование");

    QAction *newBookingAction = bookingMenu->addAction("&Новое бронирование");
    connect(newBookingAction, &QAction::triggered, this, [this](){
        QMessageBox::information(this, "Новое бронирование", "Функция в разработке...");
    });

    QAction *viewBookingsAction = bookingMenu->addAction("&Просмотр бронирований");
    connect(viewBookingsAction, &QAction::triggered, this, [this](){
        if (database->isDatabaseConnected()) {
            QSqlQuery query(database->getDatabase());
            query.exec("SELECT room_number, bed_number, client_name, check_in_date, "
                      "check_out_date, total_price FROM current_bookings ORDER BY check_in_date");

            QString bookingsList = "<html><body><h3>Текущие бронирования</h3><table border='1' width='100%'>"
                                  "<tr><th>Комната</th><th>Койка</th><th>Клиент</th><th>Заезд</th><th>Выезд</th><th>Стоимость</th></tr>";

            while (query.next()) {
                bookingsList += "<tr>";
                for (int i = 0; i < 6; ++i) {
                    bookingsList += "<td>" + query.value(i).toString() + "</td>";
                }
                bookingsList += "</tr>";
            }
            bookingsList += "</table></body></html>";

            QMessageBox::information(this, "Просмотр бронирований", bookingsList);
        } else {
            QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        }
    });

    QAction *editBookingAction = bookingMenu->addAction("&Изменить бронирование");
    connect(editBookingAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Изменить бронирование", "Изменение данных бронирования...");
        qDebug() << "Меню: Бронирование -> Изменить бронирование";
    });

    QAction *cancelBookingAction = bookingMenu->addAction("&Отменить бронирование");
    connect(cancelBookingAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Отменить бронирование", "Отмена бронирования...");
        qDebug() << "Меню: Бронирование -> Отменить бронирование";
    });

    QAction *checkInAction = bookingMenu->addAction("&Заселение");
    connect(checkInAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Заселение", "Процедура заселения...");
        qDebug() << "Меню: Бронирование -> Заселение";
    });

    QAction *checkOutAction = bookingMenu->addAction("&Выселение");
    connect(checkOutAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Выселение", "Процедура выселения...");
        qDebug() << "Меню: Бронирование -> Выселение";
    });

    // Создаем меню "Доп. услуги"
    QMenu *servicesMenu = menuBar->addMenu("&Доп. услуги");

    QAction *addServiceAction = servicesMenu->addAction("&Добавить услугу");
    connect(addServiceAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Добавить услугу", "Добавление новой услуги...");
        qDebug() << "Меню: Доп. услуги -> Добавить услугу";
    });

    QAction *viewServicesAction = servicesMenu->addAction("&Просмотр услуг");
    connect(viewServicesAction, &QAction::triggered, this, [this](){
        if (database->isDatabaseConnected()) {
            QSqlQuery query(database->getDatabase());
            query.exec("SELECT service_name, description, price FROM services ORDER BY service_name");

            QString servicesList = "<html><body><h3>Дополнительные услуги</h3><table border='1' width='100%'>"
                                  "<tr><th>Название услуги</th><th>Описание</th><th>Цена</th></tr>";

            while (query.next()) {
                servicesList += "<tr>";
                servicesList += "<td>" + query.value(0).toString() + "</td>";
                servicesList += "<td>" + query.value(1).toString() + "</td>";
                servicesList += "<td>" + QString::number(query.value(2).toDouble(), 'f', 2) + " руб.</td>";
                servicesList += "</tr>";
            }
            servicesList += "</table></body></html>";

            QMessageBox::information(this, "Просмотр услуг", servicesList);
        } else {
            QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        }
    });

    QAction *assignServiceAction = servicesMenu->addAction("&Назначить услугу");
    connect(assignServiceAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Назначить услугу", "Назначение услуги клиенту...");
        qDebug() << "Меню: Доп. услуги -> Назначить услугу";
    });

    // Создаем меню "Статистика"
    QMenu *statsMenu = menuBar->addMenu("&Статистика");

    QAction *occupancyStatsAction = statsMenu->addAction("&Загрузка номеров");
    connect(occupancyStatsAction, &QAction::triggered, this, [this](){
        if (database->isDatabaseConnected()) {
            QSqlQuery query(database->getDatabase());
            query.exec("SELECT * FROM occupancy_stats");

            QString occupancyList = "<html><body><h3>Загрузка номеров</h3><table border='1' width='100%'>"
                                   "<tr><th>Комната</th><th>Всего коек</th><th>Занято коек</th><th>Загрузка</th></tr>";

            while (query.next()) {
                int totalBeds = query.value(1).toInt();
                int occupiedBeds = query.value(2).toInt();
                double occupancyRate = totalBeds > 0 ? (occupiedBeds * 100.0 / totalBeds) : 0;

                occupancyList += "<tr>";
                occupancyList += "<td>" + query.value(0).toString() + "</td>";
                occupancyList += "<td>" + QString::number(totalBeds) + "</td>";
                occupancyList += "<td>" + QString::number(occupiedBeds) + "</td>";
                occupancyList += "<td>" + QString::number(occupancyRate, 'f', 1) + "%</td>";
                occupancyList += "</tr>";
            }
            occupancyList += "</table></body></html>";

            QMessageBox::information(this, "Загрузка номеров", occupancyList);
        } else {
            QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        }
    });

    QAction *revenueStatsAction = statsMenu->addAction("&Финансовая статистика");
    connect(revenueStatsAction, &QAction::triggered, this, [this](){
        QMessageBox::information(this, "Финансовая статистика", "Функция в разработке...");
    });

    QAction *clientStatsAction = statsMenu->addAction("&Статистика по клиентам");
    connect(clientStatsAction, &QAction::triggered, this, [this](){
        QMessageBox::information(this, "Статистика по клиентам", "Функция в разработке...");
    });

    // Создаем меню "О программе"
    QMenu *helpMenu = menuBar->addMenu("&О программе");

    QAction *aboutAction = helpMenu->addAction("&О программе");
    connect(aboutAction, &QAction::triggered, this, [](){
        QMessageBox::about(nullptr, "О программе",
            "<h3>Hotel Manager - Система управления бронированиями</h3>"
            "<p>Версия: 1.0.0</p>"
            "<p>Разработано для управления хостелом</p>"
            "<p>База данных: SQLite (BD_Kolcovo.sqlite)</p>"
            "<p>© 2024 Все права защищены</p>");
        qDebug() << "Меню: О программе -> О программе";
    });

    QAction *aboutQtAction = helpMenu->addAction("&О Qt");
    connect(aboutQtAction, &QAction::triggered, this, [](){
        QMessageBox::aboutQt(nullptr, "О Qt");
        qDebug() << "Меню: О программе -> О Qt";
    });

    QAction *helpAction = helpMenu->addAction("&Справка");
    helpAction->setShortcut(QKeySequence::HelpContents);
    connect(helpAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Справка", "Открытие справки...");
        qDebug() << "Меню: О программе -> Справка";
    });
}

void HostelManager::on_btnToday_clicked()
{
    qDebug() << "Кнопка 'Сегодня' нажата";
    currentStartDate = QDate::currentDate();
    ui->dateEdit->setDate(currentStartDate);
    updateTableHeaders();
    updateTableColors();
}

void HostelManager::on_btnRefresh_clicked()
{
    qDebug() << "Кнопка 'Обновить' нажата";
    updateTableColors();
}

void HostelManager::on_dateEdit_dateChanged(const QDate &date)
{
    qDebug() << "Дата изменена на:" << date.toString("dd.MM.yyyy");
    currentStartDate = date;
    updateTableHeaders();
    updateTableColors();
}

void HostelManager::initializeTable()
{
    qDebug() << "Инициализация таблицы начата";

    try {
        // Настраиваем таблицу
        int totalColumns = 3 + DAYS_COUNT; // 3 основных столбца + дни
        ui->tableWidget->setColumnCount(totalColumns);

        // Обновляем заголовки
        updateTableHeaders();

        // Получаем данные из базы данных или используем тестовые
        QStringList rooms;
        QMap<QString, QString> roomCategories;

        if (database->isDatabaseConnected()) {
            QSqlQuery query(database->getDatabase());
            query.exec("SELECT room_number, category FROM rooms ORDER BY room_number");
            while (query.next()) {
                QString roomNumber = query.value(0).toString();
                QString category = query.value(1).toString();
                rooms.append(roomNumber);
                roomCategories.insert(roomNumber, category);
            }
        } else {
            // Если база данных не подключена, используем тестовые данные
            rooms = {"101", "102", "103", "104", "105", "201", "202", "203", "204", "205"};
            roomCategories = {
                {"101", "Эконом"}, {"102", "Эконом"}, {"103", "Стандарт"},
                {"104", "Стандарт"}, {"105", "Комфорт"}, {"201", "Эконом"},
                {"202", "Эконом"}, {"203", "Стандарт"}, {"204", "Комфорт"}, {"205", "Люкс"}
            };
        }

        // Рассчитываем количество строк: количество комнат * 4 кровати на комнату
        int rowCount = rooms.size() * 4;
        ui->tableWidget->setRowCount(rowCount);

        qDebug() << "Создана таблица:" << rowCount << "строк," << totalColumns << "столбцов";

        // Заполняем таблицу данными
        for (int row = 0; row < rowCount; ++row) {
            int roomIndex = row / 4; // 4 кровати на комнату
            int bedIndex = row % 4;

            if (roomIndex < rooms.size()) {
                QString roomNumber = rooms[roomIndex];

                // Номер комнаты
                QTableWidgetItem *roomItem = new QTableWidgetItem(roomNumber);
                roomItem->setTextAlignment(Qt::AlignCenter);
                roomItem->setFlags(roomItem->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->setItem(row, 0, roomItem);

                // Номер койки
                QTableWidgetItem *bedItem = new QTableWidgetItem(QString::number(bedIndex + 1));
                bedItem->setTextAlignment(Qt::AlignCenter);
                bedItem->setFlags(bedItem->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->setItem(row, 1, bedItem);

                // Категория
                QString category = roomCategories.value(roomNumber, "Стандарт");
                QTableWidgetItem *categoryItem = new QTableWidgetItem(category);
                categoryItem->setTextAlignment(Qt::AlignCenter);
                categoryItem->setFlags(categoryItem->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->setItem(row, 2, categoryItem);

                // Заполняем столбцы дней
                for (int col = 3; col < totalColumns; ++col) {
                    QTableWidgetItem *dayItem = new QTableWidgetItem("");
                    dayItem->setTextAlignment(Qt::AlignCenter);
                    dayItem->setFlags(dayItem->flags() & ~Qt::ItemIsEditable);
                    ui->tableWidget->setItem(row, col, dayItem);
                }
            }
        }

        qDebug() << "Ячейки созданы";

        // Настраиваем ширину столбцов
        ui->tableWidget->setColumnWidth(0, 120);  // Номер комнаты
        ui->tableWidget->setColumnWidth(1, 100);  // Номер койки
        ui->tableWidget->setColumnWidth(2, 100);  // Категория

        // Устанавливаем ширину для столбцов дней
        for (int col = 3; col < totalColumns; ++col) {
            ui->tableWidget->setColumnWidth(col, 35);
        }

        // Позволяем горизонтальную прокрутку
        ui->tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        // Устанавливаем высоту строк
        for (int row = 0; row < rowCount; ++row) {
            ui->tableWidget->setRowHeight(row, 25);
        }

        // Настраиваем заголовки столбцов
        QHeaderView *header = ui->tableWidget->horizontalHeader();
        header->setSectionResizeMode(0, QHeaderView::Fixed);
        header->setSectionResizeMode(1, QHeaderView::Fixed);
        header->setSectionResizeMode(2, QHeaderView::Fixed);

        // Для столбцов дней фиксированный размер
        for (int col = 3; col < totalColumns; ++col) {
            header->setSectionResizeMode(col, QHeaderView::Fixed);
        }

        // Делаем заголовки жирными
        QFont headerFont = header->font();
        headerFont.setBold(true);
        header->setFont(headerFont);

        // Устанавливаем стиль для четных и нечетных строк
        ui->tableWidget->setAlternatingRowColors(true);

        qDebug() << "Настройки таблицы применены";

        // Обновляем цвета на основе реальных данных
        updateTableColors();

        qDebug() << "Инициализация таблицы завершена успешно";

    } catch (const std::exception& e) {
        qDebug() << "Ошибка при инициализации таблицы:" << e.what();
    } catch (...) {
        qDebug() << "Неизвестная ошибка при инициализации таблицы";
    }
}

void HostelManager::updateTableHeaders()
{
    qDebug() << "Обновление заголовков таблицы";

    try {
        // Обновляем заголовки столбцов дней на основе текущей даты
        QStringList headers;
        headers << "Номер комнаты" << "Номер койки" << "Категория";

        // Добавляем заголовки для дней
        for (int day = 0; day < DAYS_COUNT; ++day) {
            QDate currentDate = currentStartDate.addDays(day);
            QString headerText = QString("%1\n%2")
                .arg(currentDate.toString("dd"))
                .arg(currentDate.toString("MMM"));
            headers << headerText;
        }

        ui->tableWidget->setHorizontalHeaderLabels(headers);

        // Устанавливаем подсказки для заголовков дней
        for (int day = 0; day < DAYS_COUNT; ++day) {
            QDate currentDate = currentStartDate.addDays(day);
            QString tooltip = currentDate.toString("dd.MM.yyyy - dddd");

            // Проверяем, что заголовок существует
            QTableWidgetItem *headerItem = ui->tableWidget->horizontalHeaderItem(3 + day);
            if (!headerItem) {
                headerItem = new QTableWidgetItem();
                ui->tableWidget->setHorizontalHeaderItem(3 + day, headerItem);
            }
            headerItem->setToolTip(tooltip);
        }

        qDebug() << "Заголовки обновлены";

    } catch (const std::exception& e) {
        qDebug() << "Ошибка при обновлении заголовков:" << e.what();
    }
}

void HostelManager::updateTableColors()
{
    qDebug() << "Обновление цветов таблицы";

    try {
        int rowCount = ui->tableWidget->rowCount();

        // Проверяем, что таблица инициализирована
        if (rowCount == 0) {
            qDebug() << "Таблица пуста, пропускаем обновление цветов";
            return;
        }

        // Цвета
        QColor freeColor(240, 240, 240);      // Светло-серый цвет для свободных
        QColor occupiedColor(200, 200, 0);    // Желтый для занятых
        QColor weekendColor(220, 220, 255);   // Светло-синий для выходных

        // Получаем данные о бронированиях из базы данных
        QMap<QString, QSet<QDate>> occupiedDates; // Ключ: "комната_койка", значение: набор занятых дат

        if (database->isDatabaseConnected()) {
            QSqlQuery query(database->getDatabase());
            query.prepare("SELECT r.room_number, b.bed_number, bk.check_in_date, bk.check_out_date "
                         "FROM bookings bk "
                         "JOIN beds b ON bk.bed_id = b.id "
                         "JOIN rooms r ON b.room_id = r.id "
                         "WHERE bk.status = 'active' "
                         "AND NOT (bk.check_out_date <= :start_date OR bk.check_in_date >= :end_date)");

            query.bindValue(":start_date", currentStartDate.toString("yyyy-MM-dd"));
            query.bindValue(":end_date", currentStartDate.addDays(DAYS_COUNT - 1).toString("yyyy-MM-dd"));

            if (query.exec()) {
                while (query.next()) {
                    QString roomNumber = query.value(0).toString();
                    int bedNumber = query.value(1).toInt();
                    QDate checkIn = QDate::fromString(query.value(2).toString(), "yyyy-MM-dd");
                    QDate checkOut = QDate::fromString(query.value(3).toString(), "yyyy-MM-dd");

                    QString key = roomNumber + "_" + QString::number(bedNumber);

                    // Добавляем все даты бронирования в набор
                    QDate date = checkIn;
                    while (date <= checkOut && date <= currentStartDate.addDays(DAYS_COUNT - 1)) {
                        if (date >= currentStartDate) {
                            occupiedDates[key].insert(date);
                        }
                        date = date.addDays(1);
                    }
                }
                qDebug() << "Получены данные о" << occupiedDates.size() << "занятых койках";
            } else {
                qDebug() << "Ошибка запроса занятых дат:" << query.lastError().text();
            }
        }

        for (int row = 0; row < rowCount; ++row) {
            // Проверяем, что строка существует
            if (row >= ui->tableWidget->rowCount()) {
                qDebug() << "Строка" << row << "не существует";
                break;
            }

            // Получаем номер комнаты и койки из таблицы
            QTableWidgetItem *roomItem = ui->tableWidget->item(row, 0);
            QTableWidgetItem *bedItem = ui->tableWidget->item(row, 1);

            if (!roomItem || !bedItem) {
                continue;
            }

            QString roomNumber = roomItem->text();
            QString bedNumber = bedItem->text();
            QString key = roomNumber + "_" + bedNumber;

            // Устанавливаем разные цвета фона для информационных колонок
            QColor infoColor = (row % 2 == 0) ? QColor(255, 255, 255) : QColor(245, 245, 245);

            for (int col = 0; col < 3; ++col) {
                QTableWidgetItem *item = ui->tableWidget->item(row, col);
                if (item) {
                    item->setBackground(infoColor);
                }
            }

            // Обрабатываем столбцы дней
            for (int day = 0; day < DAYS_COUNT; ++day) {
                int col = 3 + day;

                // Проверяем, что столбец существует
                if (col >= ui->tableWidget->columnCount()) {
                    qDebug() << "Столбец" << col << "не существует";
                    break;
                }

                QDate currentDate = currentStartDate.addDays(day);
                QTableWidgetItem *item = ui->tableWidget->item(row, col);
                if (item) {
                    // Определяем, является ли день выходным
                    bool isWeekend = (currentDate.dayOfWeek() == 6 || currentDate.dayOfWeek() == 7);

                    // Проверяем, занята ли койка на эту дату
                    bool isOccupied = occupiedDates.contains(key) &&
                                     occupiedDates[key].contains(currentDate);

                    // Устанавливаем цвет в зависимости от статуса
                    if (isOccupied) {
                        item->setBackground(occupiedColor);
                        item->setText("●"); // Маркер занятости
                        item->setForeground(Qt::black);
                    } else {
                        // Для свободных: выходные - светло-синий, рабочие - светло-серый
                        QColor baseColor = isWeekend ? weekendColor : freeColor;
                        item->setBackground(baseColor);
                        item->setText(""); // Очищаем текст
                    }

                    item->setTextAlignment(Qt::AlignCenter);

                    // Устанавливаем подсказку для ячейки
                    QString status = isOccupied ? "Занято" : "Свободно";
                    QString tooltip = QString("Комната: %1, Койка: %2\nДата: %3\nСтатус: %4")
                        .arg(roomNumber)
                        .arg(bedNumber)
                        .arg(currentDate.toString("dd.MM.yyyy"))
                        .arg(status);

                    if (isWeekend) {
                        tooltip += "\nВыходной день";
                    }

                    if (isOccupied && database->isDatabaseConnected()) {
                        // Получаем информацию о бронировании
                        QSqlQuery query(database->getDatabase());
                        query.prepare("SELECT c.first_name || ' ' || c.last_name as client_name, "
                                     "bk.check_in_date, bk.check_out_date "
                                     "FROM bookings bk "
                                     "JOIN clients c ON bk.client_id = c.id "
                                     "JOIN beds b ON bk.bed_id = b.id "
                                     "JOIN rooms r ON b.room_id = r.id "
                                     "WHERE r.room_number = ? AND b.bed_number = ? "
                                     "AND bk.status = 'active' "
                                     "AND ? BETWEEN bk.check_in_date AND bk.check_out_date");
                        query.addBindValue(roomNumber);
                        query.addBindValue(bedNumber);
                        query.addBindValue(currentDate.toString("yyyy-MM-dd"));

                        if (query.exec() && query.next()) {
                            QString clientName = query.value(0).toString();
                            QDate checkIn = QDate::fromString(query.value(1).toString(), "yyyy-MM-dd");
                            QDate checkOut = QDate::fromString(query.value(2).toString(), "yyyy-MM-dd");

                            tooltip += QString("\nКлиент: %1\nПериод: %2 - %3")
                                .arg(clientName)
                                .arg(checkIn.toString("dd.MM.yyyy"))
                                .arg(checkOut.toString("dd.MM.yyyy"));
                        }
                    }

                    item->setToolTip(tooltip);
                }
            }
        }

        // Обновляем статус
        QDate endDate = currentStartDate.addDays(DAYS_COUNT - 1);
        QString dbStatus = database->isDatabaseConnected() ? "подключена" : "не подключена (демо)";
        ui->lblStatus->setText(QString("Период: %1 - %2 | База данных: %3 | Обновлено: %4")
            .arg(currentStartDate.toString("dd.MM.yyyy"))
            .arg(endDate.toString("dd.MM.yyyy"))
            .arg(dbStatus)
            .arg(QTime::currentTime().toString("hh:mm:ss")));

        // Обновляем название группы
        ui->groupBox_2->setTitle(QString("Расписание занятости номеров (%1 дней)").arg(DAYS_COUNT));

        qDebug() << "Цвета таблицы обновлены успешно";

    } catch (const std::exception& e) {
        qDebug() << "Ошибка при обновлении цветов:" << e.what();
    } catch (...) {
        qDebug() << "Неизвестная ошибка при обновлении цветов";
    }
}
