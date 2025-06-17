#include <QGuiApplication>
#include <QCommandLineParser>

#include "genericqmlbridge.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument("qml", "QML file to load");
    parser.addOption({{"p", "port"}, "Serial port", "port", "/dev/ttyUSB0"});
    parser.addOption({{"b", "baudrate"}, "Baud rate", "baudrate", "115200"});
    parser.addOption({{"t", "tcp"}, "TCP port", "tcpport", "0"});
    parser.process(app);

    QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        qDebug() << "Usage: program file.qml (--port /dev/ttyUSB0 --baudrate 115200) | (--tcp port)";
        return 1;
    }

    QString port = parser.value("port");
    int baudRate = parser.value("baudrate").toInt();
    int tcpPort = parser.value("tcp").toInt();

    bool use_serial = parser.isSet("port");
    bool use_tcp = parser.isSet("tcp");

    if (use_serial && use_tcp) {
        qDebug() << "Error: Cannot use serial port and TCP simultaneously";
        qDebug() << "Use --port for serial connection OR --tcp for TCP connection";
        return 1;
    }

    if (!use_serial && !use_tcp) {
        qDebug() << "Error: Must specify a communication method";
        qDebug() << "Use --port for serial connection OR --tcp for TCP connection";
        return 1;
    }

    GenericQMLBridge bridge;

    if (!bridge.loadQML(args.first())) {
        return 1;
    }

    if (use_serial) {
        if (!bridge.setupSerial(port, baudRate)) {
            qDebug() << "Error initializing serial port";
            return 1;
        }
    } else {
        if (!bridge.setupTCP(tcpPort)) {
            qDebug() << "Error initializing TCP server";
            return 1;
        }
        qDebug() << "TCP server listening on port" << tcpPort;
    }

    qDebug() << "Generic bridge started. QML:" << args.first();
    if (!port.isEmpty()) qDebug() << "Serial:" << port;
    if (tcpPort > 0) qDebug() << "TCP puerto:" << tcpPort;

    return app.exec();
}
