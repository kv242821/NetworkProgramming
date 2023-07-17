#include "widget.h"
#include "ui_widget.h"


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    QPixmap pixmapApp;
    QIcon iconApp;
    iconApp.addPixmap(pixmapApp);
    QWidget::setWindowIcon(iconApp);
    QWidget::setWindowTitle("Chat Application");
    QWidget::setFixedSize(QSize(800,600));


    ui->lineEdit_2->setEchoMode(QLineEdit::Password);
    ui->lineEdit_3->setEchoMode(QLineEdit::Password);
    ui->lineEdit_6->setEchoMode(QLineEdit::Password);

    ui->stackedWidget->setCurrentIndex(0);

    socket.connectToHost(QHostAddress("127.0.0.1"),5500);
    imageSocket.connectToHost(QHostAddress("127.0.0.1"),5501);
    connect(&socket,SIGNAL(readyRead()),this,SLOT(receiveMessage()));
    connect(&imageSocket,SIGNAL(readyRead()),this,SLOT(receiveImage()));

}

Widget::~Widget()
{
    delete ui;
}


void Widget::receiveMessage() {

     QByteArray data = socket.readAll();
     QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
     QJsonObject object = jsonDocument.object();

     int type = object.value("Type").toInt();
        qDebug() << type;
     if(type == INVALID_USERNAME) {
         QMessageBox msgBox;
         msgBox.setWindowTitle("ERROR");
         msgBox.setText("Username not found");
         msgBox.setIcon(QMessageBox::Critical);
         msgBox.exec();
         ui->stackedWidget->setCurrentIndex(1);
     }
     else if(type == INVALID_PASSWORD) {
         QMessageBox msgBox;
         msgBox.setWindowTitle("ERROR");
         msgBox.setText("You have entered wrong password");
         msgBox.setIcon(QMessageBox::Critical);
         msgBox.exec();
     }
     else if(type == SUCCESS_LOGIN) {
         userName = ui->lineEdit_4->text();
         chatScreen = new ChatScreen(&socket,&imageSocket,userName);
         close();
         chatScreen->show();
     }
     else if(type == SUCCESS_SIGNUP) {
         QMessageBox msgBox;
         msgBox.setWindowTitle("SUCCESS");
         msgBox.setText("Create account successfully");
         msgBox.setIcon(QMessageBox::Information);
         msgBox.exec();
         ui->stackedWidget->setCurrentIndex(0);
     }

     else if(type == CHAT_ALL) {
          QJsonArray cipherSender = object.value("Sender").toArray();
          QJsonArray cipherMessage = object.value("Message").toArray();

          QString sender = decrypt(cipherSender);
          QString message = decrypt(cipherMessage);

          chatScreen->ui->textBrowser->append(sender+" : "+message);
         }
     else if(type == ACTIVE_CLIENTS) {
          QJsonArray cipherActiveClients = object.value("Message").toArray();
          QString activeClients = decrypt(cipherActiveClients);
          QStringList activeClientsList = activeClients.split(" ");
          chatScreen->ui->listWidget->clear();
             for(int i=0 ; i<activeClientsList.size() ; i++) {
              if(userName != activeClientsList.at(i))
                 chatScreen->ui->listWidget->addItem(activeClientsList.at(i));
             }
         }

    else if(type == CHAT_PRIVATE) {

         QJsonArray cipherSender = object.value("Sender").toArray();
         QJsonArray cipherMessage = object.value("Message").toArray();

         QString sender = decrypt(cipherSender);
         QString message = decrypt(cipherMessage);

         if(!chatScreen->isActiveWindow()) {
            chatScreen->systemTrayIcon->showMessage(sender,message,QSystemTrayIcon::MessageIcon::Information);
         }

          QMap<QString , PrivateChat *>::iterator iter = chatScreen->chatRooms.find(sender);
          if(iter != chatScreen->chatRooms.end()) {
              iter.value()->ui->textBrowser->append(sender + " : " + message);
          }
          else{
              PrivateChat *privateRoom = new PrivateChat(&socket,userName,sender);
              chatScreen->chatRooms.insert(sender,privateRoom);
              chatScreen->ui->tabWidget->insertTab(chatScreen->ui->tabWidget->count(),privateRoom,sender);
              privateRoom->ui->textBrowser->append(sender + " : " + message);
            }

        }
 }


void Widget::receiveImage() {

    QByteArray data = imageSocket.readAll();
    image.append(data);
    QString code = "*#%END%#*";
    int index = image.indexOf(code.toUtf8());
    if(index != -1){
         image = image.remove(index,code.size());
         QPixmap imageReceive;
         imageReceive.loadFromData(image);
//         chatScreen->ui->label_12->setPixmap(imageReceive.scaled(chatScreen->ui->label_12->size()));
         image.clear();
         image.shrink_to_fit();
     }

}


void Widget::on_pushButton_2_clicked()
{
    QString userName = ui->lineEdit_4->text();
    QString password = ui->lineEdit_6->text();

    QJsonArray cipherUserName = encrypt(userName);
    QJsonArray cipherPassword = encrypt(password);

    QJsonObject jsonObject;
    jsonObject.insert("Type", LOGIN);
    jsonObject.insert("UserName",cipherUserName);
    jsonObject.insert("Password",cipherPassword);

    QJsonDocument jsonDoc;
    jsonDoc.setObject(jsonObject);
    QByteArray data = jsonDoc.toJson();

    if(socket.state() == QTcpSocket::ConnectedState) {
        socket.write(data);
    }

}

void Widget::on_checkBox_2_stateChanged(int arg1)
{
    if(arg1 == 2) {
        ui->lineEdit_6->setEchoMode(QLineEdit::Normal);
    }
    else {
        ui->lineEdit_6->setEchoMode(QLineEdit::Password);
    }
}


void Widget::on_checkBox_stateChanged(int arg1)
{
    if(arg1 == 2) {
        ui->lineEdit_2->setEchoMode(QLineEdit::Normal);
        ui->lineEdit_3->setEchoMode(QLineEdit::Normal);
    }
    else {
        ui->lineEdit_2->setEchoMode(QLineEdit::Password);
        ui->lineEdit_3->setEchoMode(QLineEdit::Password);
    }
}


void Widget::on_pushButton_clicked()
{
    QString userName = ui->lineEdit->text();
    QString password = ui->lineEdit_2->text();
    QString repeatedPassword = ui->lineEdit_3->text();

    if(password != repeatedPassword){
        QMessageBox msgBox;
        msgBox.setWindowTitle("ERROR");
        msgBox.setText("Password does not match");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
    else{
        QJsonArray cipherUserName = encrypt(userName);
        QJsonArray cipherPassword = encrypt(password);

        QJsonObject jsonObject;
        jsonObject.insert("Type", SIGNUP);
        jsonObject.insert("UserName",cipherUserName);
        jsonObject.insert("Password",cipherPassword);

        QJsonDocument jsonDoc;
        jsonDoc.setObject(jsonObject);
        QByteArray data = jsonDoc.toJson();

        if(socket.state() == QTcpSocket::ConnectedState) {
            qDebug() << "send data";
            qDebug() << socket.write(data);
        }
    }
}

void Widget::on_pushButton_3_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void Widget::on_pushButton_6_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
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
