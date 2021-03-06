// This module used to be based on a QOpenGLWidget, and was fairly smooth, but
// then I found an image greater than 4096 pixels in one dimension.  :-(

#include <QDebug>
#include <cmath>
#include <QMessageBox>
#include <QAction>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>
#include <QProcess>
#include <QProcessEnvironment>
#include <QCloseEvent>
#include "imagewindow.h"


ImageCropping::ImageCropping()
    : scaling(1), rotation(0), translation(0,0) {}

ImageCropping ImageCropping::fromImage(const QImage &image)
{
    ImageCropping ic;
    int w = image.width();
    int h = image.height();
    ic.scaling = 1;
    ic.translation = QPointF(w&1 ? 0.5f : 0, h&1 ? 0.5f : 0);
    ic.rotation = 0;
    return ic;
}

void ImageCropping::sourceScaledBy(int powerOf2)
{
    scaling /= 1<<powerOf2;
}

QTransform ImageCropping::transform(qreal initialScaling)
{
    QTransform m;
    m.translate(translation.x()*initialScaling, translation.y()*initialScaling);
    m.rotate(rotation);
    m.scale(scaling * initialScaling, scaling * initialScaling);
    return m;
}

QString ImageCropping::toDisplayString()
{
    return QString("%1%, %2˚, (%3,%4)")
            .arg(scaling*100).arg(rotation).arg(translation.x()).arg(translation.y());
}



ImageWindow::ImageWindow(QWidget *parent)
    : QWidget(parent),
      done(true),
      displayScale(1.0),
      background(64, 64, QImage::Format_RGB32),
      processor(-1),
      noise(NoNoise),
      multiplying(false),
      rulesShown(false),
      opacity(0),
      doubler(NULL)
{
    setWindowTitle("Dark Cropper Manipulation");
    setWindowIcon(QIcon(":/images/logo-48x48.png"));
    setupBackground();
    setupActions();
    setExecutable();
    setModelDir();
}

ImageWindow::~ImageWindow()
{
    stop();
    cleanupActions();
    removeWorkingCopy();
}

ImageCropping ImageWindow::getTransform()
{
    return transform;
}

void ImageWindow::setDisplayScale(qreal factor)
{
    displayScale = factor / devicePixelRatio();
}

bool ImageWindow::setExecutable(const QString &folder)
{
    QStringList candidates;
    if (!folder.isEmpty())
        candidates << folder + "/waifu2x-converter-cpp";
    else
        candidates << "./waifu2x-converter-cpp"
                   << "/usr/local/bin/waifu2x-converter-cpp"
                   << QStandardPaths::findExecutable("waifu2x-converter-cpp");

    for (const QString &binary : candidates) {
        executable = binary;
        if (QFileInfo(executable).isExecutable())
            return true;
    }
    executable.clear();
    return false;
}

bool ImageWindow::setModelDir(const QString &folder)
{
    QStringList candidates;
    if (!folder.isEmpty()) {
        candidates << folder;
    } else {
        candidates << QFileInfo(executable).dir().path() + "/models_rgb"
                   << QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.waifu2x/models_rgb"
                   << QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.waifu2x"
                   << "/usr/local/share/waifu2x-converter-cpp"
                   << "/usr/share/waifu2x-converter-cpp";
    }

    for (const QString &baseFolder : candidates) {
        modelFolder = baseFolder;
        if (!modelFolder.isEmpty() && QDir(modelFolder).exists())
            return true;
    }
    modelFolder.clear();
    return false;
}

void ImageWindow::setProcessor(int index)
{
    processor = index;
}

void ImageWindow::setEmulatedSize(QSize size)
{
    emulatedSize_ = size / displayScale;
}

QStringList ImageWindow::processors()
{
    QProcess p;
    p.setProgram(executable);
    p.setArguments({"--list-processor"});
    p.start();
    p.waitForFinished();
    QString processOutput = QString::fromLocal8Bit(p.readAll());
    QStringList lines = processOutput.split('\n', QString::SkipEmptyParts);
    QStringList out;
    for (QString &line : lines) {
        line = line.trimmed();
        if (line.count() && line.at(0).isNumber())
            out << line;
    }
    return out;
}

QSize ImageWindow::emulatedSize()
{
    return emulatedSize_;
}

bool ImageWindow::isDone()
{
    return done;
}

void ImageWindow::stop()
{
    if (doubler) {
        doubler->terminate();
        doubler->deleteLater();
        doubler = NULL;
    }
}

void ImageWindow::setExportShortcut(const QKeySequence &shortcut)
{
    actionExport->setShortcut(shortcut);
}

void ImageWindow::setEscapeShortcut(const QKeySequence &shortcut)
{
    actionEscape->setShortcut(shortcut);
}

void ImageWindow::setSkipShortcut(const QKeySequence &shortcut)
{
    actionSkip->setShortcut(shortcut);
}

void ImageWindow::setDoubleShortcut(const QKeySequence &shortcut)
{
    actionDouble->setShortcut(shortcut);
}

void ImageWindow::setNoiseShortcut(const QKeySequence &shortcut)
{
    actionNoise->setShortcut(shortcut);
}

void ImageWindow::setMultiplyShortcut(const QKeySequence &shortcut)
{
    actionMultiply->setShortcut(shortcut);
}

void ImageWindow::setWidthShortcut(const QKeySequence &shortcut)
{
    actionWidth->setShortcut(shortcut);
}

void ImageWindow::setHeightShortcut(const QKeySequence &shortcut)
{
    actionHeight->setShortcut(shortcut);
}

void ImageWindow::setResetZoomShortcut(const QKeySequence &shortcut)
{
    actionResetZoom->setShortcut(shortcut);
}

void ImageWindow::setResetRotationShortcut(const QKeySequence &shortcut)
{
    actionResetRotation->setShortcut(shortcut);
}

void ImageWindow::setResetLocationShortcut(const QKeySequence &shortcut)
{
    actionResetLocation->setShortcut(shortcut);
}

void ImageWindow::setShowRulesShortcut(const QKeySequence &shortcut)
{
    actionShowRules->setShortcut(shortcut);
}

void ImageWindow::setSource(const QString &filename)
{
    done = false;
    source.load(filename);
    sourceFilename = workingFilename = filename;
    transform = ImageCropping::fromImage(source);
    noise = NoNoise;
    updateFields();
    calculateDrawPoint();
    update();
}

void ImageWindow::setScaledSource(const QString &filename, int powerOf2)
{
    source.load(filename);
    transform.sourceScaledBy(powerOf2);
    calculateDrawPoint();
    update();
}

void ImageWindow::showMessage(const QString &message)
{
    opacity = 2.5;
    this->message = message;
    update();
}

void ImageWindow::paintEvent(QPaintEvent *ev)
{
     (void)ev;
    QPainter p;
    p.begin(this);

    QBrush fillBrush;
    fillBrush.setTextureImage(background);
    p.fillRect(QRect(0, 0, glWidth+1, glHeight+1), fillBrush);

    QRect windowRect = QRect(-glWidth/2.0, -glHeight/2.0,
                             glWidth, glHeight);
    p.setWindow(windowRect);
    if (!source.isNull()) {
        QTransform oldTransform = p.transform();
        p.setWorldTransform(transform.transform(displayScale));
        p.setCompositionMode(multiplying ? QPainter::CompositionMode_Multiply
                                         : QPainter::CompositionMode_SourceOver);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.drawImage(drawPoint, source);
        p.setTransform(oldTransform);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    p.setWindow(QRect(0, 0, glWidth, glHeight));
    QFont f;
    auto drawMessage = [&] (qreal opacity, int size,
                            qreal weightX, qreal weightY,
                            const QString &message) {
        p.setBrush(QColor(0, 0, 0));
        p.setPen(QColor(224, 224, 224));
        f.setPixelSize(size);
        p.setFont(f);
        QRect bounding = p.fontMetrics().boundingRect(message);
        if (bounding.width() > glWidth * 0.80)
            bounding.setWidth(glWidth * 0.80);
        QRectF textArea((glWidth - bounding.width() - 40) * weightX + 20,
                        (glHeight - bounding.height() - 40) * weightY + 20,
                        bounding.width(), bounding.height());
        p.setOpacity(std::min(1.0, opacity)*0.25);
        p.drawRoundedRect(textArea.adjusted(-10, -10, 10, 10), 10, 10);
        p.setOpacity(opacity);
        p.drawText(textArea, 0, message);
    };


    if (!message.isEmpty() && opacity > 0.001) {
        opacity -= 0.05;
        drawMessage(opacity, 30, 1.0, 1.0, message);
        QTimer::singleShot(100, this, SLOT(update()));
    }
    drawMessage(1.0, 15, 0.0, 0.0, transform.toDisplayString());
    drawMessage(1.0, 15, 0.5, 0.0, fileField);
    drawMessage(1.0, 15, 0.0, 1.0, noiseField);

    if (rulesShown) {
        qreal x2 = width() - 1;
        qreal y2 = height() - 1;
        QPainterPath path;

        //rule of thirds
        path.addRect(0, height()/3.0, x2, height()/3.0);
        path.addRect(width()/3.0, 0, width()/3.0, y2);

        //rule of diagonal
        path.moveTo(0, 0);
        path.lineTo(x2, x2);
        path.moveTo(0, y2);
        path.lineTo(y2, 0);

        path.moveTo(x2, 0);
        path.lineTo(x2-y2, y2);
        path.moveTo(x2, y2);
        path.lineTo(x2-y2, 0);

        //rebatement
        if (x2 > y2) {
            path.addRect(0, 0, y2, y2);
            path.addRect(x2-y2, 0, y2, y2);
        } else {
            path.addRect(0, 0, x2, x2);
            path.addRect(0, y2-x2, x2, x2);
        }

        p.setBrush(QBrush());
        p.setPen(QColor(0xba, 0xba, 0xba));
        p.drawPath(path);
    }
    p.end();
}

void ImageWindow::resizeEvent(QResizeEvent *ev)
{
    glWidth = (ev->size().width() & ~1);
    glHeight = (ev->size().height() & ~1);
    calculateDrawPoint();
}

void ImageWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    actionEscape_triggered();
}

void ImageWindow::mousePressEvent(QMouseEvent *event)
{
    mouseTransform = transform;
    mouseLast = event->localPos();
    mouseCause = event->button();
}

void ImageWindow::mouseMoveEvent(QMouseEvent *event)
{
    float ts = (event->modifiers() & Qt::ShiftModifier) ? 0.25 : 1;
    if (mouseCause == Qt::MiddleButton) {
        // FIXME: move along angles when ctrl is pressed
        transform.scaling = mouseTransform.scaling + ts*(mouseLast.y() - event->localPos().y())/100;
    } else if (mouseCause == Qt::LeftButton) {
        transform.translation = mouseTransform.translation + ts*(event->localPos() - mouseLast)/displayScale;
    } else if (mouseCause == Qt::RightButton) {
        // FIXME: rotate around point of click
        if (std::abs(transform.scaling) > 0.0001 )
            transform.rotation = mouseTransform.rotation + ts*(event->localPos().x() - mouseLast.x())/transform.scaling;
    }
    mouseLast = event->localPos();
    mouseTransform = transform;
    update();
}

void ImageWindow::actionExport_triggered()
{
    done = true;
    if (doubler) {
        doubler->terminate();
        doubler->deleteLater();
        doubler = NULL;
    }
    emit exportFile(sourceFilename, workingFilename, transform);
}

void ImageWindow::actionEscape_triggered()
{
    done = true;
    removeWorkingCopy();
    emit escape();
}

void ImageWindow::actionSkip_triggered()
{
    done = true;
    removeWorkingCopy();
    emit skip();
}

void ImageWindow::actionDouble_triggered()
{
    if (doubler) {
        showMessage("Please wait for the previous job to finish.");
        return;
    }
    if (modelFolder.isEmpty() || executable.isEmpty()) {
        showMessage("waifu2x not configured");
        return;
    }
    showMessage("Doubling in progress. Please wait.");

    doubledFilename = QString("/dev/shm/darkcropper-%1.%2")
            .arg(QUuid::createUuid().toString())
            .arg(QFileInfo(workingFilename).suffix());

    doubler = new QProcess();
    QString model = noise != NoNoise ? "noise-scale" : "scale";
    QStringList args = {
        "--scale-ratio", "2.000",
        "-m", model,
        "--model-dir", modelFolder,
        "-i", workingFilename,
        "-o", doubledFilename
    };
    if (noise != NoNoise)
        args << "--noise-level" << QString::number((int)noise);
    if (processor >= 0)
        args << "--processor" << QString::number(processor);
    doubler->setArguments(args);
    doubler->setProgram(executable);
    qDebug() << executable << args;
    connect(doubler, SIGNAL(finished(int)),
            this, SLOT(process_finished(int)));
    doubler->start();
}

void ImageWindow::actionNoise_triggered()
{
    QVector<NoiseLevel> nextNoise = { SlightNoise, HeavyNoise, NoNoise };
    if (QFileInfo(modelFolder + "/noise3_model.json").exists())
        nextNoise.insert(2, ExcessiveNoise);
    noise = nextNoise.at(noise);
    updateFields();
    update();
}

void ImageWindow::actionMultiply_triggered()
{
    multiplying ^= true;
    update();
}

void ImageWindow::actionWidth_triggered()
{
    if (source.isNull())
        return;
    transform.scaling = emulatedSize_.width() / (double)source.width();
    update();
}

void ImageWindow::actionHeight_triggered()
{
    if (source.isNull())
        return;
    transform.scaling = emulatedSize_.height() / (double)source.height();
    update();
}

void ImageWindow::actionResetZoom_triggered()
{
    transform.scaling = 1.0;
    update();
}

void ImageWindow::actionResetRotation_triggered()
{
    transform.rotation = 0.0;
    update();
}

void ImageWindow::actionResetLocation_triggered()
{
    transform.translation = {0,0};
    update();
}

void ImageWindow::actionShowRules_triggered()
{
    rulesShown ^= true;
    update();
}

void ImageWindow::process_finished(int exitCode)
{
    if (exitCode) {
        QString message = "The program said:\n"
                + QString::fromUtf8(doubler->readAllStandardError());
        QMessageBox::critical(NULL, "Doubler failed.", message);
        goto end;
    }
    if (workingFilename != sourceFilename)
        QFile(workingFilename).remove();
    workingFilename = doubledFilename;
    setScaledSource(doubledFilename, 1);
    showMessage("Doubling done");
    end:
    doubler->deleteLater();
    doubler = NULL;
}

void ImageWindow::setupBackground()
{
    std::random_device rseed;
    std::mt19937 rgen(rseed());
    std::uniform_int_distribution<int> dist(0x25, 0x35);

    background.fill(QColor(0x30, 0x30, 0x30));
    QPainter p(&background);
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++)
            background.setPixel(x, y, 0x010101 * dist(rgen));
}


void ImageWindow::setupActions()
{
#define MAKE_ACTION(x, y) \
    x = new QAction(y, this); \
    connect(x, &QAction::triggered, this, &ImageWindow::x##_triggered); \
    addAction(x)

    MAKE_ACTION(actionExport, "Export");
    MAKE_ACTION(actionEscape, "Escape");
    MAKE_ACTION(actionSkip, "Skip");
    MAKE_ACTION(actionDouble, "Double");
    MAKE_ACTION(actionNoise, "Toggle Noise Factor");
    MAKE_ACTION(actionMultiply, "Toggle Multiply");
    MAKE_ACTION(actionWidth, "Fit Width");
    MAKE_ACTION(actionHeight, "Fit Height");
    MAKE_ACTION(actionResetZoom, "Reset Zoom");
    MAKE_ACTION(actionResetRotation, "Reset Rotation");
    MAKE_ACTION(actionResetLocation, "Reset Location");
    MAKE_ACTION(actionShowRules, "Show Rules");

#undef MAKE_ACTION
}

void ImageWindow::cleanupActions()
{
    delete actionExport;
    delete actionEscape;
    delete actionSkip;
    delete actionDouble;
    delete actionNoise;
    delete actionMultiply;
}

void ImageWindow::calculateDrawPoint()
{
    drawPoint = -QPointF(source.width()/2.0, source.height()/2.0);

}

void ImageWindow::updateFields()
{
    fileField = QFileInfo(sourceFilename).fileName();
    noiseField = QStringList({"0: No noise", "1: Slight noise", "2: Heavy noise", "3: Excessive noise"}).at(noise);
}

void ImageWindow::removeWorkingCopy()
{
    if (workingFilename != sourceFilename) {
        QFile f(workingFilename);
        f.remove();
    }
}
