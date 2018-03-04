#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <opencv2/imgproc/imgproc.hpp>
#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class FiberViewer : public QMainWindow
{
    Q_OBJECT

public:
    explicit FiberViewer(QWidget *parent = 0);
    ~FiberViewer();


public slots:
    void openImage();


private:
    Ui::MainWindow *ui;
    QImage image;

    bool loadImage(const QString &filename);
    void useImage(const QImage &image);

    cv::vector<cv::Vec3f> findCore(cv::Mat &one_channel_image);
    cv::vector<cv::Vec3f> findFiber(cv::Mat &one_channel_image);
    void drawCircles(cv::Mat &image, cv::vector<cv::Vec3f>& circles);
    QImage processGeometry(const QString &fileName);
    void fillInfoWidget(cv::Vec3f coreCircle, cv::Vec3f fiberCircle, float totalTime);
};

#endif // MAINWINDOW_H
