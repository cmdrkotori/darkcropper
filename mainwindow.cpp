#include <QApplication>
#include <QDesktopWidget>
#include <QSettings>
#include <QFileDialog>
#include <QColorDialog>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "imagewindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cropper = new ImageWindow();
    connect(ui->exportEdit, &QKeySequenceEdit::keySequenceChanged,
            cropper, &ImageWindow::setExportShortcut);
    connect(ui->escapeEdit, &QKeySequenceEdit::keySequenceChanged,
            cropper, &ImageWindow::setEscapeShortcut);
    connect(ui->skipEdit, &QKeySequenceEdit::keySequenceChanged,
            cropper, &ImageWindow::setSkipShortcut);
    connect(ui->doubleEdit, &QKeySequenceEdit::keySequenceChanged,
            cropper, &ImageWindow::setDoubleShortcut);
    connect(ui->noiseEdit, &QKeySequenceEdit::keySequenceChanged,
            cropper, &ImageWindow::setNoiseShortcut);
    connect(ui->multiplyEdit, &QKeySequenceEdit::keySequenceChanged,
            cropper, &ImageWindow::setMultiplyShortcut);
    connect(ui->fitWidthEdit, &QKeySequenceEdit::keySequenceChanged,
            cropper, &ImageWindow::setWidthShortcut);
    connect(ui->fitHeightEdit, &QKeySequenceEdit::keySequenceChanged,
            cropper, &ImageWindow::setHeightShortcut);
    connect(ui->resetZoomEdit, &QKeySequenceEdit::keySequenceChanged,
            cropper, &ImageWindow::setResetZoomShortcut);
    connect(ui->resetRotationEdit, &QKeySequenceEdit::keySequenceChanged,
            cropper, &ImageWindow::setResetRotationShortcut);
    connect(ui->resetLocationEdit, &QKeySequenceEdit::keySequenceChanged,
            cropper, &ImageWindow::setResetLocationShortcut);

    connect(cropper, &ImageWindow::exportFile,
            this, &MainWindow::cropper_export);
    connect(cropper, &ImageWindow::escape,
            this, &MainWindow::cropper_escape);
    connect(cropper, &ImageWindow::skip,
            this, &MainWindow::cropper_skip);

    populateScreens();
    loadSettings();
    updateActions();
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
    delete cropper;
}

void MainWindow::cropper_export(QString sourceFilename,
                                QString workingFilename,
                                ImageCropping transform)
{
    QFileInfo info(sourceFilename);
    QString outfile = QString("%1/%2_cropped.png")
            .arg(ui->sameFolder->isChecked() ? info.absolutePath()
                                             : ui->otherFolderText->text())
            .arg(info.completeBaseName());

    auto sizeToString = [](const QSize &sz) {
        return QString("%1x%2")
                .arg(sz.width())
                .arg(sz.height());
    };
    auto placeToString = [](const QPointF &sz) {
        return QString("%1%2%3%4")
                .arg(sz.x() < 0 ? "" : "+").arg(sz.x(), 0, 'f', 5)
                .arg(sz.y() < 0 ? "" : "+").arg(sz.y(), 0, 'f', 5);
    };
    QColor light = QColor(ui->lightColor->text());
    if (!light.isValid())
        light = QColor("#FFFFFF");
    QProcess *p = new QProcess;
    QStringList args;
    args << workingFilename
         << "-colorspace" << "RGB"
         << "-virtual-pixel" << "white"
         << "+distort" << "SRT"
         << QString("%1,%2 %3 %4 %1,%2").arg(transform.image.width()/2)
                                        .arg(transform.image.height()/2)
                                        .arg(transform.scaling, 0, 'f', 5)
                                        .arg(transform.rotation, 0, 'f', 5)
         << "-write" << "mpr:src"
         << "+delete"
         << "-background" << "rgba(48,48,48)"
         << "-size" << sizeToString(cropper->emulatedSize())
         << "mpr:src"
         << "-gravity" << "center"
         << "-geometry" << placeToString(transform.translation)
         << "-size" << sizeToString(cropper->emulatedSize())
         << "-colorspace" << "sRGB"
         << QString("xc:%1").arg(light.name())
         << "+swap"
         << "-compose" << "multiply"
         << "-composite" << outfile;
    p->setProgram("convert");
    p->setArguments(args);
    QString fileToRemove = sourceFilename != workingFilename ? workingFilename
                                                             : QString();
    connect(p, static_cast<void (QProcess::*)(int)>(&QProcess::finished), [=]() {
        process_finished(fileToRemove);
    });
    connect(p, static_cast<void (QProcess::*)(int)>(&QProcess::finished), [p]() {
        p->deleteLater();
    });
    p->start();
    cropper->showMessage("Beginning export");
    fileList_chewTop();
    cropper_nextFile();
}

void MainWindow::cropper_escape()
{
    cropper->hide();
}

void MainWindow::cropper_skip()
{
    fileList_chewTop();
    cropper_nextFile();
}

void MainWindow::cropper_nextFile()
{
    if (!cropper->isDone()) {
        cropper_show();
        return;
    }
    if (ui->fileList->count() > 0) {
        auto item = ui->fileList->item(0);
        if (!item)
            return;
        cropper->setSource(item->text());
        cropper_show();
    } else {
        cropper->hide();
    }
}

void MainWindow::cropper_show()
{
    QDesktopWidget *desktop = QApplication::desktop();
    QWidget *cropwin = cropper->window();
    if (ui->fullscreen->isChecked()) {
        QRect sg = desktop->screenGeometry(ui->fullscreenScreen->currentIndex());
        cropwin->setGeometry(sg);
        cropwin->showFullScreen();
        cropper->setEmulatedSize(sg.size());
    } else {
        qreal scale = ui->windowedSize->currentText().remove('%').toDouble()/100;
        cropper->setDisplayScale(scale);
        QRect available = desktop->screenGeometry(this);
        QSize window = available.size() * scale;
        QRect placement = QStyle::alignedRect(
                    Qt::LeftToRight,
                    Qt::AlignCenter,
                    window,
                    available
                );
        cropwin->setGeometry(placement);
        cropper->setEmulatedSize(placement.size());
    }
    cropwin->show();
}

void MainWindow::fileList_chewTop()
{
    auto item = ui->fileList->takeItem(0);
    if (item)
        delete item;
}

void MainWindow::process_finished(QString fileToRemove)
{
    cropper->showMessage("Export finished");
    if (!fileToRemove.isEmpty())
        QFile(fileToRemove).remove();
}

void MainWindow::populateScreens() {
    QDesktopWidget *desktop = QApplication::desktop();
    for (int i = 0; i < desktop->screenCount(); i++) {
        QRect r = desktop->screenGeometry(i);
        QString text("%1x%2+%3+%4");
        text = text.arg(r.width()).arg(r.height()).arg(r.left()).arg(r.top());
        ui->fullscreenScreen->addItem(text);
    }
}

#define LOAD_WIDGET(x, d, t, p) \
    x->set##p(s.value(x->objectName(), d).value<t>());

#define LOAD_WIDGET_LIST(x, d) \
    x->setCurrentIndex(std::max(0, x->findText(s.value(x->objectName(), d).toString())))

#define SAVE_WIDGET(x, p) \
    s.setValue(x->objectName(), x->p())

void MainWindow::loadSettings()
{
    QSettings s;
    LOAD_WIDGET(ui->singleFileText, QString(), QString, Text);
    LOAD_WIDGET(ui->batchFileText, QString(), QString, Text);
    LOAD_WIDGET(ui->folderText, QString(), QString, Text);
    LOAD_WIDGET(ui->otherFolderText, QString(), QString, Text);
    LOAD_WIDGET(ui->lightColor, "#303030", QString, Text);
    LOAD_WIDGET(ui->singleFile, true, bool, Checked);
    LOAD_WIDGET(ui->batchFile, false, bool, Checked);
    LOAD_WIDGET(ui->folder, false, bool, Checked);
    LOAD_WIDGET(ui->otherFolder, true, bool, Checked);
    LOAD_WIDGET(ui->sameFolder, true, bool, Checked);
    LOAD_WIDGET(ui->fullscreen, true, bool, Checked);
    LOAD_WIDGET(ui->windowed, false, bool, Checked);
    LOAD_WIDGET(ui->exportEdit, QKeySequence("Return"), QKeySequence, KeySequence);
    LOAD_WIDGET(ui->escapeEdit, QKeySequence("Q"), QKeySequence, KeySequence);
    LOAD_WIDGET(ui->skipEdit, QKeySequence("S"), QKeySequence, KeySequence);
    LOAD_WIDGET(ui->doubleEdit, QKeySequence("D"), QKeySequence, KeySequence);
    LOAD_WIDGET(ui->noiseEdit, QKeySequence("N"), QKeySequence, KeySequence);
    LOAD_WIDGET(ui->multiplyEdit, QKeySequence("M"), QKeySequence, KeySequence);
    LOAD_WIDGET(ui->fitWidthEdit, QKeySequence("W"), QKeySequence, KeySequence);
    LOAD_WIDGET(ui->fitHeightEdit, QKeySequence("H"), QKeySequence, KeySequence);
    LOAD_WIDGET(ui->resetZoomEdit, QKeySequence("1"), QKeySequence, KeySequence);
    LOAD_WIDGET(ui->resetRotationEdit, QKeySequence("2"), QKeySequence, KeySequence);
    LOAD_WIDGET(ui->resetLocationEdit, QKeySequence("3"), QKeySequence, KeySequence);

    LOAD_WIDGET_LIST(ui->fullscreenScreen, "1920x1080+0+0");
    LOAD_WIDGET_LIST(ui->windowedSize, "75%");
}

void MainWindow::saveSettings()
{
    QSettings s;
    SAVE_WIDGET(ui->singleFileText, text);
    SAVE_WIDGET(ui->batchFileText, text);
    SAVE_WIDGET(ui->folderText, text);
    SAVE_WIDGET(ui->otherFolderText, text);
    SAVE_WIDGET(ui->lightColor, text);
    SAVE_WIDGET(ui->singleFile, isChecked);
    SAVE_WIDGET(ui->batchFile, isChecked);
    SAVE_WIDGET(ui->folder, isChecked);
    SAVE_WIDGET(ui->otherFolder, isChecked);
    SAVE_WIDGET(ui->sameFolder, isChecked);
    SAVE_WIDGET(ui->fullscreen, isChecked);
    SAVE_WIDGET(ui->windowed, isChecked);
    SAVE_WIDGET(ui->exportEdit, keySequence);
    SAVE_WIDGET(ui->escapeEdit, keySequence);
    SAVE_WIDGET(ui->skipEdit, keySequence);
    SAVE_WIDGET(ui->doubleEdit, keySequence);
    SAVE_WIDGET(ui->noiseEdit, keySequence);
    SAVE_WIDGET(ui->multiplyEdit, keySequence);
    SAVE_WIDGET(ui->fitWidthEdit, keySequence);
    SAVE_WIDGET(ui->fitHeightEdit, keySequence);
    SAVE_WIDGET(ui->resetZoomEdit, keySequence);
    SAVE_WIDGET(ui->resetLocationEdit, keySequence);

    SAVE_WIDGET(ui->fullscreenScreen, currentText);
    SAVE_WIDGET(ui->windowedSize, currentText);
}

void MainWindow::updateActions()
{
    cropper->setExportShortcut(ui->exportEdit->keySequence());
    cropper->setEscapeShortcut(ui->escapeEdit->keySequence());
    cropper->setSkipShortcut(ui->skipEdit->keySequence());
    cropper->setDoubleShortcut(ui->doubleEdit->keySequence());
    cropper->setNoiseShortcut(ui->noiseEdit->keySequence());
    cropper->setMultiplyShortcut(ui->multiplyEdit->keySequence());
}

void MainWindow::importBatchFile(QString fileName)
{
    QFile f(fileName);
    if (!f.open(QFile::ReadOnly | QFile::Text))
        return;
    QTextStream s(&f);
    ui->fileList->addItems(s.readAll().split('\n', QString::SkipEmptyParts));
}

void MainWindow::exportBatchFile(QString fileName)
{
    QFile f(fileName);
    if (!f.open(QFile::ReadWrite | QFile::Truncate | QFile::Text))
        return;
    QTextStream s(&f);
    for (int i = 0; i < ui->fileList->count(); i++)
        s << ui->fileList->item(i)->text() << '\n';
    s.flush();
}

void MainWindow::on_singleFileBrowse_clicked()
{
    QString m = QFileDialog::getOpenFileName(this, "Select File");
    if (m.isNull())
        return;
    ui->singleFileText->setText(m);
}

void MainWindow::on_batchFileBrowse_clicked()
{
    QString m = QFileDialog::getOpenFileName(this, "Select File");
    if (m.isNull())
        return;
    ui->batchFileText->setText(m);
}

void MainWindow::on_folderBrowse_clicked()
{
    QString m = QFileDialog::getExistingDirectory(this, "Select Folder");
    if (m.isNull())
        return;
    ui->folderText->setText(m);
}

void MainWindow::on_otherFolderBrowse_clicked()
{
    QString m = QFileDialog::getExistingDirectory(this, "Select Folder");
    if (m.isNull())
        return;
    ui->otherFolderText->setText(m);
}

void MainWindow::on_lightSelect_clicked()
{
    QColor c = QColorDialog::getColor(QColor(ui->lightColor->text()), this);
    if (!c.isValid())
        return;
    ui->lightColor->setText(c.name());
}
void MainWindow::on_exportReset_clicked()
{
    ui->exportEdit->clear();
}

void MainWindow::on_escapeReset_clicked()
{
    ui->escapeEdit->clear();
}

void MainWindow::on_skipReset_clicked()
{
    ui->skipEdit->clear();
}

void MainWindow::on_doubleReset_clicked()
{
    ui->doubleEdit->clear();
}

void MainWindow::on_noiseReset_clicked()
{
    ui->noiseEdit->clear();
}

void MainWindow::on_multiplyReset_clicked()
{
    ui->multiplyEdit->clear();
}

void MainWindow::on_start_clicked()
{
    cropper_nextFile();
}

void MainWindow::on_stop_clicked()
{
    cropper->stop();
    cropper->hide();
}


void MainWindow::on_singleFileSend_clicked()
{
    ui->fileList->addItem(ui->singleFileText->text());
}

void MainWindow::on_batchFileSend_clicked()
{
    importBatchFile(ui->batchFileText->text());
}

void MainWindow::on_folderSend_clicked()
{
    QString dirText = ui->folderText->text();
    if (!dirText.endsWith("/"))
        dirText += "/";
    QDir d(dirText);
    QFileInfoList l = d.entryInfoList({"*.png","*.jpg","*.jpeg"}, QDir::Files | QDir::NoDotAndDotDot);
    if (l.isEmpty())
        return;
    for (const QFileInfo &info : l)
        ui->fileList->addItem(info.absoluteFilePath());
}

void MainWindow::on_listImport_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, "Open file");
    if (!file.isEmpty())
        importBatchFile(file);
}

void MainWindow::on_listExport_clicked()
{
    QString file = QFileDialog::getSaveFileName(this, "Save file");
    if (!file.isEmpty())
        exportBatchFile(file);
}
