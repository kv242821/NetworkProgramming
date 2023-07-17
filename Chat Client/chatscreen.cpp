#include "chatscreen.h"
#include "ui_chatscreen.h"

ChatScreen::ChatScreen(QTcpSocket *socket,QTcpSocket *imageSocket,QString userName) :
    ui(new Ui::ChatScreen)
{
    ui->setupUi(this);

    ui->stackedWidget->setCurrentIndex(1);
    QPixmap pixmapApp;
    QIcon iconApp;
    pixmapApp.load(":/resources/images/appIcon.png");
    iconApp.addPixmap(pixmapApp);
    QWidget::setWindowIcon(iconApp);
    QWidget::setFixedSize(QSize(800,600));
    QWidget::setWindowTitle("Chat Application");

    this->socket = socket;
    this->imageSocket = imageSocket;
    this->userName = userName;

    systemTrayIcon = new QSystemTrayIcon(this);
    systemTrayIcon->setIcon(iconApp);
    systemTrayIcon->setVisible(true);
    QMenu *menu = new QMenu();
    QAction *quit = new QAction();
    quit->setText("Quit");
    menu->addAction(quit);
    systemTrayIcon->setContextMenu(menu);
    connect(menu,SIGNAL(triggered(QAction *)),this,SLOT(quit(QAction *)));
    connect(systemTrayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(activated(QSystemTrayIcon::ActivationReason)));
    connect(systemTrayIcon,SIGNAL(messageClicked()),this,SLOT(activate()));
    QApplication::setQuitOnLastWindowClosed(false);
    connect(this->socket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(stateChanged(QAbstractSocket::SocketState)));
}

ChatScreen::~ChatScreen()
{
    delete ui;
}

void ChatScreen::quit(QAction *action)
{
    QJsonObject jsonObject;
    jsonObject.insert("Type", EXIT);

    QJsonDocument jsonDoc;
    jsonDoc.setObject(jsonObject);
    QByteArray data = jsonDoc.toJson();

    if(this->socket->state() == QTcpSocket::ConnectedState) {
        socket->write(data);
        socket->waitForBytesWritten(30000);
    }
    disconnect(this->socket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(stateChanged(QAbstractSocket::SocketState)));
    QCoreApplication::quit();
}

void ChatScreen::activated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::ActivationReason::DoubleClick) this->show();
}

void ChatScreen::activate()
{
    this->show();
}

QJsonArray ChatScreen::encrypt(QString message)
{
    QJsonArray cipher;
    for(int j = 0 ; j < message.length() ; j++) {
        int result = 1;
        int letter = message.at(j).toLatin1();
        for(int i = 0 ; i < 17 ; i++) {
            result = (result*letter)%n;
        }
        cipher.push_back(result);
    }
    return cipher;
}

void ChatScreen::on_lineEdit_7_returnPressed()
{
    QString message = ui->lineEdit_7->text();
    if(!message.isEmpty()) {

        ui->textBrowser->append("You : " + message);
        ui->lineEdit_7->clear();

        QJsonArray cipherSender = encrypt(userName);
        QJsonArray cipherMessage = encrypt(message);

        QJsonObject jsonObject;
        jsonObject.insert("Type", CHAT_ALL);
        jsonObject.insert("Sender",cipherSender);
        jsonObject.insert("Message",cipherMessage);

        QJsonDocument jsonDoc;
        jsonDoc.setObject(jsonObject);
        QByteArray data = jsonDoc.toJson();

        if(this->socket->state() == QTcpSocket::ConnectedState) {
            socket->write(data);
        }
    }
}


void ChatScreen::on_pushButton_4_clicked()
{
    QJsonObject jsonObject;
    jsonObject.insert("Type", ACTIVE_CLIENTS);

    QJsonDocument jsonDoc;
    jsonDoc.setObject(jsonObject);
    QByteArray data = jsonDoc.toJson();

    if(this->socket->state() == QTcpSocket::ConnectedState) {
        socket->write(data);
    }

}

void ChatScreen::on_pushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void ChatScreen::on_pushButton_2_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void ChatScreen::on_tabWidget_tabCloseRequested(int index)
{
    int remove ;
    QMap<QString , PrivateChat *>::iterator iter = chatRooms.begin();
    if(iter != chatRooms.end()) {
        remove = ui->tabWidget->indexOf(iter.value());
        if(remove == index) {
            chatRooms.take(iter.key());
        }
    }
    ui->tabWidget->removeTab(index);

}

void ChatScreen::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    if(userName != item->text()) {
        PrivateChat *privateRoom = new PrivateChat(socket,userName,item->text());
        this->chatRooms.insert(item->text(),privateRoom);
        this->ui->tabWidget->insertTab(this->ui->tabWidget->count(),privateRoom,item->text());
    }
}

void ChatScreen::stateChanged(QAbstractSocket::SocketState newState)
{
    if(newState == QAbstractSocket::UnconnectedState) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("ERROR");
        msgBox.setText("Connection Lost");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
}
