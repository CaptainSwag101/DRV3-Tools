#include <QDir>
#include <QDirIterator>
#include <QTextCodec>
#include <QTextStream>
#include "../utils/binarydata.h"

const QString STX_MAGIC = "STXT";
static QTextStream cout(stdout);
static QDir inDir;
static QDir exDir;
static bool repack = false;

int unpack();
int unpack_data(QString file, BinaryData &data);
int pack();
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
            repack = true;
    }

    if (repack)
        return pack();
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

        int errCode = unpack_data(file, data);
        if (errCode != 0)
            return errCode;
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

int pack()
{

    return 0;
}

BinaryData pack_data(BinaryData &data)
{
    BinaryData result;

    return result;
}
