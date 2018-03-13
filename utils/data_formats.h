#pragma once

#include "utils_global.h"
#include <cmath>
#include <QDir>
#include <QException>
#include <QFileInfo>
#include <QHash>
#include <QTextCodec>
#include <QTextStream>
#include <QVector>
#include "binarydata.h"

const QString SPC_MAGIC = "CPS.";
const QString SPC_TABLE_MAGIC = "Root";
const QString STX_MAGIC = "STXT";

struct UTILSSHARED_EXPORT SpcSubfile
{
    QString filename;
    QByteArray data;
    ushort cmp_flag;
    ushort unk_flag;
    int cmp_size;
    int dec_size;
    int name_len;
};

struct UTILSSHARED_EXPORT SpcFile
{
    QString filename;
    QByteArray unk1;
    uint unk2;
    QList<SpcSubfile> subfiles;
};

struct UTILSSHARED_EXPORT WrdCmd
{
    ushort opcode;
    QList<ushort> args;
};

struct UTILSSHARED_EXPORT WrdFile
{
    QString filename;
    QStringList labels;
    QStringList flags;
    QStringList strings;
    QList<WrdCmd> cmds;
};

UTILSSHARED_EXPORT inline uchar bit_reverse(uchar b);
UTILSSHARED_EXPORT SpcFile spc_from_data(const QByteArray &data);
UTILSSHARED_EXPORT QByteArray spc_to_data(const SpcFile &spc);
UTILSSHARED_EXPORT QByteArray spc_dec(const QByteArray &data, int dec_size = -1);
UTILSSHARED_EXPORT QByteArray spc_cmp(const QByteArray &data);
UTILSSHARED_EXPORT QByteArray srd_dec(const QByteArray &data);
UTILSSHARED_EXPORT QByteArray srd_dec_chunk(const QByteArray &chunk, QString cmp_mode);
UTILSSHARED_EXPORT QStringList get_stx_strings(const QByteArray &data);
UTILSSHARED_EXPORT QByteArray repack_stx_strings(QStringList strings);
UTILSSHARED_EXPORT WrdFile wrd_from_data(const QByteArray &data, QString filename);
UTILSSHARED_EXPORT QByteArray wrd_to_data(const WrdFile &wrd);
