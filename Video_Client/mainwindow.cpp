#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    udpSocket = new QUdpSocket;             /* UDP套接字 */
    tcpSocket = new QTcpSocket;             /* TCP套接字 */

    label[0] = new QLabel;                  /* 标签文本 */
    label[1] = new QLabel;
    label[2] = new QLabel;
    label[3] = new QLabel;
    lineEdit = new QLineEdit;               /* 用于输入IP */
    spinBox = new QSpinBox;                 /* 用于选择端口 */
    comboBox = new QComboBox;               /* 用于选择协议类型 */
    pushButton[0] = new QPushButton;        /* 按钮 */
    pushButton[1] = new QPushButton;

    hBoxLayout = new QHBoxLayout;           /* 水平布局 */
    vBoxLayout = new QVBoxLayout;           /* 垂直布局 */
    hWidget = new QWidget;                  /* 水平容器 */
    vWidget = new QWidget;                  /* 垂直容器 */

    label[0]->setText("视频监控客户端\n");    /* 填充文本 */
    label[0]->setStyleSheet("background:rgb(255,255,255); font:60px; color:rgb(200,200,200)");
    label[0]->setAlignment(Qt::AlignCenter);
    label[1]->setText("IP地址:");
    lineEdit->setText("192.168.1.1");
    label[2]->setText(" 端口:");
    spinBox->setRange(0, 65535);
    spinBox->setValue(2000);
    label[3]->setText(" 协议:");
    comboBox->addItem("UDP");
    comboBox->addItem("TCP");
    pushButton[0]->setText("连接");
    pushButton[1]->setText("断开");
    pushButton[1]->setEnabled(false);

    label[0]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);    /* 设置控件大小策略 */
    label[0]->setMinimumSize(640, 480);
    label[1]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lineEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label[2]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    spinBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label[3]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    comboBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    pushButton[0]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    pushButton[1]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    hBoxLayout->addWidget(label[1]);        /* 设置布局 */
    hBoxLayout->addWidget(lineEdit);
    hBoxLayout->addWidget(label[2]);
    hBoxLayout->addWidget(spinBox);
    hBoxLayout->addWidget(label[3]);
    hBoxLayout->addWidget(comboBox);
    hBoxLayout->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
    hBoxLayout->addWidget(pushButton[0]);
    hBoxLayout->addWidget(pushButton[1]);
    hWidget->setLayout(hBoxLayout);
    vBoxLayout->addWidget(hWidget);
    vBoxLayout->addWidget(label[0]);
    vWidget->setLayout(vBoxLayout);

    setCentralWidget(vWidget);              /* 居中显示 */

    connect(pushButton[0], SIGNAL(clicked()), this, SLOT(toConnect()));    /* 连接信号与槽 */
    connect(pushButton[1], SIGNAL(clicked()), this, SLOT(toDisconnect()));
    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(udpReceiveMessages()));
    connect(tcpSocket, SIGNAL(connected()), this, SLOT(connected()));
    connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(tcpReceiveMessages()));
}


MainWindow::~MainWindow()
{
}

/**
 * 描述：连接事件检测
 * 参数：无
 * 返回值：无
 * */
void MainWindow::toConnect()
{
    if (comboBox->currentText() == "UDP") {
        udpSocket->writeDatagram("Connect", QHostAddress(lineEdit->text()), spinBox->value());
    }
    else {
        tcpSocket->connectToHost(lineEdit->text(), spinBox->value());
    }
    connectingFlag = 1;
    QTimer::singleShot(1000, this, SLOT(connectTimeOut()));    /* 连接超时时间1秒 */
}
/**
 * 描述：已连接，并令其他按钮无法触发
 * 参数：无
 * 返回值：无
 * */
void MainWindow::connected()
{
    connectingFlag = 0;
    reconnectFlag = 1;
    lineEdit->setEnabled(false);
    spinBox->setEnabled(false);
    comboBox->setEnabled(false);
    pushButton[0]->setEnabled(false);
    pushButton[1]->setEnabled(true);
}
/**
 * 描述：连接超时检测
 * 参数：无
 * 返回值：无
 * */
void MainWindow::connectTimeOut()
{
    if (connectingFlag == 1) {
        if (comboBox->currentText() == "TCP") {
            tcpSocket->disconnectFromHost();
        }
        connectingFlag = 0;
    }
}
/**
 * 描述：取消连接事件检测
 * 参数：无
 * 返回值：无
 * */
void MainWindow::toDisconnect()
{
    if (comboBox->currentText() == "UDP") {
        udpSocket->writeDatagram("Disconnect", QHostAddress(lineEdit->text()), spinBox->value());
        disconnected();
        udpConnectedFlag = 0;
    }
    else {
        tcpSocket->disconnectFromHost();
    }
}

/**
 * 描述：取消链接，令其他按钮可触发
 * 参数：无
 * 返回值：无
 * */
void MainWindow::disconnected()
{
    lineEdit->setEnabled(true);
    spinBox->setEnabled(true);
    comboBox->setEnabled(true);
    pushButton[0]->setEnabled(true);
    pushButton[1]->setEnabled(false);
}

/**
 * 描述：YUYV（即YUV422格式数据包）转RGB功能
 * 参数：图像数据，宽度，高度
 * 返回值：QImage类型的RGB数据
 * */
QImage yuyvToRgb(const QByteArray& yuyvData, int width, int height) {
    QImage rgbImage(width, height, QImage::Format_RGB888);
    unsigned char* yuyvDataPtr = (unsigned char*)yuyvData.data();
    unsigned char* rgbImageData = rgbImage.bits();

    /*码流Y0 U0 Y1 V1 Y2 U2 Y3 V3 -->YUYV像素[Y0 U0 V1] [Y1 U0 V1] [Y2 U2 V3] [Y3 U2 V3]-->RGB像素*/
    int r1, g1, b1;
    int r2, g2, b2;
    int i;

    for (i = 0; i < width * height / 2; i++) {
        unsigned char Y0 = yuyvDataPtr[i * 4];
        unsigned char U0 = yuyvDataPtr[i * 4 + 1];
        unsigned char Y1 = yuyvDataPtr[i * 4 + 2];
        unsigned char V1 = yuyvDataPtr[i * 4 + 3];

        r1 = Y0 + 1.4075 * (V1 - 128);
        if(r1 > 255) r1 = 255;
        if(r1 < 0) r1 = 0;

        g1 = Y0 - 0.3455 * (U0 - 128) - 0.7169 * (V1 - 128);
        if(g1 > 255) g1 = 255;
        if(g1 < 0) g1 = 0;

        b1 = Y0 + 1.779 * (U0 - 128);
        if(b1 > 255) b1 = 255;
        if(b1 < 0) b1 = 0;

        r2 = Y1 + 1.4075 * (V1 - 128);
        if(r2 > 255) r2 = 255;
        if(r2 < 0) r2 = 0;

        g2 = Y1 - 0.3455 * (U0 - 128) - 0.7169 * (V1 - 128);
        if(g2 > 255) g2 = 255;
        if(g2 < 0) g2 = 0;

        b2 = Y1 + 1.779 * (U0 - 128);
        if(b2 > 255) b2 = 255;
        if(b2 < 0) b2 = 0;

        rgbImageData[i * 6] = r1;
        rgbImageData[i * 6 + 1] = g1;
        rgbImageData[i * 6 + 2] = b1;
        rgbImageData[i * 6 + 3] = r2;
        rgbImageData[i * 6 + 4] = g2;
        rgbImageData[i * 6 + 5] = b2;
    }

    return rgbImage;
}
/**
 * 描述：TCP接收消息功能
 * 参数：无
 * 返回值：无
 * */
void MainWindow::tcpReceiveMessages()
{
    long ret;
    static QByteArray frame;
    static uint frameBytes;
    static uint readBytes = 0;
    static int readFinish = 1;

    if (reconnectFlag == 1) {
        readBytes = 0;
        readFinish = 1;
        reconnectFlag = 0;
    }

    while (tcpSocket->atEnd() == false) {                       /* 如果有可读取的数据 */
        if (readFinish == 1) {                                  /*上一帧图像的内容已读取完成 */
            ret = tcpSocket->read((char *)&frameBytes + readBytes, 4 - readBytes);    /* 读取本帧图像的字节数 */
            if (ret <= 0)                                       /*读取出错，将重新读取*/
                continue;
            readBytes += ret;
            if (readBytes < 4)                                  /* 长度不足，将继续读取 */
                continue;

            frameBytes = qFromBigEndian<uint>(frameBytes);      /* 调整字节序 */
            frame.resize(frameBytes);                           /*本帧图像的字节数已读取完成，为frame重设大小*/
            qDebug("frameBytes: %u", frameBytes);

            readBytes = 0;
            readFinish = 0;                                     /*即将重新读取本帧图像的内容*/
        }
        else {                                                  /*本帧图像的字节数已读取完成*/
            ret = tcpSocket->read(frame.data() + readBytes, frameBytes - readBytes);    /* 读取本帧图像的内容 */
            if (ret <= 0)                                       /*读取出错，将重新读取*/
                continue;
            readBytes += ret;
            if (readBytes < frameBytes)                         /*长度不足，将继续读取*/
                continue;

            QImage image = yuyvToRgb(frame, 640, 480);          /*调用格式转换函数*/
            QPixmap pixmap = QPixmap::fromImage(image);         /*将格式转换后的图像映射*/
            pixmap = pixmap.scaled(label[0]->size(), Qt::KeepAspectRatio,Qt::SmoothTransformation);

            QPainter painter(&pixmap);
            painter.drawImage(0, 0, image);

            label[0]->setScaledContents(true);
            label[0]->setPixmap(QPixmap::fromImage(image));

            readBytes = 0;
            readFinish = 1;                                     /*即将读取下一帧图像的字节数*/
        }
    }
}
/**
 *描述：UDP接收消息功能
 *参数：无
 *返回值：无
 */
void MainWindow::udpReceiveMessages()
{
    int ret;
    static QByteArray data;
    static int dataBytes;
    static QByteArray frame;
    static uint frameBytes;
    static uint readBytes;
    static int readFinish = 1;

    if (reconnectFlag == 1) {
        readFinish = 1;
        reconnectFlag = 0;
    }

    while (udpSocket->hasPendingDatagrams()) {
        dataBytes = udpSocket->pendingDatagramSize();
        qDebug("dataBytes: %d", dataBytes);
        data.resize(dataBytes);
        ret = udpSocket->readDatagram(data.data(), dataBytes);      /*读取本帧图像的字节数 */
        if (ret <= 0)
            continue;
        /*判断UDP连接状态*/
        if (udpConnectedFlag == 0 && connectingFlag == 1 && dataBytes == 10 && (QString)data.data() == "Connected") {
            connected();
            udpConnectedFlag = 1;
        }
        else if (udpConnectedFlag == 1) {
            if (readFinish == 1) {
                if (dataBytes == 6 && (uchar)data[0] == 0xA5 && (uchar)data[1] == 0x5A) {
                    frameBytes = qFromBigEndian<uint>(data.data() + 2);
                    qDebug("frameBytes: %u", frameBytes);
                    frame.resize(0);
                    readBytes = 0;
                    readFinish = 0;
                }
            }
            else {
                    if (readBytes < frameBytes) {
                        frame += data;
                        readBytes += dataBytes;
                    }
                    if (readBytes >= frameBytes) {
                        QImage image = yuyvToRgb(frame, 640, 480);
                        QPixmap pixmap = QPixmap::fromImage(image); /*调用格式转换函数*/
                        pixmap = pixmap.scaled(label[0]->size(), Qt::KeepAspectRatio,Qt::SmoothTransformation);
                        QPainter painter(&pixmap);
                        painter.drawImage(0, 0, image);
                        label[0]->setScaledContents(true);
                        label[0]->setPixmap(QPixmap::fromImage(image));
                        readFinish = 1;
                    }
                }
        }
    }
}
