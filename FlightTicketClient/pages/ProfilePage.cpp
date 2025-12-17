#include "ProfilePage.h"
#include "ui_ProfilePage.h"

ProfilePage::ProfilePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ProfilePage)
{
    ui->setupUi(this);
}

ProfilePage::~ProfilePage()
{
    delete ui;
}
