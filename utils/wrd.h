#ifndef WRD_H
#define WRD_H

#include "utils_global.h"
#include "binarydata.h"
#include "stx.h"

struct UTILS_EXPORT WrdCmd
{
    uchar opcode;
    QString name;
    QVector<ushort> args;
    QVector<uchar> arg_types;   // 0 = flag, 1 = raw number, 2 = string
};

// Official command names found in game_resident/command_label.dat
const WrdCmd KNOWN_CMDS[] = {
    {0x00, "FLG", {}, {0, 0}},              // Set Flag
    {0x01, "IFF", {}, {0, 0, 0}},           // If Flag
    {0x02, "WAK", {}, {0, 0, 0}},           // Wake? Work? (Seems to be used to configure game engine parameters)
    {0x03, "IFW", {}, {0, 0, 1}},           // If WAK?
    {0x04, "SWI", {}, {0}},                 // Begin switch statement
    {0x05, "CAS", {}, {1}},                 // Switch Case
    {0x06, "MPF", {}, {0, 0, 0}},           // Map Flag?
    {0x07, "SPW", {}, {}},
    {0x08, "MOD", {}, {0, 0, 0, 0}},        // Set Modifier (Also used to configure game engine parameters)
    {0x09, "HUM", {}, {0}},                 // Human? Seems to be used to initialize "interactable" objects in a map?
    {0x0A, "CHK", {}, {0}},                 // Check?
    {0x0B, "KTD", {}, {0, 0}},              // Kotodama?
    {0x0C, "CLR", {}, {}},                  // Clear?
    {0x0D, "RET", {}, {}},                  // Return? There's another command later which is definitely return, though...
    {0x0E, "KNM", {}, {0, 0, 0, 0, 0}},     // Kinematics (camera movement)
    {0x0F, "CAP", {}, {}},                  // Camera Parameters?
    {0x10, "FIL", {}, {0, 0}},              // Load Script File & jump to label
    {0x11, "END", {}, {}},                  // End of script or switch case
    {0x12, "SUB", {}, {0, 0}},              // Jump to subroutine
    {0x13, "RTN", {}, {}},                  // Return (called inside subroutine)
    {0x14, "LAB", {}, {1}},                 // Label number
    {0x15, "JMP", {}, {0}},                 // Jump to label
    {0x16, "MOV", {}, {0, 0}},              // Movie
    {0x17, "FLS", {}, {0, 0, 0, 0}},        // Flash
    {0x18, "FLM", {}, {0, 0, 0, 0, 0, 0}},  // Flash Modifier?
    {0x19, "VOI", {}, {0, 0}},              // Play voice clip
    {0x1A, "BGM", {}, {0, 0, 0}},           // Play BGM
    {0x1B, "SE_", {}, {0, 0}},              // Play sound effect
    {0x1C, "JIN", {}, {0, 0}},              // Play jingle
    {0x1D, "CHN", {}, {0}},                 // Set active character ID (current person speaking)
    {0x1E, "VIB", {}, {0, 0, 0}},           // Camera Vibration
    {0x1F, "FDS", {}, {0, 0, 0}},           // Fade Screen
    {0x20, "FLA", {}, {}},
    {0x21, "LIG", {}, {0, 1, 0}},           // Lighting Parameters
    {0x22, "CHR", {}, {0, 0, 0, 0, 0}},     // Character Parameters
    {0x23, "BGD", {}, {0, 0, 0, 0}},        // Background Parameters
    {0x24, "CUT", {}, {0, 0}},              // Cutin (display image for things like Truth Bullets, etc.)
    {0x25, "ADF", {}, {0, 0, 0, 0 ,0}},     // Character Vibration?
    {0x26, "PAL", {}, {}},
    {0x27, "MAP", {}, {0, 0, 0}},           // Load Map
    {0x28, "OBJ", {}, {0, 0, 0}},           // Load Object
    {0x29, "BUL", {}, {0,0,0,0,0,0,0,0}},
    {0x2A, "CRF", {}, {0,0,0,0,0,0,0}},     // Cross Fade
    {0x2B, "CAM", {}, {0, 0, 0, 0, 0}},     // Camera command
    {0x2C, "KWM", {}, {0}},                 // Game/UI Mode
    {0x2D, "ARE", {}, {0, 0, 0}},
    {0x2E, "KEY", {}, {0, 0}},              // Enable/disable "key" items for unlocking areas
    {0x2F, "WIN", {}, {0, 0, 0, 0}},        // Window parameters
    {0x30, "MSC", {}, {}},
    {0x31, "CSM", {}, {}},
    {0x32, "PST", {}, {0, 0, 1, 1, 1}},     // Post-Processing
    {0x33, "KNS", {}, {0, 1, 1, 1, 1}},     // Kinematics Numeric parameters?
    {0x34, "FON", {}, {1, 1}},              // Set Font
    {0x35, "BGO", {}, {0, 0, 0, 0, 0}},     // Load Background Object
    {0x36, "LOG", {}, {}},                  // Edit Text Backlog?
    {0x37, "SPT", {}, {0}},                 // Used only in Class Trial? Always set to "non"?
    {0x38, "CDV", {}, {0,0,0,0,0,0,0,0,0,0}},
    {0x39, "SZM", {}, {0, 0, 0, 0}},        // Size Modifier (Class Trial)?
    {0x3A, "PVI", {}, {0}},                 // Class Trial Chapter? Pre-trial intermission?
    {0x3B, "EXP", {}, {0}},                 // Give EXP
    {0x3C, "MTA", {}, {0}},                 // Used only in Class Trial? Usually set to "non"?
    {0x3D, "MVP", {}, {0, 0, 0}},           // Move object to its designated position?
    {0x3E, "POS", {}, {0, 0, 0, 0, 0}},     // Object/Exisal position
    {0x3F, "ICO", {}, {0,0,0,0}},           // Display a Program World character portrait
    {0x40, "EAI", {}, {0,0,0,0,0,0,0,0,0,0}},  // Exisal AI
    {0x41, "COL", {}, {0, 0, 0}},           // Set object collision
    {0x42, "CFP", {}, {0,0,0,0,0,0,0,0,0}}, // Camera Follow Path? Seems to make the camera move in some way
    {0x43, "CLT=", {}, {0}},                // Text modifier command
    {0x44, "R=", {}, {}},
    {0x45, "PAD=", {}, {0}},                // Gamepad button symbol
    {0x46, "LOC", {}, {2}},                 // Display text string
    {0x47, "BTN", {}, {}},                  // Wait for button press
    {0x48, "ENT", {}, {}},
    {0x49, "CED", {}, {}},                  // Check End (Used after IFF and IFW commands)
    {0x4A, "LBN", {}, {1}},                 // Local Branch Number (for branching case statements)
    {0x4B, "JMN", {}, {1}}                  // Jump to Local Branch (for branching case statements)
};

struct UTILS_EXPORT WrdFile
{
    QString filename;
    QStringList labels;
    QStringList params;
    QStringList strings;
    QVector<QVector<WrdCmd>> code;
    //QVector<ushort> sublabel_offsets;
    bool external_strings;
};

UTILS_EXPORT WrdFile wrd_from_bytes(const QByteArray &bytes, QString filename);
UTILS_EXPORT QByteArray wrd_to_bytes(const WrdFile &wrd);
//UTILS_EXPORT QVector<QVector<WrdCmd>> wrd_code_to_cmds(const QByteArray &bytes);
//UTILS_EXPORT QByteArray wrd_cmds_to_code(const QByteArray &wrd);

#endif // WRD_H
