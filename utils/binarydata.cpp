#include "binarydata.h"

BinaryData::BinaryData()
{
    this->Position = 0;
}

BinaryData::BinaryData(QByteArray bytes)
{
    this->Bytes = bytes;
    this->Position = 0;
}

QByteArray BinaryData::get(int len)
{
    QByteArray result = this->Bytes.mid(this->Position, len);
    this->Position += len;
    return result;
}

QString BinaryData::get_str(int len, int bytes_per_char)
{
    QString result;

    int i = 0;
    while (len <= 0 || i < len * bytes_per_char)
    {
        QChar c;
        if (bytes_per_char == 1)
        {
            c = QChar(get_u8());
            i++;
        }
        else if (bytes_per_char == 2)
        {
            c = QChar(get_u16());
            i += 2;
        }

        if (len <= 0 && c == QChar(0))
            break;

        result.append(c);
    }

    return result;
}

uchar BinaryData::get_u8()
{
    return (uchar)(this->Bytes[this->Position++]);
}

ushort BinaryData::get_u16()
{
    QByteArray b = this->Bytes.mid(this->Position, 2);
    ushort result = ((b[1] & 0xFF) << 8) + (b[0] & 0xFF);
    this->Position += 2;
    return result;
}
ushort BinaryData::get_u16be()
{
    QByteArray b = this->Bytes.mid(this->Position, 2);
    ushort result = ((b[1] & 0xFF) << 8) + (b[0] & 0xFF);
    this->Position += 2;
    return result;
}

uint BinaryData::get_u32()
{
    QByteArray b = this->Bytes.mid(this->Position, 4);
    uint result = ((b[3] & 0xFF) << 24) + ((b[2] & 0xFF) << 16) + ((b[1] & 0xFF) << 8) + (b[0] & 0xFF);
    this->Position += 4;
    return result;
}
uint BinaryData::get_u32be()
{
    QByteArray b = this->Bytes.mid(this->Position, 4);
    uint result = ((b[0] & 0xFF) << 24) + ((b[1] & 0xFF) << 16) + ((b[2] & 0xFF) << 8) + (b[3] & 0xFF);
    this->Position += 4;
    return result;
}

int BinaryData::size()
{
    return this->Bytes.size();
}
