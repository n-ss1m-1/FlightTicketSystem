#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    bool m_reconnectDialogShowing = false; // 正在弹出网络重连窗口
    bool m_reconnectPending = false; // 正在重连
};

#endif // MAINWINDOW_H
