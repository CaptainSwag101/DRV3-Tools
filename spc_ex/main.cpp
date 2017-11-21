#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include "../utils/binarydata.h"
#include "../utils/drv3_dec.h"


const QString SPC_MAGIC = "CPS.";
const QString CMP_MAGIC = "$CMP";
const QString TABLE_MAGIC = "Root";
static QTextStream cout(stdout);
static QDir inDir;
static QDir decDir;
static bool cmp = false;

int extract();
int extract_data(QString file, BinaryData &data);
int compress();
int compress_data(QString file, BinaryData &data);

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
        if (QString(argv[i]) == "-c" || QString(argv[i]) == "--compress")
            cmp = true;
    }

    if (cmp)
        return compress();
    else
        return extract();
}

int extract()
{
    decDir = inDir;
    decDir.cdUp();
    if (!decDir.exists("dec"))
    {
        if (!decDir.mkdir("dec"))
        {
            cout << "Error: Failed to create \"dec\" directory.\n";
            return 1;
        }
    }
    decDir.cd("dec");
    if (!decDir.exists(inDir.dirName()))
    {
        if (!decDir.mkdir(inDir.dirName()))
        {
            cout << "Error: Failed to create \"dec" << QDir::separator() << inDir.dirName() << "\" directory.\n";
            return 1;
        }
    }
    decDir.cd(inDir.dirName());

    QDirIterator it(inDir.path(), QStringList() << "*.spc", QDir::Files, QDirIterator::Subdirectories);
    QStringList spcFiles;
    while (it.hasNext())
        spcFiles.append(it.next());

    spcFiles.sort();
    for (int i = 0; i < spcFiles.length(); i++)
    {
        QString file = inDir.relativeFilePath(spcFiles[i]);
        cout << "Extracting file " << (i + 1) << "/" << spcFiles.length() << ": \"" << file << "\"\n";
        cout.flush();
        QFile f(spcFiles[i]);
        f.open(QFile::ReadOnly);
        QByteArray allBytes = f.readAll();
        f.close();
        BinaryData data(allBytes);

        int errCode = extract_data(file, data);
        if (errCode != 0)
            return errCode;
    }

    return 0;
}

int extract_data(QString file, BinaryData &data)
{
    decDir.mkpath(file);

    QString magic = data.get_str(4);
    if (magic == CMP_MAGIC)
    {
        data = srd_dec(data);
        magic = data.get_str(4);
    }

    if (magic != SPC_MAGIC)
    {
        cout << "Invalid SPC file.\n";
        return 1;
    }

    data.Position += 0x24; // unk1
    uint file_count = data.get_u32();
    data.Position += 0x14; // unk2, 0x10 bytes padding?

    QString table_magic = data.get_str(4);
    data.Position += 0x0C;

    if (table_magic != TABLE_MAGIC)
    {
        cout << "Invalid SPC file.\n";
        return 1;
    }

    for (uint i = 0; i < file_count; i++)
    {
        ushort cmp_flag = data.get_u16();
        ushort unk_flag = data.get_u16();
        uint cmp_size = data.get_u32();
        uint dec_size = data.get_u32();
        uint name_len = data.get_u32() + 1;    // Null terminator excluded from count
        data.Position += 0x10; // Padding?

        // Everything's aligned to multiples of 0x10
        uint name_padding = (0x10 - name_len % 0x10) % 0x10;
        uint data_padding = (0x10 - cmp_size % 0x10) % 0x10;

        // We don't actually want the null terminator byte, so pretend it's padding
        QString file_name = data.get_str(name_len - 1);
        data.Position += name_padding + 1;

        BinaryData file_data(data.get(cmp_size));
        data.Position += data_padding;

        switch (cmp_flag)
        {
        case 0x01:  // Uncompressed, don't do anything
            break;

        case 0x02:  // Compressed
            file_data = spc_dec(file_data);

            if (file_data.size() != dec_size)
                cout << "Size mismatch: size was " << file_data.size() << " but should be " << dec_size << "\n";
            break;

        case 0x03:  // Load from external file
            QString ext_file = file + "_" + file_name;
            //file_data = srd_dec(ext_file);
            break;
        }

        if (file_name.endsWith(".spc"))
        {
            QString sub_file = file + QDir::separator() + file_name;
            return extract_data(sub_file, file_data);
        }
        else
        {
            QFile out(decDir.path() + QDir::separator() + file + QDir::separator() + file_name);
            out.open(QFile::WriteOnly);
            out.write(file_data.Bytes);
            out.close();
        }
    }

    return 0;
}

int compress()
{
    decDir = inDir;
    decDir.cdUp();
    if (!decDir.exists("cmp"))
    {
        if (!decDir.mkdir("cmp"))
        {
            cout << "Error: Failed to create \"cmp\" directory.\n";
            return 1;
        }
    }
    decDir.cd("cmp");
    if (!decDir.exists(inDir.dirName()))
    {
        if (!decDir.mkdir(inDir.dirName()))
        {
            cout << "Error: Failed to create \"cmp" << QDir::separator() << inDir.dirName() << "\" directory.\n";
            return 1;
        }
    }
    decDir.cd(inDir.dirName());

    QDirIterator it(inDir.path(), QStringList() << "*.*", QDir::Files, QDirIterator::Subdirectories);
    QStringList spcSubfiles;
    while (it.hasNext())
        spcSubfiles.append(it.next());

    spcSubfiles.sort();
    for (int i = 0; i < spcSubfiles.length(); i++)
    {
        QString file = inDir.relativeFilePath(spcSubfiles[i]);
        cout << "Compressing file " << (i + 1) << "/" << spcSubfiles.length() << ": \"" << file << "\"\n";
        cout.flush();
        QFile f(spcSubfiles[i]);
        f.open(QFile::ReadOnly);
        QByteArray allBytes = f.readAll();
        f.close();
        BinaryData data(allBytes);

        int errCode = compress_data(file, data);
        if (errCode != 0)
            return errCode;
    }

    return 0;
}

int compress_data(QString file, BinaryData &data)
{
    BinaryData outData = spc_cmp(data);
    QFile outFile(decDir.path() + QDir::separator() + file + ".cmp");
    outFile.open(QFile::WriteOnly);
    outFile.write(outData.Bytes);
    outFile.close();

    return 0;
}
