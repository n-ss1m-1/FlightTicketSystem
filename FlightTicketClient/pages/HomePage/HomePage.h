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

private:
    Ui::HomePage *ui;
};

#endif // HOMEPAGE_H
