#include <QDir>
#include <QDirIterator>
#include <QTextCodec>
#include <QTextStream>
#include "../utils/binarydata.h"
#include "../utils/drv3_dec.h"

static QTextStream cout(stdout);
QDir inDir;
QDir exDir;
bool pack = false;

int unpack();
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

        exDir.mkpath(file.left(QDir::toNativeSeparators(file).lastIndexOf(QDir::separator())));

        QString outFile = file;
        outFile.replace(".stx", ".txt");

        QStringList strings = get_stx_strings(data);

        QFile textFile(exDir.path() + QDir::separator() + outFile);
        textFile.open(QFile::WriteOnly);
        for (int i = 0; i < strings.size(); i++)
        {
            textFile.write(QString("##### " + QString::number(i).rightJustified(4, '0') + "\n").toUtf8());
            textFile.write(QString(strings[i] + "\n\n").toUtf8());
        }
        textFile.close();

    }

    return 0;
}

int repack()
{
    // IMPORTANT: This feature is currently not re-packing the STX files properly,
    // so although they may load just fine in the game, they probably won't
    // behave correctly. Please use the STX editor instead!

    exDir = inDir;
    exDir.cdUp();
    if (!exDir.exists("cmp"))
    {
        if (!exDir.mkdir("cmp"))
        {
            cout << "Error: Failed to create \"cmp\" directory.\n";
            return 1;
        }
    }

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

        // The original files (usually) use Unix-style line endings.
        // A couple files sometimes have the occasional Windows-style line ending,
        // but since these are almost certainly not intentional, we sanitize them anyway.
        QStringList allText = QString(txt.readAll()).replace("\r\n", "\n").replace("\r", "\n").split("\n", QString::SkipEmptyParts);

        txt.close();

        QMap<int, QString> strings;
        QString str;
        int index = 0;
        int table_len = 0;
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
                table_len++;
                continue;
            }

            if (!str.isEmpty())
            {
                str += "\n";
            }
            str += strPart;
        }
        strings[index] = str;
        str.clear();

        // Table must contain an even number of entries?
        //if (table_len % 2 != 0)
        //    table_len += (table_len % 2);

        BinaryData stxData;
        stxData.append(QString("STXTJPLL").toUtf8());   // header
        stxData.append(from_u32(0x01));                 // table_count?
        stxData.append(from_u32(0x20));                 // table_off
        stxData.append(from_u32(0x08));                 // unk2
        stxData.append(from_u32(table_len));            // count
        stxData.append(QByteArray(0x20 - stxData.Position, (char)0x00));   // padding

        int *offsets = new int[table_len];
        int highest_index = 0;
        for (int i = 0; i < table_len; i++)
        {
            int str_off = 0;
            for (int j = 0; j < i; j++)
            {
                if (strings[i] == strings[j])
                {
                    str_off = offsets[j];
                }
            }

            // If there are no previous occurences of this string or it's the first one
            if (i == 0)
            {
                str_off = 0x20 + (8 * table_len);   // 8 bytes per table entry
            }
            else if (str_off == 0)
            {
                str_off = offsets[highest_index] + ((strings[highest_index].size() + 1) * 2);
                highest_index = i;
            }

            offsets[i] = str_off;

            stxData.append(from_u32(i));        // str_index
            stxData.append(from_u32(str_off));  // str_off
        }

        QStringList written;
        for (int i = 0; i < table_len; i++)
        {
            if (written.contains(strings[i]))
                continue;

            QTextCodec *codec = QTextCodec::codecForName("UTF-16");
            QTextEncoder *encoder = codec->makeEncoder(QTextCodec::IgnoreHeader);
            QByteArray bytes = encoder->fromUnicode(strings[i]);

            stxData.append(bytes);
            stxData.append(from_u16(0x00)); // Null terminator
            written.append(strings[i]);
        }
        delete[] offsets;

        QString outPath = QDir::toNativeSeparators(exDir.path() + QDir::separator() + file);
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
