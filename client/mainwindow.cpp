#include "publicheader.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "registerwindow.h"
#include "QPaintEvent"
#include "QPainter"
#include <QMessageBox>
#include <QHostAddress>
#include <QNetworkInterface>
//备注:在不同主机上运行服务器要在客户端的chatwindow.cpp文件切换至对应的ip地址。

MainWindow::MainWindow(QWidget * parent): QMainWindow(parent), ui(new Ui::MainWindow), chatwindow(nullptr) {

    ui -> setupUi(this);
    this -> setWindowTitle(tr("登录界面"));

    setAttribute(Qt::WA_TranslucentBackground); //透明
    setWindowFlags(Qt::FramelessWindowHint);

    this->setStyleSheet(R"(
            QMainWindow#MainWindow {
                border-radius: 20px;
                background-color: #fff;
                box-shadow: 15px 15px 10px rgba(33,45,58,0.3);
            }
        )");


    socket = new QTcpSocket(this);

    //192.168.12.13测试用网线ip地址
    //192.168.82.198测试用热点ip地址

    //在不同主机上运行服务器要切换至对应的ip地址。
    //ip地址较为常用，封装在程序内部
    socket -> connectToHost("192.168.1.29", 2024);

    connect(socket, & QTcpSocket::readyRead, this, & MainWindow::processResponse);
    registerWindow = new RegisterWindow(this, this -> socket);

    //插入图标
    QIcon windowIcon(":/images/logo.png");
    setWindowIcon(windowIcon);


    connect(socket, & QTcpSocket::connected, this, [this]() {
        qDebug() << "登录客户端连接成功";
    });
    connect(socket, & QTcpSocket::disconnected, this, [this]() {
        qDebug() << "登录客户端连接断开";
    });

}

MainWindow::~MainWindow() {
    delete ui;
    delete registerWindow;
    if (chatwindow) {
        delete chatwindow;
    }
}


//处理服务器信息的函数
void MainWindow::processResponse() {



    QTcpSocket * clientSocket = qobject_cast < QTcpSocket * > (sender());
    QDataStream in (clientSocket);
    in.setVersion(QDataStream::Qt_5_14);

    while (clientSocket -> bytesAvailable() > 0) {
        qint64 packetType;
        in >> packetType;

        //debug信息
        if(packetType!=1){
            qDebug()<<"信息类型不为1";
            return;}
        //要删掉的


        QByteArray messageData;
        in>>messageData;
        QString response=QString::fromUtf8(messageData).trimmed();

        qDebug() << "登录客户端收到数据：" << response;


        //注册成功
        if (response.startsWith("REGISTER_SUCCESS")) {


            QMessageBox::information(this, tr("注册成功"), tr("注册成功！"));
            this -> show(); // 显示登录界面
            registerWindow -> close(); // 关闭注册界面

        }

        //注册失败，进行反馈
        else if (response.startsWith("REGISTER_FAIL")) {

            registerWindow -> registerFail(); // 调用注册窗口的失败处理函数

        }


        //登录成功
        else if (response.startsWith("LOGIN_SUCCESS")) {

            //注册成功
            if (!chatwindow) {
                QString clientName = response.section(' ', 1);
                chatwindow = new ChatWindow(clientName, socket);
                disconnect(socket, & QTcpSocket::readyRead, this, & MainWindow::processResponse);
                this -> close();
                chatwindow -> show();
            }

        } else if (response.startsWith("LOGIN_FAIL")) {
            QMessageBox::warning(this, tr("登录失败"), tr("登录失败：请检查您的输入信息或稍后再试。"));
        } else if (response.startsWith("DISCONNECTED")) {

            //    通知其他用户该用户离线
        } else if (response.startsWith("CONNECTED")) {
            //    通知其他用户该用户登录
            //  这个还是在LOGIN_SUCCESS一起写吧
            //  放在
        } else if (response.startsWith("ERROR")) {
            QString error = response.section(' ', 1);
            QMessageBox::warning(this, tr("错误"), error);
        }


        //登录成功后加载群聊页
        else if (response.startsWith("LOGIN_SUCCESS_GROUPS")) {
            //用户加入的群聊列表
            QStringList data = response.split(" ");
            data.removeFirst();
            for (QString groupName: data) {
                chatwindow -> addChatPage(groupName);
            }
        }
        //登录成功加载用户信息页
        else if (response.startsWith("LOGIN_SUCCESS_USERS")) {

            QStringList data = response.split(" ");
            data.removeFirst();
            for (QString userName: data) {
                chatwindow -> addChatPage(userName);
            }

        }

        //补充发送信息的函数

    }
}


//登录槽函数
void MainWindow::on_loginButton_3_clicked() {
    QString loginMsg = "";
    loginMsg += "LOGIN ";
    //username
    loginMsg += ui -> username_3 -> text() + " ";

    //password
    loginMsg += ui -> password_3 -> text();
    QByteArray block;
    QDataStream out( & block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_14);
    out << qint64(1);
    out << (loginMsg + "\n").toUtf8();
    socket -> write(block);



    //服务器的判断功能另外放到别的函数


}
//注册槽函数
void MainWindow::on_registerButton_3_clicked() {

    registerWindow -> show();
    this -> close();

}


//处理界面函数
void MainWindow::paintEvent(QPaintEvent * event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QBrush brush(Qt::white);
    painter.setBrush(brush);
    painter.setPen(Qt::transparent);
    QRect rect = this -> rect();
    rect.setWidth(rect.width() - 1);
    rect.setHeight(rect.height() - 1);
    painter.drawRoundedRect(rect, 30, 30);
}

//按住鼠标
void MainWindow::mousePressEvent(QMouseEvent * event) {
    if (event -> button() == Qt::LeftButton) {
        m_dragPosition = event -> globalPos() - frameGeometry().topLeft();
        event -> accept();
    }
}
//移动鼠标
void MainWindow::mouseMoveEvent(QMouseEvent * event) {
    if (event -> buttons() & Qt::LeftButton) {
        move(event -> globalPos() - m_dragPosition);
        event -> accept();
    }
}
