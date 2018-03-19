#ifndef WRD_H
#define WRD_H

#include "utils_global.h"
#include "binarydata.h"
#include "stx.h"

enum opcodes : ushort
{
    BOOL_FLAG = 0x7000,
    SET_VALUE = 0x7002,
    CHECK_VALUE = 0x7003,
    UNK_NUM_1 = 0x7005,
    SET_MODE = 0x7008,
    UNK_NUM_2 = 0x700A,
    SET_FLAG = 0x700B,
    CAMERA_MOVE = 0x700E,
    GOTO_EXTERN = 0x7010,
    SCRIPT_END = 0x7011,
    SUB_CALL = 0x7012,
    SUB_RETURN = 0x7013,
    LABEL_INDEX = 0x7014,
    GOTO = 0x7015,
    EVENT_SCENE = 0x7017,
    PLAY_VOICE = 0x7019,
    PLAY_BGM = 0x701A,
    PLAY_SFX = 0x701B,
    PLAY_JINGLE = 0x701C,
    SPEAKER_ID = 0x701D,
    CAMERA_SHAKE = 0x701E,
    SCENE_TRANS = 0x701F,
    MAP_PARAM = 0x7021,
    CHARA_PARAM = 0x7022,
    CHARA_SHAKE = 0x7025,
    LOAD_MAP = 0x7027,
    OBJ_PARAM = 0x7028,
    CAMERA_MODE = 0x702B,
    CAMERA_TRANS = 0x7033,
    LOAD_OBJ = 0x7035,
    LOAD_STRING = 0x7046,
    WAIT_FOR_INPUT = 0x7047,
    UNK_END = 0x704A,
    UNK_START = 0x704B,
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
    QList<QList<WrdCmd>> code;
    QList<uint> unk_data;
    bool external_strings;
};

UTILSSHARED_EXPORT WrdFile wrd_from_data(const QByteArray &data, QString filename);
UTILSSHARED_EXPORT QByteArray wrd_to_data(const WrdFile &wrd);
QList<QList<WrdCmd>> wrd_code_to_cmds(const QByteArray &data);
QByteArray wrd_cmds_to_code(const QByteArray &wrd);

#endif // WRD_H
