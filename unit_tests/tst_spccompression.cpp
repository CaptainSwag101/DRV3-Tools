#include <QFile>
#include <QtTest>
#include "../utils/binarydata.h"
#include "../utils/spc.h"

class SpcCompression : public QObject
{
    Q_OBJECT

public:
    SpcCompression();

private Q_SLOTS:
    void testCase1();
};

SpcCompression::SpcCompression()
{
}

void SpcCompression::testCase1()
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

QTEST_APPLESS_MAIN(SpcCompression)

#include "tst_spccompression.moc"
