#include "reportwindow.h"
#include "database.h"
#include <QTextEdit>
#include <QDateEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTable>
#include <QTextTableFormat>
#include <QTextCharFormat>
#include <QTextBlockFormat>
#include <QTextStream>
#include <QDateTime>
#include <QFile>
#include <QSqlQuery>
#include <QColor>

// Для печати в Qt 6
#include <QPrinter>
#include <QPrintDialog>
#include <QPageSize>
#include <QPageLayout>

ReportWindow::ReportWindow(Database *db, QWidget *parent)
    : QMainWindow(parent)
    , database(db)
{
    setupUi();
    createMenuBar();
    setWindowTitle("Отчеты");
    setMinimumSize(900, 700);

    // Устанавливаем даты по умолчанию (текущий месяц)
    QDate today = QDate::currentDate();
    QDate startOfMonth = QDate(today.year(), today.month(), 1);
    QDate endOfMonth = QDate(today.year(), today.month(), today.daysInMonth());

    startDateEdit->setDate(startOfMonth);
    endDateEdit->setDate(endOfMonth);
}

ReportWindow::~ReportWindow()
{
}

void ReportWindow::setupUi()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Группа для выбора периода
    QGroupBox *periodGroup = new QGroupBox("Период отчета", this);
    QHBoxLayout *periodLayout = new QHBoxLayout(periodGroup);

    periodLayout->addWidget(new QLabel("С:", this));
    startDateEdit = new QDateEdit(this);
    startDateEdit->setCalendarPopup(true);
    startDateEdit->setDisplayFormat("dd.MM.yyyy");
    startDateEdit->setDate(QDate::currentDate().addMonths(-1));
    periodLayout->addWidget(startDateEdit);

    periodLayout->addWidget(new QLabel("По:", this));
    endDateEdit = new QDateEdit(this);
    endDateEdit->setCalendarPopup(true);
    endDateEdit->setDisplayFormat("dd.MM.yyyy");
    endDateEdit->setDate(QDate::currentDate());
    periodLayout->addWidget(endDateEdit);

    generateButton = new QPushButton("Сформировать отчет", this);
    periodLayout->addWidget(generateButton);

    periodLayout->addStretch();

    mainLayout->addWidget(periodGroup);

    // Группа для отчета
    QGroupBox *reportGroup = new QGroupBox("Отчет", this);
    QVBoxLayout *reportLayout = new QVBoxLayout(reportGroup);

    reportTextEdit = new QTextEdit(this);
    reportTextEdit->setReadOnly(true);
    reportLayout->addWidget(reportTextEdit);

    // Таблица для детализации
    reportTable = new QTableWidget(this);
    reportTable->setAlternatingRowColors(true);
    reportTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    reportTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    reportLayout->addWidget(reportTable);

    mainLayout->addWidget(reportGroup, 1); // 1 - растягиваемый

    // Группа для кнопок
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    printButton = new QPushButton("Печать отчета", this);
    exportButton = new QPushButton("Экспорт в Excel", this);
    QPushButton *closeButton = new QPushButton("Закрыть", this);

    buttonLayout->addWidget(printButton);
    buttonLayout->addWidget(exportButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    // Соединяем сигналы
    connect(generateButton, &QPushButton::clicked, this, &ReportWindow::generateSummaryReport);
    connect(printButton, &QPushButton::clicked, this, &ReportWindow::printReport);
    connect(exportButton, &QPushButton::clicked, this, &ReportWindow::exportToExcel);
    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
}

void ReportWindow::createMenuBar()
{
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu *reportMenu = menuBar->addMenu("&Отчет");

    QAction *summaryAction = reportMenu->addAction("&Итоги");
    summaryAction->setShortcut(Qt::Key_F7);
    connect(summaryAction, &QAction::triggered, this, &ReportWindow::generateSummaryReport);

    QAction *printAction = reportMenu->addAction("&Печать");
    printAction->setShortcut(QKeySequence::Print);
    connect(printAction, &QAction::triggered, this, &ReportWindow::printReport);

    reportMenu->addSeparator();

    QAction *closeAction = reportMenu->addAction("&Закрыть");
    closeAction->setShortcut(QKeySequence::Close);
    connect(closeAction, &QAction::triggered, this, &QWidget::close);
}

void ReportWindow::generateSummaryReport()
{
    if (!database || !database->isDatabaseConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }

    QDate startDate = startDateEdit->date();
    QDate endDate = endDateEdit->date();

    if (startDate > endDate) {
        QMessageBox::warning(this, "Ошибка", "Дата начала не может быть позже даты окончания");
        return;
    }

    // Получаем данные из базы
    double totalRevenue = 0;
    double paidAmount = 0;
    double unpaidAmount = 0;
    int totalBookings = 0;

    QSqlQuery query(database->getDatabase());

    // 1. Общая статистика по бронированиям за период
    query.prepare("SELECT "
                  "COUNT(*) as bookings_count, "
                  "SUM(total_price) as total_revenue, "
                  "SUM(paid_amount) as total_paid "
                  "FROM bookings "
                  "WHERE status = 'active' "
                  "AND check_in_date <= ? "
                  "AND check_out_date >= ?");
    query.addBindValue(endDate.toString("yyyy-MM-dd"));
    query.addBindValue(startDate.toString("yyyy-MM-dd"));

    if (query.exec() && query.next()) {
        totalBookings = query.value(0).toInt();
        totalRevenue = query.value(1).toDouble();
        paidAmount = query.value(2).toDouble();
        unpaidAmount = totalRevenue - paidAmount;
    }

    // 2. Статистика по способам оплаты
    query.prepare("SELECT "
                  "payment_method, "
                  "SUM(paid_amount) as paid_by_method "
                  "FROM bookings "
                  "WHERE status = 'active' "
                  "AND check_in_date <= ? "
                  "AND check_out_date >= ? "
                  "AND paid_amount > 0 "
                  "GROUP BY payment_method "
                  "ORDER BY paid_by_method DESC");
    query.addBindValue(endDate.toString("yyyy-MM-dd"));
    query.addBindValue(startDate.toString("yyyy-MM-dd"));

    QMap<QString, double> paymentsByMethod;
    if (query.exec()) {
        while (query.next()) {
            QString method = query.value(0).toString();
            double amount = query.value(1).toDouble();
            paymentsByMethod[method] = amount;
        }
    }

    // 3. Занятость мест на последнюю дату периода
    query.prepare("SELECT "
                  "COUNT(DISTINCT bed_id) as occupied_beds "
                  "FROM bookings "
                  "WHERE status = 'active' "
                  "AND ? BETWEEN check_in_date AND check_out_date");
    query.addBindValue(endDate.toString("yyyy-MM-dd"));

    int occupiedBeds = 0;
    int totalBeds = 0;
    if (query.exec() && query.next()) {
        occupiedBeds = query.value(0).toInt();
    }

    // 4. Общее количество коек
    query.exec("SELECT COUNT(*) FROM beds WHERE is_active = 1");
    if (query.next()) {
        totalBeds = query.value(0).toInt();
    }

    // 5. Статистика по комнатам
    query.prepare("SELECT "
                  "r.category, "
                  "COUNT(DISTINCT bk.id) as bookings_count, "
                  "SUM(bk.total_price) as category_revenue, "
                  "SUM(bk.paid_amount) as category_paid "
                  "FROM bookings bk "
                  "JOIN beds b ON bk.bed_id = b.id "
                  "JOIN rooms r ON b.room_id = r.id "
                  "WHERE bk.status = 'active' "
                  "AND bk.check_in_date <= ? "
                  "AND bk.check_out_date >= ? "
                  "GROUP BY r.category "
                  "ORDER BY category_revenue DESC");
    query.addBindValue(endDate.toString("yyyy-MM-dd"));
    query.addBindValue(startDate.toString("yyyy-MM-dd"));

    QList<QString> categories;
    QList<int> catBookings;
    QList<double> catRevenues;
    QList<double> catPaids;

    if (query.exec()) {
        while (query.next()) {
            categories.append(query.value(0).toString());
            catBookings.append(query.value(1).toInt());
            catRevenues.append(query.value(2).toDouble());
            catPaids.append(query.value(3).toDouble());
        }
    }

    // 6. Детализированные данные
    query.prepare("SELECT "
                  "bk.check_in_date, "
                  "r.room_number, "
                  "b.bed_number, "
                  "c.last_name || ' ' || c.first_name as client_name, "
                  "bk.total_price, "
                  "bk.paid_amount, "
                  "bk.payment_method "
                  "FROM bookings bk "
                  "JOIN beds b ON bk.bed_id = b.id "
                  "JOIN rooms r ON b.room_id = r.id "
                  "JOIN clients c ON bk.client_id = c.id "
                  "WHERE bk.status = 'active' "
                  "AND bk.check_in_date <= ? "
                  "AND bk.check_out_date >= ? "
                  "ORDER BY bk.check_in_date, r.room_number, b.bed_number");
    query.addBindValue(endDate.toString("yyyy-MM-dd"));
    query.addBindValue(startDate.toString("yyyy-MM-dd"));

    QList<QVariantMap> detailedData;
    if (query.exec()) {
        while (query.next()) {
            QVariantMap item;
            item["check_in_date"] = query.value(0);
            item["room_number"] = query.value(1);
            item["bed_number"] = query.value(2);
            item["client_name"] = query.value(3);
            item["total_price"] = query.value(4);
            item["paid_amount"] = query.value(5);
            item["payment_method"] = query.value(6);
            detailedData.append(item);
        }
    }

    // Формируем текстовый отчет
    QString report;
    report += QString("<html><body>"
                     "<h2 align='center'>СВОДНЫЙ ОТЧЕТ</h2>"
                     "<h3 align='center'>за период с %1 по %2</h3>"
                     "<hr>")
                     .arg(startDate.toString("dd.MM.yyyy"))
                     .arg(endDate.toString("dd.MM.yyyy"));

    // Общая статистика
    report += "<h3>1. Общие показатели:</h3>";
    report += QString("<table border='1' cellpadding='5' style='border-collapse: collapse; width: 100%;'>"
                     "<tr><td width='70%'><b>Показатель</b></td><td width='30%' align='right'><b>Значение</b></td></tr>"
                     "<tr><td>Количество бронирований</td><td align='right'>%1</td></tr>"
                     "<tr><td>Общая стоимость бронирований</td><td align='right'>%2 руб.</td></tr>"
                     "<tr><td>Полученная оплата</td><td align='right'>%3 руб.</td></tr>"
                     "<tr><td>Задолженность</td><td align='right'>%4 руб.</td></tr>"
                     "<tr><td>Процент оплаты</td><td align='right'>%5%</td></tr>"
                     "</table><br>")
                     .arg(totalBookings)
                     .arg(totalRevenue, 0, 'f', 2)
                     .arg(paidAmount, 0, 'f', 2)
                     .arg(unpaidAmount, 0, 'f', 2)
                     .arg(totalRevenue > 0 ? QString::number((paidAmount / totalRevenue) * 100, 'f', 1) : "0");

    // Статистика занятости
    double occupancyRate = totalBeds > 0 ? (occupiedBeds * 100.0 / totalBeds) : 0;
    report += "<h3>2. Занятость мест (на " + endDate.toString("dd.MM.yyyy") + "):</h3>";
    report += QString("<table border='1' cellpadding='5' style='border-collapse: collapse; width: 100%;'>"
                     "<tr><td width='70%'><b>Показатель</b></td><td width='30%' align='right'><b>Значение</b></td></tr>"
                     "<tr><td>Всего мест</td><td align='right'>%1</td></tr>"
                     "<tr><td>Занято мест</td><td align='right'>%2</td></tr>"
                     "<tr><td>Свободно мест</td><td align='right'>%3</td></tr>"
                     "<tr><td>Процент загрузки</td><td align='right'>%4%</td></tr>"
                     "</table><br>")
                     .arg(totalBeds)
                     .arg(occupiedBeds)
                     .arg(totalBeds - occupiedBeds)
                     .arg(occupancyRate, 0, 'f', 1);

    // Статистика по способам оплаты
    if (!paymentsByMethod.isEmpty()) {
        report += "<h3>3. Распределение оплаты по способам:</h3>";
        report += "<table border='1' cellpadding='5' style='border-collapse: collapse; width: 100%;'>"
                 "<tr><td width='60%'><b>Способ оплаты</b></td><td width='20%' align='right'><b>Сумма</b></td><td width='20%' align='right'><b>Доля</b></td></tr>";

        double totalPaid = 0;
        for (auto it = paymentsByMethod.begin(); it != paymentsByMethod.end(); ++it) {
            totalPaid += it.value();
        }

        for (auto it = paymentsByMethod.begin(); it != paymentsByMethod.end(); ++it) {
            double percentage = totalPaid > 0 ? (it.value() * 100 / totalPaid) : 0;
            report += QString("<tr>"
                             "<td>%1</td>"
                             "<td align='right'>%2 руб.</td>"
                             "<td align='right'>%3%</td>"
                             "</tr>")
                             .arg(it.key())
                             .arg(it.value(), 0, 'f', 2)
                             .arg(percentage, 0, 'f', 1);
        }
        report += "</table><br>";
    }

    // Статистика по категориям комнат
    if (!categories.isEmpty()) {
        report += "<h3>4. Статистика по категориям номеров:</h3>";
        report += "<table border='1' cellpadding='5' style='border-collapse: collapse; width: 100%;'>"
                 "<tr><td><b>Категория</b></td><td align='right'><b>Броней</b></td><td align='right'><b>Выручка</b></td><td align='right'><b>Оплачено</b></td><td align='right'><b>% оплаты</b></td></tr>";

        for (int i = 0; i < categories.size(); ++i) {
            double catPaymentRate = catRevenues[i] > 0 ? (catPaids[i] * 100 / catRevenues[i]) : 0;
            report += QString("<tr>"
                             "<td>%1</td>"
                             "<td align='right'>%2</td>"
                             "<td align='right'>%3 руб.</td>"
                             "<td align='right'>%4 руб.</td>"
                             "<td align='right'>%5%</td>"
                             "</tr>")
                             .arg(categories[i])
                             .arg(catBookings[i])
                             .arg(catRevenues[i], 0, 'f', 2)
                             .arg(catPaids[i], 0, 'f', 2)
                             .arg(catPaymentRate, 0, 'f', 1);
        }
        report += "</table><br>";
    }

    // Заполняем таблицу детализированными данными
    reportTable->clear();
    reportTable->setRowCount(0);
    reportTable->setColumnCount(8);
    reportTable->setHorizontalHeaderLabels({
        "Дата", "Комната", "Койка", "Клиент",
        "Общая сумма", "Оплачено", "Остаток", "Способ оплаты"
    });

    for (int i = 0; i < detailedData.size(); ++i) {
        const QVariantMap &item = detailedData[i];
        reportTable->insertRow(i);

        QDate checkIn = QDate::fromString(item["check_in_date"].toString(), "yyyy-MM-dd");
        double totalPrice = item["total_price"].toDouble();
        double paid = item["paid_amount"].toDouble();
        double balance = totalPrice - paid;

        reportTable->setItem(i, 0, new QTableWidgetItem(checkIn.toString("dd.MM.yyyy")));
        reportTable->setItem(i, 1, new QTableWidgetItem(item["room_number"].toString()));
        reportTable->setItem(i, 2, new QTableWidgetItem(item["bed_number"].toString()));
        reportTable->setItem(i, 3, new QTableWidgetItem(item["client_name"].toString()));
        reportTable->setItem(i, 4, new QTableWidgetItem(QString::number(totalPrice, 'f', 2) + " руб."));
        reportTable->setItem(i, 5, new QTableWidgetItem(QString::number(paid, 'f', 2) + " руб."));
        reportTable->setItem(i, 6, new QTableWidgetItem(QString::number(balance, 'f', 2) + " руб."));
        reportTable->setItem(i, 7, new QTableWidgetItem(item["payment_method"].toString()));

        // Раскрашиваем строку в зависимости от оплаты
        QColor rowColor;
        if (balance <= 0) {
            // Полностью оплачено - зеленый
            rowColor = QColor(200, 255, 200);
        } else if (paid > 0) {
            // Частично оплачено - желтый
            rowColor = QColor(255, 255, 200);
        } else {
            // Не оплачено - красный
            rowColor = QColor(255, 200, 200);
        }

        for (int col = 0; col < 8; ++col) {
            reportTable->item(i, col)->setBackground(rowColor);
        }
    }

    reportTable->resizeColumnsToContents();
    reportTable->horizontalHeader()->setStretchLastSection(true);

    // Дата и время формирования отчета
    report += QString("<hr><p align='right' style='font-size: 10pt; color: gray;'>"
                     "Отчет сформирован: %1 %2</p>"
                     "</body></html>")
                     .arg(QDate::currentDate().toString("dd.MM.yyyy"))
                     .arg(QTime::currentTime().toString("hh:mm:ss"));

    reportTextEdit->setHtml(report);
}

void ReportWindow::printReport()
{
    QPrinter printer;
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setOutputFormat(QPrinter::NativeFormat);

    QPrintDialog printDialog(&printer, this);
    if (printDialog.exec() == QDialog::Accepted) {
        QTextDocument document;

        // Создаем форматированный документ для печати
        QString html = reportTextEdit->toHtml();
        html = "<html><body style='font-family: Arial; font-size: 10pt;'>" + html + "</body></html>";

        document.setHtml(html);
        document.print(&printer);

        QMessageBox::information(this, "Печать", "Отчет отправлен на печать");
    }
}

void ReportWindow::exportToExcel()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "Экспорт в Excel",
        "Отчет_" + QDate::currentDate().toString("yyyy-MM-dd") + ".csv",
        "CSV файлы (*.csv);;Все файлы (*)");

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось создать файл");
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Записываем заголовки
    for (int col = 0; col < reportTable->columnCount(); ++col) {
        if (col > 0) out << ";";
        out << "\"" << reportTable->horizontalHeaderItem(col)->text() << "\"";
    }
    out << "\n";

    // Записываем данные
    for (int row = 0; row < reportTable->rowCount(); ++row) {
        for (int col = 0; col < reportTable->columnCount(); ++col) {
            if (col > 0) out << ";";
            QString text = reportTable->item(row, col) ?
                          reportTable->item(row, col)->text() : "";
            out << "\"" << text << "\"";
        }
        out << "\n";
    }

    file.close();

    QMessageBox::information(this, "Экспорт",
        QString("Данные успешно экспортированы в файл:\n%1").arg(fileName));
}
