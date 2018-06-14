#include <QFile>
#include <QTextStream>
#include <QVector>
#include <QtTest>
#include "../utils/binarydata.h"
#include "../utils/dat.h"
#include "../utils/spc.h"
#include "../utils/wrd.h"

class UnitTests : public QObject
{
    Q_OBJECT

public:
    UnitTests();

private Q_SLOTS:
    void spcCompression();
    void datParser();
    void findWrdVersionChanges();
    void findBadWrdParams();
};

UnitTests::UnitTests()
{
}

void UnitTests::spcCompression()
{
    QString data_dir = QDir::currentPath() + QDir::separator() + "test_data";
    QDirIterator it(data_dir, QStringList(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QByteArray orig_data;
        QByteArray cmp_data;
        QByteArray dec_data;

        QFile test_file(it.next());
        test_file.open(QFile::ReadOnly);
        orig_data = test_file.readAll();
        test_file.close();
        cmp_data = spc_cmp(orig_data);
        dec_data = spc_dec(cmp_data);

        qDebug() << it.fileName() << ": filesize reduced by " << orig_data.size() - cmp_data.size() << " bytes.";

        for (int i = 0; i < (int)std::min(orig_data.size(), dec_data.size()); i++)
        {
            if (orig_data.at(i) != dec_data.at(i))
            {
                qDebug() << "The byte at " + QString::number(i, 16) + " differs. (orig: " + orig_data.at(i) + ", dec: " + dec_data.at(i) + ")";
            }
        }

        //QCOMPARE(dec_data, orig_data);
    }
}

void UnitTests::datParser()
{
    QString data_dir = QDir::currentPath() + QDir::separator() + "test_data";
    QDirIterator it(data_dir, QStringList() << "*.dat", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QFile f(it.next());
        if (!f.open(QFile::ReadOnly))
            continue;
        qDebug() << it.fileName();
        dat_from_bytes(f.readAll());
        f.close();
    }
}

void UnitTests::findWrdVersionChanges()
{
    QFile logfile(QDir::currentPath() + QDir::separator() + "wrd_version_changes.log");
    if (logfile.exists() && !logfile.remove())
    {
        qDebug() << "Failed to overwrite log file.";
        return;
    }

    logfile.open(QFile::WriteOnly);
    QTextStream log(&logfile);

    QString script_dir = QDir::currentPath() + QDir::separator() + "wrd_script-dec";
    QString default_subdir = "003";

    QHash<QString, QByteArray> script_data;

    QDirIterator def(script_dir + QDir::separator() + default_subdir, QStringList() << "*.wrd" << "*.stx", QDir::Files, QDirIterator::Subdirectories);
    while (def.hasNext())
    {
        QFile f(def.next());
        if (!f.open(QFile::ReadOnly))
            continue;

        QString rel_path = QDir(script_dir + QDir::separator() + default_subdir).relativeFilePath(def.filePath());
        script_data[rel_path] = f.readAll();
        f.close();
    }


    QDirIterator dir(script_dir, QStringList() << "00*", QDir::Dirs);
    while (dir.hasNext())
    {
        dir.next();
        if (dir.fileName() == default_subdir)
            continue;

        QDirIterator it(dir.filePath(), QStringList() << "*.wrd" << "*.stx", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            QFile f(it.next());
            if (!f.open(QFile::ReadOnly))
                continue;
            //qDebug() << it.filePath();
            QString rel_path = QDir(dir.filePath()).relativeFilePath(it.filePath());
            if (!script_data.contains(rel_path) || script_data[rel_path] != f.readAll())
                log << "File " + dir.fileName() + '/' + rel_path + " differs from the file in folder " + default_subdir + ".\n";
                //qDebug() << "File differs from the file in folder " << default_subdir << ".";
            f.close();
        }
    }

    logfile.close();
}

void UnitTests::findBadWrdParams()
{
    QString script_dir = QDir::currentPath() + QDir::separator() + "wrd_script-dec";
    QDirIterator it(script_dir, QStringList() << "*.wrd", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QFile f(it.next());
        if (!f.open(QFile::ReadOnly))
            continue;
        //qDebug() << it.filePath();
        wrd_from_bytes(f.readAll(), f.fileName());
        f.close();
    }
}

QTEST_APPLESS_MAIN(UnitTests)

#include "unit_tests.moc"
