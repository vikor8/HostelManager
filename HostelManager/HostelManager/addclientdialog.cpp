#include "addclientdialog.h"
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>

AddClientDialog::AddClientDialog(QWidget *parent, Mode mode, int clientId)
    : QDialog(parent)
    , dialogMode(mode)
    , clientId(clientId)
{
    setupUi();
    loadCountries();

    if (mode == Add) {
        setWindowTitle("Добавить клиента");
    } else {
        setWindowTitle("Редактировать клиента");
    }

    // Настройка валидатора для телефона
    QRegularExpression phoneRegex("^[+0-9\\s()-]*$");
    QRegularExpressionValidator *phoneValidator = new QRegularExpressionValidator(phoneRegex, this);
    phoneEdit->setValidator(phoneValidator);

    // Подключение сигналов
    connect(lastNameEdit, &QLineEdit::textChanged, this, &AddClientDialog::validateInputs);
    connect(firstNameEdit, &QLineEdit::textChanged, this, &AddClientDialog::validateInputs);
    connect(passportEdit, &QLineEdit::textChanged, this, &AddClientDialog::validateInputs);
    connect(phoneEdit, &QLineEdit::textChanged, this, &AddClientDialog::validateInputs);
    connect(birthDateEdit, &QDateEdit::dateChanged, this, &AddClientDialog::validateInputs);
}

void AddClientDialog::setupUi()
{
    setMinimumWidth(500);

    QFormLayout *formLayout = new QFormLayout(this);

    // Создание элементов формы

    lastNameEdit = new QLineEdit(this);
    lastNameEdit->setPlaceholderText("Введите фамилию");

    firstNameEdit = new QLineEdit(this);
    firstNameEdit->setPlaceholderText("Введите имя");

    middleNameEdit = new QLineEdit(this);
    middleNameEdit->setPlaceholderText("Введите отчество");

    passportEdit = new QLineEdit(this);
    passportEdit->setPlaceholderText("Например: 1234 567890");

    phoneEdit = new QLineEdit(this);
    phoneEdit->setPlaceholderText("Например: +7 999 123-45-67");

    birthDateEdit = new QDateEdit(this);
    birthDateEdit->setDate(QDate::currentDate().addYears(-18));
    birthDateEdit->setCalendarPopup(true);
    birthDateEdit->setDisplayFormat("dd.MM.yyyy");
    birthDateEdit->setMinimumDate(QDate(1900, 1, 1));
    birthDateEdit->setMaximumDate(QDate::currentDate());

    countryCombo = new QComboBox(this);
    countryCombo->setEditable(true);
    countryCombo->setPlaceholderText("Выберите страну");

    // Добавление элементов в форму
    formLayout->addRow("*Фамилия:", lastNameEdit);
    formLayout->addRow("*Имя:", firstNameEdit);
    formLayout->addRow("Отчество:", middleNameEdit);
    formLayout->addRow("*Паспорт:", passportEdit);
    formLayout->addRow("*Телефон:", phoneEdit);
    formLayout->addRow("*Дата рождения:", birthDateEdit);
    formLayout->addRow("Страна:", countryCombo);

    // Добавление подсказки об обязательных полях
    QLabel *requiredLabel = new QLabel("* - обязательные поля", this);
    requiredLabel->setStyleSheet("color: gray; font-style: italic;");
    formLayout->addRow("", requiredLabel);

    // Кнопки
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, this);

    formLayout->addRow(buttonBox);

    // Соединение сигналов
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddClientDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Изначально кнопка OK неактивна
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void AddClientDialog::loadCountries()
{
    // Список популярных стран
    QStringList countries = {
        "Россия", "Беларусь", "Казахстан", "Украина", "Узбекистан",
        "Таджикистан", "Кыргызстан", "Туркменистан", "Армения", "Азербайджан",
        "Грузия", "Молдова", "Латвия", "Литва", "Эстония",
        "Германия", "Франция", "Италия", "Испания", "Великобритания",
        "США", "Китай", "Япония", "Южная Корея", "Турция"
    };

    countryCombo->addItem(""); // Пустой элемент
    countryCombo->addItems(countries);
    countryCombo->setCurrentIndex(0);
}

void AddClientDialog::validateInputs()
{
    bool isValid = validateForm();

    // Получаем кнопку OK
    QDialogButtonBox *buttonBox = findChild<QDialogButtonBox*>();
    if (buttonBox) {
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid);
    }
}

bool AddClientDialog::validateForm()
{
    // Проверка обязательных полей
    if (firstName().isEmpty()) return false;
    if (lastName().isEmpty()) return false;
    if (passport().isEmpty()) return false;
    if (phone().isEmpty()) return false;

    // Проверка даты рождения
    if (!birthDate().isValid()) return false;

    // Проверка телефона (только цифры и +)
    QString phoneText = phone();
    for (int i = 0; i < phoneText.length(); ++i) {
        QChar ch = phoneText.at(i);
        if (!ch.isDigit() && ch != '+' && ch != ' ' && ch != '-' && ch != '(' && ch != ')') {
            return false;
        }
    }

    return true;
}

void AddClientDialog::onAccept()
{
    if (!validateForm()) {
        QMessageBox::warning(this, "Ошибка",
            "Пожалуйста, заполните все обязательные поля корректно.\n"
            "Телефон должен содержать только цифры и знак '+'");
        return;
    }

    accept();
}

void AddClientDialog::setClientData(const QString& firstName, const QString& lastName,
                                   const QString& middleName, const QString& passport,
                                   const QString& phone, const QDate& birthDate,
                                   const QString& country)
{
    firstNameEdit->setText(firstName);
    lastNameEdit->setText(lastName);
    middleNameEdit->setText(middleName);
    passportEdit->setText(passport);
    phoneEdit->setText(phone);
    birthDateEdit->setDate(birthDate);

    int index = countryCombo->findText(country);
    if (index >= 0) {
        countryCombo->setCurrentIndex(index);
    } else {
        countryCombo->setCurrentText(country);
    }

    validateInputs();
}
