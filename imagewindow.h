#ifndef IMAGEWINDOW_H
#define IMAGEWINDOW_H

#include <QOpenGLWidget>
#include <QKeySequence>
#include <QPixmap>
#include <ext/random>
#include <QProcess>

class QAction;

class ImageCropping {
public:
    ImageCropping();
    static ImageCropping fromImage(const QImage &image);
    void sourceScaledBy(int powerOf2);
    QTransform transform(qreal initialScaling = 1.0);
    QString toDisplayString();

    QSize image;
    qreal scaling;
    qreal rotation;
    QPointF translation;
};

class ImageWindow : public QWidget {
    Q_OBJECT
    enum NoiseLevel { NoNoise, SlightNoise, HeavyNoise };
public:
    ImageWindow(QWidget *parent = 0);
    ~ImageWindow();
    ImageCropping getTransform();
    void setDisplayScale(qreal factor);
    void setEmulatedSize(QSize size);
    QSize emulatedSize();
    bool isDone();
    void stop();

signals:
    void exportFile(QString sourceFilename,
                    QString workingFilename,
                    ImageCropping transform);
    void escape();
    void skip();

public slots:
    void setExportShortcut(const QKeySequence &shortcut);
    void setEscapeShortcut(const QKeySequence &shortcut);
    void setSkipShortcut(const QKeySequence &shortcut);
    void setDoubleShortcut(const QKeySequence &shortcut);
    void setNoiseShortcut(const QKeySequence &shortcut);
    void setMultiplyShortcut(const QKeySequence &shortcut);
    void setWidthShortcut(const QKeySequence &shortcut);
    void setHeightShortcut(const QKeySequence &shortcut);
    void setResetZoomShortcut(const QKeySequence &shortcut);
    void setResetRotationShortcut(const QKeySequence &shortcut);
    void setResetLocationShortcut(const QKeySequence &shortcut);
    void setShowRulesShortcut(const QKeySequence &shortcut);
    void setSource(const QString &filename);
    void setScaledSource(const QString &filename, int powerOf2);
    void showMessage(const QString &message);

protected:
    void paintEvent(QPaintEvent *ev);
    void resizeEvent(QResizeEvent *ev);
    void closeEvent(QCloseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent * event);

private slots:
    void actionExport_triggered();
    void actionEscape_triggered();
    void actionSkip_triggered();
    void actionDouble_triggered();
    void actionNoise_triggered();
    void actionMultiply_triggered();
    void actionWidth_triggered();
    void actionHeight_triggered();
    void actionResetZoom_triggered();
    void actionResetRotation_triggered();
    void actionResetLocation_triggered();
    void actionShowRules_triggered();
    void process_finished(int exitCode);

private:
    void setupBackground();
    void setupActions();
    void cleanupActions();
    void calculateDrawPoint();
    void updateFields();
    void removeWorkingCopy();

    bool done;

    QSize emulatedSize_;
    qreal displayScale;
    QImage background;
    QImage source;
    QString sourceFilename;
    QString workingFilename;
    QString doubledFilename;
    NoiseLevel noise;
    bool multiplying;
    bool rulesShown;

    ImageCropping transform;
    int glWidth;
    int glHeight;
    QPointF drawPoint;

    QPointF mouseLast;
    ImageCropping mouseTransform;
    Qt::MouseButton mouseCause;

    QString fileField;
    QString noiseField;
    QString message;
    qreal opacity;

    QAction *actionExport;
    QAction *actionEscape;
    QAction *actionSkip;
    QAction *actionDouble;
    QAction *actionNoise;
    QAction *actionMultiply;
    QAction *actionWidth;
    QAction *actionHeight;
    QAction *actionResetZoom;
    QAction *actionResetRotation;
    QAction *actionResetLocation;
    QAction *actionShowRules;

    QProcess *doubler;
};


#endif // IMAGEWINDOW_H
