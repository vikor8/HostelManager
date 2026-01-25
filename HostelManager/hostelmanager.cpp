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

    // Создаем меню "Клиенты" (ОБНОВЛЕНО)
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
    connect(roomNumberEdit, &QLineEdit::textChanged, [&dialog, roomNumberEdit, buttonBox]() {
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
    query.exec("SELECT id, room_number, category, beds_count FROM rooms ORDER BY room_number");

    while (query.next()) {
        int id = query.value(0).toInt();
        QString roomNumber = query.value(1).toString();
        QString category = query.value(2).toString();
        int bedsCount = query.value(3).toInt();

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

    // Получаем данные о выбранной комнате
    QVariantList data = roomData[roomNumber].toList();
    int roomId = data[0].toInt();
    QString currentCategory = data[1].toString();
    int currentBedsCount = data[2].toInt();

    // Получаем текущую цену (берем цену первой койки)
    double currentPrice = 500.0;
    QSqlQuery priceQuery(database->getDatabase());
    priceQuery.prepare("SELECT price_per_day FROM beds WHERE room_id = ? LIMIT 1");
    priceQuery.addBindValue(roomId);
    if (priceQuery.exec() && priceQuery.next()) {
        currentPrice = priceQuery.value(0).toDouble();
    }

    // Получаем список категорий из базы данных
    QList<QPair<QString, QString>> categories = database->getAllCategories();

    // Создаем диалоговое окно
    QDialog dialog(this);
    dialog.setWindowTitle("Редактировать комнату: " + roomNumber);
    dialog.setFixedSize(400, 350);

    QFormLayout *layout = new QFormLayout(&dialog);

    QLabel *roomLabel = new QLabel(roomNumber, &dialog);
    roomLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addRow("Номер комнаты:", roomLabel);

    QSpinBox *bedsCountSpin = new QSpinBox(&dialog);
    bedsCountSpin->setRange(1, 10);
    bedsCountSpin->setValue(currentBedsCount);
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

    // Устанавливаем текущую категорию
    int index = categoryCombo->findText(currentCategory);
    if (index >= 0) {
        categoryCombo->setCurrentIndex(index);
    }

    layout->addRow("Категория:", categoryCombo);

    QDoubleSpinBox *priceSpin = new QDoubleSpinBox(&dialog);
    priceSpin->setRange(100, 10000);
    priceSpin->setValue(currentPrice);
    priceSpin->setSuffix(" руб./день");
    priceSpin->setDecimals(2);
    layout->addRow("Стоимость за день:", priceSpin);

    // Чекбокс для обновления цены всех коек
    QCheckBox *updateAllBedsCheck = new QCheckBox("Обновить цену для всех коек в комнате", &dialog);
    updateAllBedsCheck->setChecked(true);
    layout->addRow("", updateAllBedsCheck);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        int newBedsCount = bedsCountSpin->value();
        QString newCategory = categoryCombo->currentText();
        double newPrice = priceSpin->value();
        bool updateAllBeds = updateAllBedsCheck->isChecked();

        // Обновляем данные комнаты
        QSqlQuery updateQuery(database->getDatabase());
        updateQuery.prepare("UPDATE rooms SET category = ?, beds_count = ? WHERE id = ?");
        updateQuery.addBindValue(newCategory);
        updateQuery.addBindValue(newBedsCount);
        updateQuery.addBindValue(roomId);

        if (!updateQuery.exec()) {
            QMessageBox::warning(this, "Ошибка", "Не удалось обновить данные комнаты: " + updateQuery.lastError().text());
            return;
        }

        // Обновляем цену коек
        if (updateAllBeds) {
            QSqlQuery priceUpdateQuery(database->getDatabase());
            priceUpdateQuery.prepare("UPDATE beds SET price_per_day = ? WHERE room_id = ?");
            priceUpdateQuery.addBindValue(newPrice);
            priceUpdateQuery.addBindValue(roomId);

            if (!priceUpdateQuery.exec()) {
                QMessageBox::warning(this, "Ошибка", "Не удалось обновить цены коек: " + priceUpdateQuery.lastError().text());
            }
        }

        // Если изменилось количество коек
        if (newBedsCount != currentBedsCount) {
            // Получаем текущее количество коек
            QSqlQuery countQuery(database->getDatabase());
            countQuery.prepare("SELECT COUNT(*) FROM beds WHERE room_id = ?");
            countQuery.addBindValue(roomId);

            if (countQuery.exec() && countQuery.next()) {
                int currentBedsInDb = countQuery.value(0).toInt();

                if (newBedsCount > currentBedsInDb) {
                    // Добавляем недостающие койки
                    for (int i = currentBedsInDb + 1; i <= newBedsCount; ++i) {
                        QSqlQuery addBedQuery(database->getDatabase());
                        addBedQuery.prepare("INSERT INTO beds (room_id, bed_number, price_per_day) VALUES (?, ?, ?)");
                        addBedQuery.addBindValue(roomId);
                        addBedQuery.addBindValue(i);
                        addBedQuery.addBindValue(newPrice);
                        addBedQuery.exec();
                    }
                } else if (newBedsCount < currentBedsInDb) {
                    // Удаляем лишние койки (только если они не заняты)
                    QMessageBox::StandardButton reply = QMessageBox::question(this, "Удаление коек",
                        QString("Вы хотите уменьшить количество коек с %1 до %2.\n"
                               "Койки с номерами больше %2 будут удалены.\n\n"
                               "Продолжить?")
                            .arg(currentBedsInDb).arg(newBedsCount),
                        QMessageBox::Yes | QMessageBox::No);

                    if (reply == QMessageBox::Yes) {
                        QSqlQuery deleteQuery(database->getDatabase());
                        deleteQuery.prepare("DELETE FROM beds WHERE room_id = ? AND bed_number > ?");
                        deleteQuery.addBindValue(roomId);
                        deleteQuery.addBindValue(newBedsCount);
                        deleteQuery.exec();
                    }
                }
            }
        }

        QMessageBox::information(this, "Успех", "Данные комнаты обновлены!");
        updateRoomIdMap();
        loadCategories();
        initializeTable();
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
    if (!database->isDatabaseConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }

    AddClientDialog dialog(this, AddClientDialog::Add);

    if (dialog.exec() == QDialog::Accepted) {
        // Проверяем, не существует ли уже клиент с таким паспортом
        if (database->clientExists(dialog.passport())) {
            QMessageBox::warning(this, "Ошибка",
                "Клиент с таким номером паспорта уже существует!");
            return;
        }

        // Добавляем клиента в базу данных
        if (database->addClient(dialog.firstName(), dialog.lastName(),
                               dialog.middleName(), dialog.passport(),
                               dialog.phone(), dialog.birthDate(),
                               dialog.country())) {
            QMessageBox::information(this, "Успех", "Клиент успешно добавлен!");
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось добавить клиента");
        }
    }
}

// Слот для редактирования клиента
void HostelManager::onEditClient()
{
    if (!database->isDatabaseConnected()) {
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

    // Создаем диалог редактирования
    AddClientDialog dialog(this, AddClientDialog::Edit, clientId);

    // Заполняем поля данными клиента
    dialog.setClientData(
        clientData["first_name"].toString(),
        clientData["last_name"].toString(),
        clientData["middle_name"].toString(),
        clientData["passport_number"].toString(),
        clientData["phone_number"].toString(),
        QDate::fromString(clientData["birth_date"].toString(), "yyyy-MM-dd"),
        clientData["country"].toString()
    );

    if (dialog.exec() == QDialog::Accepted) {
        // Проверяем, не изменился ли паспорт на уже существующий
        QString newPassport = dialog.passport();
        QString oldPassport = clientData["passport_number"].toString();

        if (newPassport != oldPassport && database->clientExists(newPassport)) {
            QMessageBox::warning(this, "Ошибка",
                "Клиент с таким номером паспорта уже существует!");
            return;
        }

        // Обновляем данные клиента
        if (database->updateClient(clientId,
                                  dialog.firstName(), dialog.lastName(),
                                  dialog.middleName(), dialog.passport(),
                                  dialog.phone(), dialog.birthDate(),
                                  dialog.country())) {
            QMessageBox::information(this, "Успех", "Данные клиента обновлены!");
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось обновить данные клиента");
        }
    }
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
                        item->setBackground(QBrush(occupiedColor));
                        item->setText("●"); // Маркер занятости
                        item->setForeground(QColor(Qt::black));
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
                    QString status = isOccupied ? "Занято" : "Свободно";
                    QString tooltip = QString("Комната: %1, Койка: %2\nДата: %3\nСтатус: %4\nКатегория: %5")
                        .arg(roomNumber)
                        .arg(bedNumber)
                        .arg(currentDate.toString("dd.MM.yyyy"))
                        .arg(status)
                        .arg(category);

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
