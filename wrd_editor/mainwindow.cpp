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
    /*
    currentWrd.labels.clear();
    for (int i = 0; i < this->ui->comboBox_SelectLabel->count(); i++)
    {
        currentWrd.labels.append(this->ui->comboBox_SelectLabel->itemText(i));
    }
    */

    const QByteArray out_data = wrd_to_data(currentWrd);
    QString out_file = currentWrd.filename;
    QFile f(out_file);
    f.open(QFile::WriteOnly);
    f.write(out_data);
    f.close();

    if (currentWrd.external_strings)
    {
        // Strings are stored in the "(current spc name)_text_(region).spc" file,
        // within an STX file with the same name as the current WRD file.
        QString stx_file = QFileInfo(out_file).absolutePath();
        if (stx_file.endsWith(".SPC", Qt::CaseInsensitive))
            stx_file.chop(4);
        if (stx_file.endsWith("_US"))
            stx_file.chop(3);
        stx_file.append("_text_US.SPC");
        stx_file.append(QDir::separator());
        stx_file.append(QFileInfo(out_file).fileName());
        stx_file.replace(".wrd", ".stx");

        const QByteArray stx_data = repack_stx_strings(currentWrd.strings);
        QFile f2(stx_file);
        f2.open(QFile::WriteOnly);
        f2.write(stx_data);
        f2.close();
    }

    unsavedChanges = false;
}

void MainWindow::on_actionSaveAs_triggered()
{
    QString newFilename = QFileDialog::getSaveFileName(this, "Save WRD file", QString(), "WRD files (*.wrd);;All files (*.*)");
    if (newFilename.isEmpty())
        return;

    currentWrd.filename = newFilename;
    this->setWindowTitle("WRD Editor: " + QFileInfo(newFilename).fileName());
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

void MainWindow::reloadLists()
{
    this->ui->comboBox_SelectLabel->blockSignals(true);
    this->ui->comboBox_SelectLabel->clear();
    for (QString label_name : currentWrd.labels)
    {
        this->ui->comboBox_SelectLabel->addItem(label_name);
    }
    this->ui->comboBox_SelectLabel->blockSignals(false);
    this->on_comboBox_SelectLabel_currentIndexChanged(0);

    this->ui->listWidget_Flags->blockSignals(true);
    this->ui->listWidget_Flags->clear();
    for (int i = 0; i < currentWrd.flags.count(); i++)
    {
        QListWidgetItem *item = new QListWidgetItem(currentWrd.flags.at(i));
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        this->ui->listWidget_Flags->addItem(item);
    }
    this->ui->listWidget_Flags->blockSignals(false);

    this->ui->listWidget_Strings->blockSignals(true);
    this->ui->listWidget_Strings->clear();
    for (int i = 0; i < currentWrd.strings.count(); i++)
    {
        QString str = currentWrd.strings.at(i);
        str.replace("\n", "\\n");
        QListWidgetItem *item = new QListWidgetItem(str);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        this->ui->listWidget_Strings->addItem(item);
    }
    this->ui->listWidget_Strings->blockSignals(false);

    unsavedChanges = false;
}

void MainWindow::on_comboBox_SelectLabel_currentIndexChanged(int index)
{
    if (index < 0 || index > currentWrd.cmds.count())
        return;

    const QList<WrdCmd> cur_label_cmds = currentWrd.cmds.at(index);

    this->ui->tableWidget_LabelCode->blockSignals(true);
    this->ui->tableWidget_LabelCode->clearContents();
    this->ui->tableWidget_LabelCode->setRowCount(cur_label_cmds.count());

    for (int i = 0; i < cur_label_cmds.count(); i++)
    {
        const WrdCmd cmd = cur_label_cmds.at(i);

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
    this->ui->tableWidget_LabelCode->blockSignals(false);
}

void MainWindow::on_listWidget_Strings_itemChanged(QListWidgetItem *item)
{
    QString str = item->text();
    str.replace("\\n", "\n");
    currentWrd.strings[this->ui->listWidget_Strings->row(item)] = str;
    unsavedChanges = true;
}

void MainWindow::on_tableWidget_LabelCode_cellChanged(int row, int column)
{
    QTableWidgetItem *item = this->ui->tableWidget_LabelCode->item(row, column);

    if (row == 0)   // Opcode
    {
        const QString opText = item->text();

        bool ok;
        ushort val = opText.toUShort(&ok, 16);
        if (!ok)
        {
            QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Hex Conversion Error", QMessageBox::Ok, this);
            errorMsg.exec();
            return;
        }

        currentWrd.cmds[this->ui->comboBox_SelectLabel->currentIndex()][column].opcode = val;
    }
    else            // Args
    {
        const QString argText = item->text();

        currentWrd.cmds[this->ui->comboBox_SelectLabel->currentIndex()][column].args.clear();
        for (int argNum = 0; argNum < argText.count() / 4; argNum++)
        {
            bool ok;
            ushort val = argText.mid(argNum * 4, 4).toUShort(&ok, 16);
            if (!ok)
            {
                QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Hex Conversion Error", QMessageBox::Ok, this);
                errorMsg.exec();
                return;
            }

            currentWrd.cmds[this->ui->comboBox_SelectLabel->currentIndex()][column].args.append(val);
        }
    }

    unsavedChanges = true;
}

void MainWindow::on_listWidget_Flags_itemChanged(QListWidgetItem *item)
{
    QString flag = item->text();
    currentWrd.flags[this->ui->listWidget_Flags->row(item)] = flag;
    unsavedChanges = true;
}
