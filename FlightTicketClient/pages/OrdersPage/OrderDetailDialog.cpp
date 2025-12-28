#include "OrderDetailDialog.h"
#include "ui_OrderDetailDialog.h"

OrderDetailDialog::OrderDetailDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OrderDetailDialog)
{
    ui->setupUi(this);
}

OrderDetailDialog::~OrderDetailDialog()
{
    delete ui;
}
