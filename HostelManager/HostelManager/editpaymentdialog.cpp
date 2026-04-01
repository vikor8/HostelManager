#include "editpaymentdialog.h"
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFrame>

EditPaymentDialog::EditPaymentDialog(QWidget *parent)
    : QDialog(parent)
    , m_totalPrice(0.0)
{
    setupUi();
    setWindowTitle("Редактирование оплаты");
    setMinimumWidth(500);
}

EditPaymentDialog::~EditPaymentDialog()
{
}

void EditPaymentDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Группа с информацией о бронировании
    QGroupBox *infoGroup = new QGroupBox("Информация о бронировании", this);
    QFormLayout *infoLayout = new QFormLayout(infoGroup);

    roomLabel = new QLabel(this);
    bedLabel = new QLabel(this);
    dateLabel = new QLabel(this);
    clientLabel = new QLabel(this);
    totalPriceLabel = new QLabel(this);
    daysCountLabel = new QLabel(this);
    pricePerDayLabel = new QLabel(this);

    daysCountLabel->setStyleSheet("font-weight: bold; color: #2E8B57;");
    pricePerDayLabel->setStyleSheet("font-weight: bold; color: #2E8B57;");

    infoLayout->addRow("Комната:", roomLabel);
    infoLayout->addRow("Койка:", bedLabel);
    infoLayout->addRow("Период:", dateLabel);
    infoLayout->addRow("Клиент:", clientLabel);
    infoLayout->addRow("Количество суток:", daysCountLabel);
    infoLayout->addRow("Стоимость за сутки:", pricePerDayLabel);
    infoLayout->addRow("Общая стоимость:", totalPriceLabel);

    mainLayout->addWidget(infoGroup);

    // Разделитель
    QFrame *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(separator);

    // Группа с редактированием оплаты
    QGroupBox *paymentGroup = new QGroupBox("Оплата", this);
    QFormLayout *paymentLayout = new QFormLayout(paymentGroup);

    paidSpin = new QDoubleSpinBox(this);
    paidSpin->setRange(0, 1000000);
    paidSpin->setSuffix(" руб.");
    paidSpin->setDecimals(2);
    paidSpin->setSingleStep(100);
    paidSpin->setButtonSymbols(QDoubleSpinBox::UpDownArrows);

    paymentCombo = new QComboBox(this);
    // Убираем "Другое" из списка способов оплаты
    paymentCombo->addItems({"Наличные", "Безнал", "Перевод", "На р/с юрлица", "Карта"});

    notesEdit = new QLineEdit(this);
    notesEdit->setPlaceholderText("Примечание к оплате...");

    balanceLabel = new QLabel(this);
    balanceLabel->setStyleSheet("font-weight: bold;");

    paymentLayout->addRow("Оплачено:", paidSpin);
    paymentLayout->addRow("Способ оплаты:", paymentCombo);
    paymentLayout->addRow("Примечание:", notesEdit);
    paymentLayout->addRow("Остаток:", balanceLabel);

    mainLayout->addWidget(paymentGroup);

    // Кнопки
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        Qt::Horizontal, this);

    mainLayout->addWidget(buttonBox);

    // Соединяем сигналы
    connect(paidSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &EditPaymentDialog::updateBalance);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updateBalance();
}

// Метод для обновления информации о количестве суток и стоимости за сутки
void EditPaymentDialog::updatePaymentInfo()
{
    if (m_checkInDate.isValid() && m_checkOutDate.isValid()) {
        int days = m_checkInDate.daysTo(m_checkOutDate);
        double pricePerDay = (days > 0) ? (m_totalPrice / days) : 0;

        daysCountLabel->setText(QString::number(days));
        pricePerDayLabel->setText(QString("%1 руб.").arg(pricePerDay, 0, 'f', 2));

        // Добавляем подсказку с деталями расчета
        QString tooltip = QString("Период: %1 - %2\nКоличество суток: %3\nСтоимость за сутки: %4 руб.\nОбщая стоимость: %5 руб.")
            .arg(m_checkInDate.toString("dd.MM.yyyy"))
            .arg(m_checkOutDate.toString("dd.MM.yyyy"))
            .arg(days)
            .arg(pricePerDay, 0, 'f', 2)
            .arg(m_totalPrice, 0, 'f', 2);

        daysCountLabel->setToolTip(tooltip);
        pricePerDayLabel->setToolTip(tooltip);
    } else {
        daysCountLabel->setText("0");
        pricePerDayLabel->setText("0.00 руб.");
    }
}

// Существующий метод для обратной совместимости
void EditPaymentDialog::setBookingInfo(const QString &roomNumber, int bedNumber,
                                      const QDate &date, const QString &clientName,
                                      double totalPrice, double paidAmount,
                                      const QString &paymentMethod)
{
    roomLabel->setText(roomNumber);
    bedLabel->setText(QString::number(bedNumber));
    dateLabel->setText(date.toString("dd.MM.yyyy"));
    clientLabel->setText(clientName);
    totalPriceLabel->setText(QString("%1 руб.").arg(totalPrice, 0, 'f', 2));

    paidSpin->setValue(paidAmount);
    m_totalPrice = totalPrice;

    // Устанавливаем способ оплаты
    int index = paymentCombo->findText(paymentMethod);
    if (index >= 0) {
        paymentCombo->setCurrentIndex(index);
    } else {
        paymentCombo->setCurrentText(paymentMethod);
    }

    // Для обратной совместимости устанавливаем одинаковые даты
    m_checkInDate = date;
    m_checkOutDate = date;
    updatePaymentInfo();
    updateBalance();
}

// Новый метод с полной информацией
void EditPaymentDialog::setBookingInfo(const QString &roomNumber, int bedNumber,
                                      const QDate &checkInDate, const QDate &checkOutDate,
                                      const QString &clientName, double totalPrice,
                                      double paidAmount, const QString &paymentMethod)
{
    roomLabel->setText(roomNumber);
    bedLabel->setText(QString::number(bedNumber));
    dateLabel->setText(QString("%1 - %2")
        .arg(checkInDate.toString("dd.MM.yyyy"))
        .arg(checkOutDate.toString("dd.MM.yyyy")));
    clientLabel->setText(clientName);
    totalPriceLabel->setText(QString("%1 руб.").arg(totalPrice, 0, 'f', 2));

    paidSpin->setValue(paidAmount);
    m_totalPrice = totalPrice;
    m_checkInDate = checkInDate;
    m_checkOutDate = checkOutDate;

    // Устанавливаем способ оплаты
    int index = paymentCombo->findText(paymentMethod);
    if (index >= 0) {
        paymentCombo->setCurrentIndex(index);
    } else {
        paymentCombo->setCurrentText(paymentMethod);
    }

    updatePaymentInfo();
    updateBalance();
}

void EditPaymentDialog::updateBalance()
{
    double paid = paidSpin->value();
    double balance = m_totalPrice - paid;

    if (balance > 0) {
        balanceLabel->setText(QString("%1 руб. к оплате").arg(balance, 0, 'f', 2));
        balanceLabel->setStyleSheet("font-weight: bold; color: #FF0000;");
    } else if (balance < 0) {
        balanceLabel->setText(QString("Переплата %1 руб.").arg(-balance, 0, 'f', 2));
        balanceLabel->setStyleSheet("font-weight: bold; color: #0000FF;");
    } else {
        balanceLabel->setText("Оплачено полностью");
        balanceLabel->setStyleSheet("font-weight: bold; color: #008000;");
    }
}

double EditPaymentDialog::paidAmount() const
{
    return paidSpin->value();
}

QString EditPaymentDialog::paymentMethod() const
{
    return paymentCombo->currentText();
}

QString EditPaymentDialog::notes() const
{
    return notesEdit->text().trimmed();
}
