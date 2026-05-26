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

    // Переменные для расчета
    int totalBookedBeds = 0;        // Количество забронированных койко-мест
    double totalBookedBedsCost = 0; // Общая стоимость забронированных койко-мест
    double totalFreeBedsCost = 0;   // Стоимость свободных койко-мест
    double totalUnpaidBedsCost = 0; // Стоимость не оплаченных койко-мест
    double paidAmount = 0;

    QSqlQuery query(database->getDatabase());

    // ================= ИЗМЕНЕННЫЙ ЗАПРОС =================
    // Теперь получаем детальную информацию о бронировании для расчета неоплаченных дней
    query.prepare("SELECT "
                  "b.price_per_day, "
                  "bk.id as booking_id, "
                  "bk.total_price, "
                  "bk.paid_amount, "
                  "bk.check_in_date, "
                  "bk.check_out_date "
                  "FROM beds b "
                  "LEFT JOIN bookings bk ON b.id = bk.bed_id "
                  "    AND bk.status = 'active' "
                  "    AND bk.check_in_date <= :end_date "
                  "    AND bk.check_out_date >= :start_date "
                  "WHERE b.is_active = 1");

    query.bindValue(":start_date", startDate.toString("yyyy-MM-dd"));
    query.bindValue(":end_date", endDate.toString("yyyy-MM-dd"));

    if (query.exec()) {
        while (query.next()) {
            double pricePerDay = query.value(0).toDouble();
            QVariant bookingIdVar = query.value(1);
            bool isBooked = !bookingIdVar.isNull();

            if (isBooked) {
                totalBookedBeds++;
                totalBookedBedsCost += pricePerDay;

                // Получаем данные о бронировании
                double totalPrice = query.value(2).toDouble();
                double paidAmountForBooking = query.value(3).toDouble();
                QDate checkInDate = QDate::fromString(query.value(4).toString(), "yyyy-MM-dd");
                QDate checkOutDate = QDate::fromString(query.value(5).toString(), "yyyy-MM-dd");

                // Рассчитываем количество дней в бронировании
                int totalBookingDays = checkInDate.daysTo(checkOutDate);

                // Цена за сутки для этого бронирования
                double bookingPricePerDay = (totalBookingDays > 0) ? (totalPrice / totalBookingDays) : pricePerDay;

                // Определяем, является ли это койко-место неоплаченным
                bool isUnpaid = false;

                if (paidAmountForBooking <= 0) {
                    // Совсем не оплачено - все дни неоплачены
                    isUnpaid = true;
                } else if (paidAmountForBooking < totalPrice) {
                    // Оплачено частично - определяем, какие дни оплачены
                    int fullyPaidDays = static_cast<int>(paidAmountForBooking / bookingPricePerDay);

                    // Дата, с которой начинаются неоплаченные дни
                    QDate firstUnpaidDate = checkInDate.addDays(fullyPaidDays);

                    // Текущая дата для этой записи (приблизительно)
                    // Так как мы не знаем точную дату для каждой записи в этом запросе,
                    // используем упрощенную логику: если есть хотя бы один неоплаченный день
                    // в периоде, считаем койко-место неоплаченным
                    QDate periodStart = (checkInDate > startDate) ? checkInDate : startDate;
                    QDate periodEnd = (checkOutDate < endDate) ? checkOutDate : endDate;

                    if (firstUnpaidDate <= periodEnd && firstUnpaidDate <= endDate) {
                        isUnpaid = true;
                    }
                }
                // else: paidAmountForBooking >= totalPrice - полностью оплачено

                if (isUnpaid) {
                    totalUnpaidBedsCost += pricePerDay;
                }
            } else {
                totalFreeBedsCost += pricePerDay;
            }
        }
    }
    // =====================================================

    // 2. Получаем сумму оплат, произведенных в период (БЕЗ ИЗМЕНЕНИЙ)
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
    double totalPaidInPeriod = 0;

    if (query.exec()) {
        while (query.next()) {
            QString method = query.value(0).toString();
            double amount = query.value(1).toDouble();
            paymentsByMethod[method] = amount;
            totalPaidInPeriod += amount;
            paidAmount += amount;
        }
    }

    // 3. Расчет занятости мест (БЕЗ ИЗМЕНЕНИЙ)
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

    // 4. Очищаем и заполняем таблицу детализации (БЕЗ ИЗМЕНЕНИЙ)
    reportTable->clear();
    reportTable->setRowCount(0);
    reportTable->setColumnCount(13);
    QStringList headers;
    headers << "Дата заезда" << "Дата выезда" << "Комната" << "Койка" << "Тип"
            << "Клиент" << "Общая сумма" << "Стоимость за сутки"
            << "Дней в периоде" << "Сумма за период"
            << "Оплачено всего" << "Не оплачено в периоде" << "Дата оплаты";
    reportTable->setHorizontalHeaderLabels(headers);

    // 5. Формируем HTML отчет (БЕЗ ИЗМЕНЕНИЙ, кроме примечания)
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
    html += "  <tr><th>Показатель</th><th>Значение</th></tr>\n";
    html += QString("   <tr><td>Количество забронированных койко-мест</td><td align='right'>%1</td></tr>\n").arg(totalBookedBeds);
    html += QString("   <tr><td>Общая стоимость забронированных койко-мест</td><td align='right'>%1 руб.</td></tr>\n").arg(totalBookedBedsCost, 0, 'f', 2);
    html += QString("   <tr><td>Стоимость свободных койко-мест</td><td align='right'>%1 руб.</td></tr>\n").arg(totalFreeBedsCost, 0, 'f', 2);
    html += QString("   <tr><td><b>Стоимость не оплаченных койко-мест</b></td><td align='right'><b>%1 руб.</b></td></tr>\n").arg(totalUnpaidBedsCost, 0, 'f', 2);
    html += "</table>\n";

    // 2. Распределение оплаты по способам (БЕЗ ИЗМЕНЕНИЙ)
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

    // 3. Занятость мест (БЕЗ ИЗМЕНЕНИЙ)
    double occupancyRate = totalBeds > 0 ? (occupiedBeds * 100.0 / totalBeds) : 0;
    html += "<h3>3. Занятость мест (на " + endDate.toString("dd.MM.yyyy") + "):</h3>\n";
    html += "<table>\n";
    html += "   <tr><th>Показатель</th><th>Значение</th></tr>\n";
    html += QString("   <tr><td>Всего мест</td><td align='right'>%1</td></tr>\n").arg(totalBeds);
    html += QString("   <tr><td>Занято мест</td><td align='right'>%1</td></tr>\n").arg(occupiedBeds);
    html += QString("   <tr><td>Свободно мест</td><td align='right'>%1</td></tr>\n").arg(totalBeds - occupiedBeds);
    html += QString("   <tr><td>Процент загрузки</td><td align='right'>%1%</td></tr>\n").arg(occupancyRate, 0, 'f', 1);
    html += "</table>\n";

    // Обновленное примечание
    html += "<div class='note'>\n";
    html += "<i>Примечание:<br>\n";
    html += "• Все стоимостные показатели рассчитаны на основе цены за день для каждого койко-места.<br>\n";
    html += "• В графе \"Стоимость свободных койко-мест\" учитывается сумма цен всех незабронированных мест в периоде.<br>\n";
    html += "• В графе \"Стоимость не оплаченных койко-мест\" учитываются:<br>\n";
    html += "&nbsp;&nbsp;- Полностью неоплаченные бронирования<br>\n";
    html += "&nbsp;&nbsp;- Частично оплаченные бронирования, где неоплаченные дни попадают в отчетный период<br>\n";
    html += "&nbsp;&nbsp;- Если бронирование оплачено полностью, оно не учитывается в этой графе</i>\n";
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
