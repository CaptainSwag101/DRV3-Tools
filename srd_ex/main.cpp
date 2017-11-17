#include <cmath>
#include <detex.h>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include "../utils/binarydata.h"
#include "../utils/drv3_dec.h"

QDir inDir;
QDir exDir;
static QTextStream cout(stdout);

int extract(QString file, BinaryData &data, bool crop = false);
void read_srd_item(BinaryData &data, QString &data_type, BinaryData &srd_data, BinaryData &srd_subdata);
QString read_rsf(BinaryData &srd_data);
QList<detexTexture> read_txr(BinaryData &srd_data, BinaryData &srd_subdata, QString file, bool crop = false, bool keep_mipmaps = false);

struct Mipmap
{
    uint start;
    uint len;
    uint unk1;
    uint unk2;
};

int main(int argc, char *argv[])
{
    // Parse args
    if (argc < 2)
    {
        cout << "No input path specified.\n";
        return 1;
    }
    inDir = QDir(argv[1]);

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

    QDirIterator it(inDir.path(), QStringList() << "*.srd" << "*.stx", QDir::Files, QDirIterator::Subdirectories);
    QStringList srdFiles;
    while (it.hasNext())
        srdFiles.append(it.next());

    srdFiles.sort();
    for (int i = 0; i < srdFiles.length(); i++)
    {
        QString file = inDir.relativeFilePath(srdFiles[i]);
        cout << "Extracting file " << (i + 1) << "/" << srdFiles.length() << ": \"" << file << "\"\n";
        cout.flush();
        QFile f(srdFiles[i]);
        f.open(QFile::ReadOnly);
        QByteArray allBytes = f.readAll();
        f.close();
        BinaryData data(allBytes);

        int errCode = extract(file, data, true);
        if (errCode != 0)
            return errCode;
    }
}

int extract(QString file, BinaryData &data, bool crop)
{
    while (true)
    {
        QString data_type;
        BinaryData srd_data, srd_subdata;

        read_srd_item(data, data_type, srd_data, srd_subdata);

        if (data_type == "" || data_type == "\0\0\0\0")
            break;

        exDir.mkpath(file);

        // Header
        if (data_type == "$CFH")
            continue;
        else if (data_type == "$CT0")
            continue;
        else if (data_type == "$RSF")
        {
            // Resource folder
            QString subdir = read_rsf(srd_data);
            exDir.mkpath(subdir);
        }
        else if (data_type == "$TXR")
        {
            // Texture (srdv file?)
            QList<detexTexture> textures = read_txr(srd_data, srd_subdata, file, crop);
        }
    }
}

void read_srd_item(BinaryData &data, QString &data_type, BinaryData &srd_data, BinaryData &srd_subdata)
{
    data_type = data.get_str(4);

    if (!data_type.startsWith('$'))
        return;

    uint data_len = data.get_u32be();
    uint subdata_len = data.get_u32be();
    uint padding = data.get_u32be();

    uint data_padding = (0x10 - data_len % 0x10) % 0x10;
    uint subdata_padding = (0x10 - subdata_len % 0x10) % 0x10;

    srd_data = BinaryData(data.get(data_len));
    data.Position += data_padding;

    srd_subdata = BinaryData(data.get(subdata_len));
    data.Position += subdata_padding;

    return;
}

QString read_rsf(BinaryData &srd_data)
{
    srd_data.Position += 16;
    return srd_data.get_str();
}

// Via http://stackoverflow.com/a/14267825
inline uint power_of_two(uint x)
{
    //return std::pow(2, (x - 1)).bit_length();
}

QList<detexTexture> read_txr(BinaryData &srd_data, BinaryData &srd_subdata, QString file, bool crop, bool keep_mipmaps)
{
    uint unk1 = srd_data.get_u32();
    ushort swiz = srd_data.get_u16();
    ushort disp_width = srd_data.get_u16();
    ushort disp_height = srd_data.get_u16();
    ushort scanline = srd_data.get_u16();
    uchar fmt = srd_data.get_u8();
    uchar unk2 = srd_data.get_u8();
    uchar palette = srd_data.get_u8();
    uchar palette_id = srd_data.get_u8();

    QString img_data_type;
    BinaryData img_data, img_subdata;
    read_srd_item(srd_subdata, img_data_type, img_data, img_subdata);

    img_data.Position += 2;
    uchar unk5 = img_data.get_u8();
    uchar mipmap_count = img_data.get_u8();
    img_data.Position += 8;
    uint name_offset = img_data.get_u32();

    QList<Mipmap> mipmaps;

    for (int i = 0; i < mipmap_count; i++)
    {
        Mipmap m;
        m.start = img_data.get_u32();
        m.len = img_data.get_u32();
        m.unk1 = img_data.get_u32();
        m.unk2 = img_data.get_u32();
        mipmaps.append(m);
    }

    // Do we have a palette?
    uint pal_start;
    uint pal_len;

    // FIXME: ???
    if (palette == 0x01)
    {
        pal_start = mipmaps.at(palette_id).start;
        pal_len = mipmaps.at(palette_id).len;
    }

    img_data.Position = name_offset;
    QString name = img_data.get_str();

    QString filename_base = file;
    filename_base.truncate(filename_base.size() - 5);
    QString img_filename = filename_base + ".srdv";

    if (!QFile(img_filename).exists())
        img_filename = filename_base + ".srdi";

    QList<detexTexture> textures;

    // The last mipmap is the main texture
    if (!keep_mipmaps)
        while (mipmaps.size() > 1)
            mipmaps.pop_front();

    for (int i = 0; i < mipmaps.size(); i++)
    {
        QString mipmap_name = name;
        mipmap_name.truncate(mipmap_name.lastIndexOf('.'));
        if (mipmaps.size() > 1)
        {
            mipmap_name += QString(disp_width) + "x" + QString(disp_height);
        }
        mipmap_name += name.right(name.size() - name.lastIndexOf('.'));

        Mipmap current_mipmap = mipmaps[i];
        QFile f(img_filename);
        f.open(QFile::ReadOnly);
        BinaryData file_data(f.readAll());
        f.close();
        file_data.Position = current_mipmap.start;
        img_data = file_data.get(current_mipmap.len);
        QByteArray pal_data;

        if (pal_start != NULL && pal_start > 0)
        {
            file_data.Position = pal_start;
            pal_data = file_data.get(pal_len);
        }

        bool swizzled = !(swiz & 1);


    }
}
