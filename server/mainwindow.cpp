#include "publicheader.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <string>
#include <QCryptographicHash>
#include <QDebug>
#include <QTcpSocket>
#include <QString>
#include <QFile>
#include <QStandardPaths>
#include <QMessageBox>

using namespace std;
//备注:在不同主机上运行服务器要在客户端的chatwindow.cpp文件切换至对应的ip地址。
MainWindow::MainWindow(QWidget * parent): QMainWindow(parent), ui(new Ui::MainWindow) {

    ui -> setupUi(this);
    setWindowTitle(tr("服务器窗口"));

    QMessageBox::information(this,"测试需知","此提示为答辩后补充的,原程序已将ip地址封装为常用地址,使用前需配置client项目mainwindow.cpp第37行代码ip地址为当前服务器ip地址");

    setupDatebase();

    //创建QTcpServer对象
    tcpServer = new QTcpServer(this);

    //调用槽函数，若有新连接，则调用onNewConnection函数，检测是否有新客户端连接
    connect(tcpServer, & QTcpServer::newConnection, this, & MainWindow::onNewConnection);


    //设置服务器监听
    short port = 2024;
    if (tcpServer -> listen(QHostAddress::Any, port)) {
        ui -> textEdit -> append(tr("服务器已开始监听"));
    } else {
        ui -> textEdit -> append(tr("无法启动服务器监听"));
    }

    QString publicChatRoomName = tr("公共聊天室");
    groups[publicChatRoomName] = QList < QTcpSocket * > ();
    ui -> textEdit -> append(tr("公共聊天室初始化完成"));
}

MainWindow::~MainWindow() {
    delete ui;
}


//初始化数据库函数
void MainWindow::setupDatebase() {
    //初始化数据库,初始化为SQLite类型
    userinfo = QSqlDatabase::addDatabase("QSQLITE", "userinfoConnection");
    userinfo.setDatabaseName("userinfo.db");

    // 打开数据库
    if (!userinfo.open()) {
        ui -> textEdit -> append(tr("无法打开用户信息数据库: ") + userinfo.lastError().text());
        return;
    }

    //创建QSQLQuery对象，用于执行SQL查询
    QSqlQuery userQuery(userinfo);

    //创建表格
    userQuery.exec("CREATE TABLE IF NOT EXISTS userinfoTable ("
        "username TEXT UNIQUE,"
        "password TEXT)");

    if (userQuery.lastError().isValid()) {
        //在服务器输出面板上显示创建用户表格结果
        ui -> textEdit -> append(tr("创建用户表格失败: ") + userQuery.lastError().text());
    } else {
        ui -> textEdit -> append(tr("用户信息数据库初始化完成"));
    }


    //创建群聊的数据库
    groupinfo = QSqlDatabase::addDatabase("QSQLITE", "groupConnection");
    userinfo.setDatabaseName("groupinfo.db");
    if (!groupinfo.open()) {
        ui -> textEdit -> append(tr("无法打开群聊数据库: ") + groupinfo.lastError().text());
        return;
    }
    //这个好像无法创建，检查一下
    QSqlQuery groupQuery(groupinfo);

    groupQuery.exec("CREATE TABLE IF NOT EXISTS groupTable ("
        "groupName TEXT PRIMARY KEY)");
    groupQuery.exec("CREATE TABLE IF NOT EXISTS groupMembersTable ("
        "groupName TEXT,"
        "memberName TEXT,"
        "PRIMARY KEY (groupName, memberName))");
    groupQuery.exec("CREATE TABLE IF NOT EXISTS groupinfoTable ("
        "groupName TEXT UNIQUE, memberName TEXT)");

    if (groupQuery.lastError().isValid()) {
        ui -> textEdit -> append(tr("创建群聊表失败: ") + groupQuery.lastError().text());
    } else {
        ui -> textEdit -> append(tr("数据库初始化完成"));
    }

}

//有新客户端连接，创建新连接
void MainWindow::onNewConnection() {

    QTcpSocket * clientSocket = tcpServer -> nextPendingConnection();
    //若连接不成功
    if (!clientSocket) {
        return;
    }

    ui -> textEdit -> append(tr("成功和客户端建立新的连接"));

    connect(clientSocket, & QTcpSocket::readyRead, this, & MainWindow::onReadyRead);
    connect(clientSocket, & QTcpSocket::disconnected, this, & MainWindow::onDisconnected);
    //将客户端连接存储到哈希表
    clients[clientSocket] = "";

    ui -> connectStatus -> setPixmap(QPixmap(":/images/connectsuccessed.jpg").scaled(20, 20));

    connect(clientSocket, & QTcpSocket::readyRead, this, & MainWindow::onReadyRead);
    connect(clientSocket, QOverload < QAbstractSocket::SocketError > ::of( & QTcpSocket::error), this, [this](QAbstractSocket::SocketError socketError) {
        qDebug() << "服务器网络错误:" << socketError;
    });



}


void MainWindow::onReadyRead() {
    QTcpSocket * clientSocket = qobject_cast < QTcpSocket * > (sender());
    //传入数据
    QDataStream in (clientSocket);
    in.setVersion(QDataStream::Qt_5_14);

    while (clientSocket -> bytesAvailable() > 0) {

        qint64 packetType;
        in >> packetType;

        //若类型为1，则按以前的逻辑，进入文本形式的操作
        if (packetType == 1) {
            qDebug()<<"信息类型为1";
            QByteArray messageData;
            in >> messageData;
            QString data = QString::fromUtf8(messageData).trimmed();

            ui -> textEdit -> append(tr("客户端发送信息:") + data);
            qDebug() << "服务器收到文本信息信息:" << data;
            if (data.isEmpty()) {
                return;
            }
            //按空格处理客户登录、注册信息
            QStringList params = data.split(" ");
            if (params.isEmpty()) {
                return;
            }

            //用户命令类别:
            QString userCommand = params.takeFirst();


            //执行注册操作
            if (userCommand == "REGISTER") {

                handleRegister(clientSocket, params);
                ui -> textEdit -> append(tr("进入注册操作"));
            }

            //执行登录操作
            else if (userCommand == "LOGIN") {
                handleLogin(clientSocket, params);
                ui -> textEdit -> append(tr("进入登录操作"));
            }

            //
            else if (userCommand == "GROUP_MESSAGE") {
                QString groupName = params.takeFirst();

                handleGroupChat(clientSocket, groupName, params.join(" "));
            }
            //私聊
            else if (userCommand == "PRIVATE") {
                QString recipient = params.takeFirst();
                handlePrivateChat(clientSocket, recipient, params.join(" "));

            } else if (userCommand == "CREATE_GROUP") {
                ui -> textEdit -> append("服务器收到了创建群聊通知");
                QString groupName = params.takeFirst();
                handleGroupRegister(clientSocket, groupName, params);
                ui -> textEdit -> append(("服务器完成了创建群聊" + groupName + "创建结果存储在groups中"));
            } else if (userCommand == "GET_CONTACT_LIST") {

                handleLoadContacts(clientSocket);
                ui -> textEdit -> append(tr("进入加载联系人操作"));
            } else if (userCommand == "CHATROOM") {

                QString sender = clients[clientSocket];
                QString message = params.join(" ");
                sendToAll("CHATROOM_MESSAGE " + sender + " " + message);

            }
        }


        //若类型为2，则按文件进行发送
        else if(packetType==2){
        qDebug()<<"收到类型为2的信息";
        QString fileName;
        qint64 fileSize;
        in>>fileName>>fileSize;

        //文件传输:文件名，文件大小
        //文件保存地址
        QString saveFilePath=QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)+"/"+fileName;

        QFile file(saveFilePath);

        if(!file.open(QIODevice::WriteOnly)){
            qDebug()<<"无法打开文件进行写入:"<<saveFilePath;
            return;

        }
        //缓存区
        qint64 bytesReceived=0;
        const int bufferSize=4096;
        char buffer[bufferSize];
        //若收到的还没完
        while (bytesReceived < fileSize) {
            qint64 bytesRead = in.device()->read(buffer, qMin(bufferSize, int(fileSize - bytesReceived)));
            if (bytesRead > 0) {
                //写入buffer
                file.write(buffer, bytesRead);
                bytesReceived += bytesRead;
            }
            else{
                qDebug()<<"文件读取错误";
                break;
            }
        }

        file.close();
        qDebug() << "文件"+fileName+"接收完成，地址:" << saveFilePath;

        //将信息拆分，获取发送对象
        QStringList params = QString(fileName).split("_");
        if (params.size() > 1) {
            QString recipient = params[0];
            QTcpSocket *recipientSocket=nullptr;
            for(auto i=clients.begin();i!=clients.end();++i){
            if(i.value()==recipient){
                recipientSocket=i.key();
                break;
                }

            }

            if (recipientSocket) {
                sendFileToClient(recipientSocket, saveFilePath, fileName, fileSize);
            }
        }


    }

    }



}


//客户端断开连接，槽函数，返回给客户端"DISCONNECTED "+clients[clientSocket]
void MainWindow::onDisconnected() {
    QTcpSocket * clientSocket = qobject_cast < QTcpSocket * > (sender());
    QString disconnectedUser = clients[clientSocket];
    this -> sendToAll("DISCONNECTED " + disconnectedUser);

    clients.remove(clientSocket);
    clientSocket -> deleteLater();

    ui -> textEdit -> append((tr("客户端") + disconnectedUser + tr("已经断开了连接")));
}


//注册功能，槽函数，若注册成功，发送给客户端“REGISTER_SUCCESS”
void MainWindow::handleRegister(QTcpSocket * client,
    const QStringList & params) {



    //后续要加一下防止重复用户名功能，遍历数据库,完成
    //添加重复确认密码功能
    //密保信息功能？
    //若注册信息无效
    ui -> textEdit -> append(tr("进入注册操作槽函数"));


    if (params.size() != 2) {
        QString returnQString = "REGISTER_FAIL" + tr(" 你输入的注册信息无效，请重新输入") + "\n";

        QByteArray block;

        QDataStream out( & block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_14);
        out << qint64(1);
        out<<returnQString.toUtf8();
        client -> write(block);

        ui -> textEdit -> append(tr("输入的注册信息无效"));
        return;
    }


    QString username = params[0];
    QString password = params[1];


    //存入数据库
    QSqlQuery query(userinfo);
    query.prepare("INSERT INTO userinfoTable (username, password) VALUES (?, ?)");
    query.addBindValue(username);
    query.addBindValue(password);


    bool operateStatus = query.exec();

    if (!operateStatus) {

        QString returnQString = ("REGISTER_FAIL ");
        ui -> textEdit -> append(tr("注册失败"));
        string returnString = returnQString.toStdString();

        QByteArray block;
            QDataStream out( & block, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_14);
            out << qint64(1);
            out<<((returnQString) + query.lastError().text()+ "\n").toUtf8();
            client->write(block);
//        client -> write((returnQString.toUtf8()) + query.lastError().text().toUtf8() + "\n");

    } else {

        QString returnQString = tr("REGISTER_SUCCESS\n");

        ui -> textEdit -> append(username + tr(":注册成功"));
        string returnString = returnQString.toStdString();

        QByteArray block;
        QDataStream out( & block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_14);
        out << qint64(1);
        out<<returnQString.toUtf8();

        client -> write(block);
        //发送给所有人，有新注册产生，在离线用户列表增加新用户
        sendToAll("NEW_REGISTER " + username);
    }
}

//处理登录功能，槽函数
void MainWindow::handleLogin(QTcpSocket * client,
    const QStringList & params) {
    //登录无效
    if (params.size() != 2) {

        QString returnQString = tr("你输入的登录信息无效，请正确输入");
        string returnString = returnQString.toStdString();

        QByteArray block;
            QDataStream out( & block, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_14);
            out << qint64(1);
            out<<(("LOGIN_FAIL " + returnQString) + "\n").toUtf8();
            client->write(block);
      //  client -> write((("LOGIN_FAIL " + returnQString).toUtf8() + "\n"));

        return;
    }

    QString username = params[0];
    QString password = params[1];

    //查询数据库
    QSqlQuery query(userinfo);
    query.prepare("SELECT password FROM userinfoTable WHERE username = ?");
    query.addBindValue(username);
    //数据库查询错误
    if (!query.exec()) {


        QByteArray block;
            QDataStream out( & block, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_14);
            out << qint64(1);
            out<<(("LOGIN_FAIL " + tr("数据库连接错误\n"))).toUtf8();
        client->write(block);


     //   client -> write(((("LOGIN_FAIL " + tr("数据库连接错误\n")).toStdString()).c_str()));
        ui -> textEdit -> append(("LOGIN_FAIL" + tr("数据库连接错误\n")) + clients[client]);
        return;
    }
    //未查询到账户信息
    if (!query.next()) {

        QByteArray block;
                QDataStream out( & block, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_5_14);
                out << qint64(1);
                out<<(("LOGIN_FAIL " + tr("未查询到你的账户信息，请重新输入\n"))).toUtf8();
            client->write(block);

  //      client -> write(((("LOGIN_FAIL " + tr("未查询到你的账户信息，请重新输入\n")).toStdString()).c_str()));
        ui -> textEdit -> append(("LOGIN_FAIL " + tr("未查询到账户信息\n")) + clients[client]);

        return;
    }

    QString storedPassword = query.value(0).toString();

    //密码正确，进入客户端聊天界面
    if (storedPassword == password) {
        clients[client] = username;

        ui -> textEdit -> append(clients[client] + tr("登录成功"));

        //这个要优化下，直接进入客户端聊天页面7.5已完成
        //返回给客户端登录成功信息
        groups[tr("公共聊天室")].append(client);

        QByteArray block;
        QDataStream out( & block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_14);
        out << qint64(1);
        out << ("LOGIN_SUCCESS " + username + "\n").toUtf8();
        qDebug()<<"发送loginsuccess的block";
        client -> write(block);

        // 发送在线和离线用户列表
        sendOnlineUsers(client);
        sendOfflineUsers(client);
        sendToAll("CONNECTED " + username);

    } else {
        //返回密码错误的结果

        QByteArray block;
            QDataStream out( & block, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_14);
            out << qint64(1);
            QString passwordError="LOGIN_FAIL 密码错误，请重新输入\n";
            out<<(passwordError).toUtf8();
        client->write(block);
//        client -> write("LOGIN_FAIL 密码错误，请重新输入\n");
    }

    //ps:groups是QHash<QString,QList<QTcpSocket*>>
    //ps:clients是QHash QTcpSocket* 和QString

    //新建socket对象恢复与群聊的连接


}

//处理群聊
void MainWindow::handleGroupChat(QTcpSocket * client,
    const QString & groupName,
        const QString & message) {
    QString sender = clients[client];
    if (sender.isEmpty()) {

        QByteArray block;
            QDataStream out( & block, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_14);
            out << qint64(1);
            out<<(("ERROR " + tr("Not logged in\n"))).toUtf8();
        client->write(block);


        //client -> write(("ERROR " + tr("Not logged in\n")).toUtf8());
        return;
    }

    QString fullMessage = sender + ": " + message;
    //发送至群聊中
    sendToGroup(groupName, fullMessage);

}


//处理私聊
void MainWindow::handlePrivateChat(QTcpSocket * client,
    const QString & recipient,
        const QString & message) {

    QString sender = clients[client];

    //防止出现未登录就弹出chatwindow的局面
    if (sender.isEmpty()) {
        QByteArray block;
                QDataStream out( & block, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_5_14);
                out << qint64(1);
                out<<(("ERROR" + tr(" Not logged in\n"))).toUtf8();
            client->write(block);

    //    client -> write(("ERROR" + tr(" Not logged in\n")).toUtf8());
        return;
    }

    //遍历在线用户列表
    qDebug("正在遍历在线用户列表");
    qDebug() << "发送者为" << sender << "对象为" << recipient;
    for (QTcpSocket * otherClient: clients.keys()) {
        if (clients[otherClient] == recipient) {
            QString fullMessage = "PRIVATE_FROM " + sender + " : " + message;
            sendToClient(otherClient, fullMessage);
            return;
        }
    }

    //若不成功则返回错误给客户端,没有找到对应的发送对象
    QByteArray block;
    QDataStream out( & block, QIODevice::WriteOnly);
     out.setVersion(QDataStream::Qt_5_14);
     out<<qint64(1);
     out<<("ERROR " + tr("对方当前不在线\n")).toUtf8();
     client->write(block);
//    client -> write(("ERROR " + tr("对方当前不在线\n")).toUtf8());

}


//处理群聊创建
void MainWindow::handleGroupRegister(QTcpSocket * client,
    const QString & groupName,
        const QStringList & members) {

    QSqlQuery groupQuery(groupinfo);
    // 检查群聊是否已经存在
    if (groups.contains(groupName)) {

        QByteArray block;
            QDataStream out( & block, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_14);
            out << qint64(1);
            out<<(("CREATE_GROUP_FAIL " + tr("group already exists\n"))).toUtf8();
        client->write(block);

        //client -> write(("CREATE_GROUP_FAIL " + tr("group already exists\n")).toUtf8());
        return;
    }

    QString memberList = members.join(" ");

    //memberName以字符串形式插入，"名字1 名字2 名字3..."
    //将群聊名与成员名单插入groupinfoTable中
    groupQuery.prepare("INSERT INTO groupMembersTable (groupName, memberName) VALUES (?, ?)");

    foreach(const QString & member, members) {
        groupQuery.addBindValue(groupName);
        groupQuery.addBindValue(member);
        //若创建不成功
        if (!groupQuery.exec()) {
            ui -> textEdit -> append(tr("创建群聊成员失败: ") + groupQuery.lastError().text());
            QByteArray block;
                    QDataStream out( & block, QIODevice::WriteOnly);
                    out.setVersion(QDataStream::Qt_5_14);
                    out << qint64(1);
                    out<<("CREATE_GROUP_FAIL " + groupQuery.lastError().text() + "\n").toUtf8();
                client->write(block);


//            client -> write(("CREATE_GROUP_FAIL " + groupQuery.lastError().text() + "\n").toUtf8());
            return;
        }
    }

    groups[groupName] = QList < QTcpSocket * > ();
    ui -> textEdit -> append("创建群聊 " + groupName);

    QByteArray block;
            QDataStream out( & block, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_14);
            out << qint64(1);
            out<<("CREATE_GROUP_SUCCESS " + groupName + " " + members.join(" ") + "\n").toUtf8();
        client->write(block);

    //client -> write(("CREATE_GROUP_SUCCESS " + groupName + " " + members.join(" ") + "\n").toUtf8());
    qDebug() << ("CREATE_GROUP_SUCCESS " + groupName + " " + members.join(" ") + "\n");
}



//加载聊天列表
void MainWindow::handleLoadContacts(QTcpSocket * client) {
    QString username = clients[client];
    if (username.isEmpty()) {
        qDebug("加载聊天列表，未登录，直接return");

        QByteArray block;
                QDataStream out( & block, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_5_14);
                out << qint64(1);
                out<<("ERROR " + tr("Not logged in\n")).toUtf8();
            client->write(block);
       // client -> write(("ERROR " + tr("Not logged in\n")).toUtf8());
        return;
    }



    qDebug("加载聊天列表能进入下一步状态");
    ui -> textEdit -> append("服务器加载聊天列表操作");
    QString contactList;
    QSqlQuery query(userinfo);
    query.exec("SELECT username FROM userinfoTable");
    while (query.next()) {
        QString contact = query.value(0).toString();
        if (contact != username) { // 不要把自己加入联系人列表
            contactList += contact + " ";
        }

    }

    //    query.exec("SELECT groupName FROM groupMembersTable");
    //    while (query.next()) {
    //        QString contact=query.value(0).toString();;
    //        contactList += contact+" ";


    //    }


    QByteArray block;
            QDataStream out( & block, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_14);
            out << qint64(1);
            out<<("CONTACTLIST " + contactList + "\n").toUtf8();
        client->write(block);

   // client -> write(("CONTACTLIST " + contactList + "\n").toUtf8());
    ui -> textEdit -> append(tr("已发送联系人列表给客户端"));
    qDebug() << "已发送联系人列表给客户端，contactList" << contactList;


    //发送在线用户列表给客户端，用于更新在线用户列表
    QHash < QTcpSocket * , QString > clients;

    sendOnlineUsers(client);
}

//发送到群聊
void MainWindow::sendToGroup(const QString & groupName,
    const QString & message) {
    if (!groups.contains(groupName)) return;

    for (QTcpSocket * member: groups[groupName]) {
        QString returnMessage = ("GROUP_MESSAGE_FROM " + groupName + " " + message + "\n");

        QByteArray block;
                QDataStream out( & block, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_5_14);
                out << qint64(1);
                out<<returnMessage.toUtf8();
            member->write(block);

        //member -> write(returnMessage.toUtf8());
    }



}


//发送给所有人
void MainWindow::sendToAll(const QString & message) {

    //遍历
    foreach(QTcpSocket * otherclient, clients.keys()) {


        QByteArray block;
                QDataStream out( & block, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_5_14);
                out << qint64(1);
                out<<(message + "\n").toUtf8();
            otherclient->write(block);

//        otherclient -> write(message.toUtf8() + "\n");
    }
}



void MainWindow::sendToClient(QTcpSocket * client,
    const QString & message) {



    QByteArray block;
            QDataStream out( & block, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_14);
            out << qint64(1);
            out<<(message + "\n").toUtf8();
        client->write(block);
//    client -> write(message.toUtf8() + "\n");

}

//返回给客户端在线用户名单
void MainWindow::sendOnlineUsers(QTcpSocket * client) {
    QString onlineUsers = "ONLINE_USER_LIST ";
    for (QTcpSocket * otherClient: clients.keys()) {
        if (otherClient -> state() == QAbstractSocket::ConnectedState) {
            onlineUsers += clients[otherClient] + " ";
        }
    }


    QByteArray block;
            QDataStream out( & block, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_14);
            out << qint64(1);
            out<<(onlineUsers+ "\n").toUtf8();
        client->write(block);

  //  client -> write(onlineUsers.toUtf8() + "\n");
    //返回"ONLINE_USERS 和带空格的在线用户名单"



}

//返回给客户端离线用户名单，"OFFLINE_USERS 和带空格的离线用户名单"
void MainWindow::sendOfflineUsers(QTcpSocket * client) {
    QString offlineUsers = "OFFLINE_USERS ";
    QSqlQuery query(userinfo);
    query.exec("SELECT username FROM userinfoTable");
    while (query.next()) {
        QString username = query.value(0).toString();
        if (!clients.values().contains(username)) {
            offlineUsers += username + " ";
        }
    }


    QByteArray block;
    QDataStream out( & block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_14);
    out << qint64(1);
    out<<(offlineUsers + "\n").toUtf8();
    client->write(block);
    //client -> write(offlineUsers.toUtf8() + "\n");

}

//将文件发送给客户端
void MainWindow::sendFileToClient(QTcpSocket *clientSocket, const QString &filePath, const QString &fileName, qint64 fileSize) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件进行读取:" << filePath;
        return;
    }

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_14);
    out << qint64(2);  // 2表示文件消息
    out << fileName;
    out << fileSize;

    clientSocket->write(block);

    const int bufferSize = 4096;
    char buffer[bufferSize];
    qint64 bytesRead = 0;

    while (!file.atEnd() && (bytesRead = file.read(buffer, bufferSize)) > 0) {
        clientSocket->write(buffer, bytesRead);
    }

    file.close();
    clientSocket->flush();
    qDebug()<<"已经将文件"<<fileName<<"发送给对象";
}
