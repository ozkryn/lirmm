#include "epos3.h"
#include "ui_epos3.h"

epos3::epos3(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::epos3)
{
    ui->setupUi(this);
}

epos3::~epos3()
{
    delete ui;
}
