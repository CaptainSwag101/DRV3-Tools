#include "binarydata.h"

QByteArray from_u16(ushort n)
{
    QByteArray byteArray;
    for (int i = 0; i != 2; ++i)
    {
        byteArray.append((char)(n >> (i * 8)));
    }
    return byteArray;
}

QByteArray from_u32(uint n)
{
    QByteArray byteArray;
    for (int i = 0; i != 4; ++i)
    {
        byteArray.append((char)(n >> (i * 8)));
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

QByteArray BinaryData::get(int len)
{
    QByteArray result = this->Bytes.mid(this->Position, len);
    this->Position += len;

    return result;
}

QString BinaryData::get_str(int len, bool utf16)
{
    QString result;
    result.reserve(len);    // Reserve memory for "len" amount of UTF-16 characters, just in case

    int i = 0;
    while (len < 0 || i < len)
    {
        QChar c;
        if (utf16)
            c = QChar(get_u16());
        else
            c = QChar(get_u8());

        i++;

        if (len < 0 && c == QChar(0))
            break;

        result.append(c);
    }

    return result;
}

char BinaryData::get_u8()
{
    return this->Bytes.at(this->Position++);
}

ushort BinaryData::get_u16()
{
    ushort result = ((this->Bytes.at(this->Position + 1) & 0xFF) << 8) + (this->Bytes.at(this->Position + 0) & 0xFF);
    this->Position += 2;
    return result;
}
ushort BinaryData::get_u16be()
{
    ushort result = ((this->Bytes.at(this->Position + 0) & 0xFF) << 8) + (this->Bytes.at(this->Position + 1) & 0xFF);
    this->Position += 2;
    return result;
}

uint BinaryData::get_u32()
{
    uint result = ((this->Bytes.at(this->Position + 3) & 0xFF) << 24) + ((this->Bytes.at(this->Position + 2) & 0xFF) << 16) + ((this->Bytes.at(this->Position + 1) & 0xFF) << 8) + (this->Bytes.at(this->Position + 0) & 0xFF);
    this->Position += 4;
    return result;
}
uint BinaryData::get_u32be()
{
    uint result = ((this->Bytes.at(this->Position + 0) & 0xFF) << 24) + ((this->Bytes.at(this->Position + 1) & 0xFF) << 16) + ((this->Bytes.at(this->Position + 2) & 0xFF) << 8) + (this->Bytes.at(this->Position + 3) & 0xFF);
    this->Position += 4;
    return result;
}

int BinaryData::size()
{
    return this->Bytes.size();
}

QByteArray& BinaryData::append(char c)
{
    this->Position++;
    return this->Bytes.append(c);
}

QByteArray& BinaryData::append(QByteArray a)
{
    this->Position += a.size();
    return this->Bytes.append(a);
}

QByteArray& BinaryData::insert(int i, char c)
{
    this->Position++;
    return this->Bytes.insert(i, c);
}

int BinaryData::lastIndexOf(QByteArray a, int start, int end) const
{
    if (a.size() == 0)
        return -1;

    if (end < start)
        return -1;

    int index = this->Bytes.lastIndexOf(a, end);

    if (index < 0 || index < start)
        return -1;

    return index;
}

char BinaryData::at(int i) const
{
    return this->Bytes.at(i);
}
