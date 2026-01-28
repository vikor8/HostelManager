#include "editbookingdialog.h"
#include "database.h"
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QCompleter>
#include <QTimer>
#include <QGroupBox>
#include <QHBoxLayout>

EditBookingDialog::EditBookingDialog(Database *db, int bookingId, QWidget *parent)
    : QDialog(parent)
    , database(db)
    , bookingId(bookingId)
{
    setupUi();
    loadClients();
    loadRooms();
    loadBookingData();
    calculateTotalPrice();
    validateForm();
}

EditBookingDialog::~EditBookingDialog()
{
}

void EditBookingDialog::setupUi()
{
    setWindowTitle("Редактировать бронирование");
    setMinimumWidth(500);

    QFormLayout *formLayout = new QFormLayout(this);

    // Комбобокс для выбора клиента с возможностью поиска
    clientCombo = new QComboBox(this);
    clientCombo->setEditable(true);
    clientCombo->setInsertPolicy(QComboBox::NoInsert);
    clientCombo->setPlaceholderText("Выберите или введите фамилию клиента");

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

    // Поле для оплаченной суммы
    paidSpin = new QDoubleSpinBox(this);
    paidSpin->setRange(0, 100000);
    paidSpin->setSuffix(" руб.");
    paidSpin->setDecimals(2);
    paidSpin->setSingleStep(100);

    // Метка для отображения общей стоимости
    totalPriceLabel = new QLabel(this);
    totalPriceLabel->setStyleSheet("font-weight: bold; color: #2E8B57;");

    // Метка для отображения остатка
    balanceLabel = new QLabel(this);
    balanceLabel->setStyleSheet("font-weight: bold;");

    // Добавляем элементы в форму
    formLayout->addRow("*Клиент:", clientCombo);
    formLayout->addRow("*Дата заезда:", checkInEdit);
    formLayout->addRow("*Дата выезда:", checkOutEdit);
    formLayout->addRow("*Номер комнаты:", roomCombo);
    formLayout->addRow("*Койко-место:", bedCombo);
    formLayout->addRow("*Стоимость в сутки:", priceSpin);
    formLayout->addRow("Оплачено:", paidSpin);
    formLayout->addRow("Общая стоимость:", totalPriceLabel);
    formLayout->addRow("Остаток к оплате:", balanceLabel);

    // Добавляем разделитель или заголовок для способа оплаты
    QLabel *paymentTitle = new QLabel("Способ оплаты:", this);
    paymentTitle->setStyleSheet("font-weight: bold;");
    formLayout->addRow("", paymentTitle);

    // Создаем контейнер для радио-кнопок
    QWidget *paymentWidget = new QWidget(this);
    QHBoxLayout *paymentLayout = new QHBoxLayout(paymentWidget);
    paymentLayout->setContentsMargins(0, 0, 0, 0);

    // Создаем радио-кнопки
    cashRadio = new QRadioButton("Наличные", paymentWidget);
    cardRadio = new QRadioButton("Безнал", paymentWidget);
    transferRadio = new QRadioButton("Перевод", paymentWidget);

    // Создаем группу кнопок
    paymentGroup = new QButtonGroup(this);
    paymentGroup->addButton(cashRadio);
    paymentGroup->addButton(cardRadio);
    paymentGroup->addButton(transferRadio);

    // Добавляем кнопки в layout
    paymentLayout->addWidget(cashRadio);
    paymentLayout->addWidget(cardRadio);
    paymentLayout->addWidget(transferRadio);
    paymentLayout->addStretch();

    formLayout->addRow("", paymentWidget);

    // Добавляем подсказку об обязательных полях
    QLabel *requiredLabel = new QLabel("* - обязательные поля", this);
    requiredLabel->setStyleSheet("color: gray; font-style: italic;");
    formLayout->addRow("", requiredLabel);

    // Кнопки
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        Qt::Horizontal, this);

    formLayout->addRow(buttonBox);

    // Соединяем сигналы и слоты
    connect(clientCombo->lineEdit(), &QLineEdit::textEdited, this, &EditBookingDialog::loadClients);
    connect(checkInEdit, &QDateEdit::dateChanged, this, &EditBookingDialog::validateDates);
    connect(checkOutEdit, &QDateEdit::dateChanged, this, &EditBookingDialog::validateDates);
    connect(roomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EditBookingDialog::loadBeds);
    connect(bedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EditBookingDialog::updatePriceFromDatabase);
    connect(priceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EditBookingDialog::calculateTotalPrice);
    connect(paidSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EditBookingDialog::updatePaidAmount);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &EditBookingDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Добавляем соединения для проверки формы
    connect(clientCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EditBookingDialog::validateForm);
    connect(roomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EditBookingDialog::validateForm);
    connect(bedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EditBookingDialog::validateForm);
    connect(checkInEdit, &QDateEdit::dateChanged,
            this, &EditBookingDialog::validateForm);
    connect(checkOutEdit, &QDateEdit::dateChanged,
            this, &EditBookingDialog::validateForm);
    connect(priceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &EditBookingDialog::validateForm);

    // Изначально кнопка Save неактивна
    buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
}

void EditBookingDialog::loadBookingData()
{
    if (!database || !database->isDatabaseConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }

    QSqlQuery query(database->getDatabase());
    query.prepare("SELECT b.client_id, b.check_in_date, b.check_out_date, "
                 "bd.id as bed_id, bd.room_id, bd.price_per_day, "
                 "b.total_price, b.paid_amount, b.payment_method "
                 "FROM bookings b "
                 "JOIN beds bd ON b.bed_id = bd.id "
                 "WHERE b.id = ?");
    query.addBindValue(bookingId);

    if (query.exec() && query.next()) {
        // Загружаем данные клиента
        int clientId = query.value(0).toInt();
        populateClientCombo();

        for (int i = 0; i < clientCombo->count(); ++i) {
            if (clientCombo->itemData(i).toInt() == clientId) {
                clientCombo->setCurrentIndex(i);
                break;
            }
        }

        // Загружаем даты
        checkInEdit->setDate(QDate::fromString(query.value(1).toString(), "yyyy-MM-dd"));
        checkOutEdit->setDate(QDate::fromString(query.value(2).toString(), "yyyy-MM-dd"));

        // Загружаем комнату и койку
        int bedId = query.value(3).toInt();
        int roomId = query.value(4).toInt();

        populateRoomCombo();
        for (int i = 0; i < roomCombo->count(); ++i) {
            if (roomCombo->itemData(i).toInt() == roomId) {
                roomCombo->setCurrentIndex(i);
                loadBeds();
                break;
            }
        }

        // Выбираем койку
        for (int i = 0; i < bedCombo->count(); ++i) {
            if (bedCombo->itemData(i).toInt() == bedId) {
                bedCombo->setCurrentIndex(i);
                break;
            }
        }

        // Устанавливаем цену
        priceSpin->setValue(query.value(5).toDouble());

        // Устанавливаем оплаченную сумму
        paidSpin->setValue(query.value(7).toDouble());

        // Устанавливаем способ оплаты
        QString paymentMethod = query.value(8).toString();
        if (paymentMethod == "Безнал") {
            cardRadio->setChecked(true);
        } else if (paymentMethod == "Перевод") {
            transferRadio->setChecked(true);
        } else {
            cashRadio->setChecked(true);
        }

        calculateTotalPrice();
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить данные бронирования");
    }
}

void EditBookingDialog::updatePaidAmount()
{
    double total = totalPrice();
    double paid = paidSpin->value();
    double balance = total - paid;

    if (balance > 0) {
        balanceLabel->setText(QString("%1 руб.").arg(balance, 0, 'f', 2));
        balanceLabel->setStyleSheet("font-weight: bold; color: #FF0000;");
    } else if (balance < 0) {
        balanceLabel->setText(QString("Переплата %1 руб.").arg(-balance, 0, 'f', 2));
        balanceLabel->setStyleSheet("font-weight: bold; color: #0000FF;");
    } else {
        balanceLabel->setText("Оплачено полностью");
        balanceLabel->setStyleSheet("font-weight: bold; color: #008000;");
    }
}

void EditBookingDialog::calculateTotalPrice()
{
    QDate checkIn = checkInEdit->date();
    QDate checkOut = checkOutEdit->date();
    double pricePerDay = priceSpin->value();

    if (checkIn.isValid() && checkOut.isValid() && checkOut > checkIn) {
        int days = checkIn.daysTo(checkOut);
        double total = days * pricePerDay;
        totalPriceLabel->setText(QString("%1 руб. за %2 суток").arg(total, 0, 'f', 2).arg(days));

        if (paidSpin->value() > total) {
            paidSpin->setValue(total);
        }
    } else {
        totalPriceLabel->setText("0 руб.");
    }

    updatePaidAmount();
}

void EditBookingDialog::validateForm()
{
    QDialogButtonBox *buttonBox = findChild<QDialogButtonBox*>();
    if (!buttonBox) return;

    QPushButton *saveButton = buttonBox->button(QDialogButtonBox::Save);
    if (!saveButton) return;

    bool allFieldsValid = true;

    if (clientId() <= 0) {
        allFieldsValid = false;
    }

    if (roomId() <= 0) {
        allFieldsValid = false;
    }

    if (bedId() <= 0) {
        allFieldsValid = false;
    }

    if (!checkInEdit->date().isValid() ||
        !checkOutEdit->date().isValid() ||
        checkInEdit->date() >= checkOutEdit->date()) {
        allFieldsValid = false;
    }

    if (priceSpin->value() <= 0) {
        allFieldsValid = false;
    }

    if (paidSpin->value() < 0) {
        allFieldsValid = false;
    }

    saveButton->setEnabled(allFieldsValid);
}

void EditBookingDialog::onAccept()
{
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

    if (!checkBedAvailabilityExceptCurrent()) {
        QMessageBox::warning(this, "Ошибка",
            "Выбранное койко-место уже забронировано на указанные даты.\n"
            "Пожалуйста, выберите другие даты или другую койку.");
        return;
    }

    if (priceSpin->value() <= 0) {
        QMessageBox::warning(this, "Ошибка", "Стоимость должна быть больше 0");
        return;
    }

    if (paidSpin->value() < 0) {
        QMessageBox::warning(this, "Ошибка", "Оплаченная сумма не может быть отрицательной");
        return;
    }

    accept();
}

bool EditBookingDialog::checkBedAvailabilityExceptCurrent()
{
    int bedId = this->bedId();
    QDate checkIn = checkInEdit->date();
    QDate checkOut = checkOutEdit->date();

    if (bedId <= 0 || !checkIn.isValid() || !checkOut.isValid()) {
        return false;
    }

    if (database && database->isDatabaseConnected()) {
        QSqlQuery query(database->getDatabase());
        query.prepare("SELECT COUNT(*) FROM bookings "
                     "WHERE bed_id = ? "
                     "AND status = 'active' "
                     "AND id != ? " // Исключаем текущее бронирование
                     "AND NOT (? <= check_in_date OR ? >= check_out_date)");
        query.addBindValue(bedId);
        query.addBindValue(bookingId);
        query.addBindValue(checkOut);
        query.addBindValue(checkIn);

        if (query.exec() && query.next()) {
            return query.value(0).toInt() == 0;
        }
    }

    return true;
}

// Остальные методы аналогичны AddBookingDialog

void EditBookingDialog::loadClients()
{
    if (!database || !database->isDatabaseConnected()) {
        return;
    }

    QString filter = clientCombo->currentText().trimmed();
    populateClientCombo(filter);
}

void EditBookingDialog::populateClientCombo(const QString& filter)
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

void EditBookingDialog::loadRooms()
{
    populateRoomCombo();
}

void EditBookingDialog::populateRoomCombo()
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

void EditBookingDialog::loadBeds()
{
    int roomId = roomCombo->currentData().toInt();
    if (roomId > 0) {
        populateBedCombo(roomId);
    }
}

void EditBookingDialog::populateBedCombo(int roomId)
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

    if (bedCombo->count() > 0) {
        updatePriceFromDatabase();
    }
}

void EditBookingDialog::updatePriceFromDatabase()
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

void EditBookingDialog::validateDates()
{
    QDate checkIn = checkInEdit->date();
    QDate checkOut = checkOutEdit->date();

    if (checkOut <= checkIn) {
        checkOutEdit->setDate(checkIn.addDays(1));
    }

    calculateTotalPrice();
}

int EditBookingDialog::clientId() const
{
    return clientCombo->currentData().toInt();
}

int EditBookingDialog::roomId() const
{
    return roomCombo->currentData().toInt();
}

int EditBookingDialog::bedId() const
{
    return bedCombo->currentData().toInt();
}

double EditBookingDialog::totalPrice() const
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

QString EditBookingDialog::paymentMethod() const
{
    if (cashRadio->isChecked()) {
        return "Наличные";
    } else if (cardRadio->isChecked()) {
        return "Безнал";
    } else if (transferRadio->isChecked()) {
        return "Перевод";
    }
    return "Наличные";
}
