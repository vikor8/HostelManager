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

    double totalRevenue = 0;
    double paidAmount = 0;
    double unpaidAmount = 0;
    int totalBookings = 0;

    QSqlQuery query(database->getDatabase());

    // Получаем все бронирования
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
                  "AND bk.check_out_date >= ? "
                  "ORDER BY bk.check_in_date, r.room_number, b.bed_number");
    query.addBindValue(endDate.toString("yyyy-MM-dd"));
    query.addBindValue(startDate.toString("yyyy-MM-dd"));

    struct BookingItem {
        int id;
        QDate checkIn;
        QDate checkOut;
        double totalPrice;
        double pricePerDay;
        int totalDays;
        int daysInPeriod;
        double revenueInPeriod;
        double paidAmount;
        double unpaidInPeriod;
        QString paymentMethod;
        QDateTime paymentDate;
        bool isRoomBooking;
        QString roomNumber;
        QString category;
        int bedNumber;
        QString clientName;
    };

    struct CategoryStats {
        int bookingsCount = 0;
        double revenue = 0;
        double unpaid = 0;
        double paid = 0;
    };

    QList<BookingItem> bookings;
    QMap<QString, CategoryStats> categoryStats;
    QMap<QString, double> paymentsByMethod;

    if (query.exec()) {
        while (query.next()) {
            BookingItem booking;
            booking.id = query.value(0).toInt();
            booking.checkIn = QDate::fromString(query.value(1).toString(), "yyyy-MM-dd");
            booking.checkOut = QDate::fromString(query.value(2).toString(), "yyyy-MM-dd");
            booking.totalPrice = query.value(3).toDouble();
            booking.paidAmount = query.value(4).toDouble();
            booking.paymentMethod = query.value(5).toString();
            booking.paymentDate = QDateTime::fromString(query.value(6).toString(), "yyyy-MM-dd hh:mm:ss");
            booking.isRoomBooking = query.value(7).toBool();
            booking.roomNumber = query.value(8).toString();
            booking.category = query.value(9).toString();
            booking.bedNumber = query.value(10).toInt();
            booking.clientName = query.value(11).toString();

            // Расчет дней
            booking.totalDays = booking.checkIn.daysTo(booking.checkOut);

            // Период пересечения с отчетным периодом
            QDate actualStart = booking.checkIn > startDate ? booking.checkIn : startDate;
            QDate actualEnd = booking.checkOut < endDate ? booking.checkOut : endDate;
            booking.daysInPeriod = actualStart.daysTo(actualEnd);

            // Стоимость за день
            if (booking.totalDays > 0) {
                booking.pricePerDay = booking.totalPrice / booking.totalDays;
            } else {
                booking.pricePerDay = 0;
            }

            // Стоимость за период
            booking.revenueInPeriod = booking.pricePerDay * booking.daysInPeriod;

            // Расчет задолженности в периоде
            if (booking.paidAmount >= booking.totalPrice) {
                booking.unpaidInPeriod = 0;
            } else if (booking.paidAmount > 0) {
                int paidDays = static_cast<int>(booking.paidAmount / booking.pricePerDay);
                QDate firstUnpaidDate = booking.checkIn.addDays(paidDays);
                QDate unpaidStart = firstUnpaidDate > actualStart ? firstUnpaidDate : actualStart;

                if (unpaidStart < actualEnd) {
                    int unpaidDays = unpaidStart.daysTo(actualEnd);
                    booking.unpaidInPeriod = booking.pricePerDay * unpaidDays;
                } else {
                    booking.unpaidInPeriod = 0;
                }
            } else {
                booking.unpaidInPeriod = booking.revenueInPeriod;
            }

            bookings.append(booking);

            // Суммируем статистику
            totalRevenue += booking.revenueInPeriod;
            unpaidAmount += booking.unpaidInPeriod;
            totalBookings++;
        }
    }

    // Оплаты за период
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

    if (query.exec()) {
        while (query.next()) {
            QString method = query.value(0).toString();
            double amount = query.value(1).toDouble();
            paymentsByMethod[method] = amount;
            paidAmount += amount;
        }
    }

    // Обновляем оплаты по категориям
    for (const auto& booking : bookings) {
        if (booking.paymentDate.isValid() &&
            booking.paymentDate.date() >= startDate &&
            booking.paymentDate.date() <= endDate) {
            categoryStats[booking.category].paid += booking.paidAmount;
        }
        categoryStats[booking.category].bookingsCount++;
        categoryStats[booking.category].revenue += booking.revenueInPeriod;
        categoryStats[booking.category].unpaid += booking.unpaidInPeriod;
    }

    // Занятость мест
    int occupiedBeds = 0;
    int totalBeds = 0;

    query.prepare("SELECT COUNT(DISTINCT bed_id) FROM bookings "
                  "WHERE status = 'active' "
                  "AND ? BETWEEN check_in_date AND check_out_date");
    query.addBindValue(endDate.toString("yyyy-MM-dd"));
    if (query.exec() && query.next()) {
        occupiedBeds = query.value(0).toInt();
    }

    query.exec("SELECT COUNT(*) FROM beds WHERE is_active = 1");
    if (query.next()) {
        totalBeds = query.value(0).toInt();
    }

    // Заполняем таблицу
    reportTable->clear();
    reportTable->setRowCount(0);
    reportTable->setColumnCount(13);
    QStringList headers;
    headers << "Дата заезда" << "Дата выезда" << "Комната" << "Койка" << "Тип"
            << "Клиент" << "Общая сумма" << "Стоимость за сутки"
            << "Дней в периоде" << "Сумма за период"
            << "Оплачено всего" << "Не оплачено в периоде" << "Дата оплаты";
    reportTable->setHorizontalHeaderLabels(headers);

    for (const auto& booking : bookings) {
        int row = reportTable->rowCount();
        reportTable->insertRow(row);

        QString bookingType = booking.isRoomBooking ? "Комната целиком" : "Место";
        QString paymentDateStr = booking.paymentDate.isValid() ?
            booking.paymentDate.toString("dd.MM.yyyy hh:mm") : "Не указана";

        bool isPaymentInPeriod = booking.paymentDate.isValid() &&
                                 booking.paymentDate.date() >= startDate &&
                                 booking.paymentDate.date() <= endDate;

        QString paidDisplay = QString::number(booking.paidAmount, 'f', 2) + " руб.";
        if (booking.paidAmount > 0 && !isPaymentInPeriod) {
            paidDisplay += " (вне периода)";
        }

        bool isPartiallyPaid = (booking.paidAmount > 0 && booking.paidAmount < booking.totalPrice);

        reportTable->setItem(row, 0, new QTableWidgetItem(booking.checkIn.toString("dd.MM.yyyy")));
        reportTable->setItem(row, 1, new QTableWidgetItem(booking.checkOut.toString("dd.MM.yyyy")));
        reportTable->setItem(row, 2, new QTableWidgetItem(booking.roomNumber));
        reportTable->setItem(row, 3, new QTableWidgetItem(QString::number(booking.bedNumber)));
        reportTable->setItem(row, 4, new QTableWidgetItem(bookingType));
        reportTable->setItem(row, 5, new QTableWidgetItem(booking.clientName));
        reportTable->setItem(row, 6, new QTableWidgetItem(QString::number(booking.totalPrice, 'f', 2) + " руб."));
        reportTable->setItem(row, 7, new QTableWidgetItem(QString::number(booking.pricePerDay, 'f', 2) + " руб."));
        reportTable->setItem(row, 8, new QTableWidgetItem(QString::number(booking.daysInPeriod)));
        reportTable->setItem(row, 9, new QTableWidgetItem(QString::number(booking.revenueInPeriod, 'f', 2) + " руб."));
        reportTable->setItem(row, 10, new QTableWidgetItem(paidDisplay));
        reportTable->setItem(row, 11, new QTableWidgetItem(QString::number(booking.unpaidInPeriod, 'f', 2) + " руб."));
        reportTable->setItem(row, 12, new QTableWidgetItem(paymentDateStr));

        QColor rowColor;
        if (booking.unpaidInPeriod <= 0) {
            rowColor = QColor(200, 255, 200);
        } else if (isPartiallyPaid) {
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

    reportTable->resizeColumnsToContents();
    reportTable->horizontalHeader()->setStretchLastSection(true);

    // Формируем HTML отчет
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
    html += " <tr><th>Показатель</th><th>Значение</th></tr>\n";
    html += QString(" <tr><td>Количество бронирований (пересекающихся с периодом)</td><td align='right'>%1</td></tr>\n").arg(totalBookings);
    html += QString(" <tr><td>Общая стоимость бронирований (только дни в периоде)</td><td align='right'>%1 руб.</td></tr>\n").arg(totalRevenue, 0, 'f', 2);
    html += QString(" <tr><td>Полученная оплата (в период %1 - %2)</td><td align='right'>%3 руб.</td></tr>\n")
            .arg(startDate.toString("dd.MM.yyyy"))
            .arg(endDate.toString("dd.MM.yyyy"))
            .arg(paidAmount, 0, 'f', 2);
    html += QString(" <tr><td><b>Задолженность за период</b> (неоплаченные дни в периоде)</td><td align='right'><b>%1 руб.</b></td></tr>\n").arg(unpaidAmount, 0, 'f', 2);
    double paymentPercent = totalRevenue > 0 ? ((totalRevenue - unpaidAmount) / totalRevenue) * 100 : 0;
    html += QString(" <tr><td>Процент оплаты от общей стоимости</td><td align='right'>%1%</td></tr>\n").arg(paymentPercent, 0, 'f', 1);
    html += "</table>\n";

    // Примечание
    html += "<div class='note'>\n";
    html += "<i>Примечание:<br>\n";
    html += "• В графе \"Общая стоимость бронирований\" учитывается стоимость ТОЛЬКО тех дней, которые попадают в выбранный период.<br>\n";
    html += "• Если бронирование частично выходит за пределы периода, учитывается только пропорциональная часть стоимости.<br>\n";
    html += "• <b>Задолженность за период</b> рассчитывается следующим образом:<br>\n";
    html += "&nbsp;&nbsp;- Если бронирование оплачено полностью, задолженность = 0<br>\n";
    html += "&nbsp;&nbsp;- Если бронирование оплачено частично, определяется количество полностью оплаченных суток, и задолженность начисляется только за неоплаченные сутки, попадающие в отчетный период<br>\n";
    html += "&nbsp;&nbsp;- Если бронирование не оплачено, задолженность = стоимость всех дней в периоде<br>\n";
    html += "• В графе \"Полученная оплата\" учитываются только платежи, произведенные в выбранный период (по дате оплаты).\n";
    html += "</i>\n";
    html += "</div>\n";

    // 2. Занятость мест
    double occupancyRate = totalBeds > 0 ? (occupiedBeds * 100.0 / totalBeds) : 0;
    html += "<h3>2. Занятость мест (на " + endDate.toString("dd.MM.yyyy") + "):</h3>\n";
    html += "<table>\n";
    html += " <tr><th>Показатель</th><th>Значение</th></tr>\n";
    html += QString(" <tr><td>Всего мест</td><td align='right'>%1</td></tr>\n").arg(totalBeds);
    html += QString(" <tr><td>Занято мест</td><td align='right'>%1</td></tr>\n").arg(occupiedBeds);
    html += QString(" <tr><td>Свободно мест</td><td align='right'>%1</td></tr>\n").arg(totalBeds - occupiedBeds);
    html += QString(" <tr><td>Процент загрузки</td><td align='right'>%1%</td></tr>\n").arg(occupancyRate, 0, 'f', 1);
    html += "</table>\n";

    // 3. Распределение оплаты по способам
    if (!paymentsByMethod.isEmpty()) {
        html += "<h3>3. Распределение оплаты по способам (за период):</h3>\n";
        html += "<table>\n";
        html += " <tr><th>Способ оплаты</th><th>Сумма</th><th>Доля</th></tr>\n";

        double totalPaid = 0;
        for (auto it = paymentsByMethod.begin(); it != paymentsByMethod.end(); ++it) {
            totalPaid += it.value();
        }

        for (auto it = paymentsByMethod.begin(); it != paymentsByMethod.end(); ++it) {
            QString method = it.key();
            double amount = it.value();
            double percentage = totalPaid > 0 ? (amount * 100 / totalPaid) : 0;

            html += QString(" <tr><td>%1</td><td align='right'>%2 руб.</td><td align='right'>%3%</td></tr>\n")
                    .arg(method)
                    .arg(amount, 0, 'f', 2)
                    .arg(percentage, 0, 'f', 1);
        }

        html += QString(" <tr style='font-weight: bold; background-color: #f0f0f0;'>"
                       "<td>ИТОГО ОПЛАЧЕНО ЗА ПЕРИОД:</td>"
                       "<td align='right'>%1 руб.</td>"
                       "<td align='right'>100%</td></tr>\n")
                       .arg(totalPaid, 0, 'f', 2);
        html += "</table>\n";
    }

    // 4. Статистика по категориям
    if (!categoryStats.isEmpty()) {
        html += "<h3>4. Статистика по категориям номеров (только дни в периоде):</h3>\n";
        html += "<table>\n";
        html += " <tr><th>Категория</th><th>Броней</th><th>Выручка за период</th><th>Оплачено за период</th><th>Задолженность</th><th>% оплаты</th></tr>\n";

        for (auto it = categoryStats.begin(); it != categoryStats.end(); ++it) {
            QString category = it.key();
            int bookingsCount = it.value().bookingsCount;
            double revenue = it.value().revenue;
            double paid = it.value().paid;
            double unpaid = it.value().unpaid;
            double paymentRate = revenue > 0 ? ((revenue - unpaid) * 100 / revenue) : 0;

            html += QString(" <tr>"
                           "<td>%1</td>"
                           "<td align='right'>%2</td>"
                           "<td align='right'>%3 руб.</td>"
                           "<td align='right'>%4 руб.</td>"
                           "<td align='right'>%5 руб.</td>"
                           "<td align='right'>%6%</td>"
                           "</tr>\n")
                           .arg(category)
                           .arg(bookingsCount)
                           .arg(revenue, 0, 'f', 2)
                           .arg(paid, 0, 'f', 2)
                           .arg(unpaid, 0, 'f', 2)
                           .arg(paymentRate, 0, 'f', 1);
        }
        html += "</table>\n";
    }

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
