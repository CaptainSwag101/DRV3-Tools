#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include "../utils/binarydata.h"
#include "../utils/data_formats.h"

static QTextStream cout(stdout);
QDir inDir;
QDir decDir;
bool pack = false;

int unpack();
int unpack_data(QString file, BinaryData &data);
int repack();
BinaryData compress_data(BinaryData &data);

int main(int argc, char *argv[])
{
    // Parse args
    for (int i = 1; i < argc; i++)
    {
        if (QString(argv[i]) == "-p" || QString(argv[i]) == "--pack")
            pack = true;
        else
            inDir = QDir(argv[i]);
    }

    if (inDir == QDir(""))
    {
        cout << "Error: No input path specified.\n";
        return 1;
    }

    if (pack)
        return repack();
    else
        return unpack();
}

int unpack()
{
    decDir = inDir;
    decDir.cdUp();
    if (!decDir.exists("dec"))
    {
        if (!decDir.mkdir("dec"))
        {
            cout << "Error: Failed to create \"" << decDir.path() << QDir::separator() << "dec\" directory.\n";
            return 1;
        }
    }
    decDir.cd("dec");
    if (!decDir.exists(inDir.dirName()))
    {
        if (!decDir.mkdir(inDir.dirName()))
        {
            cout << "Error: Failed to create \"" << decDir.path() << QDir::separator() << "dec" << QDir::separator() << inDir.dirName() << "\" directory.\n";
            return 1;
        }
    }
    decDir.cd(inDir.dirName());

    QDirIterator it(inDir.path(), QStringList() << "*.spc", QDir::Files, QDirIterator::Subdirectories);
    QStringList spcNames;
    while (it.hasNext())
    {
        spcNames.append(it.next());
    }

    spcNames.sort();
    for (int i = 0; i < spcNames.length(); i++)
    {
        cout << "Extracting file " << (i + 1) << "/" << spcNames.length() << ": " << inDir.relativeFilePath(spcNames[i]) << "\n";
        cout.flush();
        QFile f(spcNames[i]);
        f.open(QFile::ReadOnly);
        QByteArray allBytes = f.readAll();
        f.close();
        BinaryData data(allBytes);

        int errCode = unpack_data(inDir.relativeFilePath(spcNames[i]), data);
        if (errCode != 0)
            return errCode;
    }

    return 0;
}

int unpack_data(QString file, BinaryData &data)
{
    decDir.mkpath(file);

    data.Position = 0;
    QString magic = data.get_str(4);
    if (magic == "$CMP")
    {
        return 0;
        data = srd_dec(data);
        data.Position = 0;
        magic = data.get_str(4);
    }

    if (magic != SPC_MAGIC)
    {
        cout << "Error: Invalid SPC file.\n";
        return 1;
    }


    // Create a text file containing index data and other info for the extracted files,
    // so we can re-pack them in the correct order (not sure if it matters though)
    QFile infoFile(decDir.path() + QDir::separator() + file + ".info");
    if (infoFile.exists())
        infoFile.remove();
    infoFile.open(QFile::WriteOnly);


    QByteArray unk1 = data.get(0x24);
    uint file_count = data.get_u32();
    uint unk2 = data.get_u32();
    data.Position += 0x10;   // padding?

    QString table_magic = data.get_str(4);
    data.Position += 0x0C;

    if (table_magic != SPC_TABLE_MAGIC)
    {
        cout << "Error: Invalid SPC file.\n";
        return 1;
    }

    infoFile.write(QString("file_count=" + QString::number(file_count) + "\n").toUtf8());

    for (uint i = 0; i < file_count; i++)
    {
        ushort cmp_flag = data.get_u16();
        ushort unk_flag = data.get_u16();
        uint cmp_size = data.get_u32();
        uint dec_size = data.get_u32();
        uint name_len = data.get_u32();
        data.Position += 0x10;  // Padding?

        // Everything's aligned to multiples of 0x10
        uint name_padding = (0x10 - (name_len + 1) % 0x10) % 0x10;
        uint data_padding = (0x10 - cmp_size % 0x10) % 0x10;

        QString file_name = data.get_str(name_len);
        // We don't want the null terminator byte, so pretend it's padding
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
            {
                cout << "Error: Size mismatch, size was " << file_data.size() << " but should be " << dec_size << "\n";
                cout.flush();
            }
            break;

        case 0x03:  // Load from external file
            QString ext_file_name = file + "_" + file_name;
            QFile ext_file(ext_file_name);
            ext_file.open(QFile::ReadOnly);
            BinaryData ext_data(ext_file.readAll());
            ext_file.close();
            file_data = srd_dec(ext_data);
            break;
        }

        /*
        if (file_name.endsWith(".spc"))
        {
            QString sub_file = file + QDir::separator() + file_name;
            return unpack_data(sub_file, file_data);
        }
        else
        {
            QFile out(decDir.path() + QDir::separator() + file + QDir::separator() + file_name);
            out.open(QFile::WriteOnly);
            out.write(file_data.Bytes);
            out.close();
        }
        */

        QFile out(decDir.path() + QDir::separator() + file + QDir::separator() + file_name);
        out.open(QFile::WriteOnly);
        out.write(file_data.Bytes);
        out.close();


        // Write file info
        infoFile.write(QString("\n").toUtf8());
        infoFile.write(QString("cmp_flag=" + QString::number(cmp_flag) + "\n").toUtf8());
        infoFile.write(QString("unk_flag=" + QString::number(unk_flag) + "\n").toUtf8());
        infoFile.write(QString("cmp_size=" + QString::number(cmp_size) + "\n").toUtf8());
        infoFile.write(QString("dec_size=" + QString::number(dec_size) + "\n").toUtf8());
        infoFile.write(QString("name_len=" + QString::number(name_len) + "\n").toUtf8());
        infoFile.write(QString("file_name=" + file_name + "\n").toUtf8());
        QString data_sha1(QCryptographicHash::hash(file_data.Bytes, QCryptographicHash::Sha1).toHex());
        infoFile.write(QString("data_sha1=" + data_sha1 + "\n").toUtf8());
    }
    infoFile.close();

    return 0;
}

int repack()
{
    decDir = inDir;
    decDir.cdUp();
    if (!decDir.exists("cmp"))
    {
        if (!decDir.mkdir("cmp"))
        {
            cout << "Error: Failed to create \"" << decDir.path() << QDir::separator() << "cmp\" directory.\n";
            cout.flush();
            return 1;
        }
    }
    decDir.cd("cmp");
    if (!decDir.exists(inDir.dirName()))
    {
        if (!decDir.mkdir(inDir.dirName()))
        {
            cout << "Error: Failed to create \"" << decDir.path() << QDir::separator() << "cmp" << QDir::separator() << inDir.dirName() << "\" directory.\n";
            return 1;
        }
    }
    decDir.cd(inDir.dirName());

    QDirIterator it(inDir.path(), QStringList() << "*.spc", QDir::Dirs, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QDir spcDir = QDir(it.next());
        QString outFile = inDir.relativeFilePath(spcDir.path());
        QFile infoFile(spcDir.path() + ".info");
        infoFile.open(QFile::ReadOnly | QIODevice::Text);
        QStringList infoStrings = QString(infoFile.readAll()).replace('\r', "").split('\n', QString::SkipEmptyParts);
        infoFile.close();

        int file_count = infoStrings[0].split('=')[1].toInt();

        BinaryData outData;
        outData.append(SPC_MAGIC.toUtf8());
        outData.append(QByteArray(0x04, 0x00));
        outData.append(QByteArray(0x08, 0xFF)); // unk1
        outData.append(QByteArray(0x18, 0x00));
        outData.append(from_u32(file_count));
        outData.append(from_u32(0x04));         // unk2
        outData.append(QByteArray(0x10, 0x00)); // padding

        outData.append(SPC_TABLE_MAGIC.toUtf8());
        outData.append(QByteArray(0x0C, 0x00)); // padding

        cout << "Compressing " << file_count << " files into " << spcDir.dirName() << "\n";
        cout.flush();
        for (int i = 1; i + 7 <= infoStrings.size(); i += 7)
        {
            QString file_name = infoStrings[i + 5].split('=')[1];
            cout << "    " << file_name << "\n";
            cout.flush();

            QFile f(spcDir.path() + QDir::separator() + file_name);
            f.open(QFile::ReadOnly);
            QByteArray allBytes = f.readAll();
            f.close();

            // TODO: Check if the file checksum matches the one in the info,
            // and re-use the existing data in the old SPC file.

            BinaryData subdata(allBytes);

            outData.append(from_u16(0x02));         // cmp_flag
            outData.append(from_u16(infoStrings[i + 1].split('=')[1].toUShort()));  // unk_flag

            uint dec_size = subdata.size();

            //subdata = spc_cmp(subdata);
            subdata = compress_data(subdata);

            uint cmp_size = subdata.size();
            outData.append(from_u32(cmp_size));     // cmp_size
            outData.append(from_u32(dec_size));     // dec_size
            uint name_len = file_name.length();     // name_len
            outData.append(from_u32(name_len));
            outData.append(QByteArray(0x10, 0x00)); // padding

            // Everything's aligned to multiples of 0x10
            uint name_padding = (0x10 - (name_len + 1) % 0x10) % 0x10;
            uint data_padding = (0x10 - cmp_size % 0x10) % 0x10;

            // We don't actually want the null terminator byte, so pretend it's padding
            outData.append(file_name.toUtf8());     // file_name
            outData.append(QByteArray(name_padding + 1, 0x00));

            outData.append(subdata.Bytes);          // data
            outData.append(QByteArray(data_padding, 0x00));

            /*
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
            */
        }

        QString outPath = QDir::toNativeSeparators(decDir.path() + QDir::separator() + outFile);
        QDir().mkpath(outPath.left(outPath.lastIndexOf(QDir::separator())));

        QFile out(outPath);
        out.open(QFile::WriteOnly);
        out.write(outData.Bytes);
        out.close();
    }

    return 0;
}

BinaryData compress_data(BinaryData &data)
{
    BinaryData outData = spc_cmp(data);
    BinaryData testData(outData.Bytes);
    testData = spc_dec(testData);

    for (int i = 0; i < data.size(); i++)
    {
        if (data[i] != testData[i])
        {
            cout << "Error: Compression test failed, inconsistent byte at 0x" << QString::number(i, 16).toUpper() << "\n";
        }
    }

    return outData;
}
