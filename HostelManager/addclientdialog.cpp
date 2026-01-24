#include "addclientdialog.h"

AddClientDialog::AddClientDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Добавить клиента");
    setMinimumWidth(400);

    QFormLayout *formLayout = new QFormLayout(this);

    firstNameEdit = new QLineEdit(this);
    lastNameEdit = new QLineEdit(this);
    middleNameEdit = new QLineEdit(this);
    passportEdit = new QLineEdit(this);
    phoneEdit = new QLineEdit(this);
    emailEdit = new QLineEdit(this);
    notesEdit = new QTextEdit(this);
    notesEdit->setMaximumHeight(100);

    formLayout->addRow("Имя*:", firstNameEdit);
    formLayout->addRow("Фамилия*:", lastNameEdit);
    formLayout->addRow("Отчество:", middleNameEdit);
    formLayout->addRow("Паспорт:", passportEdit);
    formLayout->addRow("Телефон:", phoneEdit);
    formLayout->addRow("Email:", emailEdit);
    formLayout->addRow("Примечания:", notesEdit);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, this);

    formLayout->addRow(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
