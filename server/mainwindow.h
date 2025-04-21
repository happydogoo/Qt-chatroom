#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHash>
#include <publicheader.h>


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
    //槽函数
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    void setupDatebase();

    void handleRegister(QTcpSocket *client, const QStringList &params);
    void handleLogin(QTcpSocket *client, const QStringList &params);
    void handleGroupChat(QTcpSocket *client,const QString &groupName, const QString &message);
    void handlePrivateChat(QTcpSocket* client,const QString &recipient,const QString &message);
    void handleGroupRegister(QTcpSocket*client,const QString &groupName,const QStringList &members);
    void handleLoadContacts(QTcpSocket*client);
  //  void handleCreateGroup(QTcpSocket *clientSocket, const QString &groupName, const QStringList &members) {

    void sendFileToClient(QTcpSocket *clientSocket, const QString &filePath, const QString &fileName, qint64 fileSize) ;
    void sendToAll(const QString &message);
    void sendToClient(QTcpSocket *client, const QString &message);
    void sendToGroup(const QString &groupName, const QString &message);
    void sendOnlineUsers(QTcpSocket *client);
    void sendOfflineUsers(QTcpSocket *client);


    QHash<QTcpSocket*, QString> clients; // 存储连接的客户端连接和用户名的键值对

    QHash<QString,QList<QTcpSocket*>>groups;//废案，存储群聊名称和用户连接的键值对
    Ui::MainWindow *ui;

    QTcpServer* tcpServer;
    QSqlDatabase userinfo;


    QSqlDatabase groupinfo;




};
#endif // _H
