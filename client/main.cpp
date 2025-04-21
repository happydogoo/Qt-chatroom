#include "publicheader.h"
#include "mainwindow.h"
#include <QMessageBox>

//备注:在不同主机上运行服务器要在客户端的chatwindow.cpp文件切换至对应的ip地址。
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow loginWindow;
    loginWindow.show();


    return a.exec();
}
