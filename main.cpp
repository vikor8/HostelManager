#include "hostelmanager.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    HostelManager w;
    w.show();
    return a.exec();
}
