#ifndef DATADECODER_HPP
#define DATADECODER_HPP  

#include <QByteArray>
#include <QtEndian>
#include <QVariant>

class DataDecoder {
public:
    enum Endianess {
        LittleEndian,
        BigEndian
    };

    QVariant decodeInt32(const QByteArray &data, int offset = 0) const {
        if (data.size() < offset + 4) {
            return QVariant();
        }
        int32_t value;
        if (m_endianess == LittleEndian) {
            value = qFromLittleEndian<int32_t>(reinterpret_cast<const uchar *>(data.constData() + offset));
        } else {
            value = qFromBigEndian<int32_t>(reinterpret_cast<const uchar *>(data.constData() + offset));
        }
        return QVariant(value);
    }

    QVariant decodeInt16(const QByteArray &data, int offset = 0) const {
        if (data.size() < offset + 2) {
            return QVariant();
        }
        int16_t value;
        if (m_endianess == LittleEndian) {
            value = qFromLittleEndian<int16_t>(reinterpret_cast<const uchar *>(data.constData() + offset));
        } else {
            value = qFromBigEndian<int16_t>(reinterpret_cast<const uchar *>(data.constData() + offset));
        }
        return QVariant(value);
    }

    QVariant decodeFloat(const QByteArray &data, int offset = 0) const {
        if (data.size() < offset + 4) {
            return QVariant();
        }
        float value;
        if (m_endianess == LittleEndian) {
            value = qFromLittleEndian<float>(reinterpret_cast<const uchar *>(data.constData() + offset));
        } else {
            value = qFromBigEndian<float>(reinterpret_cast<const uchar *>(data.constData() + offset));
        }
        return QVariant(value);
    }

    QVariant decodeBool(const QByteArray &data, int offset = 0) const {
        if (data.size() < offset + 1) {
            return QVariant();
        }
        quint8 byte = static_cast<quint8>(data.at(offset));
        return QVariant(byte != 0);
    }

    void setEndianess(Endianess endianess) {
        m_endianess = endianess;
    }

    Endianess endianess() const {
        return m_endianess;
    }

private:
    Endianess m_endianess = LittleEndian;
};

#endif  // DATADECODER_HPP
