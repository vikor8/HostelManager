#include "addbookingdialog.h"
#include "database.h"
#include "addclientdialog.h"
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QCompleter>
#include <QTimer>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStyle>
#include <QToolTip>
#include <QApplication>

AddBookingDialog::AddBookingDialog(Database *db, QWidget *parent)
    : QDialog(parent)
    , database(db)
    , currentBookingType(Place) // По умолчанию "Место"
{
    setupUi();
    loadClients();
    loadRooms();

    if (roomCombo->count() > 0) {
        loadBeds();
    }

    // Устанавливаем даты по умолчанию
    checkInEdit->setCalendarPopup(true);
    checkInEdit->setDisplayFormat("dd.MM.yyyy");

    calculateTotalPrice();
    validateForm();
    updateBedComboVisibility(); // Устанавливаем видимость поля с койками
}

AddBookingDialog::~AddBookingDialog()
{
}

void AddBookingDialog::setupUi()
{
    setWindowTitle("Добавить бронирование");
    setMinimumWidth(500);

    QFormLayout *formLayout = new QFormLayout(this);

    // --- Секция выбора типа бронирования ---
    QLabel *bookingTypeTitle = new QLabel("Тип бронирования:", this);
    bookingTypeTitle->setStyleSheet("font-weight: bold;");

    QWidget *bookingTypeWidget = new QWidget(this);
    QHBoxLayout *bookingTypeLayout = new QHBoxLayout(bookingTypeWidget);
    bookingTypeLayout->setContentsMargins(0, 0, 0, 0);

    placeRadio = new QRadioButton("Место", bookingTypeWidget);
    wholeRoomRadio = new QRadioButton("Комната целиком", bookingTypeWidget);

    bookingTypeGroup = new QButtonGroup(this);
    bookingTypeGroup->addButton(placeRadio);
    bookingTypeGroup->addButton(wholeRoomRadio);

    placeRadio->setChecked(true); // По умолчанию "Место"

    bookingTypeLayout->addWidget(placeRadio);
    bookingTypeLayout->addWidget(wholeRoomRadio);
    bookingTypeLayout->addStretch();

    formLayout->addRow(bookingTypeTitle, bookingTypeWidget);

    // --- Секция клиента ---
    QLabel *clientTitle = new QLabel("*Клиент:", this);
    clientTitle->setStyleSheet("font-weight: bold;");

    QWidget *clientWidget = new QWidget(this);
    QHBoxLayout *clientLayout = new QHBoxLayout(clientWidget);
    clientLayout->setContentsMargins(0, 0, 0, 0);

    clientCombo = new QComboBox(this);
    clientCombo->setEditable(true);
    clientCombo->setInsertPolicy(QComboBox::NoInsert);
    clientCombo->setPlaceholderText("Выберите или введите фамилию клиента");

    QCompleter *completer = new QCompleter(clientCombo);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    clientCombo->setCompleter(completer);

    clientLayout->addWidget(clientCombo);

    addClientButton = new QPushButton("+", this);
    addClientButton->setFixedSize(30, 30);
    addClientButton->setToolTip("Добавить нового клиента");
    addClientButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #bbbbbb;"
        "   color: white;"
        "   font-weight: bold;"
        "   font-size: 16px;"
        "   border-radius: 4px;"
        "   border: 1px solid #0066B3;"
        "   min-width: 30px;"
        "   max-width: 30px;"
        "   min-height: 30px;"
        "   max-height: 30px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #0066B3;"
        "   border-color: #005299;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #005299;"
        "   border-color: #004080;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   border-color: #bbbbbb;"
        "}"
    );

    clientLayout->addWidget(addClientButton);
    clientLayout->setSpacing(5);

    formLayout->addRow(clientTitle, clientWidget);

    // --- Поля для дат ---
    QLabel *dateTitle = new QLabel("*Даты:", this);
    dateTitle->setStyleSheet("font-weight: bold;");

    QWidget *dateWidget = new QWidget(this);
    QHBoxLayout *dateLayout = new QHBoxLayout(dateWidget);
    dateLayout->setContentsMargins(0, 0, 0, 0);

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

    dateLayout->addWidget(new QLabel("Заезд:", this));
    dateLayout->addWidget(checkInEdit);
    dateLayout->addWidget(new QLabel("Выезд:", this));
    dateLayout->addWidget(checkOutEdit);
    dateLayout->addStretch();

    formLayout->addRow(dateTitle, dateWidget);

    // --- Комната ---
    QLabel *roomTitle = new QLabel("*Номер комнаты:", this);
    roomTitle->setStyleSheet("font-weight: bold;");

    roomCombo = new QComboBox(this);
    roomCombo->setPlaceholderText("Выберите номер комнаты");
    formLayout->addRow(roomTitle, roomCombo);

    // --- Койко-место ---
    QLabel *bedTitle = new QLabel("*Койко-место:", this);
    bedTitle->setStyleSheet("font-weight: bold;");

    bedCombo = new QComboBox(this);
    bedCombo->setPlaceholderText("Выберите койко-место");
    formLayout->addRow(bedTitle, bedCombo);

    // --- Цена ---
    QLabel *priceTitle = new QLabel("*Стоимость в сутки:", this);
    priceTitle->setStyleSheet("font-weight: bold;");

    priceSpin = new QDoubleSpinBox(this);
    priceSpin->setRange(100, 10000);
    priceSpin->setSuffix(" руб./сутки");
    priceSpin->setDecimals(2);
    priceSpin->setSingleStep(100);
    formLayout->addRow(priceTitle, priceSpin);

    // --- Суммы ---
    paidSpin = new QDoubleSpinBox(this);
    paidSpin->setRange(0, 100000);
    paidSpin->setSuffix(" руб.");
    paidSpin->setDecimals(2);
    paidSpin->setSingleStep(100);
    paidSpin->setValue(0);
    formLayout->addRow("Оплачено:", paidSpin);

    totalPriceLabel = new QLabel(this);
    totalPriceLabel->setStyleSheet("font-weight: bold; color: #2E8B57;");
    formLayout->addRow("Общая стоимость:", totalPriceLabel);

    balanceLabel = new QLabel(this);
    balanceLabel->setStyleSheet("font-weight: bold;");
    formLayout->addRow("Остаток к оплате:", balanceLabel);

    // --- Способы оплаты ---
    QLabel *paymentTitle = new QLabel("Способ оплаты:", this);
    paymentTitle->setStyleSheet("font-weight: bold;");

    QWidget *paymentWidget = new QWidget(this);
    QHBoxLayout *paymentLayout = new QHBoxLayout(paymentWidget);
    paymentLayout->setContentsMargins(0, 0, 0, 0);

    cashRadio = new QRadioButton("Наличные", paymentWidget);
    cardRadio = new QRadioButton("Безнал", paymentWidget);
    transferRadio = new QRadioButton("Перевод", paymentWidget);
    legalEntityRadio = new QRadioButton("На р/с юрлица", paymentWidget); // Новый способ оплаты

    paymentGroup = new QButtonGroup(this);
    paymentGroup->addButton(cashRadio);
    paymentGroup->addButton(cardRadio);
    paymentGroup->addButton(transferRadio);
    paymentGroup->addButton(legalEntityRadio);

    cashRadio->setChecked(true); // По умолчанию "Наличные"

    paymentLayout->addWidget(cashRadio);
    paymentLayout->addWidget(cardRadio);
    paymentLayout->addWidget(transferRadio);
    paymentLayout->addWidget(legalEntityRadio);
    paymentLayout->addStretch();

    formLayout->addRow(paymentTitle, paymentWidget);

    // --- Подсказка об обязательных полях ---
    QLabel *requiredLabel = new QLabel("* - обязательные поля", this);
    requiredLabel->setStyleSheet("color: gray; font-style: italic;");
    formLayout->addRow("", requiredLabel);

    // --- Кнопки ---
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, this);
    formLayout->addRow(buttonBox);

    // --- Подключение сигналов и слотов ---
    connect(clientCombo->lineEdit(), &QLineEdit::textEdited, this, &AddBookingDialog::loadClients);
    connect(checkInEdit, &QDateEdit::dateChanged, this, &AddBookingDialog::validateDates);
    connect(checkOutEdit, &QDateEdit::dateChanged, this, &AddBookingDialog::validateDates);
    connect(roomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AddBookingDialog::loadBeds);
    connect(bedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AddBookingDialog::updatePriceFromDatabase);
    connect(priceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AddBookingDialog::calculateTotalPrice);
    connect(paidSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AddBookingDialog::updatePaidAmount);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddBookingDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(addClientButton, &QPushButton::clicked, this, &AddBookingDialog::onAddClientButtonClicked);

    // Новые соединения для типа бронирования
    connect(placeRadio, &QRadioButton::toggled, this, &AddBookingDialog::onBookingTypeChanged);
    connect(wholeRoomRadio, &QRadioButton::toggled, this, &AddBookingDialog::onBookingTypeChanged);

    // Соединения для проверки формы
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

    // Блокируем кнопку добавления клиента, если БД не подключена
    addClientButton->setEnabled(database && database->isDatabaseConnected());

    // Инициализируем отображение остатка
    updatePaidAmount();
}

// Новый слот для обработки изменения типа бронирования
void AddBookingDialog::onBookingTypeChanged()
{
    if (placeRadio->isChecked()) {
        currentBookingType = Place;
    } else if (wholeRoomRadio->isChecked()) {
        currentBookingType = WholeRoom;
    }

    updateBedComboVisibility();
    updatePriceFromDatabase(); // Обновляем цену в соответствии с типом бронирования
    validateForm();
}

// Метод для управления видимостью поля с койками
void AddBookingDialog::updateBedComboVisibility()
{
    // Находим лейбл для поля "Койко-место" (он находится в форме)
    QFormLayout *formLayout = qobject_cast<QFormLayout*>(layout());
    if (!formLayout) return;

    bool isPlaceMode = (currentBookingType == Place);

    // Делаем видимым или невидимым поле с койками
    bedCombo->setVisible(isPlaceMode);

    // Находим и скрываем/показываем соответствующий лейбл
    for (int i = 0; i < formLayout->rowCount(); ++i) {
        QLabel *label = qobject_cast<QLabel*>(formLayout->itemAt(i, QFormLayout::LabelRole)->widget());
        if (label && label->text().contains("Койко-место", Qt::CaseInsensitive)) {
            label->setVisible(isPlaceMode);
            break;
        }
    }

    // Если режим "Комната целиком", сбрасываем выбор койки
    if (!isPlaceMode) {
        bedCombo->setCurrentIndex(-1);
    }

    // Перестраиваем layout
    layout()->activate();
}

// Новый метод для проверки доступности комнаты целиком
bool AddBookingDialog::isRoomAvailable() const
{
    int roomId = this->roomId();
    QDate checkIn = checkInEdit->date();
    QDate checkOut = checkOutEdit->date();

    if (roomId <= 0 || !checkIn.isValid() || !checkOut.isValid()) {
        return false;
    }

    if (!database || !database->isDatabaseConnected()) {
        return false;
    }

    // Проверяем, что ВСЕ койки в комнате свободны
    QSqlQuery query(database->getDatabase());
    query.prepare("SELECT COUNT(*) FROM beds b "
                  "WHERE b.room_id = ? AND b.is_active = 1 "
                  "AND EXISTS ("
                  "    SELECT 1 FROM bookings bk "
                  "    WHERE bk.bed_id = b.id "
                  "    AND bk.status = 'active' "
                  "    AND bk.check_in_date < ? "
                  "    AND bk.check_out_date > ?"
                  ")");
    query.addBindValue(roomId);
    query.addBindValue(checkOut);
    query.addBindValue(checkIn);

    if (query.exec() && query.next()) {
        int occupiedBeds = query.value(0).toInt();
        return occupiedBeds == 0; // Комната доступна, если все койки свободны
    }

    return false;
}
// Новый метод для обновления оплаченной суммы и остатка
void AddBookingDialog::updatePaidAmount()
{
    double total = totalPrice();
    double paid = paidSpin->value();
    double balance = total - paid;

    if (balance > 0) {
        balanceLabel->setText(QString("Остаток: %1 руб. (%2%)")
                              .arg(balance, 0, 'f', 2)
                              .arg(qRound((paid / total) * 100)));
        balanceLabel->setStyleSheet("font-weight: bold; color: #FF0000;");
    } else if (balance < 0) {
        balanceLabel->setText(QString("Переплата %1 руб.").arg(-balance, 0, 'f', 2));
        balanceLabel->setStyleSheet("font-weight: bold; color: #0000FF;");
    } else {
        balanceLabel->setText("Оплачено полностью ✓");
        balanceLabel->setStyleSheet("font-weight: bold; color: #008000;");
    }
}

// Обновляем calculateTotalPrice чтобы также обновлять остаток
void AddBookingDialog::calculateTotalPrice()
{
    QDate checkIn = checkInEdit->date();
    QDate checkOut = checkOutEdit->date();
    double pricePerDay = priceSpin->value();

    if (checkIn.isValid() && checkOut.isValid() && checkOut > checkIn) {
        int days = checkIn.daysTo(checkOut);
        double total = days * pricePerDay;
        totalPriceLabel->setText(QString("%1 руб. за %2 суток").arg(total, 0, 'f', 2).arg(days));

        // Если оплаченная сумма больше новой общей стоимости, корректируем оплату
        if (paidSpin->value() > total) {
            paidSpin->setValue(total);
        }
    } else {
        totalPriceLabel->setText("0 руб.");
    }

    updatePaidAmount();
}
// Обновляем validateForm для проверки оплаченной суммы
void AddBookingDialog::validateForm()
{
    QDialogButtonBox *buttonBox = findChild<QDialogButtonBox*>();
    if (!buttonBox) return;

    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    if (!okButton) return;

    bool allFieldsValid = true;

    // Проверка клиента
    if (clientId() <= 0) {
        allFieldsValid = false;
    }

    // Проверка комнаты
    if (roomId() <= 0) {
        allFieldsValid = false;
    }

    // Проверка койки (только для режима "Место")
    if (currentBookingType == Place && bedId() <= 0) {
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

    // Проверка оплаты
    if (paidSpin->value() < 0) {
        allFieldsValid = false;
    }

    okButton->setEnabled(allFieldsValid);
}

// Обновляем onAccept для проверки оплаченной суммы
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

    if (currentBookingType == Place && bedId() <= 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите койко-место");
        return;
    }

    if (checkInEdit->date() >= checkOutEdit->date()) {
        QMessageBox::warning(this, "Ошибка", "Дата выезда должна быть позже даты заезда");
        return;
    }

    // Проверяем доступность в зависимости от типа бронирования
    if (currentBookingType == Place) {
        if (!isBedAvailable()) {
            QMessageBox::warning(this, "Ошибка",
                QString("Выбранное койко-место уже забронировано на указанные даты.\n"
                       "Комната: %1, Койка: %2\n"
                       "Период: %3 - %4\n\n"
                       "Пожалуйста, выберите другие даты или другую койку.")
                    .arg(roomCombo->currentText())
                    .arg(bedCombo->currentText())
                    .arg(checkInEdit->date().toString("dd.MM.yyyy"))
                    .arg(checkOutEdit->date().toString("dd.MM.yyyy")));
            return;
        }
    } else { // WholeRoom
        if (!isRoomAvailable()) {
            QMessageBox::warning(this, "Ошибка",
                QString("Комната %1 уже занята на указанные даты.\n"
                       "Период: %2 - %3\n\n"
                       "Пожалуйста, выберите другие даты или другую комнату.")
                    .arg(roomCombo->currentText())
                    .arg(checkInEdit->date().toString("dd.MM.yyyy"))
                    .arg(checkOutEdit->date().toString("dd.MM.yyyy")));
            return;
        }
    }

    // Проверяем стоимость
    if (priceSpin->value() <= 0) {
        QMessageBox::warning(this, "Ошибка", "Стоимость должна быть больше 0");
        return;
    }

    // Проверяем оплаченную сумму
    if (paidSpin->value() < 0) {
        QMessageBox::warning(this, "Ошибка", "Оплаченная сумма не может быть отрицательной");
        return;
    }

    accept();
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
    // Запоминаем текущего выбранного клиента
    int currentClientId = clientId();
    QString currentText = clientCombo->currentText();

    clientCombo->clear();
    clientMap.clear();

    QSqlQuery query(database->getDatabase());
    QString sql = "SELECT id, last_name, first_name, middle_name FROM clients ";

    if (!filter.isEmpty()) {
        sql += "WHERE last_name LIKE :filter || '%' ";
        sql += "OR first_name LIKE :filter || '%' ";
        sql += "OR middle_name LIKE :filter || '%' ";
        sql += "OR (last_name || ' ' || first_name) LIKE :filter || '%' ";
        sql += "OR (last_name || ' ' || first_name || ' ' || COALESCE(middle_name, '')) LIKE :filter || '%' ";
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

        // Восстанавливаем выбранного клиента, если он есть
        if (currentClientId > 0) {
            int index = clientCombo->findData(currentClientId);
            if (index >= 0) {
                clientCombo->setCurrentIndex(index);
            } else if (!currentText.isEmpty()) {
                clientCombo->setCurrentText(currentText);
            }
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

// Обновленный метод получения цены
void AddBookingDialog::updatePriceFromDatabase()
{
    if (currentBookingType == Place) {
        // Режим "Место" - берем цену из выбранной койки
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
    } else {
        // Режим "Комната целиком" - суммируем цены всех коек в комнате
        int roomId = this->roomId();
        if (roomId <= 0) {
            return;
        }

        QSqlQuery query(database->getDatabase());
        query.prepare("SELECT SUM(price_per_day) FROM beds WHERE room_id = ? AND is_active = 1");
        query.addBindValue(roomId);

        if (query.exec() && query.next()) {
            double totalRoomPrice = query.value(0).toDouble();
            priceSpin->setValue(totalRoomPrice);
        }
    }
}

// Вспомогательный метод для расчета общей стоимости комнаты
double AddBookingDialog::getRoomTotalPrice() const
{
    int roomId = this->roomId();
    if (roomId <= 0) return 0.0;

    QSqlQuery query(database->getDatabase());
    query.prepare("SELECT SUM(price_per_day) FROM beds WHERE room_id = ? AND is_active = 1");
    query.addBindValue(roomId);

    if (query.exec() && query.next()) {
        return query.value(0).toDouble();
    }
    return 0.0;
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

// Новый геттер для типа бронирования
AddBookingDialog::BookingType AddBookingDialog::bookingType() const
{
    return currentBookingType;
}

// Обновленный геттер для способа оплаты
QString AddBookingDialog::paymentMethod() const
{
    if (cashRadio->isChecked()) {
        return "Наличные";
    } else if (cardRadio->isChecked()) {
        return "Безнал";
    } else if (transferRadio->isChecked()) {
        return "Перевод";
    } else if (legalEntityRadio->isChecked()) {
        return "На р/с юрлица";
    }
    return "Наличные";
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

void AddBookingDialog::setRoomAndBed(int roomId, int bedId)
{
    // Находим комнату в комбобоксе
    int roomIndex = roomCombo->findData(roomId);
    if (roomIndex >= 0) {
        roomCombo->setCurrentIndex(roomIndex);
        loadBeds(); // Загружаем койки для этой комнаты

        // Даем время для загрузки коек
        QTimer::singleShot(100, [this, bedId]() {
            // Находим койку в комбобоксе
            int bedIndex = bedCombo->findData(bedId);
            if (bedIndex >= 0) {
                bedCombo->setCurrentIndex(bedIndex);
                updatePriceFromDatabase();
            }
        });
    }
}

void AddBookingDialog::onAddClientButtonClicked()
{
    // Проверяем подключение к базе данных
    if (!database || !database->isDatabaseConnected()) {
        QMessageBox::critical(this, "Ошибка подключения",
            "База данных не подключена!\n\n"
            "Пожалуйста, подключитесь к базе данных перед добавлением клиента.");
        return;
    }

    // Сохраняем текущий текст в поле клиента
    QString currentClientText = clientCombo->currentText();

    // Создаем диалог добавления клиента
    AddClientDialog *clientDialog = new AddClientDialog(this, AddClientDialog::Add);

    // Настраиваем диалог
    clientDialog->setWindowTitle("Добавление клиента для бронирования");
    clientDialog->setModal(true);

    // Показываем диалог и получаем результат
    if (clientDialog->exec() == QDialog::Accepted) {
        // Получаем данные из диалога
        QString lastName = clientDialog->lastName().trimmed();
        QString firstName = clientDialog->firstName().trimmed();
        QString middleName = clientDialog->middleName().trimmed();
        QString passport = clientDialog->passport().trimmed();
        QString phone = clientDialog->phone().trimmed();
        QDate birthDate = clientDialog->birthDate();
        QString country = clientDialog->country().trimmed();

        // Проверяем обязательные поля
        if (lastName.isEmpty() || firstName.isEmpty() || passport.isEmpty() || phone.isEmpty()) {
            QMessageBox::warning(this, "Ошибка валидации",
                "Заполните все обязательные поля (Фамилия, Имя, Паспорт, Телефон).");
            clientDialog->deleteLater();
            return;
        }

        // Проверяем, не существует ли уже клиент с таким паспортом
        if (database->clientExists(passport)) {
            // Если клиент существует, находим его и выбираем
            QSqlQuery query(database->getDatabase());
            query.prepare("SELECT id, last_name, first_name, middle_name FROM clients WHERE passport_number = ?");
            query.addBindValue(passport);

            if (query.exec() && query.next()) {
                int existingId = query.value(0).toInt();
                QString existingLastName = query.value(1).toString();
                QString existingFirstName = query.value(2).toString();
                QString existingMiddleName = query.value(3).toString();

                QString existingFullName = existingLastName + " " + existingFirstName;
                if (!existingMiddleName.isEmpty()) {
                    existingFullName += " " + existingMiddleName;
                }

                QString newFullName = lastName + " " + firstName;
                if (!middleName.isEmpty()) {
                    newFullName += " " + middleName;
                }

                QMessageBox::information(this, "Клиент уже существует",
                    QString("Клиент с таким номером паспорта уже существует в базе данных!\n\n"
                           "Будет выбран существующий клиент:\n%1")
                        .arg(existingFullName));

                // Обновляем список клиентов
                loadClients();

                // Находим и выбираем существующего клиента
                int index = clientCombo->findData(existingId);
                if (index >= 0) {
                    clientCombo->setCurrentIndex(index);
                    clientCombo->setFocus();

                    // Показываем краткое сообщение о выборе клиента
                    QTimer::singleShot(100, this, [this, existingFullName]() {
                        QToolTip::showText(clientCombo->mapToGlobal(QPoint(0, 0)),
                            QString("Выбран клиент: %1").arg(existingFullName),
                            clientCombo, QRect(), 2000);
                    });
                }
            }

            clientDialog->deleteLater();
            return;
        }

        // Добавляем клиента в базу данных
        bool success = database->addClient(firstName, lastName, middleName,
                                          passport, phone, birthDate, country);

        if (success) {
            // Получаем ID нового клиента (последний добавленный)
            QSqlQuery query(database->getDatabase());
            query.prepare("SELECT id, last_name, first_name, middle_name FROM clients WHERE passport_number = ?");
            query.addBindValue(passport);

            int newClientId = -1;
            QString dbLastName, dbFirstName, dbMiddleName;

            if (query.exec() && query.next()) {
                newClientId = query.value(0).toInt();
                dbLastName = query.value(1).toString();
                dbFirstName = query.value(2).toString();
                dbMiddleName = query.value(3).toString();
            }

            // Обновляем список клиентов
            loadClients();

            // Формируем полное имя для поиска
            QString searchName = dbLastName + " " + dbFirstName;
            if (!dbMiddleName.isEmpty()) {
                searchName += " " + dbMiddleName;
            }

            // Пытаемся найти и выбрать нового клиента несколькими способами
            bool clientSelected = false;

            // 1. Поиск по ID (самый надежный)
            if (newClientId > 0) {
                int index = clientCombo->findData(newClientId);
                if (index >= 0) {
                    clientCombo->setCurrentIndex(index);
                    clientSelected = true;
                }
            }

            // 2. Поиск по полному имени
            if (!clientSelected && !searchName.isEmpty()) {
                int index = clientCombo->findText(searchName, Qt::MatchExactly);
                if (index < 0) {
                    index = clientCombo->findText(searchName, Qt::MatchContains);
                }
                if (index >= 0) {
                    clientCombo->setCurrentIndex(index);
                    clientSelected = true;
                }
            }

            // 3. Поиск по фамилии
            if (!clientSelected && !dbLastName.isEmpty()) {
                int index = clientCombo->findText(dbLastName, Qt::MatchContains);
                if (index >= 0) {
                    clientCombo->setCurrentIndex(index);
                    clientSelected = true;
                }
            }

            // Формируем сообщение об успехе
            QString message;
            QString fullName = dbLastName + " " + dbFirstName;
            if (!dbMiddleName.isEmpty()) {
                fullName += " " + dbMiddleName;
            }

            if (clientSelected) {
                message = QString("Клиент успешно добавлен и выбран!\n\n"
                                 "ФИО: %1\n"
                                 "Паспорт: %2\n"
                                 "Телефон: %3\n\n"
                                 "Клиент автоматически выбран для бронирования.")
                    .arg(fullName)
                    .arg(passport)
                    .arg(phone);
            } else {
                message = QString("Клиент успешно добавлен!\n\n"
                                 "ФИО: %1\n"
                                 "Паспорт: %2\n"
                                 "Телефон: %3\n\n"
                                 "Пожалуйста, выберите клиента из списка вручную.")
                    .arg(fullName)
                    .arg(passport)
                    .arg(phone);
            }

            // Показываем информационное сообщение


            // Если клиент выбран, показываем подсказку
            if (clientSelected) {
                QTimer::singleShot(100, this, [this, fullName]() {
                    QToolTip::showText(clientCombo->mapToGlobal(QPoint(0, 0)),
                        QString("Выбран новый клиент: %1").arg(fullName),
                        clientCombo, QRect(), 3000);
                });
            }

            // Устанавливаем фокус обратно на поле клиента и показываем список
            clientCombo->setFocus();
            clientCombo->showPopup();

            // Автоматически переходим к следующему полю если клиент выбран
            if (clientSelected) {
                QTimer::singleShot(500, this, [this]() {
                    checkInEdit->setFocus();
                });
            }

        } else {
            QMessageBox::critical(this, "Ошибка",
                "Не удалось сохранить данные клиента.\n"
                "Проверьте подключение к базе данных и повторите попытку.");

            // Восстанавливаем предыдущий текст
            if (!currentClientText.isEmpty()) {
                clientCombo->setCurrentText(currentClientText);
            }
        }
    } else {
        // Пользователь отменил добавление клиента
        // Восстанавливаем предыдущий текст
        if (!currentClientText.isEmpty()) {
            clientCombo->setCurrentText(currentClientText);
        }
    }

    // Удаляем диалог
    clientDialog->deleteLater();
}


QList<int> AddBookingDialog::getAllBedsInRoom() const
{
    QList<int> bedIds;
    int roomId = this->roomId();

    if (roomId <= 0 || !database || !database->isDatabaseConnected()) {
        return bedIds;
    }

    QSqlQuery query(database->getDatabase());
    query.prepare("SELECT id FROM beds WHERE room_id = ? AND is_active = 1 ORDER BY bed_number");
    query.addBindValue(roomId);

    if (query.exec()) {
        while (query.next()) {
            bedIds.append(query.value(0).toInt());
        }
    } else {
        qDebug() << "Ошибка получения коек в комнате:" << query.lastError().text();
    }

    return bedIds;
}

void AddBookingDialog::setBookingType(BookingType type)
{
    if (type == Place) {
        placeRadio->setChecked(true);
    } else {
        wholeRoomRadio->setChecked(true);
    }
    onBookingTypeChanged();
}
