#include "widget.h"
#include "qapplication.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    qDebug() << QSqlDatabase::drivers();

    pixmapApp.load(":/images/appIcon.png");
    iconApp.addPixmap(pixmapApp);

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

    QWidget::setWindowIcon(iconApp);
    QWidget::setFixedSize(QSize(480,360));
    QWidget::setWindowTitle("Chat Server");

    this->db = QSqlDatabase::addDatabase("QMYSQL");
    this->db.setHostName("localhost");
    this->db.setPort(3306);
    this->db.setDatabaseName("test");
    this->db.setUserName("root");
    this->db.setPassword("soxt86fe");
    if(this->db.open()){
        qDebug("DB: connected");
    } else {
        qDebug("DB: Error connect");
    }


    ui->stackedWidget->setCurrentIndex(0);

    connect(&timer,SIGNAL(timeout()),this,SLOT(changeColor()));
    color = "White";
    activeUsers = 0;
    ui->lcdNumber->display(activeUsers);

}

Widget::~Widget()
{
    delete ui;
}


void Widget::on_startServerButton_clicked()
{
    bool status = this->server.listen(QHostAddress("127.0.0.1"),5500);
    bool status2 = imageServer.listen(QHostAddress("127.0.0.1"),5501);

    if(status && status2) {
        timer.start(1000);
        connect(&this->server,SIGNAL(newConnection()),this,SLOT(newClient()));
        connect(&imageServer,SIGNAL(newConnection()),this,SLOT(imageClient()));
    }
}


void Widget::newClient()
{
    QTcpSocket *socket = server.nextPendingConnection();
    connect(socket,SIGNAL(readyRead()),this,SLOT(receiveMessage()));
}

void Widget::quit(QAction *action)
{
    QCoreApplication::quit();
}

void Widget::sendImage()
{
    QTcpSocket *socket = static_cast<QTcpSocket *>(sender());
    QByteArray data = socket->readAll();

    QJsonDocument document = QJsonDocument::fromJson(data);
    QJsonObject object = document.object();
    QJsonArray cipherWhoseInfo = object.value("WhoseInfo").toArray();
    QString whoseInfo = decrypt(cipherWhoseInfo);

    QSqlQuery query(this->db);
    query.exec("select image from chatapp where username ='"+whoseInfo+"'");
    query.next();
    QByteArray imagev2 = query.value(0).toByteArray();

    imagev2.append("*#%END%#*");
    socket->write(imagev2);

}

void Widget::imageClient()
{
    QTcpSocket *socket = imageServer.nextPendingConnection();
    connect(socket,SIGNAL(readyRead()),this,SLOT(sendImage()));
}

void Widget::receiveMessage() {

     QTcpSocket *socket = static_cast<QTcpSocket *>(sender());

     QByteArray receivedData = socket->readAll();
     QJsonDocument receivedDocument = QJsonDocument::fromJson(receivedData);
     QJsonObject receivedObject = receivedDocument.object();

     QJsonObject sentObject;
     QJsonDocument sentDoc;

     int type = receivedObject.value("Type").toInt();

     if(type == LOGIN) {

        QJsonArray cipherUserName = receivedObject.value("UserName").toArray();
        QJsonArray cipherPassword = receivedObject.value("Password").toArray();

        QString receivedUsername = decrypt(cipherUserName);
        QString receivedpassword = decrypt(cipherPassword);

        QSqlQuery query(this->db);
        query.prepare("select * from chatapp where username=:receivedUsername");
        query.bindValue(":receivedUsername",receivedUsername);
        query.exec();
        int size = query.size();
        if(size > 0){
           query.next();
               if(!(query.value("password").toString() == receivedpassword)){
                   QJsonObject jsonObject;
                   jsonObject.insert("Type", INVALID_PASSWORD);
                   QJsonDocument jsonDoc;
                   jsonDoc.setObject(jsonObject);
                   QByteArray data = jsonDoc.toJson();
                   socket->write(data);
                }
               else{
                   QString username = receivedUsername;
                   QTcpSocket *client = socket;
                   struct Client user;
                   user.username = username;
                   user.loginTime = QTime::currentTime().toString("hh:mm");
                   user.loginDate = QDate::currentDate().toString("dd:MM:yy");
                   connectedClients.insert(client,user);
                   ui->textBrowser->setTextColor(Qt::darkGreen);
                   ui->textBrowser->append(username + " has logged in.");
                   activeUsers++;
                   ui->lcdNumber->display(activeUsers);

                   sentObject.insert("Type", SUCCESS_LOGIN);

                   sentDoc.setObject(sentObject);
                   QByteArray data = sentDoc.toJson();
                   socket->write(data);
                   }


               }
        else {
            sentObject.insert("Type", INVALID_USERNAME);
            sentDoc.setObject(sentObject);
            QByteArray data = sentDoc.toJson();
            socket->write(data);
        }
      }
     else if(type == SIGNUP) {
         QMessageBox msgBox;
         QJsonArray cipherUserName = receivedObject.value("UserName").toArray();
         QJsonArray cipherPassword = receivedObject.value("Password").toArray();

         QString receivedUsername = decrypt(cipherUserName);
         QString receivedpassword = decrypt(cipherPassword);
         qDebug() << "send data";
         qDebug() << receivedUsername << receivedpassword;
         QSqlQuery query(this->db);
         query.prepare("select * from chatapp where username=:receivedUsername");
         query.bindValue(":receivedUsername",receivedUsername);
         query.exec();
         int size = query.size();
         if(size == 0) {

            query.prepare("insert into chatapp(username, password) "
                          "VALUES (?, ?)");
            query.addBindValue(receivedUsername);
            query.addBindValue(receivedpassword);

            bool status = query.exec();
            if(status) {
                   ui->textBrowser->append(receivedUsername + " has signed up.");
                   sentObject.insert("Type", SUCCESS_SIGNUP);
                   sentDoc.setObject(sentObject);
                   QByteArray data = sentDoc.toJson();
                   socket->write(data);
            }
         }
         else {
            sentObject.insert("Type", USERNAME_EXIST);
            sentDoc.setObject(sentObject);
            QByteArray data = sentDoc.toJson();
            socket->write(data);
         }
     }

     else if(type == CHAT_ALL) {

         QMapIterator<QTcpSocket * , struct Client> iterator(connectedClients);
         while(iterator.hasNext()) {
             iterator.next();
             if(iterator.key()->socketDescriptor() == socket->socketDescriptor()) {
                 continue;
             }
             else{
                 iterator.key()->write(receivedData);
             }
         }

         QJsonArray cipherSender = receivedObject.value("Sender").toArray();
         QJsonArray cipherMessage = receivedObject.value("Message").toArray();

         QString message = decrypt(cipherMessage);
         QString sender  = decrypt(cipherSender);

         message.append("\n");
         message = sender + " : " + message;
         groupChat.append(message.toUtf8());
     }

     else if(type == CHAT_PRIVATE) {

         QJsonArray cipherSender = receivedObject.value("Sender").toArray();
         QJsonArray cipherReceiver  = receivedObject.value("Receiver").toArray();
         QJsonArray cipherMessage  = receivedObject.value("Message").toArray();

         QString sender = decrypt(cipherSender);
         QString receiver = decrypt(cipherReceiver);
         QString message = decrypt(cipherMessage);

         QMapIterator<QTcpSocket * , struct Client> iterator(connectedClients);
         while(iterator.hasNext()) {
             iterator.next();
             if(iterator.value().username == receiver) {

                 iterator.key()->write(receivedData);
             }

         }
         QMapIterator<QPair<QString,QString>,QByteArray> iterator2(chatRooms);
         int found = 0;
         while(iterator2.hasNext()) {
             iterator2.next();
             if((iterator2.key().first == sender && iterator2.key().second == receiver) || (iterator2.key().first == receiver && iterator2.key().second == sender)) {
                 chatRooms[iterator2.key()].append((sender + " : " +message+"\n").toUtf8());
                 found++;
             }

         }
         if(found == 0) {
             QPair<QString,QString> pair;
             ui->listWidget->addItem(sender + " - " + receiver);
             pair.first = sender;
             pair.second = receiver;
             QByteArray privateMessage;
             privateMessage.append((sender + " : " + message+"\n").toUtf8());
             chatRooms.insert(pair,privateMessage);
         }



     }

     else if(type == EXIT) {

         QString logOffDate = QDate::currentDate().toString("dd:MM:yy");
         QString logOffTime = QTime::currentTime().toString("hh:mm");

         QMapIterator<QTcpSocket * , struct Client> iterator(connectedClients);
         while(iterator.hasNext()) {
             iterator.next();
             if(iterator.key()->socketDescriptor() == socket->socketDescriptor()) {
                 break;
             }

         }


         QString activity;
         activity.insert(0,connectedClients.find(iterator.key()).value().username);
         activity.insert(50,connectedClients.find(iterator.key()).value().loginDate);
         activity.insert(66,connectedClients.find(iterator.key()).value().loginTime);
         activity.insert(83,logOffDate);
         activity.insert(102,logOffTime);
         activity.append("\n");
         activityDuration.append(activity.toUtf8());
         ui->textBrowser->setTextColor(Qt::red);
         ui->textBrowser->append(connectedClients.find(iterator.key()).value().username + " has logged off.");
         connectedClients.take(iterator.key());
         activeUsers--;
         ui->lcdNumber->display(activeUsers);

     }

     else if(type == ACTIVE_CLIENTS) {
         QString activeClients = "";

         sentObject.insert("Type", ACTIVE_CLIENTS);

         QMapIterator<QTcpSocket * , struct Client> iterator(connectedClients);
         while(iterator.hasNext()) {
             iterator.next();
             QString client = connectedClients.find(iterator.key()).value().username;
             activeClients += client;
             if(iterator.hasNext()) {
                 activeClients += " ";
             }
         }

         QJsonArray cipherActiveClients = encrypt(activeClients);
         sentObject.insert("Message", cipherActiveClients);
         sentDoc.setObject(sentObject);
         QByteArray data = sentDoc.toJson();
         socket->write(data);
     }



}

void Widget::on_pushButton_5_clicked()
{
    QFile activityDurationFile;
    QFileDialog dialog;
    QString directory = dialog.getSaveFileName();
    activityDurationFile.setFileName(directory);
    activityDurationFile.open(QIODevice::WriteOnly | QFile::Append);
    QString header = "User                                 -    "
                     "Log in Date  - "
                     " Log in Time   -  "
                     "Log Off Date   -   "
                     "Log Off Time\n";
    activityDurationFile.write(header.toUtf8());
    activityDurationFile.write(activityDuration);
    activityDurationFile.close();
}


void Widget::on_pushButton_4_clicked()
{
    QFile activityDurationFile;
    QFileDialog dialog;
    QString directory = dialog.getSaveFileName();
    activityDurationFile.setFileName(directory);
    activityDurationFile.open(QIODevice::WriteOnly);
    activityDurationFile.write(groupChat);
    activityDurationFile.close();
}

void Widget::changeColor()
{
    if(color == "White") {
        ui->startServerButton->setStyleSheet("background-color : rgb(85, 255, 0)");
        color = "Green";
    }
    else {
        ui->startServerButton->setStyleSheet("background-color : rgb(255, 255, 255)");
        color = "White";
    }
}

void Widget::on_stopServerButton_clicked()
{
    timer.stop();
    server.close();
    imageServer.close();
}

void Widget::activated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::ActivationReason::DoubleClick) this->show();
}


void Widget::on_pushButton_2_clicked()
{
    QStringList parts = ui->listWidget->currentItem()->text().split(" - ");
    QMapIterator<QPair<QString,QString>,QByteArray> iterator(chatRooms);
    while(iterator.hasNext()) {
        iterator.next();
        if((iterator.key().first == parts.at(0) && iterator.key().second == parts.at(1)) || (iterator.key().first == parts.at(1) && iterator.key().second == parts.at(0))) {
           {
            QByteArray message = iterator.value();
            QFile oneTOoneChat;
            QFileDialog dialog;
            QString directory = dialog.getSaveFileName();
            oneTOoneChat.setFileName(directory);
            oneTOoneChat.open(QIODevice::WriteOnly);
            oneTOoneChat.write(message);
            oneTOoneChat.close();
            }}}

}

QJsonArray Widget::encrypt(QString message)
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

QString Widget::decrypt(QJsonArray cipher)
{
    QString plain;
    for(int j = 0 ; j < cipher.size() ; j++) {
        int result = 1;
        for(int i = 0 ; i < 2753 ; i++) {
          result = (result*cipher.at(j).toInt())%n;
        }
        plain.append(QChar(result));
    }
    return plain;
}
