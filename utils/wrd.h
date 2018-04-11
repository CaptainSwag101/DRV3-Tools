#ifndef WRD_H
#define WRD_H

#include "utils_global.h"
#include "binarydata.h"
#include "stx.h"

struct UTILSSHARED_EXPORT WrdCmd
{
    uchar opcode;
    QString name;
    QList<ushort> args;
    QList<uchar> arg_types;  // 0 = flag, 1 = string, 2 = raw number
};

// Official command names found in game_resident/command_label.dat
static QList<WrdCmd> known_commands = {
    {0x00, "FLG", {}, {0, 0}},
    {0x01, "IFF", {}, {0, 0, 0}},
    {0x02, "WAK", {}, {0, 0, 0}},
    {0x03, "IFW", {}, {0, 0, 2}},
    {0x04, "SWI", {}, {}},
    {0x05, "CAS", {}, {2}},
    {0x06, "MPF", {}, {}},
    {0x07, "SPW", {}, {}},
    {0x08, "MOD", {}, {0, 0, 0, 0}},
    {0x09, "HUM", {}, {}},
    {0x0A, "CHK", {}, {0}},
    {0x0B, "KTD", {}, {0, 0}},
    {0x0C, "CLR", {}, {}},
    {0x0D, "RET", {}, {}},
    {0x0E, "KNM", {}, {0, 0, 0, 0, 0}},
    {0x0F, "CAP", {}, {}},
    {0x10, "FIL", {}, {0, 0}},
    {0x11, "END", {}, {}},
    {0x12, "SUB", {}, {0, 0}},
    {0x13, "RTN", {}, {}},
    {0x14, "LAB", {}, {2}},
    {0x15, "JMP", {}, {0}},
    {0x16, "MOV", {}, {}},
    {0x17, "FLS", {}, {0, 0, 0, 0}},
    {0x18, "FLM", {}, {}},
    {0x19, "VOI", {}, {0, 0}},
    {0x1A, "BGM", {}, {0, 0, 0}},
    {0x1B, "SE_", {}, {0, 0}},
    {0x1C, "JIN", {}, {0, 0}},
    {0x1D, "CHN", {}, {0}},
    {0x1E, "VIB", {}, {0, 0, 0}},
    {0x1F, "FDS", {}, {0, 0, 0}},
    {0x20, "FLA", {}, {}},
    {0x21, "LIG", {}, {0, 0, 0}},
    {0x22, "CHR", {}, {0, 0, 0, 0, 0}},
    {0x23, "BGD", {}, {}},
    {0x24, "CUT", {}, {}},
    {0x25, "ADF", {}, {0, 0, 0, 0 ,0}},
    {0x26, "PAL", {}, {}},
    {0x27, "MAP", {}, {0, 0, 0}},
    {0x28, "OBJ", {}, {0, 0, 0}},
    {0x29, "BUL", {}, {}},
    {0x2A, "CRF", {}, {}},
    {0x2B, "CAM", {}, {0, 0, 0, 0, 0}},
    {0x2C, "KWM", {}, {}},
    {0x2D, "ARE", {}, {}},
    {0x2E, "KEY", {}, {}},
    {0x2F, "WIN", {}, {}},
    {0x30, "MSC", {}, {}},
    {0x31, "CSM", {}, {}},
    {0x32, "PST", {}, {}},
    {0x33, "KNS", {}, {0, 0, 0, 0}},
    {0x34, "FON", {}, {}},
    {0x35, "BGO", {}, {0, 0, 0, 0, 0}},
    {0x36, "LOG", {}, {}},
    {0x37, "SPT", {}, {}},
    {0x38, "CDV", {}, {}},
    {0x39, "SZM", {}, {}},
    {0x3A, "PVI", {}, {}},
    {0x3B, "EXP", {}, {}},
    {0x3C, "MTA", {}, {}},
    {0x3D, "MVP", {}, {}},
    {0x3E, "POS", {}, {}},
    {0x3F, "ICO", {}, {}},
    {0x40, "EAI", {}, {}},
    {0x41, "COL", {}, {}},
    {0x42, "CFP", {}, {}},
    {0x43, "CLT=", {}, {}},
    {0x44, "R=", {}, {}},
    {0x45, "PAD=", {}, {}},
    {0x46, "LOC", {}, {1}},
    {0x47, "BTN", {}, {}},
    {0x48, "ENT", {}, {}},
    {0x49, "CED", {}, {}},
    {0x4A, "LBN", {}, {2}},
    {0x4B, "JMN", {}, {2}}
};

struct UTILSSHARED_EXPORT WrdFile
{
    QString filename;
    QStringList labels;
    QStringList flags;
    QStringList strings;
    QList<QList<WrdCmd>> code;
    QList<uint> unk_data;
    bool external_strings;
};

UTILSSHARED_EXPORT WrdFile wrd_from_bytes(const QByteArray &bytes, QString filename);
UTILSSHARED_EXPORT QByteArray wrd_to_bytes(const WrdFile &wrd);
UTILSSHARED_EXPORT QList<QList<WrdCmd>> wrd_code_to_cmds(const QByteArray &bytes);
UTILSSHARED_EXPORT QByteArray wrd_cmds_to_code(const QByteArray &wrd);

#endif // WRD_H
