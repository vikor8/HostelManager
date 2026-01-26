#ifndef ADDCLIENTDIALOG_H
#define ADDCLIENTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QRegularExpressionValidator>
#include <QRegularExpression>

class AddClientDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode { Add, Edit };

    explicit AddClientDialog(QWidget *parent = nullptr, Mode mode = Add, int clientId = -1);

    // Геттеры для данных клиента
    QString firstName() const { return firstNameEdit->text().trimmed(); }
    QString lastName() const { return lastNameEdit->text().trimmed(); }
    QString middleName() const { return middleNameEdit->text().trimmed(); }
    QString passport() const { return passportEdit->text().trimmed(); }
    QString phone() const { return phoneEdit->text().trimmed(); }
    QDate birthDate() const { return birthDateEdit->date(); }
    QString country() const { return countryCombo->currentText(); }

    // Установка данных для редактирования
    void setClientData(const QString& firstName, const QString& lastName,
                      const QString& middleName, const QString& passport,
                      const QString& phone, const QDate& birthDate,
                      const QString& country);

private slots:
    void validateInputs();
    void onAccept();

private:
    QLineEdit *firstNameEdit;
    QLineEdit *lastNameEdit;
    QLineEdit *middleNameEdit;
    QLineEdit *passportEdit;
    QLineEdit *phoneEdit;
    QDateEdit *birthDateEdit;
    QComboBox *countryCombo;

    Mode dialogMode;
    int clientId;

    void setupUi();
    void loadCountries();
    bool validateForm();
};

#endif // ADDCLIENTDIALOG_H
