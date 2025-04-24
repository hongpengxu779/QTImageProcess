#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>
#include <QLabel>
#include <QStatusBar>
#include "ImageView/ProcessingWidget.h"
#include "ImageProcessor/ImageProcessor.h"
#include "HistogramDialog.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onSelectImage();
    void onSelectFolder();
    void onSaveImage();
    void onShowOriginal();
    void onFlipHorizontal();
    void onFlipVertical();
    void onMeanFilter();
    void onGaussianFilter();
    void onMedianFilter();
    void onBrightnessChanged(int value);
    void onGammaChanged(int value);
    void onOffsetChanged(int value);
    void onRChanged(int value);
    void onGChanged(int value);
    void onBChanged(int value);
    void onRgbToGrayChanged(bool checked);
    void onConvertToGrayscale();
    void onImageLoaded(bool success);
    void onImageProcessed();
    void onError(const QString &errorMessage);
    void onMouseClicked(const QPoint &pos, int grayValue, int r, int g, int b);
    void onMouseMoved(const QPoint &pos, int grayValue);
    void onImageStatsUpdated(double meanValue);
    void updateStatusBar(const QPoint &pos, int grayValue, int r, int g, int b);
    void updateImageStats(double meanValue);
    void onHistogramCalculated(const QVector<int> &histogram);
    void onHistogramEqualization();
    void onHistogramStretching();
    void onHistEqualClicked();
    void onResetToOriginal();
    void onShowHistogramChanged(bool show);
    
    // ROI选择相关槽函数
    void onRectangleROISelected(const QRect& rect);
    void onCircleROISelected(const QPoint& center, int radius);
    void onArbitraryROISelected(const QPolygon& polygon);
    void onApplyROI();
    void onRectangleROIButtonClicked();

private:
    void setupUI();
    void setupConnections();
    void createMenuBar();
    void setupStatusBar();
    void applyCurrentTransformations();    // 应用当前所有变换
    void updateHistogramDialog();          // Update histogram dialog with current image
    
    // ROI统计计算函数
    void calculateROIStats(const QImage& image, const QRect& roi, double& mean, double& variance);
    void calculateCircleROIStats(const QImage& image, const QPoint& center, int radius, double& mean, double& variance);

    ProcessingWidget *m_processingWidget;
    ImageProcessor *imageProcessor;
    QLabel *m_statusLabel;
    QLabel *m_pixelInfoLabel;
    QLabel *m_meanValueLabel;
    HistogramDialog *m_histogramDialog;    // Histogram dialog
};

#endif // MAINWINDOW_H
