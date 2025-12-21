#ifndef FLIGHTSPAGE_H
#define FLIGHTSPAGE_H

#include <QWidget>
#include <QSqlQueryModel>

QT_BEGIN_NAMESPACE
namespace Ui { class FlightsPage; }
QT_END_NAMESPACE

class FlightsPage : public QWidget
{
    Q_OBJECT

public:
    FlightsPage(QWidget *parent = nullptr);
    ~FlightsPage();

private slots:
    void on_btnSearch_clicked(); // 查询按钮点击事件
    void on_btnBook_clicked();   // 订票按钮点击事件

private:
    Ui::FlightsPage *ui;
    QSqlQueryModel *model;       // 数据显示模型
};

#endif
