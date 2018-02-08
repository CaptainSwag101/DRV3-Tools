#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include "../utils/binarydata.h"
#include "../utils/data_formats.h"

static QTextStream cout(stdout);

int unpack(QString in_dir);
int unpack_data(BinaryData &data, QString out_dir);
int repack(QString in_dir);
BinaryData compress_data(BinaryData &data);

int main(int argc, char *argv[])
{
    QString in_dir;
    bool pack = false;

    // Parse args
    for (int i = 1; i < argc; i++)
    {
        QString arg = QString(argv[i]);

        if (arg == "-p" || arg == "--pack")
            pack = true;
        else
            in_dir = QDir(argv[i]).absolutePath();
    }

    if (in_dir.isEmpty())
    {
        cout << "Error: No input path specified.\n";
        return 1;
    }

    if (pack)
        return repack(in_dir);
    else
        return unpack(in_dir);
}

int unpack(QString in_dir)
{
    QString dec_dir = in_dir + "-dec";

    /*
    if (!QDir(dec_dir).exists())
    {
        if (!QDir().mkpath(dec_dir))
        {
            cout << "Error: Failed to create \"" << dec_dir << "\" directory.\n";
            cout.flush();
            return 1;
        }
    }
    */

    QDirIterator it(in_dir, QStringList() << "*.spc", QDir::Files, QDirIterator::Subdirectories);
    QStringList spcNames;
    while (it.hasNext())
    {
        spcNames.append(it.next());
    }

    spcNames.sort();
    for (int i = 0; i < spcNames.count(); i++)
    {
        QString rel_dir = QDir(in_dir).relativeFilePath(spcNames[i]);
        QString out_dir = dec_dir + '/' + rel_dir;
        if (!QDir(dec_dir).mkpath(rel_dir))
        {
            cout << "Error: Failed to create \"" << QDir::toNativeSeparators(out_dir) << "\" directory.\n";
            cout.flush();
            continue;
        }
        cout << "Extracting file " << (i + 1) << "/" << spcNames.length() << ": " << rel_dir << "\n";
        cout.flush();
        QFile f(spcNames[i]);
        f.open(QFile::ReadOnly);
        QByteArray allBytes = f.readAll();
        f.close();
        BinaryData data = BinaryData(allBytes);

        int errCode = unpack_data(data, out_dir);
        if (errCode != 0)
            return errCode;
    }

    return 0;
}

int unpack_data(BinaryData &data, QString out_dir)
{
    QString magic = data.get_str(4);
    if (magic == "$CMP")
    {
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
    QFile info_file(out_dir + ".info");
    if (info_file.exists())
        info_file.remove();
    info_file.open(QFile::WriteOnly);


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

    info_file.write(QString("file_count=" + QString::number(file_count) + "\n").toUtf8());

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
            file_data = spc_dec(file_data, dec_size);

            if (file_data.size() != dec_size)
            {
                cout << "Error: Size mismatch, size was " << file_data.size() << " but should be " << dec_size << "\n";
                cout.flush();
            }
            break;

        case 0x03:  // Load from external file
            /*
            QString ext_file_name = file + "_" + file_name;
            QFile ext_file(ext_file_name);
            ext_file.open(QFile::ReadOnly);
            BinaryData ext_data(ext_file.readAll());
            ext_file.close();
            file_data = srd_dec(ext_data);
            */
            break;
        }

        /*
        if (file_name.endsWith(".spc"))
        {
            QString sub_file = out_dir + QDir::separator() + file_name;
            return unpack_data(file_data, sub_file);
        }
        else
        {
            QFile out(out_dir + QDir::separator() + file_name);
            out.open(QFile::WriteOnly);
            out.write(file_data.Bytes);
            out.close();
        }
        */


        QFile out(out_dir + QDir::separator() + file_name);
        out.open(QFile::WriteOnly);
        out.write(file_data.Bytes);
        out.close();


        // Write file info
        info_file.write(QString("\n").toUtf8());
        info_file.write(QString("cmp_flag=" + QString::number(cmp_flag) + "\n").toUtf8());
        info_file.write(QString("unk_flag=" + QString::number(unk_flag) + "\n").toUtf8());
        info_file.write(QString("cmp_size=" + QString::number(cmp_size) + "\n").toUtf8());
        info_file.write(QString("dec_size=" + QString::number(dec_size) + "\n").toUtf8());
        info_file.write(QString("name_len=" + QString::number(name_len) + "\n").toUtf8());
        info_file.write(QString("file_name=" + file_name + "\n").toUtf8());
        //QString data_sha1(QCryptographicHash::hash(file_data.Bytes, QCryptographicHash::Sha1).toHex());
        //info_file.write(QString("data_sha1=" + data_sha1 + "\n").toUtf8());
    }
    info_file.close();

    return 0;
}

int repack(QString in_dir)
{
    QString cmp_dir = in_dir + "-cmp";

    /*
    if (!QDir(cmp_dir).exists())
    {
        if (!QDir().mkpath(cmp_dir))
        {
            cout << "Error: Failed to create \"" << cmp_dir << "\" directory.\n";
            cout.flush();
            return 1;
        }
    }
    */

    QDirIterator it(in_dir, QStringList() << "*.spc", QDir::Dirs, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString spc_dir = it.next();
        QString out_file = QDir(in_dir).relativeFilePath(spc_dir);
        QFile info_file(spc_dir + ".info");
        info_file.open(QFile::ReadOnly | QIODevice::Text);
        QStringList info_strings = QString(info_file.readAll()).replace('\r', "").split('\n', QString::SkipEmptyParts);
        info_file.close();

        int file_count = info_strings[0].split('=')[1].toInt();

        // Since 50MB isn't really a huge amount of memory, preallocate that much for decompressing to speed things up
        BinaryData out_data(50000000);
        out_data.append(SPC_MAGIC.toUtf8());
        out_data.append(QByteArray(0x04, 0x00));
        out_data.append(QByteArray(0x08, 0xFF));    // unk1
        out_data.append(QByteArray(0x18, 0x00));
        out_data.append(from_u32(file_count));
        out_data.append(from_u32(0x04));            // unk2
        out_data.append(QByteArray(0x10, 0x00));    // padding

        out_data.append(SPC_TABLE_MAGIC.toUtf8());
        out_data.append(QByteArray(0x0C, 0x00));    // padding

        cout << "Compressing " << file_count << " files into " << QDir(spc_dir).dirName() << "\n";
        cout.flush();
        for (int i = 1; i + 6 <= info_strings.size(); i += 6)
        {
            QString file_name = info_strings[i + 5].split('=')[1];
            cout << "\t" << file_name << "\n";
            cout.flush();

            QFile f(spc_dir + QDir::separator() + file_name);
            f.open(QFile::ReadOnly);
            QByteArray allBytes = f.readAll();
            f.close();

            out_data.append(from_u16(0x02));        // cmp_flag
            out_data.append(from_u16(info_strings[i + 1].split('=')[1].toUShort()));  // unk_flag

            BinaryData subdata(allBytes);
            uint dec_size = subdata.size();

            //subdata = spc_cmp(subdata);
            subdata = compress_data(subdata);

            uint cmp_size = subdata.size();
            out_data.append(from_u32(cmp_size));    // cmp_size
            out_data.append(from_u32(dec_size));    // dec_size
            uint name_len = file_name.length();     // name_len
            out_data.append(from_u32(name_len));
            out_data.append(QByteArray(0x10, 0x00));// padding

            // Everything's aligned to multiples of 0x10
            uint name_padding = (0x10 - (name_len + 1) % 0x10) % 0x10;
            uint data_padding = (0x10 - cmp_size % 0x10) % 0x10;

            // We don't actually want the null terminator byte, so pretend it's padding
            out_data.append(file_name.toUtf8());    // file_name
            out_data.append(QByteArray(name_padding + 1, 0x00));

            out_data.append(subdata.Bytes);         // data
            out_data.append(QByteArray(data_padding, 0x00));

            /*
            if (file_name.endsWith(".spc"))
            {
                QString sub_file = file + QDir::separator() + file_name;
                return extract_data(sub_file, file_data);
            }
            else
            {
                QFile out(dec_dir.path() + QDir::separator() + file + QDir::separator() + file_name);
                out.open(QFile::WriteOnly);
                out.write(file_data.Bytes);
                out.close();
            }
            */

        }
        cout << "\n";

        QString out_path = QDir::toNativeSeparators(cmp_dir + QDir::separator() + out_file);
        QDir().mkpath(out_path.left(out_path.lastIndexOf(QDir::separator())));

        QFile out(out_path);
        out.open(QFile::WriteOnly);
        out.write(out_data.Bytes);
        out.close();
    }

    return 0;
}

BinaryData compress_data(BinaryData &data)
{
    BinaryData outData = spc_cmp(data);
    BinaryData testData(outData.Bytes);
    testData = spc_dec(testData);

    if (data.Bytes != testData.Bytes)
    {
        cout << "\tError: Compression test failed, compression was not deterministic!\n";
    }

    return outData;
}
