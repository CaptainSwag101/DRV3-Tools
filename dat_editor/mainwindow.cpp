#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QComboBox>
#include <QDebug>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableView>
#include <QTextCodec>
#include "dat_ui_model.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //ui->tableData->setColumnWidth(1, 170);

    QStringList args = QApplication::arguments();
    if (args.count() <= 1)
        return;

    for (int i = 1; i < args.count(); ++i)
    {
        if (QFileInfo(args[i]).exists() && args[i].endsWith(".dat", Qt::CaseInsensitive))
        {
            openFile(args[i]);
            break;
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    openFile();
}

void MainWindow::on_actionSave_triggered()
{
    saveFile(currentDat.filename);
}

void MainWindow::on_actionSaveAs_triggered()
{
    saveFile();
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!confirmUnsaved())
        event->ignore();
    else
        event->accept();
}

bool MainWindow::confirmUnsaved()
{
    if (!unsavedChanges) return true;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Unsaved Changes",
                                  "Would you like to save your changes?",
                                  QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);

    if (reply == QMessageBox::Yes)
    {
        on_actionSave_triggered();
        return true;
    }
    else if (reply == QMessageBox::No)
    {
        return true;
    }
    return false;
}

bool MainWindow::openFile(QString newFilepath)
{
    if (!confirmUnsaved()) return false;

    if (newFilepath.isEmpty())
    {
        newFilepath = QFileDialog::getOpenFileName(this, "Open DAT file", QString(), "DAT files (*.dat);;All files (*.*)");
    }
    if (newFilepath.isEmpty()) return false;

    QFile f(newFilepath);
    if (!f.open(QFile::ReadOnly)) return false;

    currentDat = dat_from_bytes(f.readAll());
    currentDat.filename = newFilepath;
    f.close();

    this->setWindowTitle("DAT Editor: " + QFileInfo(newFilepath).fileName());
    ui->tableData->setModel(new DatUiModel(this, &currentDat, 0));
    ui->tableStringsAscii->setModel(new DatUiModel(this, &currentDat, 1));
    ui->tableStringsUtf16->setModel(new DatUiModel(this, &currentDat, 2));

    ui->centralWidget->setEnabled(true);
    ui->tableData->scrollToTop();
    ui->tableStringsAscii->scrollToTop();
    ui->tableStringsUtf16->scrollToTop();

    return true;
}

bool MainWindow::saveFile(QString newFilepath)
{
    if (newFilepath.isEmpty())
    {
        newFilepath = QFileDialog::getSaveFileName(this, "Save DAT file", QString(), "DAT files (*.dat);;All files (*.*)");
    }
    if (newFilepath.isEmpty()) return false;

    QFile f(newFilepath);
    if (!f.open(QFile::WriteOnly)) return false;

    const QByteArray out_data = dat_to_bytes(currentDat);
    f.write(out_data);
    f.close();

    currentDat.filename = newFilepath;
    this->setWindowTitle("DAT Editor: " + QFileInfo(newFilepath).fileName());

    unsavedChanges = false;
    return true;
}



void MainWindow::on_editCompleted(const QString & /*str*/)
{
    unsavedChanges = true;
}

void MainWindow::on_toolButton_Add_clicked()
{
    QTableView *table = ui->tabWidget->currentWidget()->findChild<QTableView *>(QString(), Qt::FindDirectChildrenOnly);
    int row = table->currentIndex().row();
    table->model()->insertRow(row + 1);
    table->selectRow(row + 1);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_Del_clicked()
{
    QTableView *table = ui->tabWidget->currentWidget()->findChild<QTableView *>(QString(), Qt::FindDirectChildrenOnly);
    int row = table->currentIndex().row();
    table->model()->removeRow(row);
    if (row - 1 >= 0)
        table->selectRow(row - 1);
    else
        table->selectRow(row);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_Up_clicked()
{
    QTableView *table = ui->tabWidget->currentWidget()->findChild<QTableView *>(QString(), Qt::FindDirectChildrenOnly);
    int row = table->currentIndex().row();
    if (row - 1 < 0)
        return;
    table->model()->moveRow(QModelIndex(), row, QModelIndex(), row - 1);
    table->selectRow(row - 1);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_Down_clicked()
{
    QTableView *table = ui->tabWidget->currentWidget()->findChild<QTableView *>(QString(), Qt::FindDirectChildrenOnly);
    int row = table->currentIndex().row();
    if (row + 1 >= table->model()->rowCount())
        return;
    table->model()->moveRow(QModelIndex(), row, QModelIndex(), row + 1);
    table->selectRow(row + 1);
    unsavedChanges = true;
}



void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls())
    {
        QList<QUrl> urlList = mimeData->urls();

        for (int i = 0; i < urlList.count(); i++)
        {
            QString filepath = urlList.at(i).toLocalFile();
            if (openFile(filepath))
            {
                event->acceptProposedAction();
                break;
            }
        }
    }
}

void MainWindow::on_actionImportCsv_triggered()
{
    if (!confirmUnsaved()) return;

    QString csvFilename = QFileDialog::getOpenFileName(this, "Open CSV file", QString(), "CSV files (*.csv);;All files (*.*)");

    if (csvFilename.isEmpty()) return;

    QFile f(csvFilename);
    if (!f.open(QFile::ReadOnly)) return;

    DatFile newDat;
    QTextStream text(&f);
    text.setCodec(QTextCodec::codecForName("UTF-8"));
    for (int row = -1; !text.atEnd(); ++row)
    {
        QStringList items = text.readLine().split(',', QString::SkipEmptyParts);
        QVector<QByteArray> dataLine;

        for (int col = 0; col < items.count(); ++col)
        {
            // First line is the headers (data names & types)
            if (row == -1)
            {
                const QStringList split = items.at(col).split(' ', QString::SkipEmptyParts);
                newDat.data_names.append(split.at(0));
                newDat.data_types.append(split.at(1));
                continue;
            }

            const QString type = currentDat.data_types.at(col);

            bool ok;
            QByteArray val;
            if (type == "LABEL" || type == "ASCII" || type == "REFER" || type == "UTF16")
            {
                val = num_to_bytes(items.at(col).toUShort(&ok));
            }
            else if (type.startsWith("u"))
            {
                if (type.right(2) == "16")
                    val = num_to_bytes(items.at(col).toUShort(&ok));
                else if (type.right(2) == "32")
                    val = num_to_bytes(items.at(col).toUInt(&ok));
                else if (type.right(2) == "64")
                    val = num_to_bytes(items.at(col).toULongLong(&ok));
            }
            else if (type.startsWith("s"))
            {
                if (type.right(2) == "16")
                    val = num_to_bytes(items.at(col).toShort(&ok));
                else if (type.right(2) == "32")
                    val = num_to_bytes(items.at(col).toInt(&ok));
                else if (type.right(2) == "64")
                    val = num_to_bytes(items.at(col).toLongLong(&ok));
            }
            else if (type.startsWith("f"))
            {
                if (type.right(2) == "32")
                    val = num_to_bytes(items.at(col).toFloat(&ok));
                else if (type.right(2) == "64")
                    val = num_to_bytes(items.at(col).toDouble(&ok));
            }

            if (!ok)
            {
                QMessageBox errorMsg(QMessageBox::Warning,
                                     "Conversion Error",
                                     "Expected a number at line " + QString::number(row + 1) + ", item " + QString::number(col) + ", but got \"" + items.at(col) + "\". Import has been aborted.",
                                     QMessageBox::Ok);
                f.close();
                errorMsg.exec();
                return;
            }

            dataLine.append(val);
        }

        if (row == -1)
            continue;

        newDat.data.append(dataLine);
    }

    f.close();

    currentDat = newDat;

    this->setWindowTitle("DAT Editor: {unnamed file}");
    ui->centralWidget->setEnabled(false);
    ui->tableData->setModel(new DatUiModel(this, &currentDat, 0));
    ui->tableStringsAscii->setModel(new DatUiModel(this, &currentDat, 1));
    ui->tableStringsUtf16->setModel(new DatUiModel(this, &currentDat, 2));
    ui->centralWidget->setEnabled(true);

    ui->tableData->scrollToTop();
    ui->tableStringsAscii->scrollToTop();
    ui->tableStringsUtf16->scrollToTop();
}

void MainWindow::on_actionExportCsv_triggered()
{
    QString csvFilename = currentDat.filename;
    csvFilename.replace(".dat", ".csv", Qt::CaseInsensitive);

    csvFilename = QFileDialog::getSaveFileName(this, "Export CSV file", csvFilename, "CSV files (*.csv);;All files (*.*)");
    if (csvFilename.isEmpty()) return;

    QFile f(csvFilename);
    if (!f.open(QFile::WriteOnly)) return;

    QTextStream text(&f);
    text.setCodec(QTextCodec::codecForName("UTF-8"));

    for (int row = -1; row < currentDat.data.count(); ++row)
    {
        for (int col = 0; col < currentDat.data_types.count(); ++col)
        {
            // Write data names & types first
            if (row == -1)
            {
                text << currentDat.data_names.at(col) + " " + currentDat.data_types.at(col);
                text << ",";
                continue;
            }


            const QByteArray data = currentDat.data.at(row).at(col);
            const QString data_type = currentDat.data_types.at(col);

            int pos = 0;
            if (data_type == "LABEL" || data_type == "ASCII")
            {
                text << QString::number(num_from_bytes<ushort>(data, pos));
            }
            else if (data_type == "REFER" || data_type == "UTF16")
            {
                text << QString::number(num_from_bytes<ushort>(data, pos));
            }
            else if (data_type.startsWith("u"))
            {
                if (data_type.right(2) == "16")
                    text << QString::number(num_from_bytes<ushort>(data, pos));
                else if (data_type.right(2) == "32")
                    text << QString::number(num_from_bytes<uint>(data, pos));
            }
            else if (data_type.startsWith("s"))
            {
                if (data_type.right(2) == "16")
                    text << QString::number(num_from_bytes<short>(data, pos));
                else if (data_type.right(2) == "32")
                    text << QString::number(num_from_bytes<int>(data, pos));
            }
            else if (data_type.startsWith("f"))
            {
                if (data_type.right(2) == "32")
                    text << QString::number(num_from_bytes<float>(data, pos));
                else if (data_type.right(2) == "64")
                    text << QString::number(num_from_bytes<double>(data, pos));
            }

            text << ",";
        }

        text << "\n";
    }
    text.flush();
    f.flush();
    f.close();
}
