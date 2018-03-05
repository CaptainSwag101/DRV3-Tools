#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include "../utils/binarydata.h"
#include "../utils/data_formats.h"

static QTextStream cout(stdout);

void unpack(const QString in_dir);
void repack(const QString in_dir);

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
        cout.flush();
        return 1;
    }

    if (pack)
        repack(in_dir);
    else
        unpack(in_dir);

    return 0;
}

void unpack(const QString in_dir)
{
    QString dec_dir = in_dir + "-dec";

    QDirIterator it(in_dir, QStringList() << "*.spc", QDir::Files, QDirIterator::Subdirectories);
    QStringList spcNames;
    while (it.hasNext())
    {
        spcNames.append(it.next());
    }

    spcNames.sort();
    for (int i = 0; i < spcNames.count(); i++)
    {
        QString rel_dir = QDir(in_dir).relativeFilePath(spcNames.at(i));
        QString out_dir = dec_dir + '/' + rel_dir;
        if (!QDir(dec_dir).mkpath(rel_dir))
        {
            cout << "Error: Failed to create \"" << QDir::toNativeSeparators(out_dir) << "\" directory.\n";
            cout.flush();
            continue;
        }
        cout << "Extracting file " << (i + 1) << "/" << spcNames.size() << ": " << rel_dir << "\n";
        cout.flush();

        QFile f(spcNames[i]);
        f.open(QFile::ReadOnly);
        SpcFile spc = spc_from_data(f.readAll());
        spc.filename = spcNames[i];


        // Create a text file containing index data and other info for the extracted files,
        // so we can re-pack them in the correct order (not sure if it matters though)
        QFile info_file(out_dir + ".info");
        if (info_file.exists())
            info_file.remove();
        info_file.open(QFile::WriteOnly);
        info_file.write(QString("file_count=" + QString::number(spc.subfiles.count()) + "\n").toUtf8());

        for (SpcSubfile subfile : spc.subfiles)
        {
            // Write subfile data
            switch (subfile.cmp_flag)
            {
            case 0x01:  // Uncompressed
            {
                QFile out(out_dir + QDir::separator() + subfile.filename);
                out.open(QFile::WriteOnly);
                out.write(subfile.data);
                out.close();
                break;
            }
            case 0x02:  // Compressed
            {
                QByteArray dec_data = spc_dec(subfile.data, subfile.dec_size);

                if (dec_data.size() != subfile.dec_size)
                {
                    cout << "Error: Size mismatch, size was " << dec_data.size() << " but should be " << subfile.dec_size << "\n";
                    cout.flush();
                }

                QFile out(out_dir + QDir::separator() + subfile.filename);
                out.open(QFile::WriteOnly);
                out.write(dec_data);
                out.close();

                break;
            }
            case 0x03:  // Load from external file
            {
                QString ext_file_name = spc.filename + "_" + subfile.filename;
                cout << "Loading from external file: " << ext_file_name << "\n";
                cout.flush();

                QFile ext_file(ext_file_name);
                ext_file.open(QFile::ReadOnly);
                QByteArray ext_data = ext_file.readAll();
                ext_file.close();

                ext_data = srd_dec(ext_data);

                QFile out(out_dir + QDir::separator() + subfile.filename);
                out.open(QFile::WriteOnly);
                out.write(ext_data);
                out.close();
                break;
            }
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

            // Write subfile info
            info_file.write(QString("\n").toUtf8());
            info_file.write(QString("cmp_flag=" + QString::number(subfile.cmp_flag) + "\n").toUtf8());
            info_file.write(QString("unk_flag=" + QString::number(subfile.unk_flag) + "\n").toUtf8());
            info_file.write(QString("cmp_size=" + QString::number(subfile.cmp_size) + "\n").toUtf8());
            info_file.write(QString("dec_size=" + QString::number(subfile.dec_size) + "\n").toUtf8());
            info_file.write(QString("filename=" + subfile.filename + "\n").toUtf8());
        }
        info_file.close();


        f.close();
    }
}

void repack(QString in_dir)
{
    QString cmp_dir = in_dir + "-cmp";

    QDirIterator it(in_dir, QStringList() << "*.spc", QDir::Dirs, QDirIterator::Subdirectories);
    QStringList spcNames;
    while (it.hasNext())
    {
        spcNames.append(it.next());
    }

    spcNames.sort();
    for (int i = 0; i < spcNames.count(); i++)
    {
        QString spc_dir = spcNames.at(i);
        QString out_file = QDir(in_dir).relativeFilePath(spc_dir);


        QFile info_file(spc_dir + ".info");
        info_file.open(QFile::ReadOnly | QIODevice::Text);
        QStringList info_strings = QString(info_file.readAll()).replace('\r', "").split('\n', QString::SkipEmptyParts);
        info_file.close();

        int file_count = info_strings.at(0).split('=').at(1).toInt();

        cout << "Compressing " << file_count << " files into " << QDir(spc_dir).dirName() << "\n";
        cout.flush();

        // Since 50MB isn't really a huge amount of memory, preallocate that much for decompressing to speed things up
        QByteArray out_data;
        out_data.reserve(50000000);

        out_data.append(SPC_MAGIC.toUtf8());        // "CPS."
        out_data.append(0x04, 0x00);                // unk1
        out_data.append(0x08, (char)0xFF);          // unk1
        out_data.append(0x18, 0x00);                // unk1
        out_data.append(num_to_bytes(file_count));  // file_count
        out_data.append(num_to_bytes((uint)0x04));  // unk2
        out_data.append(0x10, 0x00);                // padding
        out_data.append(SPC_TABLE_MAGIC.toUtf8());  // "Root"
        out_data.append(0x0C, 0x00);                // padding

        for (int i = 1; i + 5 <= info_strings.size(); i += 5)
        {
            QString file_name = info_strings.at(i + 4).split('=').at(1);
            cout << "\t" << file_name << "\n";
            cout.flush();

            QFile f(spc_dir + QDir::separator() + file_name);
            f.open(QFile::ReadOnly);
            const QByteArray dec_subdata = f.readAll();
            f.close();

            out_data.append(num_to_bytes((ushort)0x02));    // cmp_flag
            out_data.append(num_to_bytes(info_strings.at(i + 1).split('=').at(1).toUShort()));  // unk_flag

            const QByteArray cmp_subdata = spc_cmp(dec_subdata);

#ifdef QT_DEBUG
            const QByteArray test_data = spc_dec(cmp_subdata);

            if (dec_subdata != test_data)
            {
                cout << "\tError: Compression test failed, compression was not deterministic!\n";
                cout.flush();
            }
#endif

            uint cmp_size = cmp_subdata.size();
            out_data.append(num_to_bytes(cmp_size));    // cmp_size
            uint dec_size = dec_subdata.size();
            out_data.append(num_to_bytes(dec_size));    // dec_size
            uint name_len = file_name.length();
            out_data.append(num_to_bytes(name_len));    // name_len
            out_data.append(0x10, 0x00);                // padding

            // Everything's aligned to multiples of 0x10
            uint name_padding = (0x10 - (name_len + 1) % 0x10) % 0x10;
            uint data_padding = (0x10 - cmp_size % 0x10) % 0x10;

            // We don't actually want the null terminator byte, so pretend it's padding
            out_data.append(file_name.toUtf8());        // file_name
            out_data.append(name_padding + 1, 0x00);

            out_data.append(cmp_subdata);               // data
            out_data.append(data_padding, 0x00);
        }
        cout << "\n";


        QString out_path = QDir::toNativeSeparators(cmp_dir + QDir::separator() + out_file);
        QDir().mkpath(out_path.left(out_path.lastIndexOf(QDir::separator())));

        QFile out(out_path);
        out.open(QFile::WriteOnly);
        out.write(out_data);
        out.close();
    }
}
