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
#include <QApplication>

// Для печати в Qt 6
#include <QPrinter>
#include <QPrintDialog>
#include <QPageSize>
#include <QPageLayout>

// Структура для хранения данных бронирования
struct BookingData {
    int id;
    QDate checkIn;
    QDate checkOut;
    double totalPrice;
    double pricePerDay;
    int totalDays;
    int daysInPeriod;
    double revenueInPeriod;
    double paidAmount;
    int paidDays;                 // Количество полностью оплаченных суток
    double unpaidDaysInPeriod;    // Количество неоплаченных суток в периоде
    double unpaidAmountInPeriod;  // Сумма задолженности в периоде
    QString paymentMethod;
    QDateTime paymentDate;
    bool isRoomBooking;
    int roomBookingGroup;
    QString roomNumber;
    int bedNumber;
    QString clientName;
};

// Структура для статистики по категориям
struct CategoryStats {
    int bookingsCount;
    double revenue;
    double paid;
    double unpaid;      // Задолженность по категории

    CategoryStats() : bookingsCount(0), revenue(0), paid(0), unpaid(0) {}
};

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

    QApplication::setOverrideCursor(Qt::WaitCursor);

    // ================= ПЕРЕМЕННЫЕ ДЛЯ РАСЧЕТА =================
    int totalBookedBeds = 0;
    double totalBookedBedsCost = 0;
    double totalFreeBedsCost = 0;
    double totalUnpaidBedsCost = 0;

    QSqlQuery query(database->getDatabase());

    // ================= 1. РАСЧЕТ ПО ДНЯМ =================
    QList<QPair<int, double>> allBeds;
    query.exec("SELECT b.id, b.price_per_day FROM beds b WHERE b.is_active = 1");
    while (query.next()) {
        int bedId = query.value(0).toInt();
        double pricePerDay = query.value(1).toDouble();
        allBeds.append(qMakePair(bedId, pricePerDay));
    }

    for (QDate date = startDate; date <= endDate; date = date.addDays(1)) {
        QString dateStr = date.toString("yyyy-MM-dd");

        for (const auto& bed : allBeds) {
            int bedId = bed.first;
            double pricePerDay = bed.second;

            query.prepare("SELECT "
                          "bk.id, "
                          "bk.total_price, "
                          "bk.paid_amount, "
                          "bk.check_in_date, "
                          "bk.check_out_date "
                          "FROM bookings bk "
                          "WHERE bk.bed_id = ? "
                          "AND bk.status = 'active' "
                          "AND bk.check_in_date <= ? "
                          "AND bk.check_out_date > ?");
            query.addBindValue(bedId);
            query.addBindValue(dateStr);
            query.addBindValue(dateStr);

            if (query.exec() && query.next()) {
                totalBookedBeds++;
                totalBookedBedsCost += pricePerDay;

                double totalPrice = query.value(1).toDouble();
                double paidAmountForBooking = query.value(2).toDouble();
                QDate checkInDate = QDate::fromString(query.value(3).toString(), "yyyy-MM-dd");
                QDate checkOutDate = QDate::fromString(query.value(4).toString(), "yyyy-MM-dd");

                bool isDayPaid = false;

                if (paidAmountForBooking <= 0) {
                    isDayPaid = false;
                } else if (paidAmountForBooking >= totalPrice) {
                    isDayPaid = true;
                } else {
                    int totalBookingDays = checkInDate.daysTo(checkOutDate);
                    double bookingPricePerDay = (totalBookingDays > 0) ? (totalPrice / totalBookingDays) : pricePerDay;
                    int fullyPaidDays = static_cast<int>(paidAmountForBooking / bookingPricePerDay);
                    QDate firstUnpaidDate = checkInDate.addDays(fullyPaidDays);

                    if (date < firstUnpaidDate) {
                        isDayPaid = true;
                    } else {
                        isDayPaid = false;
                    }
                }

                if (!isDayPaid) {
                    totalUnpaidBedsCost += pricePerDay;
                }
            } else {
                totalFreeBedsCost += pricePerDay;
            }
        }
    }

    // ================= 2. РАСПРЕДЕЛЕНИЕ ОПЛАТЫ ПО СПОСОБАМ =================
    QMap<QString, double> paymentsByMethod;
    double totalPaidInPeriod = 0;

    query.prepare("SELECT "
                  "bk.id, "
                  "bk.check_in_date, "
                  "bk.check_out_date, "
                  "bk.total_price, "
                  "bk.paid_amount, "
                  "bk.payment_method, "
                  "bk.is_room_booking "
                  "FROM bookings bk "
                  "WHERE bk.status = 'active' "
                  "AND bk.check_in_date <= ? "
                  "AND bk.check_out_date > ? "
                  "ORDER BY bk.check_in_date");
    query.addBindValue(endDate.toString("yyyy-MM-dd"));
    query.addBindValue(startDate.toString("yyyy-MM-dd"));

    if (query.exec()) {
        while (query.next()) {
            QDate checkInDate = QDate::fromString(query.value(1).toString(), "yyyy-MM-dd");
            QDate checkOutDate = QDate::fromString(query.value(2).toString(), "yyyy-MM-dd");
            double totalPrice = query.value(3).toDouble();
            double paidAmount = query.value(4).toDouble();
            QString paymentMethod = query.value(5).toString();

            if (paidAmount <= 0) {
                continue;
            }

            int totalBookingDays = checkInDate.daysTo(checkOutDate);
            if (totalBookingDays <= 0) {
                continue;
            }

            double bookingPricePerDay = totalPrice / totalBookingDays;
            int fullyPaidDays = static_cast<int>(paidAmount / bookingPricePerDay);

            if (fullyPaidDays <= 0) {
                continue;
            }

            QDate firstPaidDate = checkInDate;
            QDate lastPaidDate = checkInDate.addDays(fullyPaidDays);

            QDate paidStart = (firstPaidDate > startDate) ? firstPaidDate : startDate;
            QDate paidEnd = (lastPaidDate < endDate.addDays(1)) ? lastPaidDate : endDate.addDays(1);

            int paidDaysInPeriod = 0;
            if (paidStart < paidEnd) {
                paidDaysInPeriod = paidStart.daysTo(paidEnd);
            }

            if (paidDaysInPeriod > 0) {
                double paidAmountInPeriod = bookingPricePerDay * paidDaysInPeriod;
                paymentsByMethod[paymentMethod] += paidAmountInPeriod;
                totalPaidInPeriod += paidAmountInPeriod;
            }
        }
    }

    // ================= 3. ЗАНЯТОСТЬ МЕСТ =================
    int occupiedBeds = 0;
    int totalBeds = 0;

    query.prepare("SELECT COUNT(DISTINCT bed_id) FROM bookings "
                  "WHERE status = 'active' "
                  "AND check_in_date <= ? "
                  "AND check_out_date > ?");
    query.addBindValue(endDate.toString("yyyy-MM-dd"));
    query.addBindValue(endDate.toString("yyyy-MM-dd"));
    if (query.exec() && query.next()) {
        occupiedBeds = query.value(0).toInt();
    }

    query.exec("SELECT COUNT(*) FROM beds WHERE is_active = 1");
    if (query.next()) {
        totalBeds = query.value(0).toInt();
    }

    // ================= 4. ВНЕСЕННЫЕ ПЛАТЕЖИ =================
    struct PaymentRecord {
        QDate date;
        QString clientName;
        double amount;
        QString method;
    };
    QList<PaymentRecord> allPayments;
    double totalPaymentsSum = 0;

    // Получаем ВСЕ платежи, у которых payment_date попадает в выбранный период
    // Независимо от дат бронирования
    query.prepare("SELECT "
                  "bk.payment_date, "
                  "c.last_name || ' ' || c.first_name || ' ' || COALESCE(c.middle_name, '') as client_name, "
                  "bk.paid_amount, "
                  "bk.payment_method "
                  "FROM bookings bk "
                  "JOIN clients c ON bk.client_id = c.id "
                  "WHERE bk.paid_amount > 0 "
                  "AND bk.payment_date IS NOT NULL "
                  "AND DATE(bk.payment_date) BETWEEN ? AND ? "
                  "ORDER BY bk.payment_date DESC");
    query.addBindValue(startDate.toString("yyyy-MM-dd"));
    query.addBindValue(endDate.toString("yyyy-MM-dd"));

    if (query.exec()) {
        while (query.next()) {
            PaymentRecord record;
            QDateTime paymentDateTime = QDateTime::fromString(query.value(0).toString(), "yyyy-MM-dd hh:mm:ss");
            record.date = paymentDateTime.date();
            record.clientName = query.value(1).toString();
            record.amount = query.value(2).toDouble();
            record.method = query.value(3).toString();

            allPayments.append(record);
            totalPaymentsSum += record.amount;
        }
    }

    // ================= 5. ТАБЛИЦА ДЕТАЛИЗАЦИИ =================
    reportTable->clear();
    reportTable->setRowCount(0);
    reportTable->setColumnCount(13);
    QStringList headers;
    headers << "Дата заезда" << "Дата выезда" << "Комната" << "Койка" << "Тип"
            << "Клиент" << "Общая сумма" << "Стоимость за сутки"
            << "Дней в периоде" << "Сумма за период"
            << "Оплачено всего" << "Не оплачено в периоде" << "Дата оплаты";
    reportTable->setHorizontalHeaderLabels(headers);

    query.prepare("SELECT "
                  "bk.id, "
                  "bk.check_in_date, "
                  "bk.check_out_date, "
                  "bk.total_price, "
                  "bk.paid_amount, "
                  "bk.payment_method, "
                  "bk.payment_date, "
                  "bk.is_room_booking, "
                  "r.room_number, "
                  "r.category, "
                  "b.bed_number, "
                  "c.last_name || ' ' || c.first_name as client_name "
                  "FROM bookings bk "
                  "JOIN beds b ON bk.bed_id = b.id "
                  "JOIN rooms r ON b.room_id = r.id "
                  "JOIN clients c ON bk.client_id = c.id "
                  "WHERE bk.status = 'active' "
                  "AND bk.check_in_date <= ? "
                  "AND bk.check_out_date > ? "
                  "ORDER BY bk.check_in_date, r.room_number, b.bed_number");
    query.addBindValue(endDate.toString("yyyy-MM-dd"));
    query.addBindValue(startDate.toString("yyyy-MM-dd"));

    if (query.exec()) {
        while (query.next()) {
            QDate checkIn = QDate::fromString(query.value(1).toString(), "yyyy-MM-dd");
            QDate checkOut = QDate::fromString(query.value(2).toString(), "yyyy-MM-dd");
            double totalPrice = query.value(3).toDouble();
            double paidAmountForBooking = query.value(4).toDouble();
            QString paymentMethod = query.value(5).toString();
            QDateTime paymentDate = QDateTime::fromString(query.value(6).toString(), "yyyy-MM-dd hh:mm:ss");
            bool isRoomBooking = query.value(7).toBool();
            QString roomNumber = query.value(8).toString();
            QString category = query.value(9).toString();
            int bedNumber = query.value(10).toInt();
            QString clientName = query.value(11).toString();

            int totalDays = checkIn.daysTo(checkOut);
            double pricePerDay = (totalDays > 0) ? (totalPrice / totalDays) : 0;

            QDate actualStart = (checkIn > startDate) ? checkIn : startDate;
            QDate actualEnd = (checkOut < endDate.addDays(1)) ? checkOut : endDate.addDays(1);
            int daysInPeriod = (actualStart < actualEnd) ? actualStart.daysTo(actualEnd) : 0;

            double revenueInPeriod = pricePerDay * daysInPeriod;

            double unpaidInPeriod = 0;
            if (paidAmountForBooking < totalPrice) {
                int fullyPaidDays = (pricePerDay > 0) ? static_cast<int>(paidAmountForBooking / pricePerDay) : 0;
                QDate firstUnpaidDate = checkIn.addDays(fullyPaidDays);

                QDate unpaidStart = (firstUnpaidDate > actualStart) ? firstUnpaidDate : actualStart;
                if (unpaidStart < actualEnd) {
                    int unpaidDays = unpaidStart.daysTo(actualEnd);
                    unpaidInPeriod = pricePerDay * unpaidDays;
                }
            }

            int row = reportTable->rowCount();
            reportTable->insertRow(row);

            QString bookingType = isRoomBooking ? "Комната целиком" : "Место";
            QString paymentDateStr = paymentDate.isValid() ?
                                         paymentDate.toString("dd.MM.yyyy hh:mm") : "Не указана";

            reportTable->setItem(row, 0, new QTableWidgetItem(checkIn.toString("dd.MM.yyyy")));
            reportTable->setItem(row, 1, new QTableWidgetItem(checkOut.toString("dd.MM.yyyy")));
            reportTable->setItem(row, 2, new QTableWidgetItem(roomNumber));
            reportTable->setItem(row, 3, new QTableWidgetItem(QString::number(bedNumber)));
            reportTable->setItem(row, 4, new QTableWidgetItem(bookingType));
            reportTable->setItem(row, 5, new QTableWidgetItem(clientName));
            reportTable->setItem(row, 6, new QTableWidgetItem(QString::number(totalPrice, 'f', 2) + " руб."));
            reportTable->setItem(row, 7, new QTableWidgetItem(QString::number(pricePerDay, 'f', 2) + " руб."));
            reportTable->setItem(row, 8, new QTableWidgetItem(QString::number(daysInPeriod)));
            reportTable->setItem(row, 9, new QTableWidgetItem(QString::number(revenueInPeriod, 'f', 2) + " руб."));
            reportTable->setItem(row, 10, new QTableWidgetItem(QString::number(paidAmountForBooking, 'f', 2) + " руб."));
            reportTable->setItem(row, 11, new QTableWidgetItem(QString::number(unpaidInPeriod, 'f', 2) + " руб."));
            reportTable->setItem(row, 12, new QTableWidgetItem(paymentDateStr));

            QColor rowColor;
            if (unpaidInPeriod <= 0) {
                rowColor = QColor(200, 255, 200);
            } else if (paidAmountForBooking > 0 && paidAmountForBooking < totalPrice) {
                rowColor = QColor(255, 255, 200);
            } else {
                rowColor = QColor(255, 200, 200);
            }

            for (int col = 0; col < 13; ++col) {
                if (reportTable->item(row, col)) {
                    reportTable->item(row, col)->setBackground(rowColor);
                }
            }
        }
    }

    reportTable->resizeColumnsToContents();
    reportTable->horizontalHeader()->setStretchLastSection(true);

    // ================= 6. ФОРМИРУЕМ HTML ОТЧЕТ =================
    QString html;
    html += "<!DOCTYPE html>\n";
    html += "<html>\n";
    html += "<head>\n";
    html += "<meta charset='UTF-8'>\n";
    html += "<style>\n";
    html += "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    html += "h2 { color: #2c3e50; text-align: center; }\n";
    html += "h3 { color: #34495e; margin-top: 20px; }\n";
    html += "table { border-collapse: collapse; width: 100%; margin: 10px 0; }\n";
    html += "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    html += "th { background-color: #4CAF50; color: white; }\n";
    html += "tr:nth-child(even) { background-color: #f2f2f2; }\n";
    html += ".total-row { font-weight: bold; background-color: #e0e0e0; }\n";
    html += ".note { color: gray; font-size: 9pt; margin-top: 20px; }\n";
    html += ".footer { text-align: right; font-size: 10pt; color: gray; margin-top: 30px; }\n";
    html += "</style>\n";
    html += "</head>\n";
    html += "<body>\n";

    // Заголовок
    html += "<h2>СВОДНЫЙ ОТЧЕТ</h2>\n";
    html += QString("<h3>за период с %1 по %2</h3>\n")
                .arg(startDate.toString("dd.MM.yyyy"))
                .arg(endDate.toString("dd.MM.yyyy"));
    html += "<hr>\n";

    // 1. Общие показатели
    html += "<h3>1. Общие показатели:</h3>\n";
    html += "<table>\n";
    html += "  <tr><th>Показатель</th><th>Значение</th></tr>\n";
    html += QString("   <tr><td>Количество забронированных койко-мест</td><td align='right'>%1</td></tr>\n").arg(totalBookedBeds);
    html += QString("   <tr><td>Общая стоимость забронированных койко-мест</td><td align='right'>%1 руб.</td></tr>\n").arg(totalBookedBedsCost, 0, 'f', 2);
    html += QString("   <tr><td>Стоимость свободных койко-мест</td><td align='right'>%1 руб.</td></tr>\n").arg(totalFreeBedsCost, 0, 'f', 2);
    html += QString("   <tr><td><b>Сумма задолженности по забронированным койко-местам</b></td><td align='right'><b>%1 руб.</b></td></tr>\n").arg(totalUnpaidBedsCost, 0, 'f', 2);
    html += "</table>\n";

    // 2. Распределение оплаты по способам
    if (!paymentsByMethod.isEmpty()) {
        html += "<h3>2. Распределение оплаты по способам (за период):</h3>\n";
        html += "<table>\n";
        html += "   <tr><th>Способ оплаты</th><th>Сумма</th><th>Доля</th></tr>\n";

        for (auto it = paymentsByMethod.begin(); it != paymentsByMethod.end(); ++it) {
            QString method = it.key();
            double amount = it.value();
            double percentage = totalPaidInPeriod > 0 ? (amount * 100 / totalPaidInPeriod) : 0;

            html += QString("   <tr>"
                            "<td>%1</td>"
                            "<td align='right'>%2 руб.</td>"
                            "<td align='right'>%3%</td>"
                            "</tr>\n")
                        .arg(method)
                        .arg(amount, 0, 'f', 2)
                        .arg(percentage, 0, 'f', 1);
        }

        html += QString(" <tr style='font-weight: bold; background-color: #f0f0f0;'>"
                        "<td>ИТОГО ОПЛАЧЕНО ЗА ПЕРИОД:</td>"
                        "<td align='right'>%1 руб.</td>"
                        "<td align='right'>100%</td>"
                        "</tr>\n")
                    .arg(totalPaidInPeriod, 0, 'f', 2);
        html += "</table>\n";
    } else {
        html += "<h3>2. Распределение оплаты по способам (за период):</h3>\n";
        html += "<p>За выбранный период не было произведено ни одного платежа.</p>\n";
    }

    // 3. Занятость мест
    double occupancyRate = totalBeds > 0 ? (occupiedBeds * 100.0 / totalBeds) : 0;
    html += "<h3>3. Занятость мест (на " + endDate.toString("dd.MM.yyyy") + "):</h3>\n";
    html += "<table>\n";
    html += "   <tr><th>Показатель</th><th>Значение</th></tr>\n";
    html += QString("   <tr><td>Всего мест</td><td align='right'>%1</td></tr>\n").arg(totalBeds);
    html += QString("   <tr><td>Занято мест</td><td align='right'>%1</td></tr>\n").arg(occupiedBeds);
    html += QString("   <tr><td>Свободно мест</td><td align='right'>%1</td></tr>\n").arg(totalBeds - occupiedBeds);
    html += QString("   <tr><td>Процент загрузки</td><td align='right'>%1%</td></tr>\n").arg(occupancyRate, 0, 'f', 1);
    html += "</table>\n";

    // 4. Внесенные платежи
    html += "<h3>4. Внесенные платежи:</h3>\n";
    if (!allPayments.isEmpty()) {
        html += "<table>\n";
        html += "   <tr><th>Дата платежа</th><th>Клиент</th><th>Сумма платежа</th><th>Способ оплаты</th></tr>\n";

        for (const auto& payment : allPayments) {
            html += QString("   <tr>"
                            "<td>%1</td>"
                            "<td>%2</td>"
                            "<td align='right'>%3 руб.</td>"
                            "<td>%4</td>"
                            "</tr>\n")
                        .arg(payment.date.toString("dd.MM.yyyy"))
                        .arg(payment.clientName)
                        .arg(payment.amount, 0, 'f', 2)
                        .arg(payment.method);
        }

        // Итого
        html += QString(" <tr class='total-row'>"
                        "<td colspan='2' align='right'><b>ИТОГО:</b></td>"
                        "<td align='right'><b>%1 руб.</b></td>"
                        "<td></td>"
                        "</tr>\n")
                    .arg(totalPaymentsSum, 0, 'f', 2);

        html += "</table>\n";
    } else {
        html += "<p>За выбранный период платежей не обнаружено.</p>\n";
    }

    // Примечание
    html += "<div class='note'>\n";
    html += "<i>Примечание:<br>\n";
    html += "• Все показатели рассчитаны посуточно для каждого дня выбранного периода.<br>\n";
    html += "• \"Количество забронированных койко-мест\" — суммарное количество занятых мест за все дни периода.<br>\n";
    html += "• День выезда не считается занятым (бронирование с 1 по 5 число занимает дни 1-4).<br>\n";
    html += "• \"Сумма задолженности по забронированным койко-местам\" учитывает частичную оплату: оплаченные дни исключаются из расчета.<br>\n";
    html += "• В разделе \"Распределение оплаты по способам\" учитываются только фактически оплаченные дни в периоде.<br>\n";
    html += "• В разделе \"Внесенные платежи\" отображаются все платежи, произведенные в выбранном периоде.</i>\n";
    html += "</div>\n";

    // Подвал
    html += "<div class='footer'>\n";
    html += QString("Отчет сформирован: %1 %2\n")
                .arg(QDate::currentDate().toString("dd.MM.yyyy"))
                .arg(QTime::currentTime().toString("hh:mm:ss"));
    html += "</div>\n";
    html += "</body>\n";
    html += "</html>\n";

    reportTextEdit->setHtml(html);

    QApplication::restoreOverrideCursor();
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
