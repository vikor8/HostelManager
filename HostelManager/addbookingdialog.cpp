#include "addbookingdialog.h"
#include "database.h"
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QCompleter>
#include <QTimer>

AddBookingDialog::AddBookingDialog(Database *db, QWidget *parent)
    : QDialog(parent)
    , database(db)
{
    setupUi();
    loadClients();
    loadRooms();

    if (roomCombo->count() > 0) {
        loadBeds();
    }

    // Устанавливаем даты по умолчанию
    checkInEdit->setDate(QDate::currentDate());
    checkOutEdit->setDate(QDate::currentDate().addDays(1));

    calculateTotalPrice();
    // Дополнительная проверка формы
       validateForm();
}

AddBookingDialog::~AddBookingDialog()
{
}

void AddBookingDialog::setupUi()
{
    setWindowTitle("Добавить бронирование");
    setMinimumWidth(500);

    QFormLayout *formLayout = new QFormLayout(this);

    // Комбобокс для выбора клиента с возможностью поиска
    clientCombo = new QComboBox(this);
    clientCombo->setEditable(true);
    clientCombo->setInsertPolicy(QComboBox::NoInsert);
    clientCombo->setPlaceholderText("Выберите или введите фамилию клиента");

    // Настраиваем автодополнение для клиентов
    QCompleter *completer = new QCompleter(clientCombo);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    clientCombo->setCompleter(completer);

    // Поля для дат
    checkInEdit = new QDateEdit(this);
    checkInEdit->setDate(QDate::currentDate());
    checkInEdit->setCalendarPopup(true);
    checkInEdit->setDisplayFormat("dd.MM.yyyy");
    checkInEdit->setMinimumDate(QDate::currentDate());

    checkOutEdit = new QDateEdit(this);
    checkOutEdit->setDate(QDate::currentDate().addDays(1));
    checkOutEdit->setCalendarPopup(true);
    checkOutEdit->setDisplayFormat("dd.MM.yyyy");
    checkOutEdit->setMinimumDate(QDate::currentDate().addDays(1));

    // Комбобокс для выбора комнаты
    roomCombo = new QComboBox(this);
    roomCombo->setPlaceholderText("Выберите номер комнаты");

    // Комбобокс для выбора койки
    bedCombo = new QComboBox(this);
    bedCombo->setPlaceholderText("Выберите койко-место");

    // Поле для стоимости в сутки
    priceSpin = new QDoubleSpinBox(this);
    priceSpin->setRange(100, 10000);
    priceSpin->setSuffix(" руб./сутки");
    priceSpin->setDecimals(2);
    priceSpin->setSingleStep(100);

    // Метка для отображения общей стоимости
    totalPriceLabel = new QLabel(this);
    totalPriceLabel->setStyleSheet("font-weight: bold; color: #2E8B57;");

    // Добавляем элементы в форму
    formLayout->addRow("*Клиент:", clientCombo);
    formLayout->addRow("*Дата заезда:", checkInEdit);
    formLayout->addRow("*Дата выезда:", checkOutEdit);
    formLayout->addRow("*Номер комнаты:", roomCombo);
    formLayout->addRow("*Койко-место:", bedCombo);
    formLayout->addRow("*Стоимость в сутки:", priceSpin);
    formLayout->addRow("Общая стоимость:", totalPriceLabel);

    // Добавляем подсказку об обязательных полях
    QLabel *requiredLabel = new QLabel("* - обязательные поля", this);
    requiredLabel->setStyleSheet("color: gray; font-style: italic;");
    formLayout->addRow("", requiredLabel);

    // Кнопки
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, this);

    formLayout->addRow(buttonBox);

    // Соединяем сигналы и слоты
    connect(clientCombo->lineEdit(), &QLineEdit::textEdited, this, &AddBookingDialog::loadClients);
    connect(checkInEdit, &QDateEdit::dateChanged, this, &AddBookingDialog::validateDates);
    connect(checkOutEdit, &QDateEdit::dateChanged, this, &AddBookingDialog::validateDates);
    connect(roomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AddBookingDialog::loadBeds);
    connect(bedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AddBookingDialog::updatePriceFromDatabase);
    connect(priceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AddBookingDialog::calculateTotalPrice);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddBookingDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Добавляем соединения для проверки формы
    connect(clientCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddBookingDialog::validateForm);
    connect(roomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddBookingDialog::validateForm);
    connect(bedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddBookingDialog::validateForm);
    connect(checkInEdit, &QDateEdit::dateChanged,
            this, &AddBookingDialog::validateForm);
    connect(checkOutEdit, &QDateEdit::dateChanged,
            this, &AddBookingDialog::validateForm);
    connect(priceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &AddBookingDialog::validateForm);

    // Изначально кнопка OK неактивна
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void AddBookingDialog::loadClients()
{
    if (!database || !database->isDatabaseConnected()) {
        return;
    }

    QString filter = clientCombo->currentText().trimmed();
    populateClientCombo(filter);
}

void AddBookingDialog::populateClientCombo(const QString& filter)
{
    clientCombo->clear();
    clientMap.clear();

    QSqlQuery query(database->getDatabase());
    QString sql = "SELECT id, last_name, first_name, middle_name FROM clients ";

    if (!filter.isEmpty()) {
        sql += "WHERE last_name LIKE :filter || '%' ";
        sql += "OR first_name LIKE :filter || '%' ";
    }

    sql += "ORDER BY last_name, first_name";

    query.prepare(sql);

    if (!filter.isEmpty()) {
        query.bindValue(":filter", filter);
    }

    if (query.exec()) {
        while (query.next()) {
            int clientId = query.value(0).toInt();
            QString lastName = query.value(1).toString();
            QString firstName = query.value(2).toString();
            QString middleName = query.value(3).toString();

            QString displayName;
            if (!middleName.isEmpty()) {
                displayName = QString("%1 %2 %3").arg(lastName).arg(firstName).arg(middleName);
            } else {
                displayName = QString("%1 %2").arg(lastName).arg(firstName);
            }

            clientMap.insert(clientId, displayName);
            clientCombo->addItem(displayName, clientId);
        }
    } else {
        qDebug() << "Ошибка загрузки клиентов:" << query.lastError().text();
    }
}

void AddBookingDialog::loadRooms()
{
    populateRoomCombo();
}

void AddBookingDialog::populateRoomCombo()
{
    roomCombo->clear();
    roomMap.clear();

    QSqlQuery query(database->getDatabase());
    query.prepare("SELECT id, room_number FROM rooms WHERE id IN "
                  "(SELECT DISTINCT room_id FROM beds) "
                  "ORDER BY room_number");

    if (query.exec()) {
        while (query.next()) {
            int roomId = query.value(0).toInt();
            QString roomNumber = query.value(1).toString();

            roomMap.insert(roomId, roomNumber);
            roomCombo->addItem(roomNumber, roomId);
        }
    } else {
        qDebug() << "Ошибка загрузки комнат:" << query.lastError().text();
    }
}

void AddBookingDialog::loadBeds()
{
    int roomId = roomCombo->currentData().toInt();
    if (roomId > 0) {
        populateBedCombo(roomId);
    }
}

void AddBookingDialog::populateBedCombo(int roomId)
{
    bedCombo->clear();
    bedMap.clear();

    if (roomId <= 0) {
        return;
    }

    QSqlQuery query(database->getDatabase());
    query.prepare("SELECT b.id, b.bed_number, b.price_per_day FROM beds b "
                  "WHERE b.room_id = ? AND b.is_active = 1 "
                  "ORDER BY b.bed_number");
    query.addBindValue(roomId);

    if (query.exec()) {
        while (query.next()) {
            int bedId = query.value(0).toInt();
            int bedNumber = query.value(1).toInt();
            double price = query.value(2).toDouble();

            QString displayName = QString("Койка №%1 (%2 руб./сутки)").arg(bedNumber).arg(price, 0, 'f', 2);

            bedMap.insert(bedId, displayName);
            bedCombo->addItem(displayName, bedId);
        }
    } else {
        qDebug() << "Ошибка загрузки коек:" << query.lastError().text();
    }

    // Если есть койки, выбираем первую и обновляем цену
    if (bedCombo->count() > 0) {
        bedCombo->setCurrentIndex(0);
        updatePriceFromDatabase();
    }
}

void AddBookingDialog::updatePriceFromDatabase()
{
    int bedId = bedCombo->currentData().toInt();
    if (bedId <= 0) {
        return;
    }

    QSqlQuery query(database->getDatabase());
    query.prepare("SELECT price_per_day FROM beds WHERE id = ?");
    query.addBindValue(bedId);

    if (query.exec() && query.next()) {
        double price = query.value(0).toDouble();
        priceSpin->setValue(price);
    }
}

void AddBookingDialog::calculateTotalPrice()
{
    QDate checkIn = checkInEdit->date();
    QDate checkOut = checkOutEdit->date();
    double pricePerDay = priceSpin->value();

    if (checkIn.isValid() && checkOut.isValid() && checkOut > checkIn) {
        int days = checkIn.daysTo(checkOut);
        double total = days * pricePerDay;
        totalPriceLabel->setText(QString("%1 руб. за %2 суток").arg(total, 0, 'f', 2).arg(days));
    } else {
        totalPriceLabel->setText("0 руб.");
    }
}

void AddBookingDialog::validateDates()
{
    QDate checkIn = checkInEdit->date();
    QDate checkOut = checkOutEdit->date();

    // Проверяем, что дата выезда позже даты заезда
    if (checkOut <= checkIn) {
        checkOutEdit->setDate(checkIn.addDays(1));
    }

    // Пересчитываем стоимость
    calculateTotalPrice();

    // Проверяем доступность койки
    isBedAvailable(); // Для информационного сообщения
}

int AddBookingDialog::clientId() const
{
    return clientCombo->currentData().toInt();
}

int AddBookingDialog::roomId() const
{
    return roomCombo->currentData().toInt();
}

int AddBookingDialog::bedId() const
{
    return bedCombo->currentData().toInt();
}

double AddBookingDialog::totalPrice() const
{
    QDate checkIn = checkInEdit->date();
    QDate checkOut = checkOutEdit->date();
    double pricePerDay = priceSpin->value();

    if (checkIn.isValid() && checkOut.isValid() && checkOut > checkIn) {
        int days = checkIn.daysTo(checkOut);
        return days * pricePerDay;
    }

    return 0.0;
}

bool AddBookingDialog::isBedAvailable() const
{
    int bedId = this->bedId();
    QDate checkIn = checkInEdit->date();
    QDate checkOut = checkOutEdit->date();

    if (bedId <= 0 || !checkIn.isValid() || !checkOut.isValid()) {
        return false;
    }

    // Проверяем через метод базы данных
    if (database && database->isDatabaseConnected()) {
        return database->isBedAvailable(bedId, checkIn, checkOut);
    }

    return true;
}

void AddBookingDialog::onAccept()
{
    // Проверяем обязательные поля
    if (clientId() <= 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите клиента");
        return;
    }

    if (roomId() <= 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите номер комнаты");
        return;
    }

    if (bedId() <= 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите койко-место");
        return;
    }

    if (checkInEdit->date() >= checkOutEdit->date()) {
        QMessageBox::warning(this, "Ошибка", "Дата выезда должна быть позже даты заезда");
        return;
    }

    // Проверяем доступность койки
    if (!isBedAvailable()) {
        QMessageBox::warning(this, "Ошибка",
            "Выбранное койко-место уже забронировано на указанные даты.\n"
            "Пожалуйста, выберите другие даты или другую койку.");
        return;
    }

    // Проверяем стоимость
    if (priceSpin->value() <= 0) {
        QMessageBox::warning(this, "Ошибка", "Стоимость должна быть больше 0");
        return;
    }

    accept();
}

void AddBookingDialog::validateForm()
{
    // Получаем кнопку OK
    QDialogButtonBox *buttonBox = findChild<QDialogButtonBox*>();
    if (!buttonBox) return;

    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    if (!okButton) return;

    // Проверяем все обязательные поля
    bool allFieldsValid = true;

    // Проверка клиента
    if (clientId() <= 0) {
        allFieldsValid = false;
    }

    // Проверка комнаты
    if (roomId() <= 0) {
        allFieldsValid = false;
    }

    // Проверка койки
    if (bedId() <= 0) {
        allFieldsValid = false;
    }

    // Проверка дат
    if (!checkInEdit->date().isValid() ||
        !checkOutEdit->date().isValid() ||
        checkInEdit->date() >= checkOutEdit->date()) {
        allFieldsValid = false;
    }

    // Проверка цены
    if (priceSpin->value() <= 0) {
        allFieldsValid = false;
    }

    // Дополнительно: проверка доступности койки
    if (allFieldsValid && !isBedAvailable()) {
        // Койка занята, но показываем предупреждение только при нажатии OK
        // Для кнопки - просто делаем неактивной
        // allFieldsValid = false; // Можно раскомментировать, если хотите блокировать кнопку
    }

    // Активируем или деактивируем кнопку OK
    okButton->setEnabled(allFieldsValid);
}

