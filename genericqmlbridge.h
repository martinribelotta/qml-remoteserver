#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlProperty>
#include <QQmlContext>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMetaObject>
#include <QMetaProperty>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileSystemWatcher>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include "slipprocessor.h"

class GenericQMLBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isTcpConnected READ isTcpConnected NOTIFY tcpConnectionStateChanged)
    Q_PROPERTY(bool isSerialConnected READ isSerialConnected NOTIFY serialConnectionStateChanged)
    Q_PROPERTY(QString lastError READ getLastError NOTIFY errorOccurred)
    Q_PROPERTY(int connectedClients READ connectedClients NOTIFY connectedClientsChanged)

public:
    enum ProtocolCommand {
        CMD_GET_PROPERTY_LIST = 0x00,
        CMD_SET_PROPERTY      = 0x10,
        CMD_INVOKE_METHOD     = 0x05,
        CMD_HEARTBEAT         = 0xFF
    };
    Q_ENUM(ProtocolCommand)

    enum ProtocolResponse {
        RESP_GET_PROPERTY_LIST = 0x80
    };
    Q_ENUM(ProtocolResponse)

    explicit GenericQMLBridge(QObject *parent = nullptr);
    virtual ~GenericQMLBridge();

    bool loadQML(const QString &qmlFile);
    bool setupSerial(const QString &portName, int baudRate);
    bool setupTCP(int port);
    void discoverProperties();
    void processCommand(const QByteArray &data);
    Q_INVOKABLE QStringList getAvailablePorts() const;
    Q_INVOKABLE bool isSerialConnected() const { return m_serialPort && m_serialPort->isOpen(); }
    Q_INVOKABLE bool isTcpConnected() const { return !m_tcpClients.isEmpty(); }
    Q_INVOKABLE int connectedClients() const { return m_tcpClients.size(); }
    Q_INVOKABLE void closeSerial();
    Q_INVOKABLE QString getLastError() const;
    Q_INVOKABLE void reconnectSerial();
    Q_INVOKABLE void reconnectTCP();
    
    void sendSlipData(const QByteArray &data);
    void sendSlipDataToSerial(const QByteArray &data);
    void sendSlipDataToTcp(const QByteArray &data);

signals:
    void tcpConnectionStateChanged(bool connected);
    void serialConnectionStateChanged(bool connected);
    void errorOccurred(const QString &error);
    void connectedClientsChanged(int count);
    void connectionLost(const QString &type);

private slots:
    void handleSerialData();
    void handleSerialError(QSerialPort::SerialPortError error);
    void onQMLFileChanged();
    void handleTcpNewConnection();
    void handleTcpData();
    void handleTcpDisconnected();
    void handleTcpError(QAbstractSocket::SocketError error);
    void checkConnections();

private:
    QQmlApplicationEngine *m_engine;
    QObject *m_rootObject;
    QSerialPort *m_serialPort;
    QFileSystemWatcher *m_fileWatcher;
    QTcpServer *m_tcpServer;
    QList<QTcpSocket*> m_tcpClients;
    QHash<QString, QQmlProperty> m_properties;
    QHash<QString, QMetaMethod> m_methods;
    QHash<quint8, QString> m_propertyIdMap;
    QHash<QString, quint8> m_propertyNameMap;
    QString m_lastError;
    QTimer *m_heartbeatTimer;
    QString m_configuredSerialPort;
    int m_configuredBaudRate;
    int m_configuredTcpPort;
    SlipProcessor *m_slipProcessor;
    QHash<QTcpSocket*, SlipProcessor*> m_tcpSlipProcessors;

    void scanObjectProperties(QObject *obj, const QString &prefix = "");
    void sendPropertyList();
    QVariant parseValue(const QByteArray &data, QMetaType::Type expectedType);
    void sendEvent(const QString &eventName, const QVariantList &args = {});
    void setLastError(const QString &error);
    void startHeartbeat();
    void stopHeartbeat();
    void sendHeartbeat();
};
