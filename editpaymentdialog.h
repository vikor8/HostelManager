#ifndef EDITPAYMENTDIALOG_H
#define EDITPAYMENTDIALOG_H

#include <QDialog>
#include <QDate>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QComboBox;
QT_END_NAMESPACE

class EditPaymentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditPaymentDialog(QWidget *parent = nullptr);
    ~EditPaymentDialog();

    void setBookingInfo(const QString &roomNumber, int bedNumber,
                       const QDate &date, const QString &clientName,
                       double totalPrice, double paidAmount,
                       const QString &paymentMethod);

    // Добавляем перегруженный метод для передачи дополнительной информации
    void setBookingInfo(const QString &roomNumber, int bedNumber,
                       const QDate &checkInDate, const QDate &checkOutDate,
                       const QString &clientName, double totalPrice,
                       double paidAmount, const QString &paymentMethod);

    double paidAmount() const;
    QString paymentMethod() const;
    QString notes() const;

private slots:
    void updateBalance();

private:
    void setupUi();

    QLabel *roomLabel;
    QLabel *bedLabel;
    QLabel *dateLabel;
    QLabel *clientLabel;
    QLabel *totalPriceLabel;
    QDoubleSpinBox *paidSpin;
    QComboBox *paymentCombo;
    QLineEdit *notesEdit;
    QLabel *balanceLabel;

    // Новые элементы для отображения количества суток и стоимости за сутки
    QLabel *daysCountLabel;
    QLabel *pricePerDayLabel;

    double m_totalPrice;
    QDate m_checkInDate;
    QDate m_checkOutDate;

    void updatePaymentInfo(); // Новый метод для обновления информации о платеже
};

#endif // EDITPAYMENTDIALOG_H
