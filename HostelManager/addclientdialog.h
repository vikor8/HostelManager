#ifndef ADDCLIENTDIALOG_H
#define ADDCLIENTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QFormLayout>
#include <QDialogButtonBox>

class AddClientDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddClientDialog(QWidget *parent = nullptr);

    QString firstName() const { return firstNameEdit->text(); }
    QString lastName() const { return lastNameEdit->text(); }
    QString middleName() const { return middleNameEdit->text(); }
    QString passport() const { return passportEdit->text(); }
    QString phone() const { return phoneEdit->text(); }
    QString email() const { return emailEdit->text(); }
    QString notes() const { return notesEdit->toPlainText(); }

private:
    QLineEdit *firstNameEdit;
    QLineEdit *lastNameEdit;
    QLineEdit *middleNameEdit;
    QLineEdit *passportEdit;
    QLineEdit *phoneEdit;
    QLineEdit *emailEdit;
    QTextEdit *notesEdit;
};

#endif // ADDCLIENTDIALOG_H
