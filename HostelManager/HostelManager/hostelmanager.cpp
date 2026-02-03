#include "hostelmanager.h"
#include "ui_hostelmanager.h"

#include "database.h"
#include "addclientdialog.h"
#include "addbookingdialog.h"
#include "editpaymentdialog.h"

#include <QTableWidget>
#include <QTableWidgetItem>
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
#include <QInputDialog>
#include <QDialog>
#include <QFormLayout>
#include <QSpinBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QListWidget>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include "reportwindow.h"
#include "cancelbookingdialog.h"

HostelManager::HostelManager(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::HostelManager)
    , currentStartDate(QDate::currentDate())
    , database(new Database(this))
    , selectedBookingIdForDeletion(-1)
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

    // Добавляем соединение для двойного клика по таблице
    connect(ui->tableWidget, &QTableWidget::doubleClicked,
            this, &HostelManager::onTableDoubleClicked);

    // Включаем контекстное меню для таблицы
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableWidget, &QTableWidget::customContextMenuRequested,
            this, &HostelManager::onCustomContextMenuRequested);

    // Создаем действие для удаления бронирования
    deleteBookingAction = new QAction("Удалить бронирование", this);
    connect(deleteBookingAction, &QAction::triggered, this, &HostelManager::deleteBooking);

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

// Метод для перевода месяца на русский
QString HostelManager::monthToRussian(const QString& month) const
{
    static QMap<QString, QString> monthMap = {
        {"Jan", "Янв"}, {"Feb", "Фев"}, {"Mar", "Мар"}, {"Apr", "Апр"},
        {"May", "Май"}, {"Jun", "Июн"}, {"Jul", "Июл"}, {"Aug", "Авг"},
        {"Sep", "Сен"}, {"Oct", "Окт"}, {"Nov", "Ноя"}, {"Dec", "Дек"},
        {"January", "Январь"}, {"February", "Февраль"}, {"March", "Март"},
        {"April", "Апрель"}, {"May", "Май"}, {"June", "Июнь"},
        {"July", "Июль"}, {"August", "Август"}, {"September", "Сентябрь"},
        {"October", "Октябрь"}, {"November", "Ноябрь"}, {"December", "Декабрь"}
    };

    QString russianMonth = monthMap.value(month, month);
    return russianMonth;
}

// Метод для получения цвета категории
QColor HostelManager::getCategoryColor(const QString& category) const
{
    if (categoryColors.contains(category)) {
        return categoryColors.value(category);
    }

    // Возвращаем цвет по умолчанию в зависимости от категории
    if (category == "Эконом") return QColor(230, 243, 255); // Светло-голубой
    if (category == "Стандарт") return QColor(230, 255, 230); // Светло-зеленый
    if (category == "Комфорт") return QColor(255, 249, 230); // Светло-желтый
    if (category == "Люкс") return QColor(255, 230, 230); // Светло-красный

    return QColor(240, 240, 240); // Светло-серый по умолчанию
}

void HostelManager::initializeDatabase()
{
    if (database->initializeDatabase()) {
        qDebug() << "База данных успешно инициализирована";
        ui->lblStatus->setText("База данных: подключена");
        updateRoomIdMap(); // Обновляем карту ID комнат
        loadCategories(); // Загружаем категории
    } else {
        qDebug() << "Ошибка инициализации базы данных";
        ui->lblStatus->setText("База данных: не подключена (демо-режим)");
    }
}

void HostelManager::updateRoomIdMap()
{
    roomIdMap.clear();
    if (database->isDatabaseConnected()) {
        QSqlQuery query(database->getDatabase());
        query.exec("SELECT id, room_number FROM rooms");
        while (query.next()) {
            int roomId = query.value(0).toInt();
            QString roomNumber = query.value(1).toString();
            roomIdMap.insert(roomNumber, roomId);
        }
        qDebug() << "Карта ID комнат обновлена, количество:" << roomIdMap.size();
    }
}

void HostelManager::loadCategories()
{
    categoryColors.clear();

    if (database->isDatabaseConnected()) {
        QList<QPair<QString, QString>> categories = database->getAllCategories();

        for (const auto& category : categories) {
            QString name = category.first;
            QString colorStr = category.second;
            QColor color(colorStr);

            if (!color.isValid()) {
                color = QColor("#FFFFFF"); // Белый по умолчанию
            }

            categoryColors.insert(name, color);
        }

        qDebug() << "Загружено категорий:" << categoryColors.size();
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
            updateRoomIdMap();
            loadCategories();
            initializeTable();
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

            query.exec("SELECT COUNT(*) FROM room_categories");
            if (query.next()) stats += "<tr><td>Категорий комнат:</td><td><b>" + query.value(0).toString() + "</b></td></tr>";

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
    connect(newRoomAction, &QAction::triggered, this, &HostelManager::onAddRoom);

    QAction *editRoomAction = roomsMenu->addAction("&Редактировать комнату");
    editRoomAction->setShortcut(Qt::CTRL | Qt::Key_E);
    connect(editRoomAction, &QAction::triggered, this, &HostelManager::onEditRoom);

    QAction *deleteRoomAction = roomsMenu->addAction("&Удалить комнату");
    deleteRoomAction->setShortcut(Qt::CTRL | Qt::Key_D);
    connect(deleteRoomAction, &QAction::triggered, this, &HostelManager::onDeleteRoom);

    roomsMenu->addSeparator();

    // Добавляем кнопку "Категории"
    QAction *manageCategoriesAction = roomsMenu->addAction("&Категории комнат");
    manageCategoriesAction->setShortcut(Qt::CTRL | Qt::Key_C);
    connect(manageCategoriesAction, &QAction::triggered, this, &HostelManager::onManageCategories);

    roomsMenu->addSeparator();

    QAction *exportRoomsAction = roomsMenu->addAction("&Экспорт списка комнат");
    exportRoomsAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_E);
    connect(exportRoomsAction, &QAction::triggered, this, [this](){
        QMessageBox::information(this, "Экспорт", "Функция экспорта в разработке...");
    });

    QAction *printRoomsAction = roomsMenu->addAction("&Печать списка комнат");
    printRoomsAction->setShortcut(QKeySequence::Print);
    connect(printRoomsAction, &QAction::triggered, this, [this](){
        QMessageBox::information(this, "Печать", "Печать списка комнат...");
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

    QAction *viewClientsAction = clientsMenu->addAction("&Просмотр клиентов");
    viewClientsAction->setShortcut(Qt::CTRL | Qt::Key_V);
    connect(viewClientsAction, &QAction::triggered, this, &HostelManager::onViewClients);

    QAction *addClientAction = clientsMenu->addAction("&Добавить клиента");
    addClientAction->setShortcut(Qt::CTRL | Qt::Key_A);
    connect(addClientAction, &QAction::triggered, this, &HostelManager::onAddClient);

    QAction *editClientAction = clientsMenu->addAction("&Редактировать клиента");
    editClientAction->setShortcut(Qt::CTRL | Qt::Key_E);
    connect(editClientAction, &QAction::triggered, this, &HostelManager::onEditClient);

    QAction *deleteClientAction = clientsMenu->addAction("&Удалить клиента");
    deleteClientAction->setShortcut(Qt::CTRL | Qt::Key_D);
    connect(deleteClientAction, &QAction::triggered, this, &HostelManager::onDeleteClient);

    clientsMenu->addSeparator();

    QAction *clientStatsAction = clientsMenu->addAction("&Статистика клиентов");
    connect(clientStatsAction, &QAction::triggered, this, [this](){
        if (database->isDatabaseConnected()) {
            QSqlQuery query(database->getDatabase());

            QString stats = "<html><body><h3>Статистика клиентов</h3><table width='100%'>";

            query.exec("SELECT COUNT(*) FROM clients");
            if (query.next()) {
                stats += "<tr><td>Всего клиентов:</td><td><b>" + query.value(0).toString() + "</b></td></tr>";
            }

            query.exec("SELECT COUNT(DISTINCT country) FROM clients WHERE country IS NOT NULL AND country != ''");
            if (query.next()) {
                stats += "<tr><td>Представлено стран:</td><td><b>" + query.value(0).toString() + "</b></td></tr>";
            }

            query.exec("SELECT COUNT(*) FROM clients WHERE birth_date > date('now', '-30 years')");
            if (query.next()) {
                stats += "<tr><td>Моложе 30 лет:</td><td><b>" + query.value(0).toString() + "</b></td></tr>";
            }

            stats += "</table></body></html>";

            QMessageBox::information(this, "Статистика клиентов", stats);
        } else {
            QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        }
    });

    QMenu *bookingMenu = menuBar->addMenu("&Бронирование");

        QAction *newBookingAction = bookingMenu->addAction("&Новое бронирование");
        newBookingAction->setShortcut(Qt::Key_F6); // Горячая клавиша F6
        connect(newBookingAction, &QAction::triggered, this, &HostelManager::onAddBooking);

        QAction *viewBookingsAction = bookingMenu->addAction("&Просмотр бронирований");
        viewBookingsAction->setShortcut(Qt::CTRL | Qt::Key_B);
        connect(viewBookingsAction, &QAction::triggered, this, [this](){
            if (!database->isDatabaseConnected()) {
                QMessageBox::warning(this, "Ошибка", "База данных не подключена");
                return;
            }

            // Показываем список активных бронирований
            QSqlQuery query(database->getDatabase());
            query.exec("SELECT "
                      "b.id as booking_id, "
                      "r.room_number, "
                      "bd.bed_number, "
                      "c.last_name || ' ' || c.first_name as client_name, "
                      "b.check_in_date, "
                      "b.check_out_date, "
                      "b.total_price "
                      "FROM bookings b "
                      "JOIN beds bd ON b.bed_id = bd.id "
                      "JOIN rooms r ON bd.room_id = r.id "
                      "JOIN clients c ON b.client_id = c.id "
                      "WHERE b.status = 'active' "
                      "ORDER BY b.check_in_date");

            if (!query.isActive()) {
                QMessageBox::warning(this, "Ошибка", "Не удалось загрузить бронирования");
                return;
            }

            QString bookingsText = "<html><body><h3>Активные бронирования</h3>";
            bookingsText += "<table border='1' cellpadding='4' style='border-collapse: collapse;'>";
            bookingsText += "<tr><th>Комната</th><th>Койка</th><th>Клиент</th><th>Заезд</th><th>Выезд</th><th>Стоимость</th></tr>";

            bool hasBookings = false;
            while (query.next()) {
                hasBookings = true;
                bookingsText += QString("<tr>"
                    "<td>%1</td>"
                    "<td align='center'>%2</td>"
                    "<td>%3</td>"
                    "<td>%4</td>"
                    "<td>%5</td>"
                    "<td align='right'>%6 руб.</td>"
                    "</tr>")
                    .arg(query.value(1).toString())
                    .arg(query.value(2).toString())
                    .arg(query.value(3).toString())
                    .arg(QDate::fromString(query.value(4).toString(), "yyyy-MM-dd").toString("dd.MM.yyyy"))
                    .arg(QDate::fromString(query.value(5).toString(), "yyyy-MM-dd").toString("dd.MM.yyyy"))
                    .arg(query.value(6).toDouble(), 0, 'f', 2);
            }

            bookingsText += "</table></body></html>";

            if (!hasBookings) {
                bookingsText = "<h3>Нет активных бронирований</h3>";
            }

            QMessageBox::information(this, "Активные бронирования", bookingsText);
        });

        QAction *cancelBookingAction = bookingMenu->addAction("&Отменить бронирование");
        cancelBookingAction->setShortcut(Qt::CTRL | Qt::Key_X);
        connect(cancelBookingAction, &QAction::triggered, this, [this]() {
            if (!database->isDatabaseConnected()) {
                QMessageBox::warning(this, "Ошибка", "База данных не подключена");
                return;
            }

            CancelBookingDialog dialog(database, this);

            if (dialog.exec() == QDialog::Accepted) {
                int bookingId = dialog.selectedBookingId();
                QString reason = dialog.cancellationReason();

                if (bookingId <= 0) {
                    QMessageBox::warning(this, "Ошибка", "Не выбрано бронирование для отмены");
                    return;
                }

                // Запрашиваем подтверждение
                QMessageBox::StandardButton confirm = QMessageBox::question(this,
                    "Подтверждение отмены",
                    QString("Вы уверены, что хотите отменить выбранное бронирование?\n\n"
                           "ID бронирования: %1\n"
                           "Причина: %2\n\n"
                           "Это действие необратимо!")
                        .arg(bookingId)
                        .arg(reason.isEmpty() ? "Не указана" : reason),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);

                if (confirm == QMessageBox::Yes) {
                    if (database->cancelBooking(bookingId)) {
                        QMessageBox::information(this, "Успех",
                            "Бронирование успешно отменено!\n\n"
                            "ID бронирования: " + QString::number(bookingId) + "\n" +
                            "Причина: " + (reason.isEmpty() ? "Не указана" : reason));

                        // Обновляем таблицу
                        updateTableColors();
                    } else {
                        QMessageBox::warning(this, "Ошибка",
                            "Не удалось отменить бронирование.\n"
                            "Возможно, оно уже было отменено или не существует.");
                    }
                }
            }
        });

        bookingMenu->addSeparator();

        QAction *bookingStatsAction = bookingMenu->addAction("&Статистика бронирований");
        connect(bookingStatsAction, &QAction::triggered, this, [this](){
            if (!database->isDatabaseConnected()) {
                QMessageBox::warning(this, "Ошибка", "База данных не подключена");
                return;
            }

            QDate today = QDate::currentDate();
            QDate startOfMonth = QDate(today.year(), today.month(), 1);
            QDate endOfMonth = QDate(today.year(), today.month(), today.daysInMonth());

            QSqlQuery query(database->getDatabase());
            query.prepare("SELECT "
                         "COUNT(*) as total_bookings, "
                         "SUM(total_price) as total_revenue, "
                         "AVG(total_price) as avg_price "
                         "FROM bookings "
                         "WHERE status = 'active' "
                         "AND check_in_date BETWEEN ? AND ?");
            query.addBindValue(startOfMonth.toString("yyyy-MM-dd"));
            query.addBindValue(endOfMonth.toString("yyyy-MM-dd"));

            QString stats = "<html><body><h3>Статистика бронирований</h3>";
            stats += QString("<p>Период: %1 - %2</p>")
                .arg(startOfMonth.toString("dd.MM.yyyy"))
                .arg(endOfMonth.toString("dd.MM.yyyy"));

            if (query.exec() && query.next()) {
                stats += "<table width='100%'>";
                stats += QString("<tr><td>Всего бронирований:</td><td><b>%1</b></td></tr>")
                    .arg(query.value(0).toString());
                stats += QString("<tr><td>Общая выручка:</td><td><b>%1 руб.</b></td></tr>")
                    .arg(query.value(1).toDouble(), 0, 'f', 2);
                stats += QString("<tr><td>Средняя стоимость:</td><td><b>%1 руб.</b></td></tr>")
                    .arg(query.value(2).toDouble(), 0, 'f', 2);
                stats += "</table>";
            }

            stats += "</body></html>";

            QMessageBox::information(this, "Статистика бронирований", stats);
        });

        // Создаем меню "Отчеты"
        QMenu *reportsMenu = menuBar->addMenu("&Отчеты");

        QAction *reportsAction = reportsMenu->addAction("&Открыть отчеты");
        reportsAction->setShortcut(Qt::Key_F8);
        connect(reportsAction, &QAction::triggered, this, [this]() {
            if (!database->isDatabaseConnected()) {
                QMessageBox::warning(this, "Ошибка", "База данных не подключена");
                return;
            }

            ReportWindow *reportWindow = new ReportWindow(database);
            reportWindow->setAttribute(Qt::WA_DeleteOnClose);
            reportWindow->show();
            reportWindow->raise();
            reportWindow->activateWindow();
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
    });
}

// Слот для добавления новой комнаты
void HostelManager::onAddRoom()
{
    if (!database->isDatabaseConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }

    // Получаем список категорий из базы данных
    QList<QPair<QString, QString>> categories = database->getAllCategories();
    if (categories.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Нет доступных категорий. Сначала добавьте категории.");
        return;
    }

    // Создаем диалоговое окно
    QDialog dialog(this);
    dialog.setWindowTitle("Добавить новую комнату");
    dialog.setFixedSize(400, 300);

    // Создаем элементы формы
    QFormLayout *layout = new QFormLayout(&dialog);

    QLineEdit *roomNumberEdit = new QLineEdit(&dialog);
    roomNumberEdit->setPlaceholderText("Например: 101");
    layout->addRow("Номер комнаты:", roomNumberEdit);

    QSpinBox *bedsCountSpin = new QSpinBox(&dialog);
    bedsCountSpin->setRange(1, 10);
    bedsCountSpin->setValue(4);
    layout->addRow("Количество коек:", bedsCountSpin);

    // Комбобокс с категориями из базы данных
    QComboBox *categoryCombo = new QComboBox(&dialog);
    for (const auto& category : categories) {
        categoryCombo->addItem(category.first);

        // Устанавливаем цвет фона для элемента (ИСПРАВЛЕНО)
        QColor color(category.second);
        if (color.isValid()) {
            categoryCombo->setItemData(categoryCombo->count() - 1, QBrush(color), Qt::BackgroundRole);
            categoryCombo->setItemData(categoryCombo->count() - 1,
                                      QColor(color.lightness() > 128 ? Qt::black : Qt::white),
                                      Qt::ForegroundRole);
        }
    }
    layout->addRow("Категория:", categoryCombo);

    QDoubleSpinBox *priceSpin = new QDoubleSpinBox(&dialog);
    priceSpin->setRange(100, 10000);
    priceSpin->setValue(500);
    priceSpin->setSuffix(" руб./день");
    priceSpin->setDecimals(2);
    layout->addRow("Стоимость за день:", priceSpin);

    QLabel *infoLabel = new QLabel("Примечание: Для каждой койки в комнате будет установлена указанная цена", &dialog);
    infoLabel->setWordWrap(true);
    layout->addRow(infoLabel);

    // Кнопки
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // Проверяем ввод
    connect(roomNumberEdit, &QLineEdit::textChanged, [ roomNumberEdit, buttonBox]() {
        bool isValid = !roomNumberEdit->text().trimmed().isEmpty();
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid);
    });

    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    if (dialog.exec() == QDialog::Accepted) {
        QString roomNumber = roomNumberEdit->text().trimmed();
        int bedsCount = bedsCountSpin->value();
        QString category = categoryCombo->currentText();
        double pricePerDay = priceSpin->value();

        // Проверяем, существует ли уже такая комната
        QSqlQuery checkQuery(database->getDatabase());
        checkQuery.prepare("SELECT COUNT(*) FROM rooms WHERE room_number = ?");
        checkQuery.addBindValue(roomNumber);

        if (checkQuery.exec() && checkQuery.next() && checkQuery.value(0).toInt() > 0) {
            QMessageBox::warning(this, "Ошибка", "Комната с таким номером уже существует!");
            return;
        }

        // Временное отключение триггера
        QSqlQuery disableTriggerQuery(database->getDatabase());
        disableTriggerQuery.exec("DROP TRIGGER IF EXISTS create_beds_after_room_insert");

        try {
            // Добавляем комнату
            QSqlQuery query(database->getDatabase());
            query.prepare("INSERT INTO rooms (room_number, category, beds_count) VALUES (?, ?, ?)");
            query.addBindValue(roomNumber);
            query.addBindValue(category);
            query.addBindValue(bedsCount);

            if (!query.exec()) {
                QMessageBox::warning(this, "Ошибка", "Не удалось добавить комнату: " + query.lastError().text());
                return;
            }

            int roomId = query.lastInsertId().toInt();

            // Добавляем койки с указанной ценой
            for (int i = 1; i <= bedsCount; ++i) {
                QSqlQuery bedQuery(database->getDatabase());
                bedQuery.prepare("INSERT INTO beds (room_id, bed_number, price_per_day) VALUES (?, ?, ?)");
                bedQuery.addBindValue(roomId);
                bedQuery.addBindValue(i);
                bedQuery.addBindValue(pricePerDay);

                if (!bedQuery.exec()) {
                    if (!bedQuery.lastError().text().contains("UNIQUE constraint")) {
                        qDebug() << "Ошибка при добавлении койки:" << bedQuery.lastError().text();
                    }
                }
            }

            QMessageBox::information(this, "Успех",
                QString("Комната %1 успешно добавлена!\nКатегория: %2\nКоличество коек: %3\nЦена за день: %4 руб.")
                    .arg(roomNumber).arg(category).arg(bedsCount).arg(pricePerDay, 0, 'f', 2));

            updateRoomIdMap();
            initializeTable();

        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Ошибка", QString("Ошибка при добавлении комнаты: %1").arg(e.what()));
        }
    }
}

// Слот для редактирования комнаты
// Слот для редактирования комнаты
void HostelManager::onEditRoom()
{
    if (!database->isDatabaseConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }

    // Получаем список комнат
    QStringList roomsList;
    QMap<QString, QVariant> roomData; // room_number -> (id, category, beds_count)

    QSqlQuery query(database->getDatabase());
    query.exec("SELECT id, room_number, category, beds_count FROM rooms "
               "ORDER BY room_number");

    while (query.next()) {
        int id = query.value(0).toInt();
        QString roomNumber = query.value(1).toString();
        QString category = query.value(2).toString();
        int bedsCount = query.value(3).toInt(); // Берем из rooms.beds_count

        roomsList << roomNumber;
        roomData[roomNumber] = QVariantList() << id << category << bedsCount;
    }

    if (roomsList.isEmpty()) {
        QMessageBox::information(this, "Редактирование", "В базе данных нет комнат для редактирования");
        return;
    }

    bool ok;
    QString roomNumber = QInputDialog::getItem(this, "Выбор комнаты",
                                              "Выберите комнату для редактирования:",
                                              roomsList, 0, false, &ok);

    if (!ok || roomNumber.isEmpty()) {
        return;
    }

    // Получаем данные о выбранной комнате из roomData
    QVariantList data = roomData[roomNumber].toList();
    int roomId = data[0].toInt();
    QString currentCategory = data[1].toString();
    int currentBedsCount = data[2].toInt(); // Берем из rooms.beds_count

    qDebug() << "Редактирование комнаты" << roomNumber
             << "ID:" << roomId
             << "Категория:" << currentCategory
             << "Количество коек (из rooms):" << currentBedsCount;

    // Получаем текущую цену (берем цену первой активной койки)
    double currentPrice = 500.0;
    QSqlQuery priceQuery(database->getDatabase());
    priceQuery.prepare("SELECT price_per_day FROM beds WHERE room_id = ? AND is_active = 1 ORDER BY bed_number LIMIT 1");
    priceQuery.addBindValue(roomId);
    if (priceQuery.exec() && priceQuery.next()) {
        currentPrice = priceQuery.value(0).toDouble();
    }

    // Получаем реальное количество активных коек для проверки
    QSqlQuery activeBedsQuery(database->getDatabase());
    activeBedsQuery.prepare("SELECT COUNT(*) FROM beds WHERE room_id = ? AND is_active = 1");
    activeBedsQuery.addBindValue(roomId);

    int activeBedsCount = currentBedsCount;
    if (activeBedsQuery.exec() && activeBedsQuery.next()) {
        activeBedsCount = activeBedsQuery.value(0).toInt();

        // Если есть рассинхронизация, предлагаем исправить
        if (activeBedsCount != currentBedsCount) {
            qDebug() << "Обнаружена рассинхронизация! rooms.beds_count =" << currentBedsCount
                     << "активных коек в beds =" << activeBedsCount;

            QMessageBox::StandardButton fixButton = QMessageBox::question(this, "Расхождение данных",
                QString("Обнаружено расхождение в данных:\n\n"
                       "В таблице rooms указано: %1 койки\n"
                       "В таблице beds активно: %2 койки\n\n"
                       "Хотите синхронизировать данные?\n"
                       "(Использовать реальное количество активных коек: %2)")
                    .arg(currentBedsCount)
                    .arg(activeBedsCount),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                QMessageBox::Yes);

            if (fixButton == QMessageBox::Yes) {
                // Обновляем rooms.beds_count
                QSqlQuery updateCountQuery(database->getDatabase());
                updateCountQuery.prepare("UPDATE rooms SET beds_count = ? WHERE id = ?");
                updateCountQuery.addBindValue(activeBedsCount);
                updateCountQuery.addBindValue(roomId);

                if (updateCountQuery.exec()) {
                    currentBedsCount = activeBedsCount;
                    QMessageBox::information(this, "Синхронизация",
                        "Данные успешно синхронизированы!");
                }
            } else if (fixButton == QMessageBox::Cancel) {
                return; // Отмена редактирования
            }
        }
    }

    // Получаем список категорий из базы данных
    QList<QPair<QString, QString>> categories = database->getAllCategories();

    // Создаем диалоговое окно
    QDialog dialog(this);
    dialog.setWindowTitle("Редактировать комнату: " + roomNumber);
    dialog.setFixedSize(450, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    // Информация о комнате
    QLabel *roomLabel = new QLabel(roomNumber, &dialog);
    roomLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #2C3E50;");
    formLayout->addRow("Номер комнаты:", roomLabel);

    // Информация о текущем количестве коек
    QLabel *currentBedsInfo = new QLabel(
        QString("Текущее количество коек: <b>%1</b>").arg(currentBedsCount),
        &dialog);
    currentBedsInfo->setStyleSheet("color: #7F8C8D;");
    formLayout->addRow("", currentBedsInfo);

    // Поле для изменения количества коек
    QSpinBox *bedsCountSpin = new QSpinBox(&dialog);
    bedsCountSpin->setRange(1, 20);
    bedsCountSpin->setValue(currentBedsCount);
    bedsCountSpin->setSuffix(" койки");
    formLayout->addRow("Новое количество коек:", bedsCountSpin);

    // Комбобокс с категориями
    QComboBox *categoryCombo = new QComboBox(&dialog);
    for (const auto& category : categories) {
        categoryCombo->addItem(category.first);

        // Устанавливаем цвет фона для элемента
        QColor color(category.second);
        if (color.isValid()) {
            categoryCombo->setItemData(categoryCombo->count() - 1, QBrush(color), Qt::BackgroundRole);
            categoryCombo->setItemData(categoryCombo->count() - 1,
                                      QColor(color.lightness() > 128 ? Qt::black : Qt::white),
                                      Qt::ForegroundRole);
        }
    }

    // Устанавливаем текущую категорию
    int categoryIndex = categoryCombo->findText(currentCategory);
    if (categoryIndex >= 0) {
        categoryCombo->setCurrentIndex(categoryIndex);
    }

    formLayout->addRow("Категория:", categoryCombo);

    // Цена за день
    QDoubleSpinBox *priceSpin = new QDoubleSpinBox(&dialog);
    priceSpin->setRange(100, 10000);
    priceSpin->setValue(currentPrice);
    priceSpin->setSuffix(" руб./день");
    priceSpin->setDecimals(2);
    priceSpin->setSingleStep(100);
    formLayout->addRow("Стоимость за день:", priceSpin);

    mainLayout->addLayout(formLayout);

    // Чекбокс для обновления цены всех коек
    QCheckBox *updateAllBedsCheck = new QCheckBox("Обновить цену для всех коек в комнате", &dialog);
    updateAllBedsCheck->setChecked(true);
    updateAllBedsCheck->setStyleSheet("margin-top: 10px;");
    mainLayout->addWidget(updateAllBedsCheck);

    // Предупреждение при уменьшении количества коек
    QLabel *warningLabel = new QLabel("", &dialog);
    warningLabel->setStyleSheet("color: #E74C3C; font-style: italic; padding: 5px;");
    warningLabel->setVisible(false);
    warningLabel->setWordWrap(true);
    mainLayout->addWidget(warningLabel);

    // Соединяем изменение количества коек с показом предупреждения
    connect(bedsCountSpin, QOverload<int>::of(&QSpinBox::valueChanged), [=](int newValue) {
        if (newValue < currentBedsCount) {
            int diff = currentBedsCount - newValue;
            warningLabel->setText(QString(
                "⚠ Внимание: количество коек будет уменьшено на %1.\n"
                "Койки с номерами больше %2 будут помечены как неактивные.")
                .arg(diff).arg(newValue));
            warningLabel->setVisible(true);
        } else {
            warningLabel->setVisible(false);
        }
    });

    mainLayout->addStretch();

    // Кнопки
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);

    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        int newBedsCount = bedsCountSpin->value();
        QString newCategory = categoryCombo->currentText();
        double newPrice = priceSpin->value();
        bool updateAllBeds = updateAllBedsCheck->isChecked();

        // Начинаем транзакцию для атомарности
        QSqlDatabase::database().transaction();

        try {
            // 1. Обновляем данные комнаты (главное - rooms.beds_count!)
            QSqlQuery updateRoomQuery(database->getDatabase());
            updateRoomQuery.prepare("UPDATE rooms SET category = ?, beds_count = ? WHERE id = ?");
            updateRoomQuery.addBindValue(newCategory);
            updateRoomQuery.addBindValue(newBedsCount); // Сохраняем новое количество!
            updateRoomQuery.addBindValue(roomId);

            if (!updateRoomQuery.exec()) {
                QSqlDatabase::database().rollback();
                QMessageBox::warning(this, "Ошибка",
                    "Не удалось обновить данные комнаты: " + updateRoomQuery.lastError().text());
                return;
            }

            // 2. Обновляем цену коек (если нужно)
            if (updateAllBeds) {
                QSqlQuery priceUpdateQuery(database->getDatabase());
                priceUpdateQuery.prepare("UPDATE beds SET price_per_day = ? WHERE room_id = ? AND is_active = 1");
                priceUpdateQuery.addBindValue(newPrice);
                priceUpdateQuery.addBindValue(roomId);

                if (!priceUpdateQuery.exec()) {
                    qDebug() << "Не удалось обновить цены коек:" << priceUpdateQuery.lastError().text();
                    // Не прерываем выполнение - это не критическая ошибка
                }
            }

            // 3. Обрабатываем изменение количества коек
            if (newBedsCount != currentBedsCount) {
                // Получаем максимальный номер активной койки
                QSqlQuery maxBedQuery(database->getDatabase());
                maxBedQuery.prepare("SELECT MAX(bed_number) FROM beds WHERE room_id = ? AND is_active = 1");
                maxBedQuery.addBindValue(roomId);

                int maxActiveBedNumber = 0;
                if (maxBedQuery.exec() && maxBedQuery.next()) {
                    maxActiveBedNumber = maxBedQuery.value(0).toInt();
                }

                // Если нужно больше коек - добавляем
                if (newBedsCount > maxActiveBedNumber) {
                    int bedsToAdd = newBedsCount - maxActiveBedNumber;
                    qDebug() << "Нужно добавить" << bedsToAdd << "койки";

                    for (int i = 1; i <= bedsToAdd; ++i) {
                        QSqlQuery addBedQuery(database->getDatabase());
                        addBedQuery.prepare(
                            "INSERT INTO beds (room_id, bed_number, price_per_day, is_active) "
                            "VALUES (?, ?, ?, 1)");
                        addBedQuery.addBindValue(roomId);
                        addBedQuery.addBindValue(maxActiveBedNumber + i);
                        addBedQuery.addBindValue(newPrice);

                        if (!addBedQuery.exec()) {
                            qDebug() << "Ошибка при добавлении койки" << i << ":" << addBedQuery.lastError().text();
                        }
                    }
                }
                // Если нужно меньше коек - помечаем как неактивные
                else if (newBedsCount < maxActiveBedNumber) {
                    // Проверяем, есть ли активные бронирования на койках, которые будут деактивированы
                    QSqlQuery checkBookingsQuery(database->getDatabase());
                    checkBookingsQuery.prepare(
                        "SELECT COUNT(*) FROM bookings bk "
                        "JOIN beds b ON bk.bed_id = b.id "
                        "WHERE b.room_id = ? "
                        "AND b.bed_number > ? "
                        "AND b.is_active = 1 "
                        "AND bk.status = 'active'"
                    );
                    checkBookingsQuery.addBindValue(roomId);
                    checkBookingsQuery.addBindValue(newBedsCount);

                    int activeBookingsOnRemovedBeds = 0;
                    if (checkBookingsQuery.exec() && checkBookingsQuery.next()) {
                        activeBookingsOnRemovedBeds = checkBookingsQuery.value(0).toInt();
                    }

                    if (activeBookingsOnRemovedBeds > 0) {
                        QSqlDatabase::database().rollback();
                        QMessageBox::warning(this, "Ошибка",
                            QString("Нельзя уменьшить количество коек до %1!\n\n"
                                   "На койках с номерами больше %1 есть %2 активных бронирований.\n"
                                   "Сначала отмените или завершите эти бронирования.")
                                .arg(newBedsCount)
                                .arg(activeBookingsOnRemovedBeds));
                        return;
                    }

                    // Помечаем лишние койки как неактивные
                    QSqlQuery deactivateQuery(database->getDatabase());
                    deactivateQuery.prepare("UPDATE beds SET is_active = 0 WHERE room_id = ? AND bed_number > ?");
                    deactivateQuery.addBindValue(roomId);
                    deactivateQuery.addBindValue(newBedsCount);

                    if (!deactivateQuery.exec()) {
                        qDebug() << "Ошибка при деактивации коек:" << deactivateQuery.lastError().text();
                    }
                }
            }

            // Фиксируем транзакцию
            if (QSqlDatabase::database().commit()) {
                QMessageBox::information(this, "Успех",
                    QString("Данные комнаты обновлены!\n\n"
                           "• Категория: %1\n"
                           "• Количество коек: %2\n"
                           "• Цена за день: %3 руб.")
                        .arg(newCategory)
                        .arg(newBedsCount)
                        .arg(newPrice, 0, 'f', 2));

                // Обновляем данные в приложении
                updateRoomIdMap();
                loadCategories();
                initializeTable();

            } else {
                QMessageBox::warning(this, "Ошибка", "Не удалось завершить операцию");
            }

        } catch (const std::exception& e) {
            QSqlDatabase::database().rollback();
            QMessageBox::warning(this, "Ошибка",
                QString("Ошибка при обновлении комнаты:\n%1").arg(e.what()));
        }
    }
}

// Слот для удаления комнаты
void HostelManager::onDeleteRoom()
{
    if (!database->isDatabaseConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }

    // Получаем список комнат
    QStringList roomsList;
    QMap<QString, int> roomIdMapLocal;

    QSqlQuery query(database->getDatabase());
    query.exec("SELECT id, room_number FROM rooms ORDER BY room_number");

    while (query.next()) {
        int id = query.value(0).toInt();
        QString roomNumber = query.value(1).toString();
        roomsList << roomNumber;
        roomIdMapLocal.insert(roomNumber, id);
    }

    if (roomsList.isEmpty()) {
        QMessageBox::information(this, "Удаление", "В базе данных нет комнат для удаления");
        return;
    }

    bool ok;
    QString roomNumber = QInputDialog::getItem(this, "Удаление комнаты",
                                              "Выберите комнату для удаления:",
                                              roomsList, 0, false, &ok);

    if (!ok || roomNumber.isEmpty()) {
        return;
    }

    int roomId = roomIdMapLocal.value(roomNumber);

    // Проверяем, есть ли активные бронирования
    QSqlQuery checkBookingQuery(database->getDatabase());
    checkBookingQuery.prepare(
        "SELECT COUNT(*) FROM bookings bk "
        "JOIN beds b ON bk.bed_id = b.id "
        "WHERE b.room_id = ? AND bk.status = 'active'"
    );
    checkBookingQuery.addBindValue(roomId);

    if (checkBookingQuery.exec() && checkBookingQuery.next()) {
        int activeBookings = checkBookingQuery.value(0).toInt();

        if (activeBookings > 0) {
            QMessageBox::warning(this, "Ошибка",
                QString("Нельзя удалить комнату %1!\n"
                       "В ней есть %2 активных бронирований.\n"
                       "Сначала отмените все бронирования.")
                    .arg(roomNumber).arg(activeBookings));
            return;
        }
    }

    // Запрос подтверждения
    QMessageBox::StandardButton confirm = QMessageBox::question(this, "Подтверждение удаления",
        QString("Вы уверены, что хотите удалить комнату %1?\n\n"
               "Это действие удалит:\n"
               "- Все данные о комнате\n"
               "- Все койки в комнате\n"
               "- Все исторические бронирования\n\n"
               "Действие необратимо!")
            .arg(roomNumber),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (confirm == QMessageBox::Yes) {
        // Удаляем комнату (каскадное удаление удалит все связанные койки)
        QSqlQuery deleteQuery(database->getDatabase());
        deleteQuery.prepare("DELETE FROM rooms WHERE id = ?");
        deleteQuery.addBindValue(roomId);

        if (deleteQuery.exec()) {
            if (deleteQuery.numRowsAffected() > 0) {
                QMessageBox::information(this, "Успех", "Комната успешно удалена!");
                updateRoomIdMap();
                initializeTable();
            } else {
                QMessageBox::warning(this, "Ошибка", "Не удалось удалить комнату");
            }
        } else {
            QMessageBox::warning(this, "Ошибка", "Ошибка при удалении: " + deleteQuery.lastError().text());
        }
    }
}

// Слот для управления категориями
void HostelManager::onManageCategories()
{
    if (!database->isDatabaseConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }

    // Создаем диалоговое окно
    QDialog dialog(this);
    dialog.setWindowTitle("Управление категориями комнат");
    dialog.setFixedSize(500, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    // Список категорий
    QListWidget *categoriesList = new QListWidget(&dialog);
    categoriesList->setAlternatingRowColors(true);
    mainLayout->addWidget(new QLabel("Список категорий:", &dialog));
    mainLayout->addWidget(categoriesList);

    // Форма для добавления/редактирования категории
    QGroupBox *editGroup = new QGroupBox("Добавить/редактировать категорию", &dialog);
    QFormLayout *editLayout = new QFormLayout(editGroup);

    QLineEdit *categoryNameEdit = new QLineEdit(&dialog);
    categoryNameEdit->setPlaceholderText("Введите название категории");
    editLayout->addRow("Название:", categoryNameEdit);

    // Виджет для выбора цвета
    QHBoxLayout *colorLayout = new QHBoxLayout();
    QPushButton *colorButton = new QPushButton("Выбрать цвет", &dialog);
    QLabel *colorPreview = new QLabel(&dialog);
    colorPreview->setFixedSize(50, 25);
    colorPreview->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    colorPreview->setStyleSheet("background-color: #FFFFFF;");

    colorLayout->addWidget(colorButton);
    colorLayout->addWidget(colorPreview);
    colorLayout->addStretch();

    editLayout->addRow("Цвет:", colorLayout);

    // Кнопки действий
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    QPushButton *addButton = new QPushButton("Добавить", &dialog);
    QPushButton *updateButton = new QPushButton("Обновить", &dialog);
    QPushButton *deleteButton = new QPushButton("Удалить", &dialog);
    QPushButton *closeButton = new QPushButton("Закрыть", &dialog);

    buttonsLayout->addWidget(addButton);
    buttonsLayout->addWidget(updateButton);
    buttonsLayout->addWidget(deleteButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(closeButton);

    editLayout->addRow(buttonsLayout);

    mainLayout->addWidget(editGroup);

    // Загружаем категории из базы данных в список
    QList<QPair<QString, QString>> categories = database->getAllCategories();
    for (const auto& category : categories) {
        QString name = category.first;
        QString colorStr = category.second;

        QListWidgetItem *item = new QListWidgetItem(name);
        QColor color(colorStr);
        if (!color.isValid()) {
            color = QColor("#FFFFFF");
        }

        item->setBackground(color);
        item->setForeground(color.lightness() > 128 ? QColor(Qt::black) : QColor(Qt::white));
        item->setToolTip(QString("Цвет: %1").arg(colorStr));

        categoriesList->addItem(item);
    }

    // Текущий выбранный цвет
    QColor currentColor = QColor("#FFFFFF");

    // Обработчики событий
    connect(colorButton, &QPushButton::clicked, [&]() {
        QColor color = QColorDialog::getColor(currentColor, &dialog, "Выберите цвет категории");
        if (color.isValid()) {
            currentColor = color;
            colorPreview->setStyleSheet(QString("background-color: %1; border: 1px solid #000000;")
                                       .arg(currentColor.name()));
        }
    });

    connect(categoriesList, &QListWidget::itemClicked, [&](QListWidgetItem *item) {
        QString categoryName = item->text();
        categoryNameEdit->setText(categoryName);

        // Получаем цвет категории из базы данных
        QString colorStr = database->getCategoryColor(categoryName);
        currentColor = QColor(colorStr);
        if (!currentColor.isValid()) {
            currentColor = QColor("#FFFFFF");
        }

        colorPreview->setStyleSheet(QString("background-color: %1; border: 1px solid #000000;")
                                   .arg(currentColor.name()));
    });

    connect(addButton, &QPushButton::clicked, [&]() {
        QString categoryName = categoryNameEdit->text().trimmed();

        if (categoryName.isEmpty()) {
            QMessageBox::warning(&dialog, "Ошибка", "Введите название категории");
            return;
        }

        // Проверяем, существует ли уже такая категория
        for (const auto& category : categories) {
            if (category.first.toLower() == categoryName.toLower()) {
                QMessageBox::warning(&dialog, "Ошибка", "Категория с таким названием уже существует");
                return;
            }
        }

        // Добавляем категорию в базу данных
        if (database->addCategory(categoryName, currentColor.name())) {
            QMessageBox::information(&dialog, "Успех", "Категория добавлена");

            // Обновляем список
            QListWidgetItem *item = new QListWidgetItem(categoryName);
            item->setBackground(currentColor);
            item->setForeground(currentColor.lightness() > 128 ? QColor(Qt::black) : QColor(Qt::white));
            item->setToolTip(QString("Цвет: %1").arg(currentColor.name()));
            categoriesList->addItem(item);

            loadCategories(); // Перезагружаем категории
            categoryNameEdit->clear();
        } else {
            QMessageBox::warning(&dialog, "Ошибка", "Не удалось добавить категорию");
        }
    });

    connect(updateButton, &QPushButton::clicked, [&]() {
        QString categoryName = categoryNameEdit->text().trimmed();

        if (categoryName.isEmpty()) {
            QMessageBox::warning(&dialog, "Ошибка", "Введите название категории");
            return;
        }

        // Обновляем цвет категории в базе данных
        if (database->updateCategoryColor(categoryName, currentColor.name())) {
            QMessageBox::information(&dialog, "Успех", "Цвет категории обновлен");

            // Обновляем элемент в списке
            for (int i = 0; i < categoriesList->count(); ++i) {
                QListWidgetItem *item = categoriesList->item(i);
                if (item->text() == categoryName) {
                    item->setBackground(currentColor);
                    item->setForeground(currentColor.lightness() > 128 ? QColor(Qt::black) : QColor(Qt::white));
                    item->setToolTip(QString("Цвет: %1").arg(currentColor.name()));
                    break;
                }
            }

            loadCategories(); // Перезагружаем категории
            updateTableColors(); // Обновляем цвета в таблице
        } else {
            QMessageBox::warning(&dialog, "Ошибка", "Не удалось обновить категорию");
        }
    });

    connect(deleteButton, &QPushButton::clicked, [&]() {
        QString categoryName = categoryNameEdit->text().trimmed();

        if (categoryName.isEmpty()) {
            QMessageBox::warning(&dialog, "Ошибка", "Выберите категорию для удаления");
            return;
        }

        // Проверяем, используется ли категория
        QSqlQuery checkQuery(database->getDatabase());
        checkQuery.prepare("SELECT COUNT(*) FROM rooms WHERE category = ?");
        checkQuery.addBindValue(categoryName);

        if (checkQuery.exec() && checkQuery.next()) {
            int usageCount = checkQuery.value(0).toInt();
            if (usageCount > 0) {
                QMessageBox::warning(&dialog, "Ошибка",
                    QString("Категория используется в %1 комнатах.\nСначала измените категории этих комнат.")
                        .arg(usageCount));
                return;
            }
        }

        QMessageBox::StandardButton reply = QMessageBox::question(&dialog, "Подтверждение",
            QString("Вы уверены, что хотите удалить категорию \"%1\"?").arg(categoryName),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            if (database->removeCategory(categoryName)) {
                QMessageBox::information(&dialog, "Успех", "Категория удалена");

                // Удаляем элемент из списка
                for (int i = 0; i < categoriesList->count(); ++i) {
                    QListWidgetItem *item = categoriesList->item(i);
                    if (item->text() == categoryName) {
                        delete categoriesList->takeItem(i);
                        break;
                    }
                }

                loadCategories(); // Перезагружаем категории
                categoryNameEdit->clear();
                colorPreview->setStyleSheet("background-color: #FFFFFF;");
            } else {
                QMessageBox::warning(&dialog, "Ошибка", "Не удалось удалить категорию");
            }
        }
    });

    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

// Слот для просмотра клиентов
void HostelManager::onViewClients()
{
    if (!database->isDatabaseConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }

    QList<QVariantMap> clients = database->getAllClients();

    if (clients.isEmpty()) {
        QMessageBox::information(this, "Клиенты", "В базе данных нет клиентов");
        return;
    }

    // Создаем диалоговое окно с таблицей клиентов
    QDialog dialog(this);
    dialog.setWindowTitle("Список клиентов");
    dialog.setMinimumSize(800, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    // Таблица для отображения клиентов
    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(8);
    table->setHorizontalHeaderLabels({"ID", "Фамилия", "Имя", "Отчество",
                                      "Паспорт", "Телефон", "Дата рождения", "Страна"});
    table->setRowCount(clients.size());

    // Заполняем таблицу данными
    for (int i = 0; i < clients.size(); ++i) {
        const QVariantMap &client = clients[i];

        table->setItem(i, 0, new QTableWidgetItem(client["id"].toString()));
        table->setItem(i, 1, new QTableWidgetItem(client["last_name"].toString()));
        table->setItem(i, 2, new QTableWidgetItem(client["first_name"].toString()));
        table->setItem(i, 3, new QTableWidgetItem(client["middle_name"].toString()));
        table->setItem(i, 4, new QTableWidgetItem(client["passport_number"].toString()));
        table->setItem(i, 5, new QTableWidgetItem(client["phone_number"].toString()));

        QDate birthDate = QDate::fromString(client["birth_date"].toString(), "yyyy-MM-dd");
        table->setItem(i, 6, new QTableWidgetItem(birthDate.toString("dd.MM.yyyy")));
        table->setItem(i, 7, new QTableWidgetItem(client["country"].toString()));
    }

    // Настраиваем таблицу
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setStretchLastSection(true);
    table->resizeColumnsToContents();

    mainLayout->addWidget(table);

    // Кнопка закрытия
    QPushButton *closeButton = new QPushButton("Закрыть", &dialog);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    dialog.exec();
}

// Слот для добавления клиента
void HostelManager::onAddClient()
{
    if (!database || !database->isDatabaseConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }

    // Используем явное объявление переменной
    AddClientDialog *clientDialog = new AddClientDialog(this, AddClientDialog::Add);

    if (clientDialog->exec() == QDialog::Accepted) {
        // Проверяем, не существует ли уже клиент с таким паспортом
        if (database->clientExists(clientDialog->passport())) {
            QMessageBox::warning(this, "Ошибка",
                "Клиент с таким номером паспорта уже существует!");
            clientDialog->deleteLater();
            return;
        }

        // Добавляем клиента в базу данных
        if (database->addClient(clientDialog->firstName(),
                               clientDialog->lastName(),
                               clientDialog->middleName(),
                               clientDialog->passport(),
                               clientDialog->phone(),
                               clientDialog->birthDate(),
                               clientDialog->country())) {
            QMessageBox::information(this, "Успех", "Клиент успешно добавлен!");
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось добавить клиента");
        }
    }

    clientDialog->deleteLater();
}

// Слот для редактирования клиента (исправленная версия)
void HostelManager::onEditClient()
{
    if (!database || !database->isDatabaseConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }

    // Получаем список клиентов для выбора
    QList<QVariantMap> clients = database->getAllClients();

    if (clients.isEmpty()) {
        QMessageBox::information(this, "Редактирование", "В базе данных нет клиентов");
        return;
    }

    // Создаем список для QInputDialog
    QStringList clientList;
    QMap<QString, int> clientIdMap;

    for (const auto &client : clients) {
        QString displayText = QString("%1 %2 %3 (%4)")
            .arg(client["last_name"].toString())
            .arg(client["first_name"].toString())
            .arg(client["middle_name"].toString())
            .arg(client["passport_number"].toString());

        clientList << displayText;
        clientIdMap[displayText] = client["id"].toInt();
    }

    bool ok;
    QString selectedClient = QInputDialog::getItem(this, "Выбор клиента",
                                                  "Выберите клиента для редактирования:",
                                                  clientList, 0, false, &ok);

    if (!ok || selectedClient.isEmpty()) {
        return;
    }

    int clientId = clientIdMap.value(selectedClient);
    QVariantMap clientData = database->getClientById(clientId);

    if (clientData.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить данные клиента");
        return;
    }

    // Используем явное объявление переменной
    AddClientDialog *clientDialog = new AddClientDialog(this, AddClientDialog::Edit, clientId);

    // Заполняем поля данными клиента
    clientDialog->setClientData(
        clientData["first_name"].toString(),
        clientData["last_name"].toString(),
        clientData["middle_name"].toString(),
        clientData["passport_number"].toString(),
        clientData["phone_number"].toString(),
        QDate::fromString(clientData["birth_date"].toString(), "yyyy-MM-dd"),
        clientData["country"].toString()
    );

    if (clientDialog->exec() == QDialog::Accepted) {
        // Проверяем, не изменился ли паспорт на уже существующий
        QString newPassport = clientDialog->passport();
        QString oldPassport = clientData["passport_number"].toString();

        if (newPassport != oldPassport && database->clientExists(newPassport)) {
            QMessageBox::warning(this, "Ошибка",
                "Клиент с таким номером паспорта уже существует!");
            clientDialog->deleteLater();
            return;
        }

        // Обновляем данные клиента
        if (database->updateClient(clientId,
                                  clientDialog->firstName(),
                                  clientDialog->lastName(),
                                  clientDialog->middleName(),
                                  clientDialog->passport(),
                                  clientDialog->phone(),
                                  clientDialog->birthDate(),
                                  clientDialog->country())) {
            QMessageBox::information(this, "Успех", "Данные клиента обновлены!");
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось обновить данные клиента");
        }
    }

    clientDialog->deleteLater();
}


// Слот для удаления клиента
void HostelManager::onDeleteClient()
{
    if (!database->isDatabaseConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }

    // Получаем список клиентов для выбора
    QList<QVariantMap> clients = database->getAllClients();

    if (clients.isEmpty()) {
        QMessageBox::information(this, "Удаление", "В базе данных нет клиентов");
        return;
    }

    // Создаем список для QInputDialog
    QStringList clientList;
    QMap<QString, int> clientIdMap;

    for (const auto &client : clients) {
        QString displayText = QString("%1 %2 %3 (%4)")
            .arg(client["last_name"].toString())
            .arg(client["first_name"].toString())
            .arg(client["middle_name"].toString())
            .arg(client["passport_number"].toString());

        clientList << displayText;
        clientIdMap[displayText] = client["id"].toInt();
    }

    bool ok;
    QString selectedClient = QInputDialog::getItem(this, "Удаление клиента",
                                                  "Выберите клиента для удаления:",
                                                  clientList, 0, false, &ok);

    if (!ok || selectedClient.isEmpty()) {
        return;
    }

    int clientId = clientIdMap.value(selectedClient);

    // Запрашиваем подтверждение
    QMessageBox::StandardButton confirm = QMessageBox::question(this, "Подтверждение",
        "Вы уверены, что хотите удалить выбранного клиента?\n\n"
        "Это действие нельзя отменить.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (confirm == QMessageBox::Yes) {
        if (database->deleteClient(clientId)) {
            QMessageBox::information(this, "Успех", "Клиент успешно удален!");
        } else {
            QMessageBox::warning(this, "Ошибка",
                "Не удалось удалить клиента.\n"
                "Возможно, у клиента есть активные бронирования.");
        }
    }
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

        // Получаем данные из базы данных
        QStringList rooms;
        QMap<QString, QString> roomCategories;
        QMap<QString, int> roomBedsCount; // room_number -> количество коек

        if (database->isDatabaseConnected()) {
            QSqlQuery query(database->getDatabase());
            query.exec("SELECT r.room_number, r.category, r.beds_count FROM rooms r "
                       "WHERE EXISTS (SELECT 1 FROM beds b WHERE b.room_id = r.id AND b.is_active = 1) "
                       "ORDER BY r.room_number");

            while (query.next()) {
                QString roomNumber = query.value(0).toString();
                QString category = query.value(1).toString();
                int bedsCount = query.value(2).toInt(); // Берем из rooms.beds_count!

                rooms.append(roomNumber);
                roomCategories.insert(roomNumber, category);
                roomBedsCount.insert(roomNumber, bedsCount);

                qDebug() << "Комната:" << roomNumber << "Категория:" << category << "Койки:" << bedsCount;
            }
        } else {
            // Если база данных не подключена, используем тестовые данные
            rooms = {"101", "102", "103", "104", "105", "201", "202", "203", "204", "205"};
            roomCategories = {
                {"101", "Эконом"}, {"102", "Эконом"}, {"103", "Стандарт"},
                {"104", "Стандарт"}, {"105", "Комфорт"}, {"201", "Эконом"},
                {"202", "Эконом"}, {"203", "Стандарт"}, {"204", "Комфорт"}, {"205", "Люкс"}
            };

            // Для тестовых данных устанавливаем по 4 койки на комнату
            for (const QString& room : rooms) {
                roomBedsCount.insert(room, 4);
            }
        }

        // Рассчитываем общее количество строк: сумма beds_count для всех комнат
        int totalRows = 0;
        for (const QString& room : rooms) {
            totalRows += roomBedsCount.value(room, 4); // Берем из roomBedsCount
        }

        ui->tableWidget->setRowCount(totalRows);

        qDebug() << "Создана таблица:" << totalRows << "строк (койко-мест)," << totalColumns << "столбцов";

        // Заполняем таблицу данными
        int currentRow = 0;
        for (int roomIndex = 0; roomIndex < rooms.size(); ++roomIndex) {
            QString roomNumber = rooms[roomIndex];
            int bedsCount = roomBedsCount.value(roomNumber, 4);
            QString category = roomCategories.value(roomNumber, "Стандарт");

            // Создаем строки для каждой койки в комнате
            for (int bedIndex = 1; bedIndex <= bedsCount; ++bedIndex) {
                if (currentRow >= totalRows) {
                    qDebug() << "Выход за границы таблицы! currentRow:" << currentRow << "totalRows:" << totalRows;
                    break;
                }

                // Номер комнаты
                QTableWidgetItem *roomItem = new QTableWidgetItem(roomNumber);
                roomItem->setTextAlignment(Qt::AlignCenter);
                roomItem->setFlags(roomItem->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->setItem(currentRow, 0, roomItem);

                // Номер койки
                QTableWidgetItem *bedItem = new QTableWidgetItem(QString::number(bedIndex));
                bedItem->setTextAlignment(Qt::AlignCenter);
                bedItem->setFlags(bedItem->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->setItem(currentRow, 1, bedItem);

                // Категория
                QTableWidgetItem *categoryItem = new QTableWidgetItem(category);
                categoryItem->setTextAlignment(Qt::AlignCenter);
                categoryItem->setFlags(categoryItem->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->setItem(currentRow, 2, categoryItem);

                // Заполняем столбцы дней
                for (int col = 3; col < totalColumns; ++col) {
                    QTableWidgetItem *dayItem = new QTableWidgetItem("");
                    dayItem->setTextAlignment(Qt::AlignCenter);
                    dayItem->setFlags(dayItem->flags() & ~Qt::ItemIsEditable);
                    ui->tableWidget->setItem(currentRow, col, dayItem);
                }

                currentRow++;
            }
        }

        qDebug() << "Создано" << currentRow << "строк из" << totalRows << "запланированных";

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
        for (int row = 0; row < totalRows; ++row) {
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
            QString englishMonth = currentDate.toString("MMM");
            QString russianMonth = monthToRussian(englishMonth);

            QString headerText = QString("%1\n%2")
                .arg(currentDate.toString("dd"))
                .arg(russianMonth);
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

        // Цвета для статусов
        QColor fullyPaidColor(144, 238, 144);    // Светло-зеленый - полностью оплачено
        QColor partiallyPaidColor(200, 200, 0); // Светло-желтый - частично оплачено
        QColor notPaidColor(255, 200, 150);       // Светло-оранжевый - не оплачено
        QColor weekendColor(220, 220, 255);       // Светло-синий для выходных

        // Получаем данные о бронированиях из базы данных
        QMap<QString, QSet<QDate>> occupiedDates; // Ключ: "комната_койка", значение: набор занятых дат
        QMap<QString, QMap<QDate, QVariantMap>> paymentInfo; // Ключ: "комната_койка", значение: map<дата, информация_об_оплате>

        if (database->isDatabaseConnected()) {
            QSqlQuery query(database->getDatabase());
            query.prepare("SELECT r.room_number, b.bed_number, bk.check_in_date, bk.check_out_date, "
                         "bk.total_price, bk.paid_amount "
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
                    int bedNumber = query.value(1).toInt(); // Получаем int
                    QDate checkIn = QDate::fromString(query.value(2).toString(), "yyyy-MM-dd");
                    QDate checkOut = QDate::fromString(query.value(3).toString(), "yyyy-MM-dd");
                    double totalPrice = query.value(4).toDouble();
                    double paidAmount = query.value(5).toDouble();

                    QString key = roomNumber + "_" + QString::number(bedNumber);

                    // Добавляем все даты бронирования в набор
                    QDate date = checkIn;
                    while (date <= checkOut && date <= currentStartDate.addDays(DAYS_COUNT - 1)) {
                        if (date >= currentStartDate) {
                            occupiedDates[key].insert(date);

                            // Сохраняем информацию об оплате для каждой даты
                            QVariantMap payment;
                            payment["total_price"] = totalPrice;
                            payment["paid_amount"] = paidAmount;
                            if (totalPrice > 0) {
                                double percentage = (paidAmount / totalPrice) * 100;
                                payment["paid_percentage"] = qRound(percentage);
                            } else {
                                payment["paid_percentage"] = 0;
                            }
                            payment["balance"] = totalPrice - paidAmount;

                            paymentInfo[key][date] = payment;
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
            QString bedNumberStr = bedItem->text();
            int bedNumber = bedNumberStr.toInt(); // Конвертируем в int
            QString key = roomNumber + "_" + bedNumberStr;

            // Получаем категорию комнаты
            QTableWidgetItem *categoryItem = ui->tableWidget->item(row, 2);
            QString category = categoryItem ? categoryItem->text() : "Стандарт";

            // Получаем цвет категории
            QColor categoryColor = getCategoryColor(category);

            // Устанавливаем цвет фона для информационных колонок на основе категории
            for (int col = 0; col < 3; ++col) {
                QTableWidgetItem *item = ui->tableWidget->item(row, col);
                if (item) {
                    // Делаем цвет немного светлее для лучшей читаемости
                    QColor cellColor = categoryColor.lighter(110);
                    item->setBackground(QBrush(cellColor));

                    // Настраиваем цвет текста для контраста
                    item->setForeground(QColor(cellColor.lightness() > 150 ? Qt::black : Qt::white));
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
                        // Получаем информацию об оплате
                        QVariantMap payment;
                        if (paymentInfo.contains(key) && paymentInfo[key].contains(currentDate)) {
                            payment = paymentInfo[key][currentDate];
                        } else {
                            // Если нет информации в кэше, получаем из базы данных
                            payment = database->getBookingInfo(roomNumber, bedNumber, currentDate);
                        }

//                        double totalPrice = payment["total_price"].toDouble();
                        double paidAmount = payment["paid_amount"].toDouble();
                        double balance = payment["balance"].toDouble();
                        int paidPercentage = payment["paid_percentage"].toInt();

                        QColor cellColor;
                        QString statusText;

                        if (balance <= 0) {
                            // Полностью оплачено
                            cellColor = fullyPaidColor;
                            statusText = "✓"; // Галочка для полностью оплаченных
                            item->setForeground(QColor(Qt::darkGreen));
                        } else if (paidAmount > 0) {
                            // Частично оплачено
                            cellColor = partiallyPaidColor;
                            statusText = QString("%1%").arg(paidPercentage);
                            item->setForeground(QColor(Qt::darkYellow));
                        } else {
                            // Не оплачено
                            cellColor = notPaidColor;
                            statusText = "●"; // Точка для неоплаченных
                            item->setForeground(QColor(Qt::darkRed));
                        }

                        // Если выходной, смешиваем с цветом выходных
                        if (isWeekend) {
                            cellColor.setRed((cellColor.red() * 0.7 + weekendColor.red() * 0.3));
                            cellColor.setGreen((cellColor.green() * 0.7 + weekendColor.green() * 0.3));
                            cellColor.setBlue((cellColor.blue() * 0.7 + weekendColor.blue() * 0.3));
                        }

                        item->setBackground(QBrush(cellColor));
                        item->setText(statusText);

                    } else {
                        // Для свободных: используем цвет категории, для выходных - смешиваем с weekendColor
                        QColor baseColor = categoryColor.lighter(120);
                        if (isWeekend) {
                            // Смешиваем цвет категории с цветом выходных (30% weekendColor)
                            baseColor.setRed((baseColor.red() * 0.7 + weekendColor.red() * 0.3));
                            baseColor.setGreen((baseColor.green() * 0.7 + weekendColor.green() * 0.3));
                            baseColor.setBlue((baseColor.blue() * 0.7 + weekendColor.blue() * 0.3));
                        }
                        item->setBackground(QBrush(baseColor));
                        item->setText(""); // Очищаем текст
                        item->setForeground(QColor(baseColor.lightness() > 150 ? Qt::black : Qt::white));
                    }

                    item->setTextAlignment(Qt::AlignCenter);

                    // Устанавливаем подсказку для ячейки
                    QString tooltip = QString("Комната: %1, Койка: %2\nДата: %3\n")
                        .arg(roomNumber)
                        .arg(bedNumber)
                        .arg(currentDate.toString("dd.MM.yyyy"));

                    if (isOccupied) {
                        // Получаем информацию об оплате для подсказки
                        QVariantMap payment;
                        if (paymentInfo.contains(key) && paymentInfo[key].contains(currentDate)) {
                            payment = paymentInfo[key][currentDate];
                        } else {
                            payment = database->getBookingInfo(roomNumber, bedNumber, currentDate);
                        }

                        if (!payment.isEmpty()) {
                            double totalPrice = payment["total_price"].toDouble();
                            double paidAmount = payment["paid_amount"].toDouble();
                            double balance = payment["balance"].toDouble();
                            int paidPercentage = payment["paid_percentage"].toInt();
                            QString clientName = payment["client_name"].toString();

                            if (!clientName.isEmpty()) {
                                tooltip += QString("Клиент: %1\n").arg(clientName);
                            }

                            tooltip += QString("Общая стоимость: %1 руб.\n"
                                             "Оплачено: %2 руб.\n"
                                             "Остаток: %3 руб.\n"
                                             "Оплачено: %4%")
                                .arg(totalPrice, 0, 'f', 2)
                                .arg(paidAmount, 0, 'f', 2)
                                .arg(balance, 0, 'f', 2)
                                .arg(paidPercentage);

                            if (balance <= 0) {
                                tooltip += "\nСтатус: Полностью оплачено";
                            } else if (paidAmount > 0) {
                                tooltip += "\nСтатус: Частично оплачено";
                            } else {
                                tooltip += "\nСтатус: Не оплачено";
                            }
                        } else {
                            tooltip += "Статус: Занято (информация об оплате недоступна)";
                        }
                    } else {
                        tooltip += "Статус: Свободно";
                    }

                    if (isWeekend) {
                        tooltip += "\nВыходной день";
                    }

                    // Получаем информацию о бронировании для занятых ячеек
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

                            if (!clientName.isEmpty() && !tooltip.contains("Клиент:")) {
                                tooltip += QString("\nКлиент: %1").arg(clientName);
                            }

                            if (!tooltip.contains("Период:")) {
                                tooltip += QString("\nПериод: %1 - %2")
                                    .arg(checkIn.toString("dd.MM.yyyy"))
                                    .arg(checkOut.toString("dd.MM.yyyy"));
                            }
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

// Слот для добавления нового бронирования
void HostelManager::onAddBooking()
{
    if (!database || !database->isDatabaseConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }

    // Используем явное объявление переменной
    AddBookingDialog *bookingDialog = new AddBookingDialog(database, this);

    if (bookingDialog->exec() == QDialog::Accepted) {
        // Создаем бронирование
        if (database->addBooking(bookingDialog->bedId(),
                                bookingDialog->clientId(),
                                bookingDialog->checkInDate(),
                                bookingDialog->checkOutDate(),
                                bookingDialog->totalPrice(),
                                bookingDialog->paidAmount(),
                                bookingDialog->paymentMethod())) {
            QMessageBox::information(this, "Успех",
                QString("Бронирование успешно создано!\n"
                       "Общая стоимость: %1 руб.")
                    .arg(bookingDialog->totalPrice(), 0, 'f', 2));

            // Обновляем таблицу
            updateTableColors();
        } else {
            QMessageBox::warning(this, "Ошибка",
                "Не удалось создать бронирование.\n"
                "Возможно, койка уже забронирована на эти даты.");
        }
    }

    bookingDialog->deleteLater();
}

// Слот для двойного клика
void HostelManager::onTableDoubleClicked(const QModelIndex &index)
{
    // Проверяем, что клик был по столбцу с датами (столбцы с 3 по DAYS_COUNT+2)
    if (index.column() < 3) {
        return; // Клик по информационным столбцам (комната, койка, категория)
    }

    int row = index.row();
    int col = index.column();

    // Получаем номер комнаты и койки
    QTableWidgetItem *roomItem = ui->tableWidget->item(row, 0);
    QTableWidgetItem *bedItem = ui->tableWidget->item(row, 1);

    if (!roomItem || !bedItem) {
        return;
    }

    QString roomNumber = roomItem->text();
    QString bedNumberStr = bedItem->text();
    int bedNumber = bedNumberStr.toInt();

    // Вычисляем дату для выбранного столбца
    int dayIndex = col - 3;
    QDate selectedDate = currentStartDate.addDays(dayIndex);

    // Проверяем занятость через базу данных (самый надежный способ)
    if (!database->isDatabaseConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }

    QVariantMap bookingInfo = database->getBookingInfoByDate(roomNumber, bedNumber, selectedDate);

    if (bookingInfo.isEmpty()) {
        QMessageBox::information(this, "Информация",
            "Эта ячейка свободна. Для бронирования используйте меню 'Бронирование'.");
        return;
    }

    // Если мы здесь, значит ячейка занята
    int bookingId = bookingInfo["booking_id"].toInt();
    double totalPrice = bookingInfo["total_price"].toDouble();
    double paidAmount = bookingInfo["paid_amount"].toDouble();
    QString paymentMethod = bookingInfo["payment_method"].toString();
    QString clientName = bookingInfo["client_name"].toString();
//    QDate checkInDate = QDate::fromString(bookingInfo["check_in_date"].toString(), "yyyy-MM-dd");
//    QDate checkOutDate = QDate::fromString(bookingInfo["check_out_date"].toString(), "yyyy-MM-dd");

    // Показываем диалог редактирования оплаты
    EditPaymentDialog dialog(this);
    dialog.setBookingInfo(roomNumber, bedNumber, selectedDate, clientName,
                         totalPrice, paidAmount, paymentMethod);

    if (dialog.exec() == QDialog::Accepted) {
        double newPaidAmount = dialog.paidAmount();
        QString newPaymentMethod = dialog.paymentMethod();
        QString notes = dialog.notes();

        // Обновляем оплату в базе данных
        if (database->updatePayment(bookingId, newPaidAmount, newPaymentMethod)) {
            QMessageBox::information(this, "Успех",
                QString("Оплата успешно обновлена!\n"
                       "Новая сумма: %1 руб.\n"
                       "Способ оплаты: %2")
                    .arg(newPaidAmount, 0, 'f', 2)
                    .arg(newPaymentMethod));

            // Обновляем таблицу
            updateTableColors();

            // Логируем изменение
            if (!notes.isEmpty()) {
                qDebug() << "Примечание к оплате:" << notes;
            }
        } else {
            QMessageBox::warning(this, "Ошибка",
                "Не удалось обновить оплату. Проверьте подключение к базе данных.");
        }
    }
}

// Слот для отображения контекстного меню
void HostelManager::onCustomContextMenuRequested(const QPoint &pos)
{
    // Сохраняем позицию для использования в других методах
    contextMenuPos = pos;

    // Получаем индекс ячейки под курсором
    QModelIndex index = ui->tableWidget->indexAt(pos);

    if (!index.isValid() || index.column() < 3) {
        return; // Клик не по ячейке с датой
    }

    int row = index.row();
    int col = index.column();

    // Получаем номер комнаты и койки
    QTableWidgetItem *roomItem = ui->tableWidget->item(row, 0);
    QTableWidgetItem *bedItem = ui->tableWidget->item(row, 1);

    if (!roomItem || !bedItem) {
        return;
    }

    QString roomNumber = roomItem->text();
    QString bedNumberStr = bedItem->text();
    int bedNumber = bedNumberStr.toInt();

    // Вычисляем дату для выбранного столбца
    int dayIndex = col - 3;
    QDate selectedDate = currentStartDate.addDays(dayIndex);

    // Проверяем, занята ли ячейка
    QVariantMap bookingInfo = database->getBookingInfoByDate(roomNumber, bedNumber, selectedDate);

    if (bookingInfo.isEmpty()) {
        // Ячейка свободна - не показываем меню удаления
        return;
    }

    // Сохраняем ID бронирования для удаления
    selectedBookingIdForDeletion = bookingInfo["booking_id"].toInt();

    // Получаем информацию о бронировании для отображения в меню
    QString clientName = bookingInfo["client_name"].toString();
    double totalPrice = bookingInfo["total_price"].toDouble();
    double paidAmount = bookingInfo["paid_amount"].toDouble();
    QDate checkInDate = QDate::fromString(bookingInfo["check_in_date"].toString(), "yyyy-MM-dd");
    QDate checkOutDate = QDate::fromString(bookingInfo["check_out_date"].toString(), "yyyy-MM-dd");

    // Создаем контекстное меню
    QMenu contextMenu(this);

    // Добавляем действие с информацией
    QAction *infoAction = contextMenu.addAction(
        QString("Бронирование №%1\nКлиент: %2\nПериод: %3 - %4\nСтоимость: %5 руб.\nОплачено: %6 руб.")
            .arg(selectedBookingIdForDeletion)
            .arg(clientName)
            .arg(checkInDate.toString("dd.MM.yyyy"))
            .arg(checkOutDate.toString("dd.MM.yyyy"))
            .arg(totalPrice, 0, 'f', 2)
            .arg(paidAmount, 0, 'f', 2)
    );
    infoAction->setEnabled(false); // Только информация, не кликабельно

    contextMenu.addSeparator();

    // Настраиваем действие удаления
    QString deleteText;
    if (paidAmount > 0) {
        deleteText = QString("Удалить бронирование (возврат %1 руб. потребуется вручную)")
                        .arg(paidAmount, 0, 'f', 2);
    } else {
        deleteText = "Удалить бронирование";
    }
    deleteBookingAction->setText(deleteText);

    // Добавляем действие в меню
    contextMenu.addAction(deleteBookingAction);

    contextMenu.addSeparator();

    // Добавляем действие отмены
    QAction *cancelAction = contextMenu.addAction("Отмена");
    connect(&contextMenu, &QMenu::triggered, cancelAction, [&](){
        // Просто закрываем меню
    });

    // Показываем контекстное меню
    contextMenu.exec(ui->tableWidget->viewport()->mapToGlobal(pos));
}

// Слот для удаления бронирования
void HostelManager::deleteBooking()
{
    if (selectedBookingIdForDeletion <= 0) {
        return;
    }

    // Получаем информацию о бронировании для подтверждения
    QSqlQuery query(database->getDatabase());
    query.prepare("SELECT "
                  "r.room_number, "
                  "b.bed_number, "
                  "c.last_name || ' ' || c.first_name as client_name, "
                  "bk.check_in_date, "
                  "bk.check_out_date, "
                  "bk.total_price, "
                  "bk.paid_amount "
                  "FROM bookings bk "
                  "JOIN beds b ON bk.bed_id = b.id "
                  "JOIN rooms r ON b.room_id = r.id "
                  "JOIN clients c ON bk.client_id = c.id "
                  "WHERE bk.id = ?");
    query.addBindValue(selectedBookingIdForDeletion);

    QString roomNumber, bedNumberStr, clientName;
    QDate checkInDate, checkOutDate;
    double totalPrice = 0, paidAmount = 0;

    if (query.exec() && query.next()) {
        roomNumber = query.value(0).toString();
        bedNumberStr = query.value(1).toString();
        clientName = query.value(2).toString();
        checkInDate = QDate::fromString(query.value(3).toString(), "yyyy-MM-dd");
        checkOutDate = QDate::fromString(query.value(4).toString(), "yyyy-MM-dd");
        totalPrice = query.value(5).toDouble();
        paidAmount = query.value(6).toDouble();
    }

    // Запрашиваем подтверждение
    QString message;
    if (paidAmount > 0) {
        message = QString("Вы уверены, что хотите удалить бронирование?\n\n"
                         "Детали бронирования:\n"
                         "ID: %1\n"
                         "Комната: %2, Койка: %3\n"
                         "Клиент: %4\n"
                         "Период: %5 - %6\n"
                         "Общая стоимость: %7 руб.\n"
                         "Оплачено: %8 руб.\n\n"
                         "ВНИМАНИЕ: Оплаченные средства (%8 руб.) будут удалены из системы!\n"
                         "Для возврата денег клиенту необходимо обратиться в бухгалтерию.\n\n"
                         "Это действие необратимо!")
                    .arg(selectedBookingIdForDeletion)
                    .arg(roomNumber)
                    .arg(bedNumberStr)
                    .arg(clientName)
                    .arg(checkInDate.toString("dd.MM.yyyy"))
                    .arg(checkOutDate.toString("dd.MM.yyyy"))
                    .arg(totalPrice, 0, 'f', 2)
                    .arg(paidAmount, 0, 'f', 2);
    } else {
        message = QString("Вы уверены, что хотите удалить бронирование?\n\n"
                         "Детали бронирования:\n"
                         "ID: %1\n"
                         "Комната: %2, Койка: %3\n"
                         "Клиент: %4\n"
                         "Период: %5 - %6\n"
                         "Общая стоимость: %7 руб.\n\n"
                         "Это действие необратимо!")
                    .arg(selectedBookingIdForDeletion)
                    .arg(roomNumber)
                    .arg(bedNumberStr)
                    .arg(clientName)
                    .arg(checkInDate.toString("dd.MM.yyyy"))
                    .arg(checkOutDate.toString("dd.MM.yyyy"))
                    .arg(totalPrice, 0, 'f', 2);
    }

    QMessageBox::StandardButton reply = QMessageBox::critical(this,
        "Подтверждение удаления",
        message,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Удаляем бронирование
        if (database->removeBooking(selectedBookingIdForDeletion)) {
            QMessageBox::information(this, "Успех",
                QString("Бронирование №%1 успешно удалено!")
                    .arg(selectedBookingIdForDeletion));

            // Обновляем таблицу
            updateTableColors();

            // Сбрасываем ID
            selectedBookingIdForDeletion = -1;
        } else {
            QMessageBox::warning(this, "Ошибка",
                "Не удалось удалить бронирование. Проверьте подключение к базе данных.");
        }
    }
}
