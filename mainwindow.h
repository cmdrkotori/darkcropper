#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "imagewindow.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void cropper_export(QString sourceFilename,
                        QString workingFilename,
                        ImageCropping transform);
    void cropper_escape();
    void cropper_skip();
    void cropper_nextFile();
    void cropper_show();
    void fileList_chewTop();
    void process_finished(QString fileToRemove);

    void on_singleFileBrowse_clicked();
    void on_batchFileBrowse_clicked();
    void on_folderBrowse_clicked();
    void on_otherFolderBrowse_clicked();
    void on_lightSelect_clicked();
    void on_exportReset_clicked();
    void on_escapeReset_clicked();
    void on_skipReset_clicked();
    void on_doubleReset_clicked();
    void on_noiseReset_clicked();
    void on_multiplyReset_clicked();
    void on_showRulesReset_clicked();

    void on_start_clicked();
    void on_stop_clicked();
    void on_singleFileSend_clicked();
    void on_batchFileSend_clicked();
    void on_folderSend_clicked();
    void on_listImport_clicked();
    void on_listExport_clicked();

    void on_listRemove_clicked();

    void on_listClear_clicked();

    void on_waifu2xExecutableBrowse_clicked();

    void on_waifu2xModelDirBrowse_clicked();

    void on_waifu2xProcessor_currentIndexChanged(int index);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
    void populateScreens();
    void loadSettings();
    void saveSettings();
    void checkFolders();
    void updateActions();
    void importBatchFile(QString fileName);
    void exportBatchFile(QString fileName);

    Ui::MainWindow *ui;
    ImageWindow *cropper;
};

#endif // MAINWINDOW_H
