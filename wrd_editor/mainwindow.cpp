#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    if (!confirmUnsaved()) return;

    QString newFilename = QFileDialog::getOpenFileName(this, "Open WRD file", QString(), "WRD files (*.wrd);;All files (*.*)");
    if (newFilename.isEmpty())
        return;

    openFile(newFilename);
}

void MainWindow::on_actionSave_triggered()
{
    return;




    QByteArray out_data = wrd_to_data(currentWrd);
    QString outName = currentWrd.filename;
    QFile f(outName);
    f.open(QFile::WriteOnly);
    f.write(out_data);
    f.close();
    unsavedChanges = false;
}

void MainWindow::on_actionSaveAs_triggered()
{
    QString newFilename = QFileDialog::getSaveFileName(this, "Save WRD file", QString(), "WRD files (*.wrd);;All files (*.*)");
    if (newFilename.isEmpty())
        return;

    currentWrd.filename = newFilename;
    on_actionSave_triggered();
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
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

void MainWindow::openFile(QString filepath)
{
    QFile f(filepath);
    f.open(QFile::ReadOnly);
    currentWrd = wrd_from_data(f.readAll());
    f.close();

    currentWrd.filename = filepath;
    this->setWindowTitle("WRD Editor: " + QFileInfo(filepath).fileName());
    reloadLists();
}

void MainWindow::reloadLists()
{
    this->ui->tableWidget_Cmds->setRowCount(currentWrd.cmds.count());
    for (int i = 0; i < currentWrd.cmds.count(); i++)
    {
        QTableWidgetItem *newItem = new QTableWidgetItem(currentWrd.cmds.at(i));
        this->ui->tableWidget_Cmds->setItem(i, 0, newItem);
    }

    this->ui->tableWidget_Strings->setRowCount(currentWrd.strings.count());
    for (int i = 0; i < currentWrd.strings.count(); i++)
    {
        QTableWidgetItem *newItem = new QTableWidgetItem(currentWrd.strings.at(i));
        this->ui->tableWidget_Strings->setItem(i, 0, newItem);
    }

    this->ui->comboBox_SelectLabel->clear();
    for (QString label_name : currentWrd.labels)
    {
        this->ui->comboBox_SelectLabel->addItem(label_name);
    }
    this->ui->comboBox_SelectLabel->setCurrentIndex(0);

}

void MainWindow::on_comboBox_SelectLabel_currentIndexChanged(int index)
{
    if (index < 0 || index > currentWrd.code.count())
        return;

    QByteArray label_code = currentWrd.code.at(index);
    this->ui->tableWidget_LabelCode->setRowCount(0);
    this->ui->tableWidget_LabelCode->setRowCount(label_code.count(0x70));

    int cmd_num = 0;
    int pos = 0;
    QString code = "0x";
    while (pos < label_code.size())
    {
        uchar c = label_code.at(pos++);

        // Don't break on the first byte, which will always be 0x70
        if (code != "0x" && c == 0x70)
        {
            QTableWidgetItem *newItem = new QTableWidgetItem(code);
            this->ui->tableWidget_LabelCode->setItem(cmd_num++, 0, newItem);
            code = "0x";
        }

        code += QString::number(c, 16).toUpper().rightJustified(2, '0');
    }
    if (code != "0x")
    {
        QTableWidgetItem *newItem = new QTableWidgetItem(code);
        this->ui->tableWidget_LabelCode->setItem(cmd_num++, 0, newItem);
    }
}
