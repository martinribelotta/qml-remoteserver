#include "slipprocessor.h"

#include <QDebug>

SlipProcessor::SlipProcessor(QObject *parent)
    : QObject(parent)
{
}

QByteArray SlipProcessor::encodeSlip(const QByteArray &input)
{
    QByteArray encoded;

    for (char byte : input) {
        if ((uint8_t)byte == SLIP_END) {
            encoded.append(SLIP_ESC);
            encoded.append(SLIP_ESC_END);
        } else if ((uint8_t)byte == SLIP_ESC) {
            encoded.append(SLIP_ESC);
            encoded.append(SLIP_ESC_ESC);
        } else {
            encoded.append(byte);
        }
    }

    encoded.append(SLIP_END);
    return encoded;
}

void SlipProcessor::onDataReceived(const QByteArray &data)
{
    for (uint8_t byte : data) {
        if (escapeNext) {
            if (byte == SLIP_ESC_END)
                buffer.append(SLIP_END);
            else if (byte == SLIP_ESC_ESC)
                buffer.append(SLIP_ESC);
            else {
                buffer.clear();
            }
            escapeNext = false;
        } else {
            if (byte == SLIP_END) {
                if (!buffer.isEmpty()) {
                    emit packetReceived(buffer);
                    buffer.clear();
                }
            } else if (byte == SLIP_ESC) {
                escapeNext = true;
            } else {
                buffer.append(byte);
            }
        }
    }
}

