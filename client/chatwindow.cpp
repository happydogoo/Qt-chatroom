#include "chatwindow.h"
#include "ui_chatwindow.h"
#include <QInputDialog>
#include "publicheader.h"

#include <QTreeWidget>
#include <QDebug>
#include <QPaintEvent>
#include <QPainter>
#include <QMenu>
#include <QFile>
#include <QFileDialog>
#include <QDataStream>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDateTime>

//构造函数
ChatWindow::ChatWindow(QString clientName, QTcpSocket * socket, QWidget * parent): QMainWindow(parent),
    ui(new Ui::ChatWindow),
    socket(socket),
    contactListModel(new QStandardItemModel(this)),
    currentRecipient(""),
    clientName(clientName) {

        ui -> setupUi(this);
        ui -> treeWidget -> setHeaderHidden(true);
        //设置窗口名
        this -> setWindowTitle(clientName);

        //从资源库中获取logo
        QIcon windowIcon(":/images/logo.png");
        setWindowIcon(windowIcon);

        // 设置页面为透明，防止遮挡圆角！！！
        setAttribute(Qt::WA_TranslucentBackground);
        setWindowFlags(Qt::FramelessWindowHint);

        //主界面的样式表，设置圆角内容
        this->setStyleSheet(R"(
                QMainWindow#MainWindow {
                    border-radius: 20px;
                    background-color: #fff;
                    box-shadow: 15px 15px 10px rgba(33,45,58,0.3);
                }
            )");

        ui -> treeWidget -> setStyleSheet(treeWidgetStyle);

        // 初始化表情菜单
        emojiMenu = new QMenu(this);
        QStringList emojis = {"😀","😁","😂","🤣","😅","😃","😄","😆","😉","😊","😋","😎","😍","😘","🥺","😡","👻","🤔","🤪","🥰","🌹","🦈","🦖","🍁"};
        foreach(const QString & emoji, emojis) {
            QAction * emojiAction = new QAction(emoji, this);
            connect(emojiAction, & QAction::triggered, this, [this, emoji]() {addEmojiToInput(emoji);});
            emojiMenu -> addAction(emojiAction);
        }


        //槽函数与Debug信息
        connect(ui -> treeWidget, & QTreeWidget::itemClicked, this, & ChatWindow::on_treeWidget_itemClicked);
        connect(socket, & QTcpSocket::readyRead, this, & ChatWindow::on_readyRead);


        //不会成功，因为还没创建聊天窗口就连上了
        connect(socket, & QTcpSocket::connected, this, [this]() {
            qDebug() << "聊天客户端连接成功";
        });
        connect(socket, & QTcpSocket::disconnected, this, [this]() {
            qDebug() << "聊天客户端连接断开";
            //处理离线信息，向服务器发送信息，所有用户都能收到离线信息
            handleDisconnect();
        });
        connect(socket, QOverload < QAbstractSocket::SocketError > ::of( & QTcpSocket::error), this, [this](QAbstractSocket::SocketError socketError) {
            qDebug() << "聊天客户端网络错误:" << socketError;
        });

        //创建treewidget的在线用户和离线用户分支
        QTreeWidgetItem * onlineUsers = new QTreeWidgetItem(ui -> treeWidget);
        onlineUsers -> setText(0, tr("在线用户"));
        onlineUsers -> setData(0, Qt::UserRole, "onlineUsers");

        QTreeWidgetItem * offlineUsers = new QTreeWidgetItem(ui -> treeWidget);
        offlineUsers -> setText(0, tr("离线用户"));
        offlineUsers -> setData(0, Qt::UserRole, "offlineUsers");

        // 添加treeWidget的公共聊天室
        QTreeWidgetItem * publicChatRoom = new QTreeWidgetItem(ui -> treeWidget);
        publicChatRoom -> setText(0, tr("公共聊天室"));
        publicChatRoom -> setData(0, Qt::UserRole, "publicChatRoom");
        ui -> treeWidget -> addTopLevelItem(publicChatRoom);
        addChatPage("公共聊天室");

        qDebug("创建了chatwindow");
        //加载聊天列表
        loadContacts();
    }

ChatWindow::~ChatWindow() {
    delete ui;
}

//发送按钮槽函数，在对应的窗口能够发送给对应的对象
void ChatWindow::on_sendButton_clicked() {
    QString message = ui -> messageTextEdit -> toPlainText();
    if (message.isEmpty()) {
        return;

    }

    //使用QDataStream序列化信息
    QByteArray block;
    QDataStream out( & block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_14);
    out << qint64(1);

    QString formattedMessage = QString("<div class='message'>%1</div>").arg(tr("我 : ") + message);
    QString separator = "<div class='separator'></div>";

    //记得在对应位置将指针置空！
    if (currentRecipient.isEmpty()) {
        qDebug("没有发送对象，返回");
        return;
    }

    //处理私聊功能
    if (currentRecipient != "公共聊天室") {

        QString fullMessage = ("PRIVATE " + currentRecipient + " " + message)+ "\n";
        out << fullMessage.toUtf8();
        socket -> write(block);

        ui -> messageTextEdit -> clear();
        addChatPage(currentRecipient);
        QTextEdit * chatHistory = ui -> stackedWidget -> findChild < QTextEdit * > (currentRecipient + "_history");
        if (chatHistory) {
            chatHistory ->insertHtml(messageStyle + formattedMessage + separator);
        }
    }
    //聊天室信息
    else {
        QString fullMessage = ("CHATROOM " + message);
        out << fullMessage.toUtf8();

        socket -> write(block);
        ui -> messageTextEdit -> clear();
        // 显示发送的消息在公共聊天室中
    }

}

//若收到信息
void ChatWindow::on_readyRead() {


    QDataStream in (socket);
    in.setVersion(QDataStream::Qt_5_14);

    //要循环读取才能读取完。
    while (socket -> bytesAvailable() > 0) {
        qint64 packetType;
        in >> packetType;

        //处理文件
        if (packetType == 2) {

            QString fileName;
            qint64 fileSize;
            in>>fileName>>fileSize;
            qDebug()<<"客户端收到文件"<<fileName<<" 文件大小:"<<fileSize;

            QString separator = "<div class='separator'></div>";
            QTextEdit * chatHistory = ui -> stackedWidget -> findChild < QTextEdit * > (currentRecipient + "_history");

            qDebug()<<"将要在聊天记录中显示收到文件";
            if (chatHistory!=nullptr) {

                qDebug()<<"在聊天记录中显示收到文件";
                chatHistory -> insertHtml(messageStyle + "收到文件: "+fileName+" ，请在桌面查看"+separator);
            }
            else{
                qDebug()<<"由于没有创建chatPage所以不能看到收到文件信息";

            }

            //文件保存到桌面
            QString saveFilePath=QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)+"/"+fileName;
            QFile file(saveFilePath);
            if(!file.open(QIODevice::WriteOnly)){
                qDebug()<<"无法进行文件写入"<<saveFilePath;
                return;
            }

            //存储已接收的字节数
            qint64 bytesReceived=0;
            //缓存区
            const int bufferSize=4096;
            char buffer[bufferSize];
            //已接收的数据字节数小于文件大小
            while (bytesReceived < fileSize) {
                //读取的数据量为bufferSize和fileSize-bytesReceived中的较小的，确保读取不会超过文件剩余大小的数据
                qint64 bytesRead = in.device()->read(buffer, qMin(bufferSize, int(fileSize - bytesReceived)));
                if (bytesRead > 0) {
                    //将读取的数据写入文件，写入的数据量为bytesRead字节
                    file.write(buffer, bytesRead);
                    bytesReceived += bytesRead;
                }
            }

            file.close();
                        qDebug() << "文件接收完成：" << saveFilePath;

        }


    //文本信息
    else if(packetType==1){

        QByteArray messageData;
        in>>messageData;

        QString data = QString::fromUtf8(messageData).trimmed();
        qDebug() << "聊天客户端收到数据：" << data;
        //消息分割符
        QString separator = "<div class='separator'></div>";
        //加载聊天列表
        if (data.startsWith("CONTACTLIST")) {
            qDebug("进入加载聊天列表");
            QStringList contacts = data.split(" ");
            contacts.removeFirst();

            for(QString user:contacts){
                addChatPage(user);

            }

            ui -> treeWidget -> clear();
            QTreeWidgetItem * onlineUsers = new QTreeWidgetItem(ui -> treeWidget);
            onlineUsers -> setText(0, tr("在线用户"));
            onlineUsers -> setData(0, Qt::UserRole, "onlineUsers");
            QTreeWidgetItem * offlineUsers = new QTreeWidgetItem(ui -> treeWidget);
            offlineUsers -> setText(0, tr("离线用户"));
            offlineUsers -> setData(0, Qt::UserRole, "offlineUsers");

            for (const QString & contact: contacts) {
                QTreeWidgetItem * item = new QTreeWidgetItem(offlineUsers);
                addChatPage(contact);
                item -> setText(0, contact);

            }

            QTreeWidgetItem * publicChatRoom = new QTreeWidgetItem(ui -> treeWidget);
            publicChatRoom -> setText(0, tr("公共聊天室"));
            publicChatRoom -> setData(0, Qt::UserRole, "publicChatRoom");
            ui -> treeWidget -> addTopLevelItem(publicChatRoom);

            addChatPage("公共聊天室");

        }

        //处理离线用户
        else if (data.startsWith("OFFLINE_USERS")) {

            qDebug() << clientName << "接收离线用户";
            QStringList users = data.split(" ");
            users.removeFirst();
            QTreeWidgetItem * offlineUsers = findCategoryItem("offlineUsers");
            for (const QString & user: users) {
                QTreeWidgetItem * item = new QTreeWidgetItem(offlineUsers);
                item -> setText(0, user);
            }
        }

        //处理在线用户
        else if (data.startsWith("ONLINE_USERS")) {
            qDebug() << clientName << "接收在线用户";
            QStringList users = data.split(" ");
            users.removeFirst();
            QTreeWidgetItem * onlineUsers = findCategoryItem("onlineUsers");
            for ( QString  user: users) {
                QTreeWidgetItem * item = new QTreeWidgetItem(onlineUsers);
                item -> setText(0, user);
                //在处理在线用户时就创建聊天页，防止未创建聊天页的情况发生
                qDebug()<<"创建了"<<user<<"聊天页面";
                addChatPage(user);

            }
        }

        //接收私聊
        else if (data.startsWith("PRIVATE_FROM")) {
            QStringList tempData = data.split(" ");
            tempData.removeFirst();
            QString sender = tempData.takeFirst();
            QString message = tempData.join(" ");
            qDebug() << "进入处理私聊信息" << sender << " :" << message;
            //message格式为 "sender: 信息"


            addChatPage(sender);

            QTextEdit * chatHistory = ui -> stackedWidget -> findChild < QTextEdit * > (sender + "_history");
            if (chatHistory) {
                qDebug() << "在聊天页中显示" << sender << "的信息";
                chatHistory -> insertHtml(messageStyle + QString("<div class='message'>%1 %2</div>").arg(sender, message) +separator);
            }
        }

        //处理在线用户
        else if (data.startsWith("CONNECTED")) {
            QStringList tempData = data.split(" ");
            tempData.removeFirst();
            QString connectedUser = tempData.takeFirst();

            //将连接的用户显示出来
            qDebug() << "用户上线：" << connectedUser;
            QTreeWidgetItem * onlineUsers = findCategoryItem("onlineUsers");
            QTreeWidgetItem * offlineUsers = findCategoryItem("offlineUsers");

            //新注册不会添加到离线用户表中，想想怎么改。7.8修改完毕
            //2024/7/9修改完毕，注册后服务器端sendToAll("NEW_REGISTER "+username);
            //客户端收到后添加到离线窗口
            if (offlineUsers) {
                for (int i = 0; i < offlineUsers -> childCount(); ++i) {
                    QTreeWidgetItem * item = offlineUsers -> child(i);
                    if (item -> text(0) == connectedUser) {
                        // 移动到在线用户
                        QTreeWidgetItem * onlineItem = new QTreeWidgetItem(onlineUsers);
                        onlineItem -> setText(0, connectedUser);
                        delete item;
                        break;
                    }
                }
            }
        }

        //处理离线信息，将离线的用户归入离线列表
        else if (data.startsWith("DISCONNECTED")) {
            QStringList tempData = data.split(" ");
            tempData.removeFirst();
            QString disconnectedUser = tempData.takeFirst();
            qDebug() << "用户离线:" << disconnectedUser;
            QTreeWidgetItem * onlineUsers = findCategoryItem("onlineUsers");
            QTreeWidgetItem * offlineUsers = findCategoryItem("offlineUsers");

            if (onlineUsers) {
                bool moved = false;
                for (int i = 0; i < onlineUsers -> childCount(); ++i) {
                    QTreeWidgetItem * item = onlineUsers -> child(i);
                    if (item -> text(0) == disconnectedUser) {
                        // 移动到离线用户
                        qDebug() << disconnectedUser << "移动到离线用户";
                        QTreeWidgetItem * offlineItem = new QTreeWidgetItem(offlineUsers);
                        offlineItem -> setText(0, disconnectedUser);
                        delete item;
                        moved = true;
                        break;
                    }
                }

                if (!moved) {
                    qDebug() << "未找到用户：" << disconnectedUser << "，无法移动到离线用户";
                }
            } else {
                if (!onlineUsers) {
                    qDebug() << "未找到在线用户列表";
                }
                if (!offlineUsers) {
                    qDebug() << "未找到离线用户列表";
                }


            }
        }
        //客户端返回的是"ONLINE_USER_LIST "+onlineUserlist+"\n"
        else if (data.startsWith("ONLINE_USER_LIST")) {
            QStringList tempData = data.split(" ");
            tempData.removeFirst();
            //获取QTreeWidget的顶层分类，在线用户和离线用户
            QTreeWidgetItem * onlineUsers = findCategoryItem("onlineUsers");
            QTreeWidgetItem * offlineUsers = findCategoryItem("offlineUsers");
            if (onlineUsers) {

                //遍历在线成员，将每个离线成员移动到在线列表
                for (int j = 0; j < tempData.count(); j++) {
                    addChatPage(tempData.at(j));
                    //遍历离线成员列表
                    for (int i = 0; i < offlineUsers -> childCount(); i++) {
                        QTreeWidgetItem * item = offlineUsers -> child(i);
                        if (item -> text(0) == tempData.at(j)) {

                            QTreeWidgetItem * onlineItem = new QTreeWidgetItem(onlineUsers);
                            onlineItem -> setText(0, tempData.at(j));
                            delete item;
                            qDebug()<<"创建了"<<tempData.at(j)<<"聊天页面";

                            addChatPage(tempData.at(j));

                            break;
                        }
                    }
                }
            }


        }
        //处理聊天室信息
        else if (data.startsWith("CHATROOM_MESSAGE")) {

            QStringList tempData = data.split(" ");
            tempData.removeFirst();
            QString sender = tempData.takeFirst();
            QString message = tempData.join(" ");
            qDebug() << "公共聊天室消息：" << sender << " :" << message;
            addChatPage("公共聊天室");
            QString formattedMessage = QString("<div class='message'>%1 : %2</div>").arg(sender, message);
            QTextEdit * chatHistory = ui -> stackedWidget -> findChild < QTextEdit * > ("公共聊天室_history");
            if (chatHistory) {
                chatHistory -> insertHtml(messageStyle + formattedMessage +separator);

            }

        }
        //处理注册信息
        else if (data.startsWith("NEW_REGISTER")) {
            QStringList tempData = data.split(" ");
            tempData.removeFirst();
            QString newUser = tempData.takeFirst();

            QTreeWidgetItem * onlineUsers = findCategoryItem("onlineUsers");
            QTreeWidgetItem * offlineUsers = findCategoryItem("offlineUsers");

            QTreeWidgetItem * offlineItem = new QTreeWidgetItem(offlineUsers);
            offlineItem -> setText(0, newUser);

        }
    }}
}




//传入一个const QModelIndex &index，用于切换当前的stackedWidget窗口（聊天窗口），
void ChatWindow::on_contactListView_clicked(const QModelIndex & index) {
    currentRecipient = (tr("公共聊天室") + (contactListModel -> itemFromIndex(index) -> text()));

    addChatPage(currentRecipient);
    ui -> stackedWidget -> setCurrentWidget(ui -> stackedWidget -> findChild < QWidget * > (currentRecipient + "_page"));

}

//添加聊天页面，若不存在聊天页面则建立一个
void ChatWindow::addChatPage(const QString & chatPageName) {
    if (!ui -> stackedWidget -> findChild < QWidget * > (chatPageName + "_page")) {

        QWidget * chatPage = new QWidget;
        chatPage -> setObjectName(chatPageName + "_page");
        QVBoxLayout * layout = new QVBoxLayout(chatPage);
        QTextEdit * chatHistory = new QTextEdit;

        chatHistory -> setObjectName(chatPageName + "_history");
        //设置为只读
        chatHistory -> setReadOnly(true);
        layout -> addWidget(chatHistory);
        ui -> stackedWidget -> addWidget(chatPage);
        qDebug()<<"创建了"<<chatPageName<<"聊天页面";
    }
}


//根据字符串查找对应的指针
QTreeWidgetItem * ChatWindow::findCategoryItem(const QString & category) {
    for (int i = 0; i < ui -> treeWidget -> topLevelItemCount(); ++i) {
        QTreeWidgetItem * item = ui -> treeWidget -> topLevelItem(i);
        if (item -> data(0, Qt::UserRole) == category) {
            return item;
        }
    }
    return nullptr;
}

//处理用户下线信息,将其移到离线用户栏
void ChatWindow::handleDisconnect() {
    qDebug() << "处理断开连接";

    // 获取在线用户和离线用户的节点
    QTreeWidgetItem * onlineUsers = findCategoryItem("onlineUsers");
    QTreeWidgetItem * offlineUsers = findCategoryItem("offlineUsers");

    // 将所有在线用户移动到离线用户
    if (onlineUsers && offlineUsers) {
        while (onlineUsers -> childCount() > 0) {
            QTreeWidgetItem * item = onlineUsers -> takeChild(0);
            QTreeWidgetItem * offlineItem = new QTreeWidgetItem(offlineUsers);
            offlineItem -> setText(0, item -> text(0));

            delete item; // 删除原来的在线用户项
        }
    }


    if (socket) {
        socket -> disconnectFromHost();
        if (socket -> state() == QAbstractSocket::UnconnectedState || socket -> waitForDisconnected(1000)) {
            delete socket;
            socket = nullptr;
        }
    }

    qApp -> quit();
}



//添加私聊到聊天列表项,
void ChatWindow::addContactItem(const QString & contact) {

    QTreeWidgetItem * contactItem = new QTreeWidgetItem(ui -> treeWidget, QStringList(contact));
    ui -> treeWidget -> addTopLevelItem(contactItem);
}

//点击对应联系人时，会切换到对应聊天页
void ChatWindow::on_treeWidget_itemClicked(QTreeWidgetItem * item, int column) {


    QString recipient = item -> text(column);
    currentRecipient = recipient;
    addChatPage(recipient);
    ui -> stackedWidget -> setCurrentWidget(ui -> stackedWidget -> findChild < QWidget * > (recipient + "_page"));

}

// 向服务器发送请求加载联系人,在构造函数里面调用了！
void ChatWindow::loadContacts() {

    // 发送请求以获取联系人列表
    QByteArray block;
    QDataStream out( & block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_14);
    out << qint64(1);

    qDebug("向服务器发送了GET_CONTACT_LIST");

    QString message = "GET_CONTACT_LIST \n";
    out << message.toUtf8();
    socket -> write(block);

}

//处理界面函数,移动窗口等
void ChatWindow::mousePressEvent(QMouseEvent * event) {
    if (event -> button() == Qt::LeftButton) {
        m_dragPosition = event -> globalPos() - frameGeometry().topLeft();
        event -> accept();
    }
}

void ChatWindow::mouseMoveEvent(QMouseEvent * event) {
    if (event -> buttons() & Qt::LeftButton) {
        move(event -> globalPos() - m_dragPosition);
        event -> accept();
    }
}

//点击表情按钮,弹出表情菜单
void ChatWindow::on_faceButton_clicked() {

    emojiMenu -> exec(QCursor::pos());

}
//将emoji表情添加至输入框
void ChatWindow::addEmojiToInput(const QString & emoji) {
    ui -> messageTextEdit -> insertPlainText(emoji);
}
//画出圆角界面
void ChatWindow::paintEvent(QPaintEvent * event) {

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 设置窗口背景为透明
    QBrush brush(Qt::white);
    painter.setBrush(brush);
    painter.setPen(Qt::transparent);

    // 定义绘制区域，并减去一像素避免绘制出界
    QRect rect = this -> rect();
    rect.setWidth(rect.width() - 1);
    rect.setHeight(rect.height() - 1);


    // 绘制圆角矩形
    painter.drawRoundedRect(rect, 30, 30);

    // 填充边角区域，使其与背景色一致
    QRegion region(rect);
    QRegion roundedRectRegion(rect, QRegion::Rectangle);
    QRegion cornerRegion = region.subtracted(roundedRectRegion);
    painter.setClipRegion(cornerRegion);
    painter.setBrush(QColor(255, 255, 255, 0)); // 设置透明色
    painter.drawRect(rect);
}

//发送文件按钮槽函数
void ChatWindow::on_fileButton_clicked()

{
    QString fileName = QFileDialog::getOpenFileName(this, tr("选择要发送的文件"), "", tr("所有文件 (*.*)"));
    if (fileName.isEmpty()) {
        return;
    }

    sendFile(fileName);
}


//发送文件
void ChatWindow::sendFile(const QString & fileName) {

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("无法打开文件"), tr("无法打开文件 %1").arg(fileName));
        return;
    }
    //获取文件信息
    QFileInfo fileInfo(file.fileName());
    QString fileBaseName = currentRecipient+"_"+fileInfo.fileName();
    qint64 fileSize = file.size();

    //以QByteArray传输
    QByteArray block;
    QDataStream out( & block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_14);

    //！！用QDataStream来序列化数据

    //2表示文件,1表示文本
    out << qint64(2);

    //文件大小
    out << fileBaseName ;
    out<< fileSize;
    socket -> write(block);

    qDebug() << "客户端发送文件:  " <<fileName;

    //循环每次读取4KB的缓存大小
    const int bufferSize = 4096;
    char buffer[bufferSize];
    qint64 bytesRead = 0;

    //若文件没结束，且读取到的字节数大于0
    while (!file.atEnd() && (bytesRead = file.read(buffer, bufferSize)) > 0) {
        //bytesRead既实际读取的字节数
        socket -> write(buffer, bytesRead);
    }

    file.close();
    socket -> flush();

    QString separator = "<div class='separator'></div>";
    addChatPage(currentRecipient);
    QTextEdit * chatHistory = ui -> stackedWidget -> findChild < QTextEdit * > (currentRecipient + "_history");
    if (chatHistory) {
        chatHistory -> insertHtml(messageStyle +"已发送文件"+fileName+ separator);
    }

}
