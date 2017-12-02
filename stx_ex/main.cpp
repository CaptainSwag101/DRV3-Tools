#include <QDir>
#include <QDirIterator>
#include <QTextCodec>
#include <QTextStream>
#include "../utils/binarydata.h"

const QString STX_MAGIC = "STXT";
static QTextStream cout(stdout);
static QDir inDir;
static QDir exDir;
static bool pack = false;

int unpack();
int unpack_data(QString file, BinaryData &data);
int repack();
BinaryData pack_data(BinaryData &data);

int main(int argc, char *argv[])
{
    // Parse args
    if (argc < 2)
    {
        cout << "No input path specified.\n";
        return 1;
    }
    inDir = QDir(argv[1]);

    for (int i = 0; i < argc; i++)
    {
        if (QString(argv[i]) == "-p" || QString(argv[i]) == "--pack")
            pack = true;
    }

    if (pack)
        return repack();
    else
        return unpack();
}

int unpack()
{
    exDir = inDir;
    exDir.cdUp();
    if (!exDir.exists("ex"))
    {
        if (!exDir.mkdir("ex"))
        {
            cout << "Error: Failed to create \"ex\" directory.\n";
            return 1;
        }
    }
    exDir.cd("ex");
    if (!exDir.exists(inDir.dirName()))
    {
        if (!exDir.mkdir(inDir.dirName()))
        {
            cout << "Error: Failed to create \"ex" << QDir::separator() << inDir.dirName() << "\" directory.\n";
            return 1;
        }
    }
    exDir.cd(inDir.dirName());

    QDirIterator it(inDir.path(), QStringList() << "*.stx", QDir::Files, QDirIterator::Subdirectories);
    QStringList stxFiles;
    while (it.hasNext())
        stxFiles.append(it.next());

    stxFiles.sort();
    for (int i = 0; i < stxFiles.length(); i++)
    {
        QString file = inDir.relativeFilePath(stxFiles[i]);
        cout << "Extracting file " << (i + 1) << "/" << stxFiles.length() << ": \"" << file << "\"\n";
        cout.flush();
        QFile f(stxFiles[i]);
        f.open(QFile::ReadOnly);
        QByteArray allBytes = f.readAll();
        f.close();
        BinaryData data(allBytes);

        unpack_data(file, data);
    }

    return 0;
}

int unpack_data(QString file, BinaryData &data)
{
    exDir.mkpath(file.left(QDir::toNativeSeparators(file).lastIndexOf(QDir::separator())));

    QString outFile = file;
    outFile.replace(".stx", ".txt");

    data.Position = 0;
    QString magic = data.get_str(4);
    if (magic != STX_MAGIC)
    {
        cout << "Invalid STX file.\n";
        return 1;
    }

    QString lang = data.get_str(4); // "JPLL" in the JP and US versions
    uint unk1 = data.get_u32();  // Table count?
    uint table_off  = data.get_u32();
    uint unk2 = data.get_u32();
    uint count = data.get_u32();

    QFile textFile(exDir.path() + QDir::separator() + outFile);
    textFile.open(QFile::WriteOnly);

    for (int i = 0; i < count; i++)
    {
        data.Position = table_off + (8 * i);
        uint str_id = data.get_u32();
        uint str_off = data.get_u32();

        data.Position = str_off;

        QString str = data.get_str(0, 2);
        //strings[str_id] = str;

        textFile.write(QString("##### " + QString::number(str_id).rightJustified(4, '0') + "\n").toUtf8());
        textFile.write(QString(str + "\n\n").toUtf8());
    }

    textFile.flush();
    textFile.close();

    return 0;
}

int repack()
{

    if (!exDir.exists("cmp"))
    {
        if (!exDir.mkdir("cmp"))
        {
            cout << "Error: Failed to create \"cmp\" directory.\n";
            return 1;
        }
    }
    exDir.cd("cmp");

    exDir.cd("cmp");
    if (!exDir.exists(inDir.dirName()))
    {
        if (!exDir.mkdir(inDir.dirName()))
        {
            cout << "Error: Failed to create \"cmp" << QDir::separator() << inDir.dirName() << "\" directory.\n";
            return 1;
        }
    }
    exDir.cd(inDir.dirName());

    QString test = inDir.path();
    QDirIterator it(inDir.path(), QStringList() << "*.txt", QDir::Files, QDirIterator::Subdirectories);
    QStringList txtFiles;
    while (it.hasNext())
        txtFiles.append(it.next());

    txtFiles.sort();
    for (int f = 0; f < txtFiles.length(); f++)
    {
        QString file = inDir.relativeFilePath(txtFiles[f]);
        cout << "Re-packing file " << (f + 1) << "/" << txtFiles.length() << ": \"" << file << "\"\n";
        cout.flush();
        QFile txt(txtFiles[f]);
        txt.open(QFile::ReadOnly | QIODevice::Text);
        QStringList allText = QString(txt.readAll()).replace("\r\n", "\n").replace("\r", "\n").split("\n", QString::SkipEmptyParts);
        txt.close();

        QMap<int, QString> strings;
        QString str;
        int index = 0;
        int highest_index = 0;
        for (QString strPart : allText)
        {
            if (strPart.startsWith("#"))
            {
                if (!str.isEmpty())
                {
                    strings[index] = str;
                    str.clear();
                }

                index = strPart.replace("##### ", "").toInt();
                if (index > highest_index)
                    highest_index = index;

                continue;
            }

            if (!str.isEmpty())
                str += "\n";
            str += strPart;
        }
        strings[index] = str;
        str.clear();

        // table must contain an even number of entries
        if (highest_index % 2 != 0)
            highest_index += (highest_index % 2);

        BinaryData stxData;
        stxData.append(QString("STXTJPLL").toUtf8());   // header
        stxData.append(from_u32(0x01));                 // table_count?
        stxData.append(from_u32(0x20));                 // table_off
        stxData.append(from_u32(0x08));                 // unk2
        stxData.append(from_u32(highest_index));        // count
        stxData.append(QByteArray(0x20 - stxData.Position, (char)0x00));   // padding

        int table_len = 8 * highest_index;      // 8 bytes per table entry

        for (int i = 0; i < highest_index; i++)
        {
            stxData.append(from_u32(i));        // str_index

            int prev_len_sum = 0;
            for (int j = 0; j < i; j++)
            {
                if (strings[j] == strings[i])
                {
                    prev_len_sum -= 2 * (i - j);
                    break;
                }

                prev_len_sum += 2 * (strings[j].size() + 1);    // 2 bytes per character
            }

            int str_off = 0x20 + table_len + prev_len_sum;
            stxData.append(from_u32(str_off));
        }

        QStringList written;
        for (QString s : strings.values())
        {
            if (written.contains(s))
                break;

            QTextCodec *codec = QTextCodec::codecForName("UTF-16");
            QTextEncoder *encoder = codec->makeEncoder(QTextCodec::IgnoreHeader);
            QByteArray bytes = encoder->fromUnicode(strings[index]);

            stxData.append(bytes);
            stxData.append(from_u16(0x00)); // null terminator
            written.append(s);
        }

        QString outPath = QDir::toNativeSeparators(inDir.path() + QDir::separator() + exDir.path() + QDir::separator() + file);
        outPath.replace(".txt", ".stx");
        QDir().mkpath(outPath.left(outPath.lastIndexOf(QDir::separator())));
        QFile outFile(outPath);
        outFile.open(QFile::WriteOnly);
        outFile.write(stxData.Bytes);
        outFile.close();
    }

    return 0;
}

BinaryData pack_data(BinaryData &data)
{
    BinaryData result;

    return result;
}
