#include "binarydata.h"

const QByteArray from_u16(ushort n)
{
    QByteArray byteArray;
    byteArray.reserve(2);
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
    byteArray.reserve(4);
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

BinaryData::BinaryData(int reserve_size)
{
    this->Bytes.reserve(reserve_size);
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
    result.reserve(len);    // Reserve memory for "len" amount of UTF-16 characters, just in case

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

const char BinaryData::get_u8()
{
    return this->Bytes.at(this->Position++);
}

const ushort BinaryData::get_u16()
{
    const ushort result = ((this->Bytes.at(this->Position + 1) & 0xFF) << 8) + (this->Bytes.at(this->Position + 0) & 0xFF);
    this->Position += 2;
    return result;
}
const ushort BinaryData::get_u16be()
{
    const ushort result = ((this->Bytes.at(this->Position + 0) & 0xFF) << 8) + (this->Bytes.at(this->Position + 1) & 0xFF);
    this->Position += 2;
    return result;
}

const uint BinaryData::get_u32()
{
    const uint result = ((this->Bytes.at(this->Position + 3) & 0xFF) << 24) + ((this->Bytes.at(this->Position + 2) & 0xFF) << 16) + ((this->Bytes.at(this->Position + 1) & 0xFF) << 8) + (this->Bytes.at(this->Position + 0) & 0xFF);
    this->Position += 4;
    return result;
}
const uint BinaryData::get_u32be()
{
    const uint result = ((this->Bytes.at(this->Position + 0) & 0xFF) << 24) + ((this->Bytes.at(this->Position + 1) & 0xFF) << 16) + ((this->Bytes.at(this->Position + 2) & 0xFF) << 8) + (this->Bytes.at(this->Position + 3) & 0xFF);
    this->Position += 4;
    return result;
}

const int BinaryData::size()
{
    return this->Bytes.size();
}

QByteArray& BinaryData::append(char c)
{
    this->Position++;
    return this->Bytes.append(c);
}

QByteArray& BinaryData::append(const QByteArray &a)
{
    this->Position += a.size();
    return this->Bytes.append(a);
}

QByteArray& BinaryData::insert(int i, char c)
{
    this->Position++;
    return this->Bytes.insert(i, c);
}

int BinaryData::lastIndexOf(const QByteArray &a, int start, int end) const
{
    if (a.size() == 0)
        return -1;

    if (end <= start)
        return -1;

    int index = this->Bytes.lastIndexOf(a, end - 1);

    if (index < 0 || index < start)
        return -1;

    return index;
}

char BinaryData::at(int i) const
{
    return this->Bytes.at(i);
}
