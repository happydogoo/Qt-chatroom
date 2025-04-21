#ifndef MAINWINDOW_H
#define MAINWINDOW_H



#include "publicheader.h"
#include <QTcpSocket>
#include "registerwindow.h"
#include <QMainWindow>

/*
mainwindow类用于处理登录窗口，以及服务器传来的各种信息
7.4完成
注册跳转进入registerwindow窗口
注册成功后返回
当登录成功后跳转至chatwindow窗口
7.8完成
对服务器传来信息的处理
*/



class ChatWindow;
class RegisterWindow;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();





private slots:
    void on_loginButton_3_clicked();
    void on_registerButton_3_clicked();
    void processResponse();


private:
    Ui::MainWindow *ui;
    //数据库放服务器中
    //Database *db;
    QTcpSocket *socket;
    RegisterWindow* registerWindow;
    ChatWindow* chatwindow;
    QPoint m_dragPosition;

    //界面相关
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

};
#endif // MAINWINDOW_H
