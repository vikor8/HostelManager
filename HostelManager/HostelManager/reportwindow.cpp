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

    CategoryStats() : bookingsCount(0), revenue(0), paid(0) {}
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

    double totalRevenue = 0;      // Стоимость только дней, попавших в период
    double paidAmount = 0;        // Оплаты, произведенные в период
    int totalBookings = 0;        // Количество бронирований, пересекающихся с периодом

    QSqlQuery query(database->getDatabase());

    // 1. Получаем все бронирования, пересекающиеся с периодом
    // и рассчитываем стоимость ТОЛЬКО за дни, попавшие в период
    query.prepare("SELECT "
                  "bk.id, "
                  "bk.bed_id, "
                  "bk.check_in_date, "
                  "bk.check_out_date, "
                  "bk.total_price, "
                  "bk.paid_amount, "
                  "bk.payment_method, "
                  "bk.payment_date, "
                  "bk.is_room_booking, "
                  "bk.room_booking_group, "
                  "r.room_number, "
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

    QList<BookingData> bookings;

    if (query.exec()) {
        while (query.next()) {
            BookingData booking;
            booking.id = query.value(0).toInt();
            booking.checkIn = QDate::fromString(query.value(2).toString(), "yyyy-MM-dd");
            booking.checkOut = QDate::fromString(query.value(3).toString(), "yyyy-MM-dd");
            booking.totalPrice = query.value(4).toDouble();
            booking.paidAmount = query.value(5).toDouble();
            booking.paymentMethod = query.value(6).toString();
            booking.paymentDate = QDateTime::fromString(query.value(7).toString(), "yyyy-MM-dd hh:mm:ss");
            booking.isRoomBooking = query.value(8).toBool();
            booking.roomBookingGroup = query.value(9).toInt();
            booking.roomNumber = query.value(10).toString();
            booking.bedNumber = query.value(11).toInt();
            booking.clientName = query.value(12).toString();

            // Рассчитываем количество дней в периоде
            booking.totalDays = booking.checkIn.daysTo(booking.checkOut);

            // Определяем фактический период пересечения с отчетным периодом
            QDate actualStart = booking.checkIn > startDate ? booking.checkIn : startDate;
            QDate actualEnd = booking.checkOut < endDate ? booking.checkOut : endDate;
            booking.daysInPeriod = actualStart.daysTo(actualEnd);

            // Рассчитываем стоимость за день
            if (booking.totalDays > 0) {
                booking.pricePerDay = booking.totalPrice / booking.totalDays;
            } else {
                booking.pricePerDay = 0;
            }

            // Стоимость только за дни, попавшие в период
            booking.revenueInPeriod = booking.pricePerDay * booking.daysInPeriod;

            bookings.append(booking);

            // Суммируем общую выручку за период
            totalRevenue += booking.revenueInPeriod;
            totalBookings++;
        }
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

    double unpaidAmount = totalRevenue - paidAmount;

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

    // 5. Статистика по комнатам (с учетом пропорционального расчета)
    QMap<QString, CategoryStats> categoryStats;

    for (const auto& booking : bookings) {
        // Получаем категорию комнаты
        QString category;
        QSqlQuery catQuery(database->getDatabase());
        catQuery.prepare("SELECT category FROM rooms WHERE room_number = ?");
        catQuery.addBindValue(booking.roomNumber);
        if (catQuery.exec() && catQuery.next()) {
            category = catQuery.value(0).toString();
        }

        if (!category.isEmpty()) {
            categoryStats[category].bookingsCount++;
            categoryStats[category].revenue += booking.revenueInPeriod;

            // Проверяем, попадает ли оплата в период
            if (booking.paymentDate.isValid() &&
                booking.paymentDate.date() >= startDate &&
                booking.paymentDate.date() <= endDate) {
                categoryStats[category].paid += booking.paidAmount;
            }
        }
    }

    // 6. Детализированные данные с учетом пропорционального расчета
    reportTable->clear();
    reportTable->setRowCount(0);
    reportTable->setColumnCount(11);
    QStringList headers;
    headers << "Дата заезда" << "Дата выезда" << "Комната" << "Койка" << "Тип" << "Клиент"
            << "Общая сумма" << "Стоимость за сутки" << "Дней в периоде" << "Сумма за период" << "Дата оплаты";
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
        reportTable->setItem(row, 10, new QTableWidgetItem(paymentDateStr));

        // Цветовая индикация
        QColor rowColor;
        if (booking.revenueInPeriod <= 0) {
            rowColor = QColor(240, 240, 240); // Серый - нет дней в периоде
        } else if (booking.revenueInPeriod <= booking.paidAmount && isPaymentInPeriod) {
            rowColor = QColor(200, 255, 200); // Зеленый - полностью оплачено в период
        } else if (booking.revenueInPeriod <= booking.paidAmount) {
            rowColor = QColor(150, 255, 150); // Темно-зеленый - полностью оплачено до периода
        } else if (booking.paidAmount > 0) {
            rowColor = QColor(255, 255, 200); // Желтый - частично оплачено
        } else {
            rowColor = QColor(255, 200, 200); // Красный - не оплачено
        }

        for (int col = 0; col < 11; ++col) {
            if (reportTable->item(row, col)) {
                reportTable->item(row, col)->setBackground(rowColor);
            }
        }
    }

    reportTable->resizeColumnsToContents();
    reportTable->horizontalHeader()->setStretchLastSection(true);

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
                     "<tr><td>Количество бронирований (пересекающихся с периодом)</td><td align='right'>%1</td></tr>"
                     "<tr><td>Общая стоимость бронирований (только дни в периоде)</td><td align='right'>%2 руб.</td></tr>"
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
             "<i>Примечание: <br>"
             "• В графе \"Общая стоимость бронирований\" учитывается стоимость ТОЛЬКО тех дней, "
             "которые попадают в выбранный период.<br>"
             "• Если бронирование частично выходит за пределы периода, учитывается только "
             "пропорциональная часть стоимости.<br>"
             "• В графе \"Полученная оплата\" учитываются только платежи, произведенные в выбранный период "
             "(по дате оплаты).</i>"
             "</p>";

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

    // Статистика по способам оплаты за период
    if (!paymentsByMethod.isEmpty()) {
        report += "<h3>3. Распределение оплаты по способам (за период):</h3>";
        report += "<table border='1' cellpadding='5' style='border-collapse: collapse; width: 100%;'>"
                 "<tr><td width='60%'><b>Способ оплаты</b></td><td width='20%' align='right'><b>Сумма</b></td><td width='20%' align='right'><b>Доля</b></td></tr>";

        double totalPaid = 0;
        for (auto it = paymentsByMethod.begin(); it != paymentsByMethod.end(); ++it) {
            totalPaid += it.value();
        }

        for (auto it = paymentsByMethod.begin(); it != paymentsByMethod.end(); ++it) {
            QString method = it.key();
            double amount = it.value();
            double percentage = totalPaid > 0 ? (amount * 100 / totalPaid) : 0;

            report += QString("<tr>"
                             "<td>%1</td>"
                             "<td align='right'>%2 руб.</td>"
                             "<td align='right'>%3%</td>"
                             "</tr>")
                             .arg(method)
                             .arg(amount, 0, 'f', 2)
                             .arg(percentage, 0, 'f', 1);
        }

        report += QString("<tr style='font-weight: bold; background-color: #f0f0f0;'>"
                         "<td>ИТОГО ОПЛАЧЕНО ЗА ПЕРИОД:</td>"
                         "<td align='right'>%1 руб.</td>"
                         "<td align='right'>100%</td>"
                         "</tr>")
                         .arg(totalPaid, 0, 'f', 2);

        report += "</table><br>";
    }

    // Статистика по категориям комнат
    if (!categoryStats.isEmpty()) {
        report += "<h3>4. Статистика по категориям номеров (только дни в периоде):</h3>";
        report += "<table border='1' cellpadding='5' style='border-collapse: collapse; width: 100%;'>"
                 "<tr><td><b>Категория</b></td><td align='right'><b>Броней</b></td>"
                 "<td align='right'><b>Выручка за период</b></td>"
                 "<td align='right'><b>Оплачено за период</b></td>"
                 "<td align='right'><b>% оплаты</b></td></tr>";

        for (auto it = categoryStats.begin(); it != categoryStats.end(); ++it) {
            QString category = it.key();
            int bookingsCount = it.value().bookingsCount;
            double revenue = it.value().revenue;
            double paid = it.value().paid;
            double paymentRate = revenue > 0 ? (paid * 100 / revenue) : 0;

            report += QString("<tr>"
                             "<td>%1</td>"
                             "<td align='right'>%2</td>"
                             "<td align='right'>%3 руб.</td>"
                             "<td align='right'>%4 руб.</td>"
                             "<td align='right'>%5%</td>"
                             "</tr>")
                             .arg(category)
                             .arg(bookingsCount)
                             .arg(revenue, 0, 'f', 2)
                             .arg(paid, 0, 'f', 2)
                             .arg(paymentRate, 0, 'f', 1);
        }
        report += "</table><br>";
    }

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
