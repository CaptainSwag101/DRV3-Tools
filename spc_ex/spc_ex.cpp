#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include "../utils/binarydata.h"
#include "../utils/data_formats.h"

static QTextStream cout(stdout);

void unpack(const QString in_dir);
void unpack_data(const QByteArray &data, const QString out_dir);
void repack(const QString in_dir);
void repack_data(const QString spc_dir, const QString cmp_dir, const QString out_file);

void main(int argc, char *argv[])
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
        cout.flush();
        return;
    }

    if (pack)
        repack(in_dir);
    else
        unpack(in_dir);
}

void unpack(const QString in_dir)
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
        unpack_data(f.readAll(), out_dir);
        f.close();
    }
}

void unpack_data(const QByteArray &data, const QString out_dir)
{
    int pos = 0;

    QString magic = bytes_to_str(data, pos, 4);
    if (magic == "$CMP")
    {
        unpack_data(srd_dec(data), out_dir);
        return;
    }

    if (magic != SPC_MAGIC)
    {
        cout << "Error: Invalid SPC file.\n";
        cout.flush();
        return;
    }


    // Create a text file containing index data and other info for the extracted files,
    // so we can re-pack them in the correct order (not sure if it matters though)
    QFile info_file(out_dir + ".info");
    if (info_file.exists())
        info_file.remove();
    info_file.open(QFile::WriteOnly);


    QByteArray unk1 = get_bytes(data, pos, 0x24);
    uint file_count = bytes_to_num<uint>(data, pos);
    uint unk2 = bytes_to_num<uint>(data, pos);
    pos += 0x10;    // padding?

    QString table_magic = bytes_to_str(data, pos, 4);
    pos += 0x0C;

    if (table_magic != SPC_TABLE_MAGIC)
    {
        cout << "Error: Invalid SPC file.\n";
        cout.flush();
        return;
    }

    info_file.write(QString("file_count=" + QString::number(file_count) + "\n").toUtf8());

    for (uint i = 0; i < file_count; i++)
    {
        ushort cmp_flag = bytes_to_num<ushort>(data, pos);
        ushort unk_flag = bytes_to_num<ushort>(data, pos);
        uint cmp_size = bytes_to_num<uint>(data, pos);
        uint dec_size = bytes_to_num<uint>(data, pos);
        uint name_len = bytes_to_num<uint>(data, pos);
        pos += 0x10;    // Padding?

        // Everything's aligned to multiples of 0x10
        uint name_padding = (0x10 - (name_len + 1) % 0x10) % 0x10;
        uint data_padding = (0x10 - cmp_size % 0x10) % 0x10;

        QString file_name = bytes_to_str(data, pos, name_len);
        // We don't want the null terminator byte, so pretend it's padding
        pos += name_padding + 1;

        QByteArray file_data = get_bytes(data, pos, cmp_size);
        pos += data_padding;

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
            unpack_data(file_data, sub_file);
            continue;
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
        out.write(file_data);
        out.close();


        // Write file info
        info_file.write(QString("\n").toUtf8());
        info_file.write(QString("cmp_flag=" + QString::number(cmp_flag) + "\n").toUtf8());
        info_file.write(QString("unk_flag=" + QString::number(unk_flag) + "\n").toUtf8());
        info_file.write(QString("cmp_size=" + QString::number(cmp_size) + "\n").toUtf8());
        info_file.write(QString("dec_size=" + QString::number(dec_size) + "\n").toUtf8());
        info_file.write(QString("name_len=" + QString::number(name_len) + "\n").toUtf8());
        info_file.write(QString("file_name=" + file_name + "\n").toUtf8());
        //QString data_sha1(QCryptographicHash::hash(file_data, QCryptographicHash::Sha1).toHex());
        //info_file.write(QString("data_sha1=" + data_sha1 + "\n").toUtf8());
    }
    info_file.close();
}

void repack(QString in_dir)
{
    QString cmp_dir = in_dir + "-cmp";

    /*
    if (!QDir(cmp_dir).exists())
    {
        if (!QDir().mkpath(cmp_dir))
        {
            cout << "Error: Failed to create \"" << cmp_dir << "\" directory.\n";
            cout.flush();
            return;
        }
    }
    */

    QDirIterator it(in_dir, QStringList() << "*.spc", QDir::Dirs, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString spc_dir = it.next();
        QString out_file = QDir(in_dir).relativeFilePath(spc_dir);
        repack_data(spc_dir, cmp_dir, out_file);
    }
}

void repack_data(const QString spc_dir, const QString cmp_dir, const QString out_file)
{
    QFile info_file(spc_dir + ".info");
    info_file.open(QFile::ReadOnly | QIODevice::Text);
    QStringList info_strings = QString(info_file.readAll()).replace('\r', "").split('\n', QString::SkipEmptyParts);
    info_file.close();

    int file_count = info_strings[0].split('=')[1].toInt();

    // Since 50MB isn't really a huge amount of memory, preallocate that much for decompressing to speed things up
    QByteArray out_data;
    out_data.reserve(50000000);

    out_data.append(SPC_MAGIC.toUtf8());
    out_data.append(0x04, 0x00);
    out_data.append(0x08, 0xFF);                // unk1
    out_data.append(0x18, 0x00);
    out_data.append(num_to_bytes(file_count));
    out_data.append(num_to_bytes((uint)0x04));  // unk2
    out_data.append(0x10, 0x00);                // padding

    out_data.append(SPC_TABLE_MAGIC.toUtf8());
    out_data.append(0x0C, 0x00);                // padding

    cout << "Compressing " << file_count << " files into " << QDir(spc_dir).dirName() << "\n";
    cout.flush();
    for (int i = 1; i + 6 <= info_strings.size(); i += 6)
    {
        QString file_name = info_strings[i + 5].split('=')[1];
        cout << "\t" << file_name << "\n";
        cout.flush();

        QFile f(spc_dir + QDir::separator() + file_name);
        f.open(QFile::ReadOnly);
        QByteArray subdata = f.readAll();
        f.close();

        out_data.append(num_to_bytes((ushort)0x02));    // cmp_flag
        out_data.append(num_to_bytes(info_strings[i + 1].split('=')[1].toUShort()));  // unk_flag

        uint dec_size = subdata.size();

        QByteArray orig_subdata = subdata;
        subdata = spc_cmp(subdata);
        QByteArray test_data = spc_dec(subdata);

        if (orig_subdata != test_data)
        {
            cout << "\tError: Compression test failed, compression was not deterministic!\n";
            cout.flush();
        }

        uint cmp_size = subdata.size();
        out_data.append(num_to_bytes(cmp_size));    // cmp_size
        out_data.append(num_to_bytes(dec_size));    // dec_size
        uint name_len = file_name.length();         // name_len
        out_data.append(num_to_bytes(name_len));
        out_data.append(0x10, 0x00);                // padding

        // Everything's aligned to multiples of 0x10
        uint name_padding = (0x10 - (name_len + 1) % 0x10) % 0x10;
        uint data_padding = (0x10 - cmp_size % 0x10) % 0x10;

        // We don't actually want the null terminator byte, so pretend it's padding
        out_data.append(file_name.toUtf8());        // file_name
        out_data.append(name_padding + 1, 0x00);

        out_data.append(subdata);                   // data
        out_data.append(data_padding, 0x00);

        /*
        if (file_name.endsWith(".spc"))
        {
            QString sub_file = file + QDir::separator() + file_name;
            extract_data(sub_file, file_data);
            continue;
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
    out.write(out_data);
    out.close();
}
