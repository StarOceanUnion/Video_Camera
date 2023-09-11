#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QSpacerItem>
#include <QtEndian>
#include <QImage>
#include <QPainter>


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:

    QUdpSocket *udpSocket;    /* UDP套接字 */
    QTcpSocket *tcpSocket;    /* TCP套接字 */
    QTimer *timer;    /* 定时器 */

    QLabel *label[4];    /* 标签文本 */
    QLineEdit *lineEdit;    /* 用于输入IP */
    QSpinBox *spinBox;    /* 用于选择端口 */
    QComboBox *comboBox;    /* 用于选择协议类型 */
    QPushButton *pushButton[2];    /* 按钮 */

    QHBoxLayout *hBoxLayout;    /* 水平布局 */
    QVBoxLayout *vBoxLayout;    /* 垂直布局 */
    QWidget *hWidget;    /* 水平容器 */
    QWidget *vWidget;    /* 垂直容器 */

    int connectingFlag = 0;    /* 正在连接标志 */
    int reconnectFlag = 0;    /* 重连标志 */
    int udpConnectedFlag = 0;    /* UDP已连接标志 */

private slots:

    void toConnect();    /* 连接 */
    void connected();    /* 已连接 */

    void connectTimeOut();

    void toDisconnect();    /* 断开 */
    void disconnected();    /* 已断开 */

    void udpReceiveMessages();    /* udp接收消息 */
    void tcpReceiveMessages();    /* tcp接收消息 */
};

#endif // MAINWINDOW_H
