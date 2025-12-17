#include "FlightsPage.h"
#include "ui_FlightsPage.h"

FlightsPage::FlightsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FlightsPage)
{
    ui->setupUi(this);
}

FlightsPage::~FlightsPage()
{
    delete ui;
}
