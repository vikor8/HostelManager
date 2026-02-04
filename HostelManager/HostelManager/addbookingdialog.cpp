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

    // Создаем layout для поля клиента с кнопкой "+"
    QHBoxLayout *clientLayout = new QHBoxLayout();
    clientLayout->addWidget(clientCombo);

    // Кнопка "+" для добавления нового клиента
    QPushButton *addClientButton = new QPushButton("+", this);
    addClientButton->setFixedSize(30, 30);
    addClientButton->setToolTip("Добавить нового клиента");
    addClientButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   font-weight: bold;"
        "   font-size: 16px;"
        "   border-radius: 4px;"
        "   border: 1px solid #45a049;"
        "   min-width: 30px;"
        "   max-width: 30px;"
        "   min-height: 30px;"
        "   max-height: 30px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"
        "   border-color: #3d8b40;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3d8b40;"
        "   border-color: #367c39;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   border-color: #bbbbbb;"
        "}"
    );

    clientLayout->addWidget(addClientButton);
    clientLayout->setSpacing(5);

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
    paidSpin->setValue(0);

    // Метка для отображения общей стоимости
    totalPriceLabel = new QLabel(this);
    totalPriceLabel->setStyleSheet("font-weight: bold; color: #2E8B57;");

    // Метка для отображения остатка
    balanceLabel = new QLabel(this);
    balanceLabel->setStyleSheet("font-weight: bold;");

    // Добавляем элементы в форму
    formLayout->addRow("*Клиент:", clientLayout);
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

    // Устанавливаем "Наличные" как выбранную по умолчанию
    cashRadio->setChecked(true);

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
    connect(paidSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AddBookingDialog::updatePaidAmount);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddBookingDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(addClientButton, &QPushButton::clicked, this, &AddBookingDialog::onAddClientButtonClicked);

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

    // Блокируем кнопку добавления клиента, если БД не подключена
    addClientButton->setEnabled(database && database->isDatabaseConnected());

    // Инициализируем отображение остатка
    updatePaidAmount();
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

    updatePaidAmount(); // Обновляем остаток
}

// Обновляем validateForm для проверки оплаченной суммы
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

    // Проверка оплаты (оплаченная сумма не может быть отрицательной)
    if (paidSpin->value() < 0) {
        allFieldsValid = false;
    }

    // Способ оплаты всегда выбран (по умолчанию "Наличные")

    // Активируем или деактивируем кнопку OK
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

QString AddBookingDialog::paymentMethod() const
{
    if (cashRadio->isChecked()) {
        return "Наличные";
    } else if (cardRadio->isChecked()) {
        return "Безнал";
    } else if (transferRadio->isChecked()) {
        return "Перевод";
    }
    return "Наличные"; // По умолчанию
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
    int dialogResult = clientDialog->exec();

    if (dialogResult == QDialog::Accepted) {
        // Получаем данные из диалога
        QString lastName = clientDialog->lastName().trimmed();
        QString firstName = clientDialog->firstName().trimmed();
        QString middleName = clientDialog->middleName().trimmed();
        QString passport = clientDialog->passport().trimmed();
        QString phone = clientDialog->phone().trimmed();
        QDate birthDate = clientDialog->birthDate();
        QString country = clientDialog->country().trimmed();

        // Дополнительная валидация (на всякий случай)
        if (lastName.isEmpty() || firstName.isEmpty()) {
            QMessageBox::warning(this, "Ошибка валидации",
                "Фамилия и имя клиента являются обязательными полями.");
            clientDialog->deleteLater();
            return;
        }

        if (passport.isEmpty()) {
            QMessageBox::warning(this, "Ошибка валидации",
                "Номер паспорта является обязательным полем.");
            clientDialog->deleteLater();
            return;
        }

        if (phone.isEmpty()) {
            QMessageBox::warning(this, "Ошибка валидации",
                "Номер телефона является обязательным полем.");
            clientDialog->deleteLater();
            return;
        }

        // Проверяем формат телефона
        QString cleanPhone = phone;
        cleanPhone.remove(QRegularExpression("[^0-9+]"));
        if (cleanPhone.length() < 10) {
            QMessageBox::warning(this, "Ошибка валидации",
                "Номер телефона должен содержать не менее 10 цифр.");
            clientDialog->deleteLater();
            return;
        }

        // Проверяем возраст клиента (должен быть не младше 18 лет)
        if (birthDate.isValid()) {
            int age = birthDate.daysTo(QDate::currentDate()) / 365;
            if (age < 18) {
                QMessageBox::StandardButton reply = QMessageBox::question(this, "Подтверждение",
                    QString("Клиенту всего %1 лет.\n"
                           "Вы уверены, что хотите добавить несовершеннолетнего клиента?")
                        .arg(age),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);

                if (reply == QMessageBox::No) {
                    clientDialog->deleteLater();
                    return;
                }
            }
        }

        // Проверяем, не существует ли уже клиент с таким паспортом
        if (database->clientExists(passport)) {
            // Пытаемся найти существующего клиента
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

                QMessageBox::StandardButton reply = QMessageBox::question(this, "Клиент уже существует",
                    QString("Клиент с таким номером паспорта уже существует в базе данных!\n\n"
                           "Существующий клиент: %1\n"
                           "Новый клиент: %2\n\n"
                           "Хотите выбрать существующего клиента?")
                        .arg(existingFullName)
                        .arg(newFullName),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                    QMessageBox::Yes);

                if (reply == QMessageBox::Yes) {
                    // Обновляем список клиентов и выбираем существующего
                    loadClients();
                    int index = clientCombo->findData(existingId);
                    if (index >= 0) {
                        clientCombo->setCurrentIndex(index);
                    }
                    clientDialog->deleteLater();
                    return;
                } else if (reply == QMessageBox::Cancel) {
                    clientDialog->deleteLater();
                    return;
                }
                // Если No - продолжаем (перезапишем клиента)
            }
        }

        // Показываем прогресс
        QApplication::setOverrideCursor(Qt::WaitCursor);

        // Добавляем клиента в базу данных
        bool success = false;

        // Проверяем, нужно ли обновлять существующего клиента или добавлять нового
        if (database->clientExists(passport)) {
            // Обновляем существующего клиента
            QMessageBox::StandardButton updateReply = QMessageBox::question(this, "Обновление данных",
                "Клиент с таким паспортом уже существует.\n"
                "Хотите обновить данные клиента?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes);

            if (updateReply == QMessageBox::Yes) {
                // Получаем ID существующего клиента
                QSqlQuery idQuery(database->getDatabase());
                idQuery.prepare("SELECT id FROM clients WHERE passport_number = ?");
                idQuery.addBindValue(passport);

                if (idQuery.exec() && idQuery.next()) {
                    int clientId = idQuery.value(0).toInt();
                    success = database->updateClient(clientId, firstName, lastName, middleName,
                                                    passport, phone, birthDate, country);

                    if (success) {
                        // Обновляем список и выбираем обновленного клиента
                        loadClients();
                        int index = clientCombo->findData(clientId);
                        if (index >= 0) {
                            clientCombo->setCurrentIndex(index);
                        }
                    }
                }
            } else {
                QApplication::restoreOverrideCursor();
                clientDialog->deleteLater();
                return;
            }
        } else {
            // Добавляем нового клиента
            success = database->addClient(firstName, lastName, middleName,
                                         passport, phone, birthDate, country);
        }

        QApplication::restoreOverrideCursor();

        if (success) {
            if (!database->clientExists(passport)) {
                // Если клиент был добавлен как новый
                // Получаем ID нового клиента
                QSqlQuery query(database->getDatabase());
                query.prepare("SELECT id FROM clients WHERE passport_number = ?");
                query.addBindValue(passport);

                int newClientId = -1;
                if (query.exec() && query.next()) {
                    newClientId = query.value(0).toInt();
                }

                // Обновляем список клиентов
                loadClients();

                // Пытаемся найти и выбрать нового клиента
                if (newClientId > 0) {
                    int index = clientCombo->findData(newClientId);
                    if (index >= 0) {
                        clientCombo->setCurrentIndex(index);
                    } else {
                        // Если не нашли по ID, ищем по имени
                        QString searchName = lastName + " " + firstName;
                        index = clientCombo->findText(searchName, Qt::MatchContains);
                        if (index >= 0) {
                            clientCombo->setCurrentIndex(index);
                        }
                    }
                }
            }

            // Формируем сообщение об успехе
            QString fullName = lastName + " " + firstName;
            if (!middleName.isEmpty()) {
                fullName += " " + middleName;
            }

            QString message = QString("Клиент успешно %1!\n\n"
                                     "ФИО: %2\n"
                                     "Паспорт: %3\n"
                                     "Телефон: %4")
                .arg(database->clientExists(passport) ? "обновлен" : "добавлен")
                .arg(fullName)
                .arg(passport)
                .arg(phone);

            if (country.isEmpty()) {
                message += "\nСтрана: не указана";
            } else {
                message += "\nСтрана: " + country;
            }

            if (birthDate.isValid()) {
                int age = birthDate.daysTo(QDate::currentDate()) / 365;
                message += QString("\nДата рождения: %1 (%2 лет)")
                    .arg(birthDate.toString("dd.MM.yyyy"))
                    .arg(age);
            }

            QMessageBox::information(this, "Успех", message);

            // Устанавливаем фокус обратно на поле клиента
            clientCombo->setFocus();
            clientCombo->showPopup(); // Показываем список клиентов

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
