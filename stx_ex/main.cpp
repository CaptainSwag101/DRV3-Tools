#include <QDir>
#include <QDirIterator>
#include "../utils/binarydata.h"
#include "../utils/stx.h"

void unpack(const QString in_path);
void unpack_file(const QString in_file, const QString ex_path);
void repack(const QString in_path);

int main(int argc, char *argv[])
{
    QString in_path;
    bool pack = false;

    // Parse args
    for (int i = 1; i < argc; i++)
    {
        QString arg = QString(argv[i]);

        if (arg == "-p" || arg == "--pack")
            pack = true;
        else
            in_path = QDir::toNativeSeparators(QDir(argv[i]).absolutePath());
    }

    if (in_path.isEmpty())
    {
        cout << "Error: No input path specified.\n";
        cout.flush();
        return 1;
    }

    if (pack)
        repack(in_path);
    else
        unpack(in_path);

    return 0;
}

void unpack(const QString in_path)
{
    if (QFileInfo(in_path).isDir())
    {
        QString ex_path = in_path + "-ex";

        QDirIterator it(in_path, QStringList() << "*.stx", QDir::Files, QDirIterator::Subdirectories);
        QStringList stxNames;
        while (it.hasNext())
        {
            stxNames.append(QDir::toNativeSeparators(it.next()));
        }

        stxNames.sort();
        for (int i = 0; i < stxNames.count(); i++)
        {
            cout << "Extracting file " << (i + 1) << "/" << stxNames.size() << ": " << QFileInfo(stxNames.at(i)).fileName() << "\n";
            cout.flush();

            unpack_file(stxNames.at(i), ex_path);
        }
    }
    else
    {
        if (QFileInfo(in_path).suffix().compare("stx", Qt::CaseInsensitive) != 0)
        {
            cout << "This is not a .stx file.\n";
            cout.flush();
            return;
        }

        QString ex_path = QFileInfo(in_path).dir().absolutePath() + "-ex";
        unpack_file(in_path, ex_path);
    }
}

void unpack_file(const QString in_file, const QString ex_path)
{
    QString rel_path = QDir::toNativeSeparators(QFileInfo(in_file).dir().relativeFilePath(in_file));
    QString out_path = ex_path + QDir::separator() + rel_path;

    QFile f(in_file);
    f.open(QFile::ReadOnly);
    QByteArray data = f.readAll();
    f.close();

    int dirSeparatorIndex = QDir::toNativeSeparators(rel_path).lastIndexOf(QDir::separator());
    if (dirSeparatorIndex > 0)
        QDir(ex_path).mkpath(QDir::toNativeSeparators(rel_path).left(dirSeparatorIndex));

    QString out_file = rel_path;
    out_file.replace(".stx", ".txt");

    QStringList strings = get_stx_strings(data);

    QFile textFile(ex_path + QDir::separator() + out_file);
    textFile.open(QFile::WriteOnly);
    for (int i = 0; i < strings.size(); i++)
    {
        textFile.write(QString("##### " + QString::number(i).rightJustified(4, '0') + "\n").toUtf8());
        textFile.write(QString(strings.at(i) + "\n\n").toUtf8());
    }
    textFile.close();

}

void repack(const QString in_path)
{
    QString cmp_dir = in_path + "-cmp";

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

    QDirIterator it(in_path, QStringList() << "*.txt", QDir::Files, QDirIterator::Subdirectories);
    QStringList txtFiles;
    while (it.hasNext())
        txtFiles.append(it.next());

    txtFiles.sort();
    for (int f = 0; f < txtFiles.count(); f++)
    {
        QString file = QDir(in_path).relativeFilePath(txtFiles.at(f));
        cout << "Re-packing file " << (f + 1) << "/" << txtFiles.count() << ": \"" << file << "\"\n";
        cout.flush();

        // The original files (usually) use Unix-style line breaks.
        // A couple files sometimes have the occasional Windows-style line break,
        // but since these are almost certainly not intentional, we sanitize them anyway.
        QFile txt(txtFiles[f]);
        txt.open(QFile::ReadOnly | QIODevice::Text);
        QStringList allText = QString(txt.readAll()).replace("\r\n", "\n").replace("\r", "\n").split("\n", QString::SkipEmptyParts);
        txt.close();

        QStringList strings;
        QString str;
        int index = 0;
        for (QString strPart : allText)
        {
            if (strPart.startsWith("#"))
            {
                if (!str.isEmpty())
                {
                    strings.append(str);
                    str.clear();
                }

                index = strPart.replace("##### ", "").toInt();
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
        QByteArray stxData = repack_stx_strings(strings);

        QString outPath = QDir::toNativeSeparators(cmp_dir + QDir::separator() + file);
        outPath.replace(".txt", ".stx");
        QDir().mkpath(outPath.left(outPath.lastIndexOf(QDir::separator())));
        QFile outFile(outPath);
        outFile.open(QFile::WriteOnly);
        outFile.write(stxData);
        outFile.close();
    }
}
