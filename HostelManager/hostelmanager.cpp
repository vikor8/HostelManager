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

HostelManager::HostelManager(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::HostelManager)
    , currentStartDate(QDate::currentDate())
{
    ui->setupUi(this);

    qDebug() << "Конструктор HostelManager начал работу";

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
}

// Реализация метода getTableWidget
QTableWidget* HostelManager::getTableWidget()
{
    return ui->tableWidget;
}

void HostelManager::createMenuBar()
{
    // Создаем меню бар
    QMenuBar *menuBar = new QMenuBar(this);
    this->setMenuBar(menuBar);

    // Создаем меню "Комнаты"
    QMenu *roomsMenu = menuBar->addMenu("&Комнаты");

    QAction *newRoomAction = roomsMenu->addAction("&Новая комната");
    newRoomAction->setShortcut(QKeySequence::New);
    connect(newRoomAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Новая комната", "Добавление новой комнаты...");
        qDebug() << "Меню: Комнаты -> Новая комната";
    });

    QAction *editRoomAction = roomsMenu->addAction("&Редактировать комнату");
    editRoomAction->setShortcut(Qt::CTRL | Qt::Key_E);
    connect(editRoomAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Редактировать комнату", "Редактирование данных комнаты...");
        qDebug() << "Меню: Комнаты -> Редактировать комнату";
    });

    QAction *deleteRoomAction = roomsMenu->addAction("&Удалить комнату");
    deleteRoomAction->setShortcut(Qt::CTRL | Qt::Key_D);
    connect(deleteRoomAction, &QAction::triggered, this, [](){
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr, "Удалить комнату",
            "Вы уверены, что хотите удалить комнату?\nЭто действие невозможно отменить.",
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            QMessageBox::information(nullptr, "Удалить комнату", "Комната удалена...");
            qDebug() << "Меню: Комнаты -> Удалить комнату (подтверждено)";
        } else {
            qDebug() << "Меню: Комнаты -> Удалить комнату (отменено)";
        }
    });

    roomsMenu->addSeparator();

    QAction *exportRoomsAction = roomsMenu->addAction("&Экспорт списка комнат");
    exportRoomsAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_E);
    connect(exportRoomsAction, &QAction::triggered, this, [](){
        QString fileName = QFileDialog::getSaveFileName(nullptr, "Экспорт списка комнат",
            "rooms_export.xlsx", "Excel Files (*.xlsx);;CSV Files (*.csv)");
        if (!fileName.isEmpty()) {
            QMessageBox::information(nullptr, "Экспорт", "Экспорт списка комнат: " + fileName);
            qDebug() << "Меню: Комнаты -> Экспорт списка комнат:" << fileName;
        }
    });

    QAction *printRoomsAction = roomsMenu->addAction("&Печать списка комнат");
    printRoomsAction->setShortcut(QKeySequence::Print);
    connect(printRoomsAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Печать", "Печать списка комнат...");
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

    // Создаем меню "Клиенты" (остается без изменений)
    QMenu *clientsMenu = menuBar->addMenu("&Клиенты");

    QAction *addClientAction = clientsMenu->addAction("&Добавить клиента");
    connect(addClientAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Добавить клиента", "Добавление нового клиента...");
        qDebug() << "Меню: Клиенты -> Добавить клиента";
    });

    QAction *viewClientsAction = clientsMenu->addAction("&Просмотр клиентов");
    connect(viewClientsAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Просмотр клиентов", "Открытие списка клиентов...");
        qDebug() << "Меню: Клиенты -> Просмотр клиентов";
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

    // Создаем меню "Бронирование" (остается без изменений)
    QMenu *bookingMenu = menuBar->addMenu("&Бронирование");

    QAction *newBookingAction = bookingMenu->addAction("&Новое бронирование");
    connect(newBookingAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Новое бронирование", "Создание нового бронирования...");
        qDebug() << "Меню: Бронирование -> Новое бронирование";
    });

    QAction *viewBookingsAction = bookingMenu->addAction("&Просмотр бронирований");
    connect(viewBookingsAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Просмотр бронирований", "Открытие списка бронирований...");
        qDebug() << "Меню: Бронирование -> Просмотр бронирований";
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

    // Создаем меню "Доп. услуги" (остается без изменений)
    QMenu *servicesMenu = menuBar->addMenu("&Доп. услуги");

    QAction *addServiceAction = servicesMenu->addAction("&Добавить услугу");
    connect(addServiceAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Добавить услугу", "Добавление новой услуги...");
        qDebug() << "Меню: Доп. услуги -> Добавить услугу";
    });

    QAction *viewServicesAction = servicesMenu->addAction("&Просмотр услуг");
    connect(viewServicesAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Просмотр услуг", "Открытие списка услуг...");
        qDebug() << "Меню: Доп. услуги -> Просмотр услуг";
    });

    QAction *assignServiceAction = servicesMenu->addAction("&Назначить услугу");
    connect(assignServiceAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Назначить услугу", "Назначение услуги клиенту...");
        qDebug() << "Меню: Доп. услуги -> Назначить услугу";
    });

    // Создаем меню "Статистика" (остается без изменений)
    QMenu *statsMenu = menuBar->addMenu("&Статистика");

    QAction *occupancyStatsAction = statsMenu->addAction("&Загрузка номеров");
    connect(occupancyStatsAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Загрузка номеров", "Статистика загрузки номеров...");
        qDebug() << "Меню: Статистика -> Загрузка номеров";
    });

    QAction *revenueStatsAction = statsMenu->addAction("&Финансовая статистика");
    connect(revenueStatsAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Финансовая статистика", "Финансовая статистика...");
        qDebug() << "Меню: Статистика -> Финансовая статистика";
    });

    QAction *clientStatsAction = statsMenu->addAction("&Статистика по клиентам");
    connect(clientStatsAction, &QAction::triggered, this, [](){
        QMessageBox::information(nullptr, "Статистика по клиентам", "Статистика по клиентам...");
        qDebug() << "Меню: Статистика -> Статистика по клиентам";
    });

    // Создаем меню "О программе" (остается без изменений)
    QMenu *helpMenu = menuBar->addMenu("&О программе");

    QAction *aboutAction = helpMenu->addAction("&О программе");
    connect(aboutAction, &QAction::triggered, this, [](){
        QMessageBox::about(nullptr, "О программе",
            "<h3>Hotel Manager - Система управления бронированиями</h3>"
            "<p>Версия: 1.0.0</p>"
            "<p>Разработано для управления хостелом</p>"
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

        // Добавляем тестовые данные (15 строк для примера)
        int rowCount = 15;
        ui->tableWidget->setRowCount(rowCount);

        qDebug() << "Создана таблица:" << rowCount << "строк," << totalColumns << "столбцов";

        // Список комнат и категорий
        QStringList rooms = {"101", "102", "103", "104", "105", "201", "202", "203", "204", "205"};
        QMap<QString, QString> roomCategories = {
            {"101", "Эконом"}, {"102", "Эконом"}, {"103", "Стандарт"}, {"104", "Стандарт"}, {"105", "Комфорт"},
            {"201", "Эконом"}, {"202", "Эконом"}, {"203", "Стандарт"}, {"204", "Комфорт"}, {"205", "Люкс"}
        };

        // Сначала создаем все ячейки
        for (int row = 0; row < rowCount; ++row) {
            // Определяем комнату для текущей строки
            QString roomNumber = rooms[row % rooms.size()];

            // Номер комнаты
            QTableWidgetItem *roomItem = new QTableWidgetItem(roomNumber);
            roomItem->setTextAlignment(Qt::AlignCenter);
            roomItem->setFlags(roomItem->flags() & ~Qt::ItemIsEditable);
            ui->tableWidget->setItem(row, 0, roomItem);

            // Номер койки (1-4 койки в каждой комнате)
            int bedNumber = (row % 4) + 1;
            QTableWidgetItem *bedItem = new QTableWidgetItem(QString::number(bedNumber));
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

        // Обновляем цвета
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
            if (ui->tableWidget->horizontalHeaderItem(3 + day)) {
                ui->tableWidget->horizontalHeaderItem(3 + day)->setToolTip(tooltip);
            }
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
        QColor lightGrayColor(240, 240, 240); // Светло-серый цвет
        QColor occupiedColor(200, 200, 0);    // Желтый для занятых
        QColor weekendColor(220, 220, 255);   // Светло-синий для выходных

        // Создаем локальный генератор случайных чисел
        QRandomGenerator generator;

        for (int row = 0; row < rowCount; ++row) {
            // Проверяем, что строка существует
            if (row >= ui->tableWidget->rowCount()) {
                qDebug() << "Строка" << row << "не существует";
                break;
            }

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

                QTableWidgetItem *item = ui->tableWidget->item(row, col);
                if (item) {
                    // Определяем, является ли день выходным
                    QDate currentDate = currentStartDate.addDays(day);
                    bool isWeekend = (currentDate.dayOfWeek() == 6 || currentDate.dayOfWeek() == 7);

                    // Базовый цвет - светло-серый, для выходных - светло-синий
                    QColor baseColor = isWeekend ? weekendColor : lightGrayColor;
                    item->setBackground(baseColor);

                    // Для примера: случайным образом отмечаем некоторые ячейки как занятые
                    // Проверяем, что информационные ячейки существуют
                    QTableWidgetItem *roomItem = ui->tableWidget->item(row, 0);
                    QTableWidgetItem *bedItem = ui->tableWidget->item(row, 1);

                    if (roomItem && bedItem) {
                        QString roomNumber = roomItem->text();
                        QString bedNumber = bedItem->text();
                        QString uniqueKey = roomNumber + bedNumber + currentDate.toString("yyyyMMdd");

                        // Используем хеш как seed для предсказуемой генерации
                        quint32 seed = qHash(uniqueKey);
                        generator.seed(seed);

                        int randomValue = generator.bounded(100);

                        // Делаем занятые дни более сгруппированными
                        bool isOccupied = false;

                        // Проверяем, занята ли предыдущая ячейка (для создания непрерывных блоков)
                        if (day > 0) {
                            QTableWidgetItem *prevItem = ui->tableWidget->item(row, col - 1);
                            if (prevItem && prevItem->background().color() == occupiedColor) {
                                // Продолжаем блок занятости с вероятностью 70%
                                isOccupied = (randomValue < 70);
                            } else {
                                // Начинаем новый блок с вероятностью 25%
                                isOccupied = (randomValue < 25);
                            }
                        } else {
                            // Для первого дня
                            isOccupied = (randomValue < 25);
                        }

                        if (isOccupied) {
                            item->setBackground(occupiedColor); // Желтый для занятых
                            item->setText("●"); // Добавляем маркер занятости
                            item->setForeground(Qt::black); // Черный текст для контраста
                        } else {
                            item->setText(""); // Очищаем текст для свободных
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
                        item->setToolTip(tooltip);
                    }
                }
            }
        }

        // Обновляем статус
        QDate endDate = currentStartDate.addDays(DAYS_COUNT - 1);
        ui->lblStatus->setText(QString("Период: %1 - %2 | База данных: подключена | Обновлено: %3")
            .arg(currentStartDate.toString("dd.MM.yyyy"))
            .arg(endDate.toString("dd.MM.yyyy"))
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
