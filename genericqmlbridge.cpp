#include "genericqmlbridge.h"
#include "slipprocessor.h"
#include "datadecoder.hpp"
#include "QmlPropertyObserver.hpp"

#include <QTimer>
#include <QCborMap>
#include <QCborValue>
#include <QCborStreamWriter>
#include <QCborArray>
#include <QSet>

GenericQMLBridge::GenericQMLBridge(QObject *parent)
    : QObject(parent)
    , m_engine(new QQmlApplicationEngine(this))
    , m_rootObject(nullptr)
    , m_serialPort(nullptr)
    , m_tcpServer(nullptr)
    , m_heartbeatTimer(new QTimer(this))
    , m_configuredBaudRate(115200)
    , m_configuredTcpPort(0)
    , m_slipProcessor(new SlipProcessor(this))
{
    m_heartbeatTimer->setInterval(5000);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &GenericQMLBridge::checkConnections);
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
    if (data.isEmpty()) return;
    quint8 cmdType = static_cast<quint8>(data[0]);
    const char* payload = data.constData() + 1;
    int payloadLen = data.size() - 1;

    switch (cmdType) {
    case CMD_GET_PROPERTY_LIST:
        sendPropertyList();
        return;
    case CMD_SET_PROPERTY: {
        if (payloadLen <= 0) {
            qDebug() << "Error: SET_PROPERTY missing CBOR map payload";
            return;
        }
        QCborValue cbor = QCborValue::fromCbor(QByteArray(payload, payloadLen));
        if (!cbor.isMap()) {
            qDebug() << "Error: SET_PROPERTY payload is not a CBOR map";
            return;
        }
        QCborMap map = cbor.toMap();
        for (auto it = map.begin(); it != map.end(); ++it) {
            QString propName = it.key().toString();
            if (!m_properties.contains(propName)) {
                qDebug() << "Unknown property in SET_PROPERTY:" << propName;
                continue;
            }
            QQmlProperty prop = m_properties[propName];
            QVariant value = it.value().toVariant();
            bool success = prop.write(value);
            qDebug() << "Property updated:" << propName << "=" << value << "Success:" << success;
        }
        return;
    }
    case CMD_INVOKE_METHOD: {
        if (payloadLen < 1) {
            qDebug() << "Error: INVOKE_METHOD missing method id";
            return;
        }
        quint8 methodId = static_cast<quint8>(payload[0]);
        QString methodName = m_propertyIdMap.value(methodId);
        QCborArray params;
        if (payloadLen > 1) {
            QCborValue cborParams = QCborValue::fromCbor(QByteArray(payload + 1, payloadLen - 1));
            if (cborParams.isArray())
                params = cborParams.toArray();
        }
        if (m_methods.contains(methodName)) {
            QMetaMethod method = m_methods[methodName];
            QList<QVariant> args;
            for (const QCborValue& v : params)
                args << v.toVariant();
            method.invoke(m_properties[methodName].object(), Qt::DirectConnection, QGenericArgument(args.value(0).typeName(), args.value(0).data()));
            qDebug() << "Method invoked:" << methodName << "with args:" << args;
        }
        return;
    }
    case CMD_WATCH_PROPERTY: {
        for (auto conn : m_watchedConnections.values())
            QObject::disconnect(conn);
        m_watchedConnections.clear();
        m_watchedPropertyIds.clear();
        if (payloadLen <= 0) {
            qDebug() << "Error: WATCH_PROPERTY missing CBOR array payload";
            return;
        }
        QCborValue cbor = QCborValue::fromCbor(QByteArray(payload, payloadLen));
        if (!cbor.isArray()) {
            qDebug() << "Error: WATCH_PROPERTY payload is not a CBOR array";
            return;
        }
        for (const QCborValue& v : cbor.toArray()) {
            if (!v.isInteger())
                continue;
            quint8 id = static_cast<quint8>(v.toInteger());
            
            m_watchedPropertyIds.insert(id);
            
            QString propName = m_propertyIdMap.value(id);
            
            if (!m_properties.contains(propName))
                continue;
            
            QQmlProperty qmlProp = m_properties[propName];

            auto observer = QmlPropertyObserver::watch(qmlProp, [this, id](QVariant newValue) {
                QCborMap change;
                change[QStringLiteral("id")] = id;
                change[QStringLiteral("value")] = QCborValue::fromVariant(newValue);
                QByteArray cborData;
                QCborStreamWriter writer(&cborData);
                QCborValue(change).toCbor(writer);
                QByteArray packet;
                packet.append(static_cast<char>(RESP_PROPERTY_CHANGE));
                packet.append(cborData);
                sendSlipData(packet);
            }, this);

            if (!observer) {
                qDebug() << "Failed to create property observer for ID:" << id;
                continue;
            }

            m_watchedConnections[id] = observer->connection();
        }
        qDebug() << "Now watching property IDs:" << m_watchedPropertyIds;
        return;
    }
    case CMD_HEARTBEAT:
        // No action needed
        return;
    default:
        qDebug() << "Unknown command type:" << cmdType;
        return;
    }
}

void GenericQMLBridge::sendPropertyList()
{
    QCborMap propList;
    for (auto it = m_propertyNameMap.begin(); it != m_propertyNameMap.end(); ++it) {
        QQmlProperty prop = m_properties[it.key()];
        QCborMap entry;
        entry[QStringLiteral("id")] = it.value();
        entry[QStringLiteral("type")] = QString::fromUtf8(prop.property().typeName());
        propList[it.key()] = entry;
    }
    QByteArray cbor;
    QCborStreamWriter writer(&cbor);
    QCborValue(propList).toCbor(writer);
    QByteArray packet;
    packet.append(static_cast<char>(RESP_GET_PROPERTY_LIST));
    packet.append(cbor);
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
    heartbeat.append(static_cast<char>(CMD_HEARTBEAT));
    
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
