#include "hostelmanager.h"
#include "ui_hostelmanager.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDate>
#include <QHeaderView>
#include <QBrush>
#include <QColor>
#include <QRandomGenerator>

HostelManager::HostelManager(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::HostelManager)
{
    ui->setupUi(this);

    // Устанавливаем текущую дату
    ui->dateEdit->setDate(QDate::currentDate());

    // Подключаем кнопки
    connect(ui->btnToday, &QPushButton::clicked, this, &HostelManager::on_btnToday_clicked);
    connect(ui->btnRefresh, &QPushButton::clicked, this, &HostelManager::on_btnRefresh_clicked);

    // Инициализируем таблицу
    initializeTable();
}

HostelManager::~HostelManager()
{
    delete ui;
}

void HostelManager::on_btnToday_clicked()
{
    ui->dateEdit->setDate(QDate::currentDate());
    updateTableColors();
}

void HostelManager::on_btnRefresh_clicked()
{
    updateTableColors();
}

void HostelManager::initializeTable()
{
    // Настраиваем таблицу
    int totalColumns = 3 + 31; // 3 основных столбца + 31 день
    ui->tableWidget->setColumnCount(totalColumns);

    // Устанавливаем заголовки столбцов
    QStringList headers;
    headers << "Номер комнаты" << "Номер койки" << "Категория";

    // Добавляем заголовки для дней (с 1 по 31)
    for (int day = 1; day <= 31; ++day) {
        headers << QString::number(day);
    }

    ui->tableWidget->setHorizontalHeaderLabels(headers);

    // Добавляем тестовые данные (10 строк для примера)
    int rowCount = 10;
    ui->tableWidget->setRowCount(rowCount);

    // Заполняем таблицу тестовыми данными
    for (int row = 0; row < rowCount; ++row) {
        // Номер комнаты (разные комнаты для разных строк)
        QString roomNumber;
        if (row < 3) roomNumber = "101";
        else if (row < 6) roomNumber = "102";
        else if (row < 8) roomNumber = "103";
        else roomNumber = "104";

        QTableWidgetItem *roomItem = new QTableWidgetItem(roomNumber);
        roomItem->setTextAlignment(Qt::AlignCenter);
        roomItem->setFlags(roomItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(row, 0, roomItem);

        // Номер койки
        int bedNumber = (row % 3) + 1; // 1-3 койки в каждой комнате
        QTableWidgetItem *bedItem = new QTableWidgetItem(QString::number(bedNumber));
        bedItem->setTextAlignment(Qt::AlignCenter);
        bedItem->setFlags(bedItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(row, 1, bedItem);

        // Категория
        QString category;
        if (roomNumber == "101" || roomNumber == "102") category = "Эконом";
        else if (roomNumber == "103") category = "Стандарт";
        else category = "Комфорт";

        QTableWidgetItem *categoryItem = new QTableWidgetItem(category);
        categoryItem->setTextAlignment(Qt::AlignCenter);
        categoryItem->setFlags(categoryItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(row, 2, categoryItem);

        // Заполняем столбцы дней (столбцы 3-33)
        for (int col = 3; col < totalColumns; ++col) {
            QTableWidgetItem *dayItem = new QTableWidgetItem("");
            dayItem->setTextAlignment(Qt::AlignCenter);
            dayItem->setFlags(dayItem->flags() & ~Qt::ItemIsEditable);
            ui->tableWidget->setItem(row, col, dayItem);
        }
    }

    // Настраиваем ширину столбцов
    ui->tableWidget->setColumnWidth(0, 120);  // Номер комнаты
    ui->tableWidget->setColumnWidth(1, 100);  // Номер койки
    ui->tableWidget->setColumnWidth(2, 100);  // Категория

    // Устанавливаем одинаковую ширину для столбцов дней
    for (int col = 3; col < totalColumns; ++col) {
        ui->tableWidget->setColumnWidth(col, 30);
    }

    // Позволяем горизонтальную прокрутку
    ui->tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Устанавливаем высоту строк
    for (int row = 0; row < rowCount; ++row) {
        ui->tableWidget->setRowHeight(row, 25);
    }

    // Настраиваем заголовки столбцов
    QHeaderView *header = ui->tableWidget->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Fixed);

    // Для столбцов дней можно разрешить фиксированный размер
    for (int col = 3; col < totalColumns; ++col) {
        header->setSectionResizeMode(col, QHeaderView::Fixed);
    }

    // Делаем заголовки дней жирными
    QFont headerFont = header->font();
    headerFont.setBold(true);
    header->setFont(headerFont);

    // Устанавливаем стиль для четных и нечетных строк
    ui->tableWidget->setAlternatingRowColors(true);

    // Обновляем цвета
    updateTableColors();
}

void HostelManager::updateTableColors()
{
    int totalColumns = ui->tableWidget->columnCount();
    int rowCount = ui->tableWidget->rowCount();

    // Цвета
    QColor lightGrayColor(240, 240, 240); // Светло-серый цвет
    QColor occupiedColor(200, 200, 0);    // Желтый для занятых

    // Инициализируем генератор случайных чисел
    QRandomGenerator *generator = QRandomGenerator::global();

    for (int row = 0; row < rowCount; ++row) {
        // Устанавливаем разные цвета фона для информационных колонок
        QColor infoColor = (row % 2 == 0) ? QColor(255, 255, 255) : QColor(245, 245, 245);

        for (int col = 0; col < 3; ++col) {
            QTableWidgetItem *item = ui->tableWidget->item(row, col);
            if (item) {
                item->setBackground(infoColor);
            }
        }

        // Обрабатываем столбцы дней
        for (int col = 3; col < totalColumns; ++col) {
            QTableWidgetItem *item = ui->tableWidget->item(row, col);
            if (item) {
                item->setBackground(lightGrayColor);

                // Для примера: случайным образом отмечаем некоторые ячейки как занятые
                // Используем более предсказуемый алгоритм для лучшей демонстрации
                int randomValue = generator->bounded(100); // 0-99

                // Делаем занятые дни более сгруппированными
                bool isOccupied = false;

                // Проверяем, занята ли предыдущая ячейка (для создания непрерывных блоков)
                if (col > 3) {
                    QTableWidgetItem *prevItem = ui->tableWidget->item(row, col - 1);
                    if (prevItem && prevItem->background().color() == occupiedColor) {
                        // Продолжаем блок занятости с вероятностью 70%
                        isOccupied = (randomValue < 70);
                    } else {
                        // Начинаем новый блок с вероятностью 20%
                        isOccupied = (randomValue < 20);
                    }
                } else {
                    // Для первой ячейки дня
                    isOccupied = (randomValue < 20);
                }

                if (isOccupied) {
                    item->setBackground(occupiedColor); // Желтый для занятых
                    item->setText("●"); // Добавляем маркер занятости
                    item->setForeground(Qt::black); // Черный текст для контраста
                } else {
                    item->setBackground(lightGrayColor);
                    item->setText(""); // Очищаем текст для свободных
                }

                item->setTextAlignment(Qt::AlignCenter);
            }
        }
    }

    // Обновляем статус
    ui->lblStatus->setText("База данных: подключена. Таблица обновлена: " +
                          QDate::currentDate().toString("dd.MM.yyyy") +
                          " (занято: жёлтый, свободно: серый)");
}
