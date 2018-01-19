#include "binarydata.h"

const QByteArray from_u16(ushort n)
{
    QByteArray byteArray;
    for (int i = 0; i != sizeof(n); ++i)
    {
        char c = (char)(n >> (i * 8));
        byteArray.append(c);
    }
    return byteArray;
}

const QByteArray from_u32(uint n)
{
    QByteArray byteArray;
    for (int i = 0; i != sizeof(n); ++i)
    {
        char c = (char)(n >> (i * 8));
        byteArray.append(c);
    }
    return byteArray;
}

BinaryData::BinaryData()
{
    this->Position = 0;
}

BinaryData::BinaryData(QByteArray bytes)
{
    this->Bytes = bytes;
    this->Position = 0;
}

const QByteArray BinaryData::get(int len)
{
    QByteArray result = this->Bytes.mid(this->Position, len);
    this->Position += len;
    return result;
}

const QString BinaryData::get_str(int len, int bytes_per_char)
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

const uchar BinaryData::get_u8()
{
    return (uchar)(this->Bytes[this->Position++]);
}

const ushort BinaryData::get_u16()
{
    QByteArray ba = this->Bytes.mid(this->Position, 2);
    ushort result = ((ba[1] & 0xFF) << 8) + (ba[0] & 0xFF);
    this->Position += 2;
    return result;
}
const ushort BinaryData::get_u16be()
{
    QByteArray ba = this->Bytes.mid(this->Position, 2);
    ushort result = ((ba[1] & 0xFF) << 8) + (ba[0] & 0xFF);
    this->Position += 2;
    return result;
}

const uint BinaryData::get_u32()
{
    QByteArray ba = this->Bytes.mid(this->Position, 4);
    uint result = ((ba[3] & 0xFF) << 24) + ((ba[2] & 0xFF) << 16) + ((ba[1] & 0xFF) << 8) + (ba[0] & 0xFF);
    this->Position += 4;
    return result;
}
const uint BinaryData::get_u32be()
{
    QByteArray ba = this->Bytes.mid(this->Position, 4);
    uint result = ((ba[0] & 0xFF) << 24) + ((ba[1] & 0xFF) << 16) + ((ba[2] & 0xFF) << 8) + (ba[3] & 0xFF);
    this->Position += 4;
    return result;
}

const int BinaryData::size()
{
    return this->Bytes.size();
}

QByteArray& BinaryData::append(uchar c)
{
    this->Position++;
    return this->Bytes.append((char)c);
}

QByteArray& BinaryData::append(const QByteArray &ba)
{
    this->Position += ba.length();
    return this->Bytes.append(ba);
}

QByteArray& BinaryData::insert(int i, uchar c)
{
    this->Position++;
    return this->Bytes.insert(i, (char)c);
}

int BinaryData::lastIndexOf(const QByteArray &ba, int start, int end) const
{
    if (ba.size() == 0)
        return -1;

    if (end <= start)
        return -1;

    int index = this->Bytes.lastIndexOf(ba, end - 1);

    if (index < 0 || index < start)
        return -1;

    return index;
}

QByteRef BinaryData::operator[](int i)
{
    return this->Bytes[i];
}
