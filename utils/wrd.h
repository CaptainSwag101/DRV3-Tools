#ifndef WRD_H
#define WRD_H

#include "utils_global.h"
#include "binarydata.h"
#include "stx.h"

enum opcodes : ushort
{
    SET_FLAG = 0x7000,
    SET_VALUE = 0x7002,
    CHECK_VALUE = 0x7003,
    SET_MODIFIER = 0x7008,
    SET_FLAG_2 = 0x700B,
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
    CAMERA_TRANS = 0x7033,
    LOAD_OBJ = 0x7035,
    DISPLAY_TEXT = 0x7046,
    WAIT_FOR_INPUT = 0x7047,
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

#endif // WRD_H
