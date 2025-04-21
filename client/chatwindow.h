#ifndef CHATWINDOW_H
#define CHATWINDOW_H


#include <QMainWindow>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QTcpServer>
#include <QWidget>
#include <QFile>
#include <QStandardItemModel>
#include <QStackedWidget>
#include <QTreeWidget>
#include "publicheader.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ChatWindow; }
QT_END_NAMESPACE

class ChatWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChatWindow(QString clientName,QTcpSocket* socket,QWidget *parent = nullptr);
    ~ChatWindow();


    void processPrivateMessage(QString sender,QString message);
    void addChatPage(const QString &recipient);
    void addContactItem(const QString &contact);
    QTreeWidgetItem* findCategoryItem(const QString &category) ;

private slots:

    //注意！槽函数不要重复连接，否则会多次触发
    void on_sendButton_clicked();
    void on_contactListView_clicked(const QModelIndex &index);
    void on_readyRead();
    void on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);
    void on_faceButton_clicked();
    void on_fileButton_clicked();
private:
    Ui::ChatWindow *ui;
    QTcpSocket *socket;
    QStandardItemModel *contactListModel;
    QString currentRecipient;
    QString clientName;
    QMenu* emojiMenu;
    QPoint m_dragPosition;
    QFile* currentFile=nullptr;
    QString fileName;
    qint64 fileSize=0;

    qint64 bytesReceived=0;
    qint64 totalBytes=0;

    void loadContacts();
    void handleDisconnect();
    void sendFile(const QString &fileName);
    void addEmojiToInput(const QString &emoji);
    void appendMessage(const QString &sender, const QString &message) ;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    void paintEvent(QPaintEvent *event) override;








    //定义样式
    //联系人列表
    QString treeWidgetStyle = R"(
        QTreeWidget {
           background-color: #fff;
           font-family: "Microsoft YaHei", Helvetica, sans-serif;
           font-size: 20px;
        }
        QTreeWidget::item {
            background-color: #fff;
            color: rgba(0,0,0,0.9);
            padding: 0 20px;
            min-height: 65px;
            display: flex;
            justify-content: space-between;
            align-items: center;
               user-select: none;
               -webkit-tap-highlight-color: transparent;
               box-sizing: border-box;
               transition: all .6s ease-in-out 0s;
               transform: translateX(0px);
               position: relative;
               border-bottom: 1px solid #e0e0e0;
           }
           QTreeWidget::item:selected {
               background-color: #fff; /* 保持原色不变 */
               color: rgba(0,0,0,0.9);
           }
           QTreeWidget::item:selected:active {
               background-color: #fff;
           }
           QTreeWidget::item:hover {
               background-color: #f7f7f7;
           }
       )";


    //信息列表
    QString messageStyle = R"(
        <style>
            .message {font-size: 31px;font-family:"Microsoft YaHei Light"}
        </style>
    )";


};


#endif // CHATWINDOW_H
