#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include "../utils/binarydata.h"
#include "../utils/data_formats.h"

int unpack();
int repack();

static QTextStream cout(stdout);
QDir inDir;
QDir outDir;
bool pack = false;

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
            pack = true;
    }

    if (pack)
        return repack();
    else
        return unpack();
}

int unpack()
{
    outDir = inDir;
    outDir.cdUp();
    if (!outDir.exists("ex"))
    {
        if (!outDir.mkdir("ex"))
        {
            cout << "Error: Failed to create \"ex\" directory.\n";
            return 1;
        }
    }
    outDir.cd("ex");
    if (!outDir.exists(inDir.dirName()))
    {
        if (!outDir.mkdir(inDir.dirName()))
        {
            cout << "Error: Failed to create \"ex" << QDir::separator() << inDir.dirName() << "\" directory.\n";
            return 1;
        }
    }
    outDir.cd(inDir.dirName());

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

        outDir.mkpath(file.left(QDir::toNativeSeparators(file).lastIndexOf(QDir::separator())));

        QString outFile = file;
        outFile.replace(".stx", ".txt");

        QStringList strings = get_stx_strings(data);

        QFile textFile(outDir.path() + QDir::separator() + outFile);
        textFile.open(QFile::WriteOnly);
        for (int i = 0; i < strings.size(); i++)
        {
            textFile.write(QString("##### " + QString::number(i).rightJustified(4, '0') + "\n").toUtf8());
            textFile.write(QString(strings[i] + "\n\n").toUtf8());
        }
        textFile.close();

    }

    return 0;
}

int repack()
{
    outDir = inDir;
    outDir.cdUp();
    if (!outDir.exists("cmp"))
    {
        if (!outDir.mkdir("cmp"))
        {
            cout << "Error: Failed to create \"cmp\" directory.\n";
            return 1;
        }
    }

    outDir.cd("cmp");
    if (!outDir.exists(inDir.dirName()))
    {
        if (!outDir.mkdir(inDir.dirName()))
        {
            cout << "Error: Failed to create \"cmp" << QDir::separator() << inDir.dirName() << "\" directory.\n";
            return 1;
        }
    }
    outDir.cd(inDir.dirName());

    QString test = inDir.path();
    QDirIterator it(inDir.path(), QStringList() << "*.txt", QDir::Files, QDirIterator::Subdirectories);
    QStringList txtFiles;
    while (it.hasNext())
        txtFiles.append(it.next());

    txtFiles.sort();
    for (int f = 0; f < txtFiles.length(); f++)
    {
        QString file = inDir.relativeFilePath(txtFiles[f]);
        cout << "Re-packing file " << (f + 1) << "/" << txtFiles.length() << ": \"" << file << "\"\n";
        cout.flush();

        // The original files (usually) use Unix-style line breaks.
        // A couple files sometimes have the occasional Windows-style line break,
        // but since these are almost certainly not intentional, we sanitize them anyway.
        QFile txt(txtFiles[f]);
        txt.open(QFile::ReadOnly | QIODevice::Text);
        QStringList allText = QString(txt.readAll()).replace("\r\n", "\n").replace("\r", "\n").split("\n", QString::SkipEmptyParts);
        txt.close();

        QMap<int, QString> strings;
        QString str;
        int index = 0;
        int table_len = 0;
        for (QString strPart : allText)
        {
            if (strPart.startsWith("#"))
            {
                if (!str.isEmpty())
                {
                    strings[index] = str;
                    str.clear();
                }

                index = strPart.replace("##### ", "").toInt();
                table_len++;
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
        BinaryData stxData = repack_stx_strings(table_len, strings);

        QString outPath = QDir::toNativeSeparators(outDir.path() + QDir::separator() + file);
        outPath.replace(".txt", ".stx");
        QDir().mkpath(outPath.left(outPath.lastIndexOf(QDir::separator())));
        QFile outFile(outPath);
        outFile.open(QFile::WriteOnly);
        outFile.write(stxData.Bytes);
        outFile.close();
    }

    return 0;
}
