#ifndef SLIPPROCESSOR_H
#define SLIPPROCESSOR_H

#include <QObject>
#include <QByteArray>

class SlipProcessor : public QObject
{
    Q_OBJECT

public:
    explicit SlipProcessor(QObject *parent = nullptr);

    static QByteArray encodeSlip(const QByteArray &data);

signals:
    void packetReceived(QByteArray packet);

public slots:
    void onDataReceived(const QByteArray &data);

private:
    QByteArray buffer;
    bool escapeNext = false;

    static constexpr uint8_t SLIP_END     = 0xC0;
    static constexpr uint8_t SLIP_ESC     = 0xDB;
    static constexpr uint8_t SLIP_ESC_END = 0xDC;
    static constexpr uint8_t SLIP_ESC_ESC = 0xDD;
};

#endif
