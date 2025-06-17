#include "genericqmlbridge.h"
#include "slipprocessor.h"

#include <QTimer>

GenericQMLBridge::GenericQMLBridge(QObject *parent)
    : QObject(parent)
    , m_engine(new QQmlApplicationEngine(this))
    , m_rootObject(nullptr)
    , m_serialPort(nullptr)
    , m_fileWatcher(new QFileSystemWatcher(this))
    , m_tcpServer(nullptr)
    , m_heartbeatTimer(new QTimer(this))
    , m_configuredBaudRate(115200)
    , m_configuredTcpPort(0)
    , m_slipProcessor(new SlipProcessor(this))
{
    m_heartbeatTimer->setInterval(5000);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &GenericQMLBridge::checkConnections);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &GenericQMLBridge::onQMLFileChanged);
    
    connect(m_slipProcessor, &SlipProcessor::packetReceived, this, &GenericQMLBridge::processCommand);
}

bool GenericQMLBridge::loadQML(const QString &qmlFile)
{
    m_properties.clear();
    m_methods.clear();
    m_propertyIdMap.clear();
    m_propertyNameMap.clear();

    m_engine->load(QUrl::fromLocalFile(qmlFile));

    if (m_engine->rootObjects().isEmpty()) {
        qDebug() << "Error: Could not load" << qmlFile;
        return false;
    }

    m_rootObject = m_engine->rootObjects().first();
    m_fileWatcher->addPath(qmlFile);
    discoverProperties();

    qDebug() << "QML loaded successfully:" << qmlFile;
    qDebug() << "Properties detected:" << m_properties.keys();

    return true;
}

void GenericQMLBridge::discoverProperties()
{
    if (!m_rootObject) return;

    scanObjectProperties(m_rootObject);
    qDebug() << "Discovered" << m_properties.size() << "properties";
}

void GenericQMLBridge::scanObjectProperties(QObject *obj, const QString &prefix)
{
    if (!obj) return;

    const QMetaObject *metaObj = obj->metaObject();
    for (int i = 0; i < metaObj->propertyCount(); ++i) {
        QMetaProperty prop = metaObj->property(i);
        QString propName = prefix.isEmpty() ? prop.name() : prefix + "." + prop.name();

        if (prop.isWritable() && prop.isReadable()) {
            QQmlProperty qmlProp(obj, prop.name());
            if (qmlProp.isValid()) {
                m_properties[propName] = qmlProp;
                quint8 id = static_cast<quint8>(m_properties.size() - 1);
                m_propertyIdMap[id] = propName;
                m_propertyNameMap[propName] = id;

                qDebug() << "Propiedad detectada:" << propName
                         << "Tipo:" << prop.typeName()
                         << "ID:" << id;
            }
        }
    }

    for (int i = 0; i < metaObj->methodCount(); ++i) {
        QMetaMethod method = metaObj->method(i);
        if (method.methodType() == QMetaMethod::Slot) {
            QString methodName = prefix.isEmpty() ? method.name() : prefix + "." + method.name();
            m_methods[methodName] = method;
            qDebug() << "Método detectado:" << methodName;
        }
    }
    QObjectList children = obj->findChildren<QObject*>(QString(), Qt::FindDirectChildrenOnly);
    for (QObject *child : children) {
        QString childName = child->objectName();
        if (!childName.isEmpty()) {
            QString childPrefix = prefix.isEmpty() ? childName : prefix + "." + childName;
            scanObjectProperties(child, childPrefix);
        }
    }
}

void GenericQMLBridge::processCommand(const QByteArray &data)
{
    if (data.length() < 2) return;

    quint8 cmdType = static_cast<quint8>(data[0]);
    quint8 propId = static_cast<quint8>(data[1]);

    QString propName = m_propertyIdMap.value(propId);

    qDebug() << "Processing command:" << cmdType
             << "Property ID:" << propId
             << "Property Name:" << propName;

    if (propName.isEmpty()) {
        qDebug() << "Property not found for ID:" << propId;
        return;
    }

    QQmlProperty prop = m_properties.value(propName);
    if (!prop.isValid()) {
        qDebug() << "Invalid property:" << propName;
        return;
    }

    QVariant value;

    switch (cmdType) {
    case 0x0:
        sendPropertyList();
        return;

    case 0x1:
        if (data.length() >= 6) {
            qint32 intVal;
            memcpy(&intVal, data.data() + 2, 4);
            value = QVariant(intVal);
        }
        break;

    case 0x2:
        if (data.length() >= 6) {
            float floatVal;
            memcpy(&floatVal, data.data() + 2, 4);
            value = QVariant(floatVal);
        }
        break;

    case 0x3:
        if (data.length() > 2) {
            QString stringVal = QString::fromUtf8(data.mid(2));
            value = QVariant(stringVal);
        }
        break;

    case 0x4:
        if (data.length() >= 3) {
            bool boolVal = data[2] != 0;
            value = QVariant(boolVal);
        }
        break;

    case 0x5:
    {
        QString methodName = m_propertyIdMap.value(propId);
        if (m_methods.contains(methodName)) {
            QMetaMethod method = m_methods[methodName];
            method.invoke(prop.object(), Qt::DirectConnection);
            qDebug() << "Method invoked:" << methodName;
        }
    }
        return;
    }

    if (value.isValid()) {
        bool success = prop.write(value);
        qDebug() << "Property updated:" << propName << "(" << propId << ")" << "=" << value << "Success:" << success;
    }
}

void GenericQMLBridge::sendPropertyList()
{
    QJsonObject propList;
    for (auto it = m_propertyNameMap.begin(); it != m_propertyNameMap.end(); ++it) {
        QQmlProperty prop = m_properties[it.key()];
        propList[it.key()] = QJsonObject{
            {"id", it.value()},
            {"type", prop.property().typeName()}
        };
    }

    QJsonDocument doc(propList);
    QByteArray json = doc.toJson(QJsonDocument::Compact);
    QByteArray packet = QByteArray("\xFF\xFF", 2) + json + QByteArray("\n");

    sendSlipData(packet);
}

bool GenericQMLBridge::setupSerial(const QString &portName, int baudRate)
{
    m_configuredSerialPort = portName;
    m_configuredBaudRate = baudRate;

    if (m_serialPort) {
        if (m_serialPort->isOpen())
            m_serialPort->close();
        delete m_serialPort;
    }

    m_serialPort = new QSerialPort(this);
    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);

    connect(m_serialPort, &QSerialPort::readyRead, this, &GenericQMLBridge::handleSerialData);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &GenericQMLBridge::handleSerialError);

    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        setLastError(tr("Failed to open serial port: %1").arg(m_serialPort->errorString()));
        emit serialConnectionStateChanged(false);
        return false;
    }

    emit serialConnectionStateChanged(true);
    startHeartbeat();
    return true;
}

void GenericQMLBridge::closeSerial()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        qDebug() << "Serial port closed";
    }
}

void GenericQMLBridge::handleSerialData()
{
    QByteArray data = m_serialPort->readAll();
    m_slipProcessor->onDataReceived(data);
}

void GenericQMLBridge::onQMLFileChanged()
{
    qDebug() << "QML file modified, reloading...";
    
    QByteArray reloadMsg = QByteArray("\xFF\xFE", 2) + QByteArray("reload\n");
    
    sendSlipData(reloadMsg);
}

QStringList GenericQMLBridge::getAvailablePorts() const
{
    QStringList result;
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        result << info.portName();
    }
    return result;
}

bool GenericQMLBridge::setupTCP(int port)
{
    m_configuredTcpPort = port;

    if (!m_tcpServer) {
        m_tcpServer = new QTcpServer(this);
        connect(m_tcpServer, &QTcpServer::newConnection, this, &GenericQMLBridge::handleTcpNewConnection);
    }

    if (!m_tcpServer->listen(QHostAddress::Any, port)) {
        setLastError(tr("Error starting TCP server: %1").arg(m_tcpServer->errorString()));
        return false;
    }

    startHeartbeat();
    return true;
}

void GenericQMLBridge::handleTcpNewConnection()
{
    QTcpSocket *clientSocket = m_tcpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &GenericQMLBridge::handleTcpData);
    connect(clientSocket, &QTcpSocket::disconnected, this, &GenericQMLBridge::handleTcpDisconnected);
    connect(clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &GenericQMLBridge::handleTcpError);

    SlipProcessor *tcpSlipProcessor = new SlipProcessor(this);
    connect(tcpSlipProcessor, &SlipProcessor::packetReceived, this, &GenericQMLBridge::processCommand);
    m_tcpSlipProcessors[clientSocket] = tcpSlipProcessor;

    m_tcpClients.append(clientSocket);
    emit connectedClientsChanged(m_tcpClients.size());
    emit tcpConnectionStateChanged(!m_tcpClients.isEmpty());

    qDebug() << "New TCP client connected. Total clients:" << m_tcpClients.size();
}

void GenericQMLBridge::handleTcpError(QAbstractSocket::SocketError error)
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    setLastError(tr("TCP Error: %1").arg(socket->errorString()));
    
    if (error != QAbstractSocket::RemoteHostClosedError) {
        socket->disconnectFromHost();
    }
}

void GenericQMLBridge::handleSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;

    setLastError(tr("Serial Error: %1").arg(m_serialPort->errorString()));
    
    if (error != QSerialPort::ResourceError) {
        emit connectionLost("serial");
        emit serialConnectionStateChanged(false);
    }
}

void GenericQMLBridge::handleTcpDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    SlipProcessor *slipProcessor = m_tcpSlipProcessors.take(socket);
    if (slipProcessor) {
        slipProcessor->deleteLater();
    }

    m_tcpClients.removeOne(socket);
    socket->deleteLater();
    
    emit connectedClientsChanged(m_tcpClients.size());
    emit tcpConnectionStateChanged(!m_tcpClients.isEmpty());
    
    if (m_tcpClients.isEmpty()) {
        emit connectionLost("tcp");
    }
}

void GenericQMLBridge::checkConnections()
{
    if (m_serialPort && !m_serialPort->isOpen()) {
        reconnectSerial();
    }

    sendHeartbeat();
}

void GenericQMLBridge::startHeartbeat()
{
    if (!m_heartbeatTimer->isActive()) {
        m_heartbeatTimer->start();
    }
}

void GenericQMLBridge::stopHeartbeat()
{
    m_heartbeatTimer->stop();
}

void GenericQMLBridge::sendHeartbeat()
{
    QByteArray heartbeat;
    heartbeat.append(static_cast<char>(0xFF));
    
    sendSlipDataToTcp(heartbeat);
    
    for (auto it = m_tcpClients.begin(); it != m_tcpClients.end();) {
        if ((*it)->state() != QAbstractSocket::ConnectedState) {
            SlipProcessor *slipProcessor = m_tcpSlipProcessors.take(*it);
            if (slipProcessor) {
                slipProcessor->deleteLater();
            }
            (*it)->deleteLater();
            it = m_tcpClients.erase(it);
        } else {
            ++it;
        }
    }
    
    emit connectedClientsChanged(m_tcpClients.size());
    emit tcpConnectionStateChanged(!m_tcpClients.isEmpty());
}

void GenericQMLBridge::setLastError(const QString &error)
{
    m_lastError = error;
    emit errorOccurred(m_lastError);
}

QString GenericQMLBridge::getLastError() const
{
    return m_lastError;
}

void GenericQMLBridge::reconnectSerial()
{
    if (!m_configuredSerialPort.isEmpty()) {
        setupSerial(m_configuredSerialPort, m_configuredBaudRate);
    }
}

void GenericQMLBridge::reconnectTCP()
{
    if (m_configuredTcpPort > 0) {
        setupTCP(m_configuredTcpPort);
    }
}

void GenericQMLBridge::handleTcpData()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        qDebug() << "Error: Could not get sender socket";
        return;
    }

    QByteArray data = socket->readAll();
    if (data.isEmpty()) {
        qDebug() << "Warning: Received empty data from TCP client";
        return;
    }

    SlipProcessor *slipProcessor = m_tcpSlipProcessors.value(socket);
    if (slipProcessor) {
        slipProcessor->onDataReceived(data);
    } else {
        qDebug() << "Error: No SlipProcessor found for TCP client";
    }
}

GenericQMLBridge::~GenericQMLBridge()
{
    stopHeartbeat();
    
    if (m_serialPort) {
        if (m_serialPort->isOpen())
            m_serialPort->close();
        delete m_serialPort;
    }

    for (auto it = m_tcpSlipProcessors.begin(); it != m_tcpSlipProcessors.end(); ++it) {
        if (it.value()) {
            it.value()->deleteLater();
        }
    }
    m_tcpSlipProcessors.clear();

    for (QTcpSocket* socket : m_tcpClients) {
        if (socket) {
            socket->disconnectFromHost();
            if (socket->state() != QAbstractSocket::UnconnectedState)
                socket->waitForDisconnected();
            socket->deleteLater();
        }
    }
    m_tcpClients.clear();
}

void GenericQMLBridge::sendSlipData(const QByteArray &data)
{
    QByteArray encodedData = SlipProcessor::encodeSlip(data);
    
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->write(encodedData);
    }
    
    for (QTcpSocket* client : m_tcpClients) {
        if (client && client->state() == QAbstractSocket::ConnectedState) {
            client->write(encodedData);
        }
    }
}

void GenericQMLBridge::sendSlipDataToSerial(const QByteArray &data)
{
    if (m_serialPort && m_serialPort->isOpen()) {
        QByteArray encodedData = SlipProcessor::encodeSlip(data);
        m_serialPort->write(encodedData);
    }
}

void GenericQMLBridge::sendSlipDataToTcp(const QByteArray &data)
{
    QByteArray encodedData = SlipProcessor::encodeSlip(data);
    for (QTcpSocket* client : m_tcpClients) {
        if (client && client->state() == QAbstractSocket::ConnectedState) {
            client->write(encodedData);
        }
    }
}
