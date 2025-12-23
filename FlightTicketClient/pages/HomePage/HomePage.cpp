#include "HomePage.h"
#include "ui_HomePage.h"

HomePage::HomePage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HomePage)
{
    ui->setupUi(this);

    m_src = QPixmap(":/images/homepageimage.PNG");

    // 检查按钮是否存在并连接信号
    if (ui->btnGoFlights) {
        connect(ui->btnGoFlights, &QPushButton::clicked, this, &HomePage::requestGoFlights);
    }
    if (ui->btnGoOrders) {
        connect(ui->btnGoOrders, &QPushButton::clicked, this, &HomePage::requestGoOrders);
    }
    if (ui->btnGoProfile) {
        connect(ui->btnGoProfile, &QPushButton::clicked, this, &HomePage::requestGoProfile);
    }
}

HomePage::~HomePage()
{
    delete ui;
}

void HomePage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_src.isNull()) return;

    const QSize box = ui->lblHomeImage->size();
    if (box.width() <= 0 || box.height() <= 0) return;

    QPixmap scaled = m_src.scaled(box, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    ui->lblHomeImage->setAlignment(Qt::AlignCenter);
    ui->lblHomeImage->setPixmap(scaled);
}
