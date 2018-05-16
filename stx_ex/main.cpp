#include <QDir>
#include <QDirIterator>
#include "../utils/binarydata.h"
#include "../utils/stx.h"

void unpack(const QString in_path);
void unpack_file(const QString in_filepath, const QString in_dirpath);
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
        else if (in_path.isEmpty())
            in_path = QDir(argv[i]).absolutePath();
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
        QDirIterator it(in_path, QStringList() << "*.stx", QDir::Files, QDirIterator::Subdirectories);
        QStringList stxNames;
        while (it.hasNext())
        {
            stxNames.append(it.next());
        }

        stxNames.sort();
        for (int i = 0; i < stxNames.count(); i++)
        {
            cout << "Extracting file " << (i + 1) << "/" << stxNames.size() << ": " << QFileInfo(stxNames.at(i)).fileName() << "\n";
            cout.flush();

            unpack_file(stxNames.at(i), in_path);
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


        unpack_file(in_path, in_path);
    }
}

void unpack_file(const QString in_filepath, const QString in_dirpath)
{
    QString ex_dirpath = in_dirpath + "-ex";
    QString rel_filepath = QDir(in_dirpath).relativeFilePath(in_filepath);
    QString out_filepath = ex_dirpath + '/' + rel_filepath;
    out_filepath.replace(".stx", ".txt");

    int dirSeparatorIndex = rel_filepath.lastIndexOf('/');
    if (dirSeparatorIndex > 0)
        QDir(ex_dirpath).mkpath(rel_filepath.left(dirSeparatorIndex));

    QFile in(in_filepath);
    in.open(QFile::ReadOnly);
    const QStringList strings = get_stx_strings(in.readAll());
    in.close();

    QFile out(out_filepath);
    out.open(QFile::WriteOnly);
    for (int i = 0; i < strings.size(); i++)
    {
        out.write(QString("##### " + QString::number(i).rightJustified(4, '0') + "\n").toUtf8());
        out.write(QString(strings.at(i) + "\n\n").toUtf8());
    }
    out.close();
}

void repack(const QString in_path)
{
    if (QFileInfo(in_path).isDir())
    {
        QString cmp_dirpath = in_path + "-cmp";

        if (!QDir(cmp_dirpath).exists() && !QDir().mkpath(cmp_dirpath))
        {
            cout << "Error: Failed to create \"" << cmp_dirpath << "\" directory.\n";
            cout.flush();
            return;
        }

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
            QStringList allText = QString(txt.readAll()).replace("\r\n", QString('\n')).replace('\r', '\n').split('\n', QString::SkipEmptyParts);
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

            QString outPath = cmp_dirpath + '/' + file;
            outPath.replace(".txt", ".stx");
            QDir().mkpath(outPath.left(outPath.lastIndexOf('/')));
            QFile outFile(outPath);
            outFile.open(QFile::WriteOnly);
            outFile.write(stxData);
            outFile.close();
        }
    }
    else
    {
        QString file = in_path;
        cout << "Re-packing file" << ": \"" << file << "\"\n";
        cout.flush();

        // The original files (usually) use Unix-style line breaks.
        // A couple files sometimes have the occasional Windows-style line break,
        // but since these are almost certainly not intentional, we sanitize them anyway.
        QFile txt(in_path);
        txt.open(QFile::ReadOnly | QIODevice::Text);
        QStringList allText = QString(txt.readAll()).replace("\r\n", QString('\n')).replace('\r', '\n').split('\n', QString::SkipEmptyParts);
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

        QString outPath = file;
        outPath.replace(".txt", ".stx");
        QDir().mkpath(outPath.left(outPath.lastIndexOf('/')));
        QFile outFile(outPath);
        outFile.open(QFile::WriteOnly);
        outFile.write(stxData);
        outFile.close();
    }
}
