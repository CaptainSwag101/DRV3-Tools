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
    for (int i = 0; i < labelCodeWidgets.count(); i++)
    {
        delete labelCodeWidgets[i];
    }
    labelCodeWidgets.clear();
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
    currentWrd = wrd_from_data(f.readAll());
    f.close();

    currentWrd.filename = filepath;
    this->setWindowTitle("SPC Editor: " + QFileInfo(filepath).fileName());
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

    labelCodeWidgets.clear();
    for (QByteArray label_code : currentWrd.code.values())
    {

        QTableWidget *widget = new QTableWidget(label_code.size() / 2, 1, this->ui->tabData);

        int pos = 0;
        while (pos + 1 < label_code.size())
        {
            QTableWidgetItem *newItem = new QTableWidgetItem(QString::number(bytes_to_num<ushort>(label_code, pos), 16).rightJustified(4, '0'));
            widget->setItem((pos / 2), 0, newItem);
        }
        //widget->setLayout(new QVBoxLayout());
        labelCodeWidgets.append(widget);
    }

    for (int i = currentWrd.code.keys().count(); i > 0; i--)
    {
        this->ui->comboBox_SelectLabel->addItem(currentWrd.code.keys().at(i - 1));
    }
    this->ui->comboBox_SelectLabel->setCurrentIndex(0);

}

void MainWindow::on_comboBox_SelectLabel_currentIndexChanged(int index)
{
    this->ui->tableWidget_LabelCode = labelCodeWidgets.at(index);
    this->ui->tableWidget_LabelCode->repaint();
}
