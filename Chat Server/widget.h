#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMessageBox>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDate>
#include <QTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMap>
#include <QPair>
#include <QFile>
#include <QPixmap>
#include <QIcon>
#include <QFileDialog>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QDebug>

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

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

    struct Client{
        QString username;
        QString  loginDate;
        QString  loginTime;
    };

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void newClient();
    void quit(QAction *);
    void activated(QSystemTrayIcon::ActivationReason);
    void sendImage();
    void imageClient();
    void receiveMessage();
    void on_startServerButton_clicked();
    void on_pushButton_5_clicked();
    void on_pushButton_4_clicked();
    void changeColor();
    void on_stopServerButton_clicked();
    void on_pushButton_2_clicked();

private:
    Ui::Widget *ui;
    QTcpServer server;
    QMap<QTcpSocket *,struct Client> connectedClients;
    QSqlDatabase db;
    QByteArray   activityDuration;
    QByteArray   groupChat;
    QByteArray   image;
    QTcpServer imageServer;
    QTimer     timer;
    QString color;
    int activeUsers;
    QMap<QPair<QString,QString>,QByteArray> chatRooms;
    QSystemTrayIcon *systemTrayIcon;
    QMenu *menu;
    QPixmap pixmapApp;
    QIcon iconApp;
    QJsonArray encrypt(QString);
    QString decrypt(QJsonArray);
    int q = 53 , p = 61 ;
    int n = p*q;

};
#endif // WIDGET_H
