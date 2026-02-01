#ifndef CANCELBOOKINGDIALOG_H
#define CANCELBOOKINGDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
class QTextEdit;
class QPushButton;
QT_END_NAMESPACE

class Database;

class CancelBookingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CancelBookingDialog(Database *db, QWidget *parent = nullptr);
    ~CancelBookingDialog();

    int selectedBookingId() const;
    QString cancellationReason() const;

private slots:
    void loadActiveBookings();
    void updateBookingDetails();
    void validateForm();

private:
    void setupUi();

    Database *database;

    QComboBox *bookingCombo;
    QTextEdit *reasonEdit;
    QPushButton *cancelButton;

    QMap<int, QString> bookingMap; // bookingId -> display string
};

#endif // CANCELBOOKINGDIALOG_H
