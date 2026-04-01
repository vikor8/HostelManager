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
//    exportButton = new QPushButton("Экспорт в Excel", this);
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

    double totalRevenue = 0;
    double paidAmount = 0;
    double unpaidAmount = 0;
    int totalBookings = 0;

    QSqlQuery query(database->getDatabase());

    // 1. Общая статистика по бронированиям за период
    query.prepare("SELECT "
                  "COUNT(*) as bookings_count, "
                  "SUM(total_price) as total_revenue "
                  "FROM bookings "
                  "WHERE status = 'active' "
                  "AND check_in_date <= ? "
                  "AND check_out_date >= ?");
    query.addBindValue(endDate.toString("yyyy-MM-dd"));
    query.addBindValue(startDate.toString("yyyy-MM-dd"));

    if (query.exec() && query.next()) {
        totalBookings = query.value(0).toInt();
        totalRevenue = query.value(1).toDouble();
    }

    // 2. Оплаты, произведенные в выбранный период (используем payment_date)
    query.prepare("SELECT "
                  "SUM(paid_amount) as total_paid_in_period "
                  "FROM bookings "
                  "WHERE status = 'active' "
                  "AND paid_amount > 0 "
                  "AND DATE(payment_date) BETWEEN ? AND ?");
    query.addBindValue(startDate.toString("yyyy-MM-dd"));
    query.addBindValue(endDate.toString("yyyy-MM-dd"));

    if (query.exec() && query.next()) {
        paidAmount = query.value(0).toDouble();
    }

    unpaidAmount = totalRevenue - paidAmount;

    // 3. Статистика по способам оплаты за период
    query.prepare("SELECT "
                  "payment_method, "
                  "SUM(paid_amount) as paid_by_method "
                  "FROM bookings "
                  "WHERE status = 'active' "
                  "AND paid_amount > 0 "
                  "AND DATE(payment_date) BETWEEN ? AND ? "
                  "GROUP BY payment_method "
                  "ORDER BY paid_by_method DESC");
    query.addBindValue(startDate.toString("yyyy-MM-dd"));
    query.addBindValue(endDate.toString("yyyy-MM-dd"));

    QMap<QString, double> paymentsByMethod;
    if (query.exec()) {
        while (query.next()) {
            QString method = query.value(0).toString();
            double amount = query.value(1).toDouble();
            paymentsByMethod[method] = amount;
        }
    }

    // 4. Занятость мест на последнюю дату периода
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

    query.exec("SELECT COUNT(*) FROM beds WHERE is_active = 1");
    if (query.next()) {
        totalBeds = query.value(0).toInt();
    }

    // 5. Статистика по комнатам
    query.prepare("SELECT "
                  "r.category, "
                  "COUNT(DISTINCT bk.id) as bookings_count, "
                  "SUM(bk.total_price) as category_revenue "
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

    if (query.exec()) {
        while (query.next()) {
            categories.append(query.value(0).toString());
            catBookings.append(query.value(1).toInt());
            catRevenues.append(query.value(2).toDouble());
        }
    }

    // 6. Статистика оплат по категориям за период
    QMap<QString, double> catPaids;
    query.prepare("SELECT "
                  "r.category, "
                  "SUM(bk.paid_amount) as category_paid "
                  "FROM bookings bk "
                  "JOIN beds b ON bk.bed_id = b.id "
                  "JOIN rooms r ON b.room_id = r.id "
                  "WHERE bk.status = 'active' "
                  "AND bk.paid_amount > 0 "
                  "AND DATE(bk.payment_date) BETWEEN ? AND ? "
                  "GROUP BY r.category");
    query.addBindValue(startDate.toString("yyyy-MM-dd"));
    query.addBindValue(endDate.toString("yyyy-MM-dd"));

    if (query.exec()) {
        while (query.next()) {
            QString category = query.value(0).toString();
            double paid = query.value(1).toDouble();
            catPaids[category] = paid;
        }
    }

    // 7. Детализированные данные
    query.prepare("SELECT "
                  "bk.check_in_date, "
                  "r.room_number, "
                  "b.bed_number, "
                  "c.last_name || ' ' || c.first_name as client_name, "
                  "bk.total_price, "
                  "bk.paid_amount, "
                  "bk.payment_method, "
                  "bk.is_room_booking, "
                  "bk.payment_date "
                  "FROM bookings bk "
                  "JOIN beds b ON bk.bed_id = b.id "
                  "JOIN rooms r ON b.room_id = r.id "
                  "JOIN clients c ON bk.client_id = c.id "
                  "WHERE bk.status = 'active' "
                  "AND (bk.check_in_date <= ? AND bk.check_out_date >= ?) "
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
            item["is_room_booking"] = query.value(7);
            item["payment_date"] = query.value(8);
            detailedData.append(item);
        }
    }

    // Формируем отчет
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
                     "<tr><td>Количество бронирований (пересекающихся с периодом)</td><td align='right'>%1</td></tr>"
                     "<tr><td>Общая стоимость бронирований за период</td><td align='right'>%2 руб.</td></tr>"
                     "<tr><td>Полученная оплата (в период %3 - %4)</td><td align='right'>%5 руб.</td></tr>"
                     "<tr><td>Задолженность по бронированиям за период</td><td align='right'>%6 руб.</td></tr>"
                     "<tr><td>Процент оплаты от общей стоимости</td><td align='right'>%7%</td></tr>"
                     "</table><br>")
                     .arg(totalBookings)
                     .arg(totalRevenue, 0, 'f', 2)
                     .arg(startDate.toString("dd.MM.yyyy"))
                     .arg(endDate.toString("dd.MM.yyyy"))
                     .arg(paidAmount, 0, 'f', 2)
                     .arg(unpaidAmount, 0, 'f', 2)
                     .arg(totalRevenue > 0 ? QString::number((paidAmount / totalRevenue) * 100, 'f', 1) : "0");

    // Добавляем пояснение
    report += "<p style='color: gray; font-size: 9pt;'>"
             "<i>Примечание: В графе \"Полученная оплата\" учитываются только платежи, "
             "произведенные в выбранный период (по дате оплаты). Если бронирование пересекается с периодом, "
             "но оплата была произведена ранее или позже, она не включается в отчет за этот период.</i>"
             "</p>";

    // ... остальная часть отчета (занятость, способы оплаты, категории) остается без изменений ...

    // Заполняем таблицу детализированными данными
    reportTable->clear();
    reportTable->setRowCount(0);
    reportTable->setColumnCount(10);
    reportTable->setHorizontalHeaderLabels({
        "Дата заезда", "Комната", "Койка", "Тип", "Клиент",
        "Общая сумма", "Оплачено", "Остаток", "Способ оплаты", "Дата оплаты"
    });

    for (int i = 0; i < detailedData.size(); ++i) {
        const QVariantMap &item = detailedData[i];
        reportTable->insertRow(i);

        QDate checkIn = QDate::fromString(item["check_in_date"].toString(), "yyyy-MM-dd");
        double totalPrice = item["total_price"].toDouble();
        double paid = item["paid_amount"].toDouble();
        double balance = totalPrice - paid;
        bool isRoomBooking = item["is_room_booking"].toBool();
        QDateTime paymentDateTime = QDateTime::fromString(item["payment_date"].toString(), "yyyy-MM-dd hh:mm:ss");

        QString bookingType = isRoomBooking ? "Комната целиком" : "Место";
        QString paymentDateStr = paymentDateTime.isValid() ?
            paymentDateTime.toString("dd.MM.yyyy hh:mm") : "Не указана";

        bool isPaymentInPeriod = paymentDateTime.isValid() &&
                                 paymentDateTime.date() >= startDate &&
                                 paymentDateTime.date() <= endDate;

        QString paidDisplay = QString::number(paid, 'f', 2) + " руб.";
        if (paid > 0 && !isPaymentInPeriod) {
            paidDisplay += " (вне периода)";
        }

        reportTable->setItem(i, 0, new QTableWidgetItem(checkIn.toString("dd.MM.yyyy")));
        reportTable->setItem(i, 1, new QTableWidgetItem(item["room_number"].toString()));
        reportTable->setItem(i, 2, new QTableWidgetItem(item["bed_number"].toString()));
        reportTable->setItem(i, 3, new QTableWidgetItem(bookingType));
        reportTable->setItem(i, 4, new QTableWidgetItem(item["client_name"].toString()));
        reportTable->setItem(i, 5, new QTableWidgetItem(QString::number(totalPrice, 'f', 2) + " руб."));
        reportTable->setItem(i, 6, new QTableWidgetItem(paidDisplay));
        reportTable->setItem(i, 7, new QTableWidgetItem(QString::number(balance, 'f', 2) + " руб."));
        reportTable->setItem(i, 8, new QTableWidgetItem(item["payment_method"].toString()));
        reportTable->setItem(i, 9, new QTableWidgetItem(paymentDateStr));

        QColor rowColor;
        if (balance <= 0 && isPaymentInPeriod) {
            rowColor = QColor(200, 255, 200);
        } else if (balance <= 0) {
            rowColor = QColor(150, 255, 150);
        } else if (paid > 0) {
            rowColor = QColor(255, 255, 200);
        } else {
            rowColor = QColor(255, 200, 200);
        }

        for (int col = 0; col < 10; ++col) {
            if (reportTable->item(i, col)) {
                reportTable->item(i, col)->setBackground(rowColor);
            }
        }
    }

    reportTable->resizeColumnsToContents();
    reportTable->horizontalHeader()->setStretchLastSection(true);

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
    // Заглушка "В разработке"
       QMessageBox::information(this,
           "Экспорт в Excel",
           "Функция экспорта в Excel находится в разработке.\n\n"
           "Ждем информацию от заазчика, какие данные нужно экспортировать.");
//    QString fileName = QFileDialog::getSaveFileName(this,
//        "Экспорт в Excel",
//        "Отчет_" + QDate::currentDate().toString("yyyy-MM-dd") + ".csv",
//        "CSV файлы (*.csv);;Все файлы (*)");

//    if (fileName.isEmpty()) {
//        return;
//    }

//    QFile file(fileName);
//    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
//        QMessageBox::warning(this, "Ошибка", "Не удалось создать файл");
//        return;
//    }

//    QTextStream out(&file);
//    out.setEncoding(QStringConverter::Utf8);

//    // Записываем заголовки
//    for (int col = 0; col < reportTable->columnCount(); ++col) {
//        if (col > 0) out << ";";
//        out << "\"" << reportTable->horizontalHeaderItem(col)->text() << "\"";
//    }
//    out << "\n";

//    // Записываем данные
//    for (int row = 0; row < reportTable->rowCount(); ++row) {
//        for (int col = 0; col < reportTable->columnCount(); ++col) {
//            if (col > 0) out << ";";
//            QString text = reportTable->item(row, col) ?
//                          reportTable->item(row, col)->text() : "";
//            out << "\"" << text << "\"";
//        }
//        out << "\n";
//    }

//    file.close();

//    QMessageBox::information(this, "Экспорт",
//        QString("Данные успешно экспортированы в файл:\n%1").arg(fileName));
}
