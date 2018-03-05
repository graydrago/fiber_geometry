#include <QByteArrayList>
#include <QFileDialog>
#include <QImageReader>
#include <QImageWriter>
#include <QStandardPaths>
#include <QStringList>
#include <QToolButton>
#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QDateTime>

#include <opencv2/highgui/highgui.hpp>

#include "fiberviewer.h"
#include "ui_fiberviewer.h"


FiberViewer::FiberViewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    for (auto &i : ui->widget_2->children())
    {
        qDebug() << i->objectName();
    }

    connect(ui->openImageButton, &QToolButton::clicked, this, &FiberViewer::openImage);
}


FiberViewer::~FiberViewer()
{
    delete ui;
}


void FiberViewer::openImage()
{
    static bool firstDialog = true;

    QFileDialog dialog{this, "Open file"};

    if (firstDialog)
    {
        firstDialog = false;
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    }

    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = QImageReader::supportedMimeTypes();
    foreach (const QByteArray &mimeTypeName, supportedMimeTypes)
    {
        mimeTypeFilters.append(mimeTypeName);
    }
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");
    dialog.setDefaultSuffix("jpg");

    while(dialog.exec() == QDialog::Accepted)
    {
        if (loadImage(dialog.selectedFiles().first()))
        {
            break;
        }
    }
}


bool FiberViewer::loadImage(const QString &fileName)
{
    QImage processedImage = processGeometry(fileName);
    useImage(processedImage);

    const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
        .arg(QDir::toNativeSeparators(fileName))
		.arg(processedImage.width())
		.arg(processedImage.height())
		.arg(processedImage.depth());
    ui->statusBar->showMessage(message);

    return true;
}


void FiberViewer::useImage(const QImage &image)
{
    QPixmap pixmap;
    pixmap.convertFromImage(image);
    ui->image->setPixmap(pixmap);
}


QImage FiberViewer::processGeometry(const QString &fileName)
{
    using namespace cv;
    static Mat src;
    src = imread(fileName.toStdString().c_str(), 1);
    Mat src_gray;

    auto startTime = QDateTime::currentMSecsSinceEpoch();

    cvtColor(src, src_gray, CV_BGR2GRAY);
    GaussianBlur(src_gray, src_gray, Size(9, 9), 3, 3);

    auto coreCircles = findCore(src_gray);
    auto fiberCircles = findFiber(src_gray);
    decltype(fiberCircles) tmp;

    if (coreCircles.size() > 0 && fiberCircles.size() > 0)
    {
        for (auto& c : fiberCircles)
        {
          Point2d a(coreCircles[0][0], coreCircles[0][1]);
          Point2d b(c[0], c[1]);
          if (norm(b - a) < coreCircles[0][2])
          {
              tmp.push_back(c);
          }
        }
        fiberCircles = tmp;

        auto endTime = QDateTime::currentMSecsSinceEpoch();
        auto totalTime = (endTime - startTime) * 0.001;

        fillInfoWidget(coreCircles[0], fiberCircles[0], totalTime);
        drawCircles(src, coreCircles, Scalar(255, 0, 0));
        drawCircles(src, fiberCircles, Scalar(0, 255, 0));
    }
    else
    {
        ui->infoWidget->clear();
        if (coreCircles.size() == 0)
        {
            ui->infoWidget->addItem(QString("Can't find any core circle."));
        }

        if (fiberCircles.size() == 0)
        {
            ui->infoWidget->addItem(QString("Can't find any fiber circle."));
        }
    }

    cvtColor(src, src, CV_BGR2RGB);

    QImage outputImage(static_cast<unsigned char*>(src.data), src.cols, src.rows, QImage::Format_RGB888);
    return outputImage;
}


void FiberViewer::fillInfoWidget(cv::Vec3f coreCircle, cv::Vec3f fiberCircle, float totalTime)
{
    cv::Point2f corePoint(coreCircle[0], coreCircle[1]);
    cv::Point2f fiberPoint(fiberCircle[0], fiberCircle[1]);
    float coreDiameter = coreCircle[2] * 2.0;
    float fiberDiametr = fiberCircle[2] * 2.0;
    float distance = cv::norm(corePoint - fiberPoint);

    ui->infoWidget->clear();
    ui->infoWidget->addItem(QString("Core diameter: %1").arg(coreDiameter));
    ui->infoWidget->addItem(QString("Fiber diameter: %1").arg(fiberDiametr));
    ui->infoWidget->addItem(QString("Core center x: %1 y: %2")
        .arg(QString::number(corePoint.x),
             QString::number(corePoint.y)));
    ui->infoWidget->addItem(QString("Fiber center x: %1 y: %2")
        .arg(QString::number(fiberPoint.x),
             QString::number(fiberPoint.y)));
    ui->infoWidget->addItem(QString("Distance: %1").arg(QString::number(distance)));
    ui->infoWidget->addItem(QString("Total time: %1").arg(QString::number(totalTime)));
}


std::vector<cv::Vec3f> FiberViewer::findCore(cv::Mat &one_channel_image)
{
    std::vector<cv::Vec3f> circles;
    HoughCircles(one_channel_image, circles, CV_HOUGH_GRADIENT, 0.5, 300, 50, 30, 500, 600);
    return circles;
}


std::vector<cv::Vec3f> FiberViewer::findFiber(cv::Mat &one_channel_image)
{
    std::vector<cv::Vec3f> circles;
    HoughCircles(one_channel_image, circles, CV_HOUGH_GRADIENT, 0.5, 50, 20, 30, 50, 400);
    return circles;
}


void FiberViewer::drawCircles(cv::Mat &image, std::vector<cv::Vec3f>& circles, cv::Scalar color)
{
    using namespace cv;

    for(size_t i = 0; i < circles.size(); i++)
    {
        Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
        int radius = cvRound(circles[i][2]);
		circle(image, center, 3, color, -1, 8, 0); // center
        circle(image, center, radius, color, 2, 8, 0); // outline
    }
}
