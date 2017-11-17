#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include "../utils/binarydata.h"
#include "../utils/drv3_dec.h"


const QString SPC_MAGIC = "CPS.";
const QString CMP_MAGIC = "$CMP";
const QString TABLE_MAGIC = "Root";
QDir inDir;
QDir decDir;
QTextStream cout(stdout);

int extract(QString file, BinaryData *data);

int main(int argc, char *argv[])
{
    // Parse args
    if (argc < 2)
    {
        cout << "No input path specified.\n";
        return 1;
    }
    inDir = QDir(argv[1]);

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

        int errCode = extract(file, &data);
        if (errCode != 0)
            return errCode;
    }
}

int extract(QString file, BinaryData *data)
{
    decDir.mkpath(file);

    QString magic = data->get_str(4);
    if (magic == CMP_MAGIC)
    {
        data = srd_dec(data);
        magic = data->get_str(4);
    }

    if (magic != SPC_MAGIC)
    {
        cout << "Invalid SPC file.\n";
        return 1;
    }

    data->Position += 0x24; // unk1
    uint file_count = data->get_u32();
    data->Position += 0x14; // unk2, 0x10 bytes padding?

    QString table_magic = data->get_str(4);
    data->Position += 0x0C;

    if (table_magic != TABLE_MAGIC)
    {
        cout << "Invalid SPC file.\n";
        return 1;
    }

    for (uint i = 0; i < file_count; i++)
    {
        ushort cmp_flag = data->get_u16();
        ushort unk_flag = data->get_u16();
        uint cmp_size = data->get_u32();
        uint dec_size = data->get_u32();
        uint name_len = data->get_u32() + 1;    // Null terminator excluded from count
        data->Position += 0x10; // Padding?

        // Everything's aligned to multiples of 0x10
        uint name_padding = (0x10 - name_len % 0x10) % 0x10;
        uint data_padding = (0x10 - cmp_size % 0x10) % 0x10;

        // We don't actually want the null terminator byte, so pretend it's padding
        QString file_name = data->get_str(name_len - 1);
        data->Position += name_padding + 1;

        BinaryData *file_data = new BinaryData(data->get(cmp_size));
        data->Position += data_padding;

        switch (cmp_flag)
        {
        case 0x01:  // Uncompressed, don't do anything
            break;

        case 0x02:  // Compressed
            file_data = spc_dec(file_data);

            if (file_data->Bytes.size() != dec_size)
                cout << "Size mismatch: size was " << file_data->Bytes.size() << " but should be " << dec_size << "\n";
            break;

        case 0x03:  // Load from external file
            QString ext_file = file + "_" + file_name;
            //file_data = srd_dec(ext_file);
            break;
        }

        if (file_name.endsWith(".spc"))
        {
            QString sub_file = file + QDir::separator() + file_name;
            return extract(sub_file, file_data);
        }
        else
        {
            QFile out(decDir.path() + QDir::separator() + file + QDir::separator() + file_name);
            out.open(QFile::WriteOnly);
            out.write(file_data->Bytes);
            out.close();
        }

        delete file_data;
        file_data = nullptr;
    }

    // Manually free up the memory just in case
    //delete data;
    //data = nullptr;

    return 0;
}