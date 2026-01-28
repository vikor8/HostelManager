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

EditPaymentDialog::EditPaymentDialog(QWidget *parent)
    : QDialog(parent)
    , m_totalPrice(0.0)
{
    setupUi();
    setWindowTitle("Редактирование оплаты");
    setMinimumWidth(400);
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

    infoLayout->addRow("Комната:", roomLabel);
    infoLayout->addRow("Койка:", bedLabel);
    infoLayout->addRow("Дата:", dateLabel);
    infoLayout->addRow("Клиент:", clientLabel);
    infoLayout->addRow("Общая стоимость:", totalPriceLabel);

    mainLayout->addWidget(infoGroup);

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
    paymentCombo->addItems({"Наличные", "Безнал", "Перевод", "Карта", "Другое"});

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

    // Обновляем баланс при инициализации
    updateBalance();
}

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
