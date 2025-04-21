#include "registerwindow.h"
#include "ui_registerwindow.h"
#include "mainwindow.h"
#include "publicheader.h"
#include "QPaintEvent"
#include "QPainter"

//备注:在不同主机上运行服务器要在客户端的chatwindow.cpp文件切换至对应的ip地址。

RegisterWindow::RegisterWindow(MainWindow * loginWindow, QTcpSocket * socket, QWidget * parent):
    QWidget(parent),
    ui(new Ui::RegisterWindow) {
        ui -> setupUi(this);
        this -> setWindowTitle(tr("注册窗口"));
        this -> loginWindow = loginWindow;
        this -> socket = socket;

        //新图标logo
        QIcon windowIcon(":/images/logo.png");
        setWindowIcon(windowIcon);

        setAttribute(Qt::WA_TranslucentBackground); //透明
        setWindowFlags(Qt::FramelessWindowHint);

        this->setStyleSheet(R"(
                    QMainWindow#MainWindow {
                        border-radius: 20px;
                        background-color: #fff;
                        box-shadow: 15px 15px 10px rgba(33,45,58,0.3);
                    }
                )");

        connect(socket, & QTcpSocket::connected, this, [this]() {
            qDebug() << "注册客户端连接成功";
        });
        connect(socket, & QTcpSocket::disconnected, this, [this]() {
            qDebug() << "注册客户端连接断开";
        });
        connect(socket, QOverload < QAbstractSocket::SocketError > ::of( & QTcpSocket::error), this, [this](QAbstractSocket::SocketError socketError) {
            qDebug() << "注册客户端网络错误:" << socketError;
        });

    }

RegisterWindow::~RegisterWindow() {
    delete ui;
}

//注册界面注册按钮槽函数
void RegisterWindow::on_registerWindowRegisterButton_clicked() {

    QByteArray block;
        QDataStream out( & block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_14);
        out << qint64(1);

    QString registerMsg = "";
    registerMsg += "REGISTER ";
    //username
    registerMsg += ui -> registerWindowUsername -> text() + " ";
    //password
    registerMsg += ui -> registerWindowPassword -> text();

    out<<(registerMsg + "\n").toUtf8();
    this->socket -> write(block);
    //注册完成后的操作，放在别的函数

}



void RegisterWindow::registerFail() {
    ui -> registerWindowUsername -> clear();
    ui -> registerWindowPassword -> clear();
}

//返回登录界面槽函数
void RegisterWindow::on_registerWindowReturnButton_clicked() {
    loginWindow -> show();
    this -> close();

}





//处理界面函数
void RegisterWindow::paintEvent(QPaintEvent * event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QBrush brush(Qt::white);
    painter.setBrush(brush);
    painter.setPen(Qt::transparent);
    QRect rect = this -> rect();
    rect.setWidth(rect.width() - 1);
    rect.setHeight(rect.height() - 1);
    painter.drawRoundedRect(rect, 20, 20);
}
void RegisterWindow::mousePressEvent(QMouseEvent * event) {
    if (event -> button() == Qt::LeftButton) {
        m_dragPosition = event -> globalPos() - frameGeometry().topLeft();
        event -> accept();
    }
}

void RegisterWindow::mouseMoveEvent(QMouseEvent * event) {
    if (event -> buttons() & Qt::LeftButton) {
        move(event -> globalPos() - m_dragPosition);
        event -> accept();
    }
}
