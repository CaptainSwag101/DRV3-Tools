#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QComboBox>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QTableView>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tableCode->setColumnWidth(1, 180);
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
    saveFile(currentWrd.filename);
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
        newFilepath = QFileDialog::getOpenFileName(this, "Open WRD file", QString(), "WRD files (*.wrd);;All files (*.*)");
    }
    if (newFilepath.isEmpty()) return false;

    QFile f(newFilepath);
    if (!f.open(QFile::ReadOnly)) return false;

    currentWrd = wrd_from_bytes(f.readAll(), newFilepath);
    f.close();

    this->setWindowTitle("WRD Editor: " + QFileInfo(newFilepath).fileName());

    reloadLabelList();
    WrdCodeModel *code = new WrdCodeModel(this, &currentWrd, ui->comboBox_SelectLabel->currentIndex());
    WrdStringsModel *strings = new WrdStringsModel(this, &currentWrd);
    WrdFlagsModel *flags = new WrdFlagsModel(this, &currentWrd);
    QObject::connect(code, &WrdCodeModel::editCompleted, this, &MainWindow::on_editCompleted);
    QObject::connect(strings, &WrdStringsModel::editCompleted, this, &MainWindow::on_editCompleted);
    QObject::connect(flags, &WrdFlagsModel::editCompleted, this, &MainWindow::on_editCompleted);
    ui->tableCode->setModel(code);
    ui->tableStrings->setModel(strings);
    ui->tableFlags->setModel(flags);

    ui->centralWidget->setEnabled(true);
    ui->tableCode->scrollToTop();
    ui->tableStrings->scrollToTop();
    ui->tableFlags->scrollToTop();
    return true;
}

bool MainWindow::saveFile(QString newFilepath)
{
    if (newFilepath.isEmpty())
    {
        newFilepath = QFileDialog::getSaveFileName(this, "Save WRD file", QString(), "WRD files (*.wrd);;All files (*.*)");
    }
    if (newFilepath.isEmpty()) return false;


    /*
    currentWrd.labels.clear();
    for (int i = 0; i < ui->comboBox_SelectLabel->count(); i++)
    {
        currentWrd.labels.append(ui->comboBox_SelectLabel->itemText(i));
    }
    */

    QFile f(newFilepath);
    if (!f.open(QFile::WriteOnly)) return false;

    const QByteArray out_data = wrd_to_bytes(currentWrd);
    f.write(out_data);
    f.close();

    // If the strings are internal, we've saved them already in wrd_to_bytes()
    if (currentWrd.external_strings && currentWrd.strings.count() > 0)
    {
        // Strings are stored in the "(current spc name)_text_(region).spc" file,
        // within an STX file with the same name as the current WRD file.
        QString stx_file = QFileInfo(newFilepath).absolutePath();
        if (stx_file.endsWith(".SPC", Qt::CaseInsensitive))
            stx_file.chop(4);

        QString region = "_US";
        if (stx_file.right(3).startsWith("_"))
        {
            region = stx_file.right(3);
            stx_file.chop(3);
        }

        stx_file.append("_text" + region + ".SPC");
        stx_file.append(QDir::separator());
        stx_file.append(QFileInfo(newFilepath).fileName());
        stx_file.replace(".wrd", ".stx");

        const QByteArray stx_data = repack_stx_strings(currentWrd.strings);
        QFile f2(stx_file);
        f2.open(QFile::WriteOnly);
        f2.write(stx_data);
        f2.close();
    }

    currentWrd.filename = newFilepath;
    this->setWindowTitle("WRD Editor: " + QFileInfo(newFilepath).fileName());

    unsavedChanges = false;
    return true;
}

void MainWindow::reloadLabelList()
{
    ui->comboBox_SelectLabel->blockSignals(true);
    ui->tableCode->blockSignals(true);
    ui->comboBox_SelectLabel->clear();

    for (QString label_name : currentWrd.labels)
    {
        ui->comboBox_SelectLabel->addItem(label_name);
    }

    ui->tableCode->blockSignals(false);
    ui->comboBox_SelectLabel->blockSignals(false);
}

void MainWindow::on_comboBox_SelectLabel_currentIndexChanged(int index)
{
    ui->tableCode->setModel(new WrdCodeModel(this, &currentWrd, index));
    ui->tableCode->scrollToTop();
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
