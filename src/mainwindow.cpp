#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtBluetooth/QBluetoothLocalDevice>
#include <QtBluetooth/QBluetoothHostInfo>
#include <QMessageBox>
#include <QDebug>
#include <QString>
#include <QVariant>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_serviceDiscoveryAgent(0),
    m_serviceInfo(0),
    m_rfcommServer(0),
    m_socket(0)
{
    ui->setupUi(this);
    this->ui->serviceListBox->setVisible(false);
    this->ui->messageGroupBox->setVisible(false);
    this->m_serviceDiscoveryAgent = new QBluetoothServiceDiscoveryAgent(this);
    connect(this->ui->scanButton, SIGNAL(clicked()), this, SLOT(startScaning()));
    connect(this->ui->serviceButton, SIGNAL(clicked()), this, SLOT(becomeService()));
    connect(this->ui->connectButton, SIGNAL(clicked()), this, SLOT(startClient()));
    connect(this->ui->sendButton, SIGNAL(clicked()), this, SLOT(sendMessage()));
    connect(this, SIGNAL(messageReceived(QString)), this->ui->messageLine, SLOT(setText(QString)));
    connect(this->m_serviceDiscoveryAgent, SIGNAL(serviceDiscovered(QBluetoothServiceInfo)), this, SLOT(addService(QBluetoothServiceInfo)));
    connect(this->m_serviceDiscoveryAgent, SIGNAL(finished()), this, SLOT(finishScaning()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startScaning()
{
    QBluetoothLocalDevice localDevice;

    this->ui->serviceListBox->clear();

    if (localDevice.isValid())
    {
        qDebug() <<  "Scan for services started";
        this->m_serviceDiscoveryAgent->start(QBluetoothServiceDiscoveryAgent::FullDiscovery);
    } else {
        QMessageBox::warning(this, "Warring!", "Bluetooth adapter not found");
    }
}

void MainWindow::finishScaning()
{
    qDebug() <<  "Scan for services finished. Find " << QString::number(this->m_serviceDiscoveryAgent->discoveredServices().count()) << " services";
}

void MainWindow::addService(const QBluetoothServiceInfo & info)
{
    this->ui->serviceListBox->setVisible(true);
    this->ui->serviceListBox->addItem(info.serviceUuid().toString(), QVariant::fromValue(info));
}

bool MainWindow::becomeService()
{
    QBluetoothLocalDevice localDevice;

    qDebug() << "Start to registering service and run rfcomm server";

    if (localDevice.isValid()) {

        bool listen_status;

        qDebug() << "Host name: " << localDevice.name();
        qDebug() << "Host adress: " << localDevice.address().toString();

        if (m_rfcommServer) {
            qDebug() << "Rfcomm server already exist";
            return false;
        }

        m_rfcommServer = new QBluetoothServer(QBluetoothServiceInfo::RfcommProtocol, this);
        connect(m_rfcommServer, SIGNAL(newConnection()), this, SLOT(clientConnected()));
        listen_status = m_rfcommServer->listen(localDevice.address());

        qDebug() << "Sever listen status: " << QString::number(listen_status);
        qDebug() << "Server listening port: " << QString::number(m_rfcommServer->serverPort());

        this->m_serviceInfo = new QBluetoothServiceInfo();

        QBluetoothServiceInfo::Sequence classId;
        //QBluetoothUuid serviceUuid = QBluetoothUuid(QBluetoothUuid::Rfcomm);
        QBluetoothUuid serviceUuid = QBluetoothUuid(QLatin1String("e8e10f95-1a70-4b27-9ccf-02010264e9c8"));

        qDebug() << "Service UUID: " << serviceUuid.toString();

        classId << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::SerialPort));
        m_serviceInfo->setAttribute(QBluetoothServiceInfo::BluetoothProfileDescriptorList, classId);

        classId.prepend(QVariant::fromValue(QBluetoothUuid(serviceUuid)));
        m_serviceInfo->setAttribute(QBluetoothServiceInfo::ServiceClassIds, classId);

        m_serviceInfo->setAttribute(QBluetoothServiceInfo::ServiceName, "Bt Messager Server");
        m_serviceInfo->setAttribute(QBluetoothServiceInfo::ServiceDescription, "Simple bluetooth message server");
        m_serviceInfo->setAttribute(QBluetoothServiceInfo::ServiceProvider, "vstu.ru");

        m_serviceInfo->setServiceUuid(QBluetoothUuid(serviceUuid));

        m_serviceInfo->setAttribute(QBluetoothServiceInfo::BrowseGroupList, QBluetoothUuid(QBluetoothUuid::PublicBrowseGroup));

        QBluetoothServiceInfo::Sequence protocolDescriptorList;
        QBluetoothServiceInfo::Sequence protocol;
        //protocol << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::L2cap));
        //protocolDescriptorList.append(QVariant::fromValue(protocol));
        //protocol.clear();
        protocol << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::Rfcomm))
                 << QVariant::fromValue(quint8(m_rfcommServer->serverPort()));
        protocolDescriptorList.append(QVariant::fromValue(protocol));
        m_serviceInfo->setAttribute(QBluetoothServiceInfo::ProtocolDescriptorList, protocolDescriptorList);

        m_serviceInfo->registerService(localDevice.address());

        qDebug() << "Service registered";
        return true;

    } else {
        QMessageBox::warning(this, "Warring!", "Bluetooth adapter not found");
    }

    qDebug() << "Service not registered";
    return false;
}

void MainWindow::clientConnected()
{
    this->m_socket = m_rfcommServer->nextPendingConnection();

    connect(m_socket, SIGNAL(stateChanged(QBluetoothSocket::SocketState)), this, SLOT(connectingStateChanged(QBluetoothSocket::SocketState)));
    connect(m_socket, SIGNAL(error(QBluetoothSocket::SocketError)), this, SLOT(connectingError(QBluetoothSocket::SocketError)));

    if (!m_socket) {
        qDebug() << "Socket is not exist";
        return;
    }

    connect(this->m_socket, SIGNAL(readyRead()), this, SLOT(readSocket()));
    connect(this->m_socket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));

    this->ui->messageGroupBox->setVisible(true);
    this->m_connected = true;

    qDebug() << "New client connected";
}

void MainWindow::clientDisconnected()
{
    if (!m_socket) {
        qDebug() << "Socket is not exist";
        return;
    }

    this->m_socket->deleteLater();

    this->ui->messageGroupBox->setVisible(false);
    this->m_connected = false;

    qDebug() << "Client was disconnected";
}

void MainWindow::startClient()
{
    const QBluetoothServiceInfo remoteService = this->ui->serviceListBox->currentData().value<QBluetoothServiceInfo>();

    if (this->m_socket) {
        qDebug() << "Socket already exist";
        return;
    }

    qDebug() << "Service info:";
    qDebug() << "Service is valid: " << QString::number(remoteService.isValid());
    qDebug() << "Service is complete: " << QString::number(remoteService.isComplete());
    qDebug() << "Service host address: " << remoteService.device().address().toString();
    qDebug() << "Service name: " << remoteService.serviceName();
    qDebug() << "Service UUID: " << remoteService.serviceUuid().toString();

    qDebug() << "Creating socket";

    this->m_socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);

    connect(m_socket, SIGNAL(readyRead()), this, SLOT(readSocket()));
    connect(m_socket, SIGNAL(connected()), this, SLOT(connectToService()));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(stopClient()));
    connect(m_socket, SIGNAL(stateChanged(QBluetoothSocket::SocketState)), this, SLOT(connectingStateChanged(QBluetoothSocket::SocketState)));
    connect(m_socket, SIGNAL(error(QBluetoothSocket::SocketError)), this, SLOT(connectingError(QBluetoothSocket::SocketError)));

    qDebug() << "Connecting to service";

    this->m_socket->connectToService(remoteService);

}

void MainWindow::connectToService() {

    this->ui->messageGroupBox->setVisible(true);
    this->m_connected = true;

    qDebug() << "Connected to service";
}

void MainWindow::stopClient()
{
    if (!m_socket) {
        qDebug() << "Socket is not exist";
        return;
    }

    this->m_socket->deleteLater();

    this->ui->messageGroupBox->setVisible(false);
    this->m_connected = false;

    qDebug() << "Connection to service closed";
}

void MainWindow::readSocket()
{
    if (!m_socket) {
        qDebug() << "Socket is not exist";
        return;
    }

    while (this->m_socket->canReadLine()) {
        QByteArray line = this->m_socket->readLine();
        emit messageReceived(QString::fromUtf8(line.constData()));
        qDebug() << "Message received";
    }
}

void MainWindow::sendMessage()
{
    const QString message = this->ui->messageLine->text();

    QByteArray text = message.toUtf8() + '\n';
    this->m_socket->write(text);

    qDebug() << "Message sended";
}

void MainWindow::connectingStateChanged(QBluetoothSocket::SocketState state)
{
    switch (state)
    {
        case QBluetoothSocket::UnconnectedState:
            qDebug() << "Current connection state is UnconnectedState";
            break;
        case QBluetoothSocket::ServiceLookupState:
            qDebug() << "Current connection state is ServiceLookupState";
            break;
        case QBluetoothSocket::ConnectingState:
            qDebug() << "Current connection state is ConnectingState";
            break;
        case QBluetoothSocket::ConnectedState:
            qDebug() << "Current connection state is ConnectedState";
            break;
        case QBluetoothSocket::BoundState:
            qDebug() << "Current connection state is BoundState";
            break;
        case QBluetoothSocket::ClosingState:
            qDebug() << "Current connection state is ClosingState";
            break;
        case QBluetoothSocket::ListeningState:
            qDebug() << "Current connection state is ListeningState";
            break;
    }
}

void MainWindow::connectingError(QBluetoothSocket::SocketError error)
{
    switch (error)
    {
        case QBluetoothSocket::UnknownSocketError:
            qDebug() << "Connection error is UnknownSocketError";
            break;
        case QBluetoothSocket::HostNotFoundError:
            qDebug() << "Connection error is HostNotFoundError";
            break;
        case QBluetoothSocket::ServiceNotFoundError:
            qDebug() << "Connection error is ServiceNotFoundError";
            break;
        case QBluetoothSocket::NetworkError:
            qDebug() << "Connection error is NetworkError";
            break;
        case QBluetoothSocket::UnsupportedProtocolError:
            qDebug() << "Connection error is UnsupportedProtocolError";
            break;
        case QBluetoothSocket::OperationError:
            qDebug() << "Connection error is OperationError";
            break;
        case QBluetoothSocket::NoSocketError:
            qDebug() << "No connection error";
            break;
    }
}
