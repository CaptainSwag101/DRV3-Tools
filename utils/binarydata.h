#ifndef BINARYDATA_H
#define BINARYDATA_H

#include <QByteArray>
#include <QString>
#include <QtEndian>

template <class T> QByteArray num_to_bytes(T num, bool big_endian = false);
template <class T> T bytes_to_num<T>(const QByteArray &data, int &pos, bool big_endian = false);
QString bytes_to_str(const QByteArray &data, int &pos, int len = -1, bool utf16 = false);
QByteArray get_bytes(const QByteArray &data, int &pos, int len = -1);

#endif // BINARYDATA_H
