#include "cancelbookingdialog.h"
#include "database.h"
#include <QComboBox>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QGroupBox>
#include <QDebug>

CancelBookingDialog::CancelBookingDialog(Database *db, QWidget *parent)
    : QDialog(parent)
    , database(db)
{
    setupUi();
    loadActiveBookings();
    setWindowTitle("Отмена бронирования");
}

CancelBookingDialog::~CancelBookingDialog()
{
}

void CancelBookingDialog::setupUi()
{
    setMinimumWidth(500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Форма для выбора бронирования
    QFormLayout *formLayout = new QFormLayout();

    bookingCombo = new QComboBox(this);
    bookingCombo->setPlaceholderText("Выберите бронирование для отмены");
    formLayout->addRow("Бронирование:", bookingCombo);

    mainLayout->addLayout(formLayout);

    // Детали бронирования
    QGroupBox *detailsGroup = new QGroupBox("Детали бронирования", this);
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsGroup);

    QLabel *detailsLabel = new QLabel(this);
    detailsLabel->setText("Выберите бронирование для просмотра деталей");
    detailsLabel->setWordWrap(true);
    detailsLabel->setStyleSheet("padding: 10px; background-color: #f0f0f0; border: 1px solid #ccc;");
    detailsLayout->addWidget(detailsLabel);

    mainLayout->addWidget(detailsGroup);

    // Причина отмены
    QLabel *reasonLabel = new QLabel("Причина отмены:", this);
    mainLayout->addWidget(reasonLabel);

    reasonEdit = new QTextEdit(this);
    reasonEdit->setPlaceholderText("Укажите причину отмены бронирования...");
    reasonEdit->setMaximumHeight(100);
    mainLayout->addWidget(reasonEdit);

    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    cancelButton = new QPushButton("Отменить бронирование", this);
    cancelButton->setStyleSheet("QPushButton { background-color: #dc3545; color: white; font-weight: bold; }");

    QPushButton *closeButton = new QPushButton("Закрыть", this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    // Соединяем сигналы
    connect(bookingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CancelBookingDialog::updateBookingDetails);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(reasonEdit, &QTextEdit::textChanged, this, &CancelBookingDialog::validateForm);

    // Динамическое обновление деталей
    detailsLabel->setObjectName("detailsLabel");
    cancelButton->setEnabled(false);
}

void CancelBookingDialog::loadActiveBookings()
{
    if (!database || !database->isDatabaseConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }

    bookingCombo->clear();
    bookingMap.clear();

    QSqlQuery query(database->getDatabase());
    query.prepare("SELECT "
                  "bk.id, "
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
                  "WHERE bk.status = 'active' "
                  "ORDER BY bk.check_in_date DESC, r.room_number, b.bed_number");

    if (query.exec()) {
        while (query.next()) {
            int bookingId = query.value(0).toInt();
            QString roomNumber = query.value(1).toString();
            int bedNumber = query.value(2).toInt();
            QString clientName = query.value(3).toString();
            QDate checkIn = QDate::fromString(query.value(4).toString(), "yyyy-MM-dd");
            QDate checkOut = QDate::fromString(query.value(5).toString(), "yyyy-MM-dd");
            double totalPrice = query.value(6).toDouble();
//            double paidAmount = query.value(7).toDouble();
            // Переменная balance не используется, поэтому можно удалить ее вычисление
            // double balance = totalPrice - paidAmount; // Убрано

            QString displayText = QString("№%1 | Комната: %2, Койка: %3 | Клиент: %4 | %5 - %6 | Стоимость: %7 руб.")
                                  .arg(bookingId)
                                  .arg(roomNumber)
                                  .arg(bedNumber)
                                  .arg(clientName)
                                  .arg(checkIn.toString("dd.MM.yyyy"))
                                  .arg(checkOut.toString("dd.MM.yyyy"))
                                  .arg(totalPrice, 0, 'f', 2);

            bookingMap.insert(bookingId, displayText);
            bookingCombo->addItem(displayText, bookingId);
        }

        if (bookingCombo->count() > 0) {
            bookingCombo->setCurrentIndex(0);
            updateBookingDetails();
        }
    } else {
        qDebug() << "Ошибка загрузки бронирований:" << query.lastError().text();
    }
}

void CancelBookingDialog::updateBookingDetails()
{
    int bookingId = bookingCombo->currentData().toInt();

    if (bookingId <= 0) {
        QLabel *detailsLabel = findChild<QLabel*>("detailsLabel");
        if (detailsLabel) {
            detailsLabel->setText("Выберите бронирование для просмотра деталей");
        }
        return;
    }

    QSqlQuery query(database->getDatabase());
    query.prepare("SELECT "
                  "r.room_number, "
                  "b.bed_number, "
                  "c.last_name || ' ' || c.first_name as client_name, "
                  "c.passport_number, "
                  "c.phone_number, "
                  "bk.check_in_date, "
                  "bk.check_out_date, "
                  "bk.total_price, "
                  "bk.paid_amount, "
                  "bk.payment_method, "
                  "bk.created_at "
                  "FROM bookings bk "
                  "JOIN beds b ON bk.bed_id = b.id "
                  "JOIN rooms r ON b.room_id = r.id "
                  "JOIN clients c ON bk.client_id = c.id "
                  "WHERE bk.id = ?");
    query.addBindValue(bookingId);

    if (query.exec() && query.next()) {
        QString roomNumber = query.value(0).toString();
        int bedNumber = query.value(1).toInt();
        QString clientName = query.value(2).toString();
        QString passport = query.value(3).toString();
        QString phone = query.value(4).toString();
        QDate checkIn = QDate::fromString(query.value(5).toString(), "yyyy-MM-dd");
        QDate checkOut = QDate::fromString(query.value(6).toString(), "yyyy-MM-dd");
        double totalPrice = query.value(7).toDouble();
        double paidAmount = query.value(8).toDouble();
        QString paymentMethod = query.value(9).toString();
        QDateTime createdAt = QDateTime::fromString(query.value(10).toString(), "yyyy-MM-dd hh:mm:ss");

        double balance = totalPrice - paidAmount;
        int nights = checkIn.daysTo(checkOut);

        QString details = QString(
            "<html><body style='font-family: Arial; font-size: 10pt;'>"
            "<h3>Детали бронирования</h3>"
            "<table cellpadding='5'>"
            "<tr><td><b>ID бронирования:</b></td><td>%1</td></tr>"
            "<tr><td><b>Комната:</b></td><td>%2</td></tr>"
            "<tr><td><b>Койка:</b></td><td>%3</td></tr>"
            "<tr><td><b>Клиент:</b></td><td>%4</td></tr>"
            "<tr><td><b>Паспорт:</b></td><td>%5</td></tr>"
            "<tr><td><b>Телефон:</b></td><td>%6</td></tr>"
            "<tr><td><b>Дата заезда:</b></td><td>%7</td></tr>"
            "<tr><td><b>Дата выезда:</b></td><td>%8</td></tr>"
            "<tr><td><b>Количество ночей:</b></td><td>%9</td></tr>"
            "<tr><td><b>Общая стоимость:</b></td><td>%10 руб.</td></tr>"
            "<tr><td><b>Оплачено:</b></td><td>%11 руб.</td></tr>"
            "<tr><td><b>Остаток:</b></td><td>%12 руб.</td></tr>"
            "<tr><td><b>Способ оплаты:</b></td><td>%13</td></tr>"
            "<tr><td><b>Дата создания:</b></td><td>%14</td></tr>"
            "</table>"
            "</body></html>")
            .arg(bookingId)
            .arg(roomNumber)
            .arg(bedNumber)
            .arg(clientName)
            .arg(passport)
            .arg(phone)
            .arg(checkIn.toString("dd.MM.yyyy"))
            .arg(checkOut.toString("dd.MM.yyyy"))
            .arg(nights)
            .arg(totalPrice, 0, 'f', 2)
            .arg(paidAmount, 0, 'f', 2)
            .arg(balance, 0, 'f', 2)
            .arg(paymentMethod)
            .arg(createdAt.toString("dd.MM.yyyy HH:mm"));

        // Предупреждение об оплаченных средствах
        if (paidAmount > 0) {
            details += QString("<p style='color: #dc3545; font-weight: bold;'>"
                              "Внимание! При отмене бронирования оплаченные средства (%1 руб.) "
                              "не будут возвращены автоматически. Возврат средств необходимо "
                              "оформить отдельно через бухгалтерию.</p>")
                              .arg(paidAmount, 0, 'f', 2);
        }

        QLabel *detailsLabel = findChild<QLabel*>("detailsLabel");
        if (detailsLabel) {
            detailsLabel->setText(details);
        }
    }
}

void CancelBookingDialog::validateForm()
{
    bool isValid = true;

    // Проверяем, что выбрано бронирование
    if (bookingCombo->currentIndex() < 0) {
        isValid = false;
    }

    // Проверяем, что указана причина отмены
    if (reasonEdit->toPlainText().trimmed().isEmpty()) {
        isValid = false;
    }

    cancelButton->setEnabled(isValid);
}

int CancelBookingDialog::selectedBookingId() const
{
    return bookingCombo->currentData().toInt();
}

QString CancelBookingDialog::cancellationReason() const
{
    return reasonEdit->toPlainText().trimmed();
}
