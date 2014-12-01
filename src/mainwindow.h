#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtBluetooth/QBluetoothServiceDiscoveryAgent>
#include <QtBluetooth/QBluetoothServiceInfo>
#include <QtBluetooth/QBluetoothServer>
#include <QtBluetooth/QBluetoothSocket>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:

    void messageReceived(const QString &message);

public slots:

    void startScaning();
    void finishScaning();
    void addService(const QBluetoothServiceInfo &info);
    bool becomeService();
    void clientConnected();
    void clientDisconnected();
    void startClient();
    void connectToService();
    void stopClient();
    void sendMessage();
    void readSocket();
    void connectingStateChanged(QBluetoothSocket::SocketState);
    void connectingError(QBluetoothSocket::SocketError);

private:

    Ui::MainWindow *ui;
    QBluetoothServiceDiscoveryAgent *m_serviceDiscoveryAgent;
    QBluetoothServiceInfo *m_serviceInfo;
    QBluetoothServer *m_rfcommServer;
    QBluetoothSocket *m_socket;
    bool m_connected;

};

#endif // MAINWINDOW_H
