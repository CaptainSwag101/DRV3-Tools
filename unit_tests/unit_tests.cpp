#include <QFile>
#include <QtTest>
#include "../utils/binarydata.h"
#include "../utils/dat.h"
#include "../utils/spc.h"

class UnitTests : public QObject
{
    Q_OBJECT

public:
    UnitTests();

private Q_SLOTS:
    void spcCompression();
    void datParser();
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

        qDebug() << it.fileName() << ": filesize reduced by " << orig_data.size() - cmp_data.size() << " bytes.\n";

        QCOMPARE(dec_data, orig_data);
    }
}

void UnitTests::datParser()
{
    QString data_dir = QDir::currentPath() + QDir::separator() + "test_data";
    QDirIterator it(data_dir, QStringList() << "*.dat", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QByteArray dat_bytes;
        DatFile result;

        QFile f(it.next());
        f.open(QFile::ReadOnly);
        dat_bytes = f.readAll();
        f.close();
        result = dat_from_bytes(dat_bytes);
    }
}

QTEST_APPLESS_MAIN(UnitTests)

#include "unit_tests.moc"
