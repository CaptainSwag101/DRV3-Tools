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

static QList<WrdCmd> known_commands = {
    {0x00, "BOOL_FLAG", {}, {0, 0}},
    {0x01, "CHECK_FLAG", {}, {0, 0, 0}},
    {0x02, "SET_VALUE", {}, {0, 0, 0}},
    {0x03, "CHECK_NUM", {}, {0, 0, 2}},
    {0x05, "UNK_NUM_1", {}, {2}},
    {0x08, "SET_MODE", {}, {0, 0, 0, 0}},
    {0x0A, "UNK_NUM_2", {}, {2}},
    {0x0B, "SET_FLAG", {}, {0, 0}},
    {0x0E, "MOVE_CAMERA", {}, {0, 0, 0, 0, 0}},
    {0x10, "GOTO_EXTERN", {}, {0, 0}},
    {0x11, "SCRIPT_END", {}, {}},
    {0x12, "SUB_CALL", {}, {0, 0}},
    {0x13, "SUB_RETURN", {}, {}},
    {0x14, "LABEL_INDEX", {}, {2}},
    {0x15, "GOTO", {}, {0}},
    {0x17, "EVENT_SCENE", {}, {0, 0, 0, 0}},
    {0x19, "PLAY_VOICE", {}, {0, 0}},
    {0x1A, "PLAY_BGM", {}, {0, 0, 0}},
    {0x1B, "PLAY_SFX", {}, {0, 0}},
    {0x1C, "PLAY_JINGLE", {}, {0, 0}},
    {0x1D, "SPEAKER_ID", {}, {0}},
    {0x1E, "CAMERA_SHAKE", {}, {0, 0, 0}},
    {0x1F, "SCENE_TRANS", {}, {0, 0, 0}},
    {0x21, "MAP_PARAM", {}, {0, 0, 0}},
    {0x22, "CHARA_PARAM", {}, {0, 0, 0, 0, 0}},
    {0x25, "CHARA_SHAKE", {}, {0, 0, 0, 0 ,0}},
    {0x27, "LOAD_MAP", {}, {0, 0, 0}},
    {0x28, "OBJ_PARAM", {}, {0, 0, 0}},
    {0x2B, "CAMERA_MODE", {}, {0, 0, 0, 0, 0}},
    {0x33, "CAMERA_TRANS", {}, {0, 0, 0, 0}},
    {0x35, "LOAD_OBJ", {}, {0, 0, 0, 0, 0}},
    {0x46, "LOAD_STRING", {}, {1}},
    {0x47, "PRINT_TEXTBOX", {}, {}},
    {0x4A, "UNK_END", {}, {2}},
    {0x4B, "UNK_START", {}, {2}}
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

UTILSSHARED_EXPORT WrdFile wrd_from_data(const QByteArray &data, QString filename);
UTILSSHARED_EXPORT QByteArray wrd_to_data(const WrdFile &wrd);
QList<QList<WrdCmd>> wrd_code_to_cmds(const QByteArray &data);
QByteArray wrd_cmds_to_code(const QByteArray &wrd);

#endif // WRD_H
