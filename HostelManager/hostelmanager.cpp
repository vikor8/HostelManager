#include "hostelmanager.h"
#include "ui_hostelmanager.h"

HostelManager::HostelManager(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::HostelManager)
{
    ui->setupUi(this);
}

HostelManager::~HostelManager()
{
    delete ui;
}

