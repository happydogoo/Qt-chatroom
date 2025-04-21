#ifndef REGISTERWINDOW_H
#define REGISTERWINDOW_H
//避免重定义
#include <QMainWindow>
#include "mainwindow.h"
#include "publicheader.h"

#include <QTcpSocket>

class MainWindow;

namespace Ui {
class RegisterWindow;
}

class RegisterWindow : public QWidget
{
    Q_OBJECT

public:

    explicit RegisterWindow(MainWindow* loginWindow,QTcpSocket* socket,QWidget *parent = nullptr);
    ~RegisterWindow();


public slots:
    void registerFail();

private slots:
    //注册窗口的注册按钮和返回按钮
    void on_registerWindowRegisterButton_clicked();
    void on_registerWindowReturnButton_clicked();

private:
    Ui::RegisterWindow *ui;
    QMainWindow* loginWindow;
    QTcpSocket *socket;

    QPoint m_dragPosition;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;


//    不需要，可以放登录窗口做
//    ChatWindow* registerSuccess();


};

#endif // REGISTERWINDOW_H
