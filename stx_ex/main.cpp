#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include "../utils/binarydata.h"
#include "../utils/data_formats.h"

void unpack(const QString in_dir);
void repack(const QString in_dir);

static QTextStream cout(stdout);

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
    QString ex_dir = in_dir + "-ex";

    /*
    if (!QDir(ex_dir).exists())
    {
        if (!QDir().mkpath(ex_dir))
        {
            cout << "Error: Failed to create \"" << ex_dir << "\" directory.\n";
            cout.flush();
            return;
        }
    }
    */

    QDirIterator it(in_dir, QStringList() << "*.stx", QDir::Files, QDirIterator::Subdirectories);
    QStringList stxFiles;
    while (it.hasNext())
        stxFiles.append(it.next());

    stxFiles.sort();
    for (int i = 0; i < stxFiles.count(); i++)
    {
        QString file = QDir(in_dir).relativeFilePath(stxFiles.at(i));
        cout << "Extracting file " << (i + 1) << "/" << stxFiles.count() << ": \"" << file << "\"\n";
        cout.flush();
        QFile f(stxFiles[i]);
        f.open(QFile::ReadOnly);
        QByteArray data = f.readAll();
        f.close();

        int dirSeparatorIndex = file.lastIndexOf(QDir::separator());
        if (dirSeparatorIndex > 0)
            QDir(ex_dir).mkpath(file.left(dirSeparatorIndex));

        QString outFile = file;
        outFile.replace(".stx", ".txt");

        QStringList strings = get_stx_strings(data);

        QFile textFile(ex_dir + QDir::separator() + outFile);
        textFile.open(QFile::WriteOnly);
        for (int i = 0; i < strings.size(); i++)
        {
            textFile.write(QString("##### " + QString::number(i).rightJustified(4, '0') + "\n").toUtf8());
            textFile.write(QString(strings.at(i) + "\n\n").toUtf8());
        }
        textFile.close();

    }
}

void repack(const QString in_dir)
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

    QDirIterator it(in_dir, QStringList() << "*.txt", QDir::Files, QDirIterator::Subdirectories);
    QStringList txtFiles;
    while (it.hasNext())
        txtFiles.append(it.next());

    txtFiles.sort();
    for (int f = 0; f < txtFiles.count(); f++)
    {
        QString file = QDir(in_dir).relativeFilePath(txtFiles.at(f));
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
