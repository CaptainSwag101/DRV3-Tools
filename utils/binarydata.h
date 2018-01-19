#ifndef BINARYDATA_H
#define BINARYDATA_H

#include <QByteArray>
#include <QtEndian>
#include <QString>

const QByteArray from_u16(ushort n);
const QByteArray from_u32(uint n);

struct BinaryData
{
    QByteArray Bytes;
    int Position;

    BinaryData();
    BinaryData(QByteArray data);
    const QByteArray get(int len);
    const QString get_str(int len = 0, int bytes_per_char = 1);
    const uchar get_u8();
    const ushort get_u16();
    const ushort get_u16be();
    const uint get_u32();
    const uint get_u32be();
    const int size();
    QByteArray& append(uchar c);
    QByteArray& append(const QByteArray &a);
    QByteArray& insert(int i, uchar c);
    int lastIndexOf(const QByteArray &ba, int start = -1, int end = 0) const;
    QByteRef operator[](int i);
};

#endif // BINARYDATA_H
