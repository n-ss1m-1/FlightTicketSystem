#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>
#include "pages/ProfilePage/ProfilePage.h"

namespace Ui {
class HomePage;
}

class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);
    ~HomePage();

    ProfilePage* m_profilePage;

signals:
    void requestGoFlights();
    void requestGoOrders();
    void requestGoProfile();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::HomePage *ui;

    QPixmap m_src; // 主页图原图
    void updateLoginButton();
};

#endif // HOMEPAGE_H
