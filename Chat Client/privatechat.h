#ifndef PRIVATECHAT_H
#define PRIVATECHAT_H

#include <QDialog>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

#define LOGIN 0
#define SIGNUP 1
#define USERNAME_EXIST 2
#define INVALID_USERNAME 3
#define INVALID_PASSWORD 4
#define SUCCESS_LOGIN 5
#define SUCCESS_SIGNUP 6
#define ACTIVE_CLIENTS 7
#define CHAT_ALL 8
#define CHAT_PRIVATE 9
#define EXIT 10


namespace Ui {
    class PrivateChat;
}

class PrivateChat : public QDialog
{
    Q_OBJECT

public:
    explicit PrivateChat(QTcpSocket *senderSocket,QString sender,QString receiver);
    ~PrivateChat();
    Ui::PrivateChat *ui;
    QTcpSocket *socket;
    QString receiver;
    QString sender;


private slots:
    void on_lineEdit_returnPressed();


private:
    QJsonArray encrypt(QString);
    int q = 53 , p = 61 ;
    int n = p*q;

};

#endif // PRIVATECHAT_H
