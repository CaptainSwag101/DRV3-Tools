#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->listWidget, &QListWidget::currentTextChanged, this, &MainWindow::on_textBox_textChanged);

    openStx.setNameFilter("STX files (*.stx)");
}

MainWindow::~MainWindow()
{
    delete ui;
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
        newFilepath = openStx.getOpenFileName(this, "Open STX file", QString(), "STX files (*.stx)");
    }
    if (newFilepath.isEmpty()) return false;

    QFile f(newFilepath);
    if(!f.open(QFile::ReadOnly))
        return false;

    currentStx = f.readAll();
    f.close();

    currentFilename = newFilepath;
    ui->listWidget->setEnabled(true);
    reloadStrings();
    return true;
}

void MainWindow::reloadStrings()
{
    ui->listWidget->clear();

    QStringList strings = get_stx_strings(currentStx);
    for (QString str : strings)
    {
        str.replace("\n", "\\n");
        QListWidgetItem *item = new QListWidgetItem(str);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->listWidget->addItem(item);
    }

    unsavedChanges = false;
}

void MainWindow::on_actionOpen_triggered()
{
    openFile();
}

void MainWindow::on_actionSave_triggered()
{
    QStringList strings;
    for(int i = 0; i < ui->listWidget->count(); i++)
    {
        QListWidgetItem *item = ui->listWidget->item(i);
        QString text = item->text();
        text.replace("\\n", "\n");
        strings.append(text);
    }

    currentStx = repack_stx_strings(strings);
    QFile f(currentFilename);
    f.open(QFile::WriteOnly);
    f.write(currentStx);
    f.close();
    unsavedChanges = false;
}

void MainWindow::on_actionSaveAs_triggered()
{
    openStx.setAcceptMode(QFileDialog::AcceptSave);
    openStx.setFileMode(QFileDialog::AnyFile);
    QString newFilename = openStx.getSaveFileName(this, "Save STX file", QString(), "STX files (*.stx)");
    if (newFilename.isEmpty()) return;
    currentFilename = newFilename;

    on_actionSave_triggered();
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

void MainWindow::on_textBox_textChanged()
{
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

            if (currentFilename.isEmpty())
            {
                if (openFile(filepath))
                {
                    event->acceptProposedAction();
                    break;
                }
            }
        }
    }
}
