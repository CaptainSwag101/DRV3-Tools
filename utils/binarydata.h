#ifndef BINARYDATA_H
#define BINARYDATA_H

#include <QByteArray>
#include <QtEndian>
#include <QString>

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
};

#endif // BINARYDATA_H
