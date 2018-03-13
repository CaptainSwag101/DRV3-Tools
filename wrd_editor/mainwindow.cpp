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
    currentWrd = wrd_from_data(f.readAll(), filepath);
    f.close();

    this->setWindowTitle("WRD Editor: " + QFileInfo(filepath).fileName());
    reloadLists();
}

// TODO: Change the UI so that everything is shown based on the currently selected label.
// Since text strings are displayed due to commands in a label, it will help us
// identify unused strings, flags, or code.
void MainWindow::reloadLists()
{
    this->ui->comboBox_SelectLabel->clear();
    for (QString label_name : currentWrd.labels)
    {
        this->ui->comboBox_SelectLabel->addItem(label_name);
    }
    this->ui->comboBox_SelectLabel->setCurrentIndex(0);

    this->ui->tableWidget_Flags->clearContents();
    this->ui->tableWidget_Flags->setRowCount(currentWrd.flags.count());
    for (int i = 0; i < currentWrd.flags.count(); i++)
    {
        QTableWidgetItem *newItem = new QTableWidgetItem(currentWrd.flags.at(i));
        this->ui->tableWidget_Flags->setItem(i, 0, newItem);
    }

    this->ui->tableWidget_Strings->clearContents();
    this->ui->tableWidget_Strings->setRowCount(currentWrd.strings.count());
    for (int i = 0; i < currentWrd.strings.count(); i++)
    {
        QString str = currentWrd.strings.at(i);
        str.replace("\n", "\\n");
        QTableWidgetItem *newItem = new QTableWidgetItem(str);
        this->ui->tableWidget_Strings->setItem(i, 0, newItem);
    }
}

void MainWindow::on_comboBox_SelectLabel_currentIndexChanged(int index)
{
    if (index < 0 || index > currentWrd.cmds.count())
        return;

    this->ui->tableWidget_LabelCode->clearContents();
    this->ui->tableWidget_LabelCode->setRowCount(currentWrd.cmds.count());

    for (int i = 0; i < currentWrd.cmds.count(); i++)
    {
        const WrdCmd cmd = currentWrd.cmds.at(i);

        QTableWidgetItem *newOpcode = new QTableWidgetItem(QString::number(cmd.opcode, 16).toUpper().rightJustified(4, '0'));
        this->ui->tableWidget_LabelCode->setItem(i, 0, newOpcode);

        QString argString;
        for (ushort arg : cmd.args)
        {
            argString += QString::number(arg, 16).toUpper().rightJustified(4, '0');
        }
        QTableWidgetItem *newArgs = new QTableWidgetItem(argString);
        this->ui->tableWidget_LabelCode->setItem(i, 1, newArgs);
    }
}
