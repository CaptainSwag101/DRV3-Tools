#ifndef BINARYDATA_H
#define BINARYDATA_H

#include <QByteArray>
#include <QtEndian>
#include <QString>

template <typename T>
QByteArray to_bytes(T value)
{
    QByteArray byteArray;
    for (int i = 0; i != sizeof(value); ++i)
    {
        char c = (char)(value >> (i * 8));
        byteArray.append(c);
    }
    return byteArray;
}

class BinaryData
{
public:
    QByteArray Bytes;
    int Position;

    BinaryData();
    BinaryData(QByteArray data);
    QByteArray get(int len);
    QString get_str(int len = 0, int bytes_per_char = 1);
    uchar get_u8();
    ushort get_u16();
    ushort get_u16be();
    uint get_u32();
    uint get_u32be();
    int size();
    QByteArray& append(uchar c);
    QByteArray& append(const QByteArray &a);
    QByteArray& insert(int i, uchar c);
    QByteRef operator[](int i);
};

#endif // BINARYDATA_H
