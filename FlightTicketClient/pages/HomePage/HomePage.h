#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>

namespace Ui {
class HomePage;
}

class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);
    ~HomePage();

signals:
    void requestGoFlights();
    void requestGoOrders();
    void requestGoProfile();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::HomePage *ui;

    QPixmap m_src; // 主页图原图
};

#endif // HOMEPAGE_H
