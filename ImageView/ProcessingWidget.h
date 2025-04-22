#ifndef PROCESSINGWIDGET_H
#define PROCESSINGWIDGET_H

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QCheckBox>
#include "../ImageProcessor/ImageProcessor.h"
#include "ImageProcessorThread.h"

class QPushButton;
class QSlider;
class QTabWidget;
class QGraphicsView;
class QLabel;
class QCheckBox;
class QGroupBox;
class QFormLayout;
class QHBoxLayout;
class QVBoxLayout;
class QFrame;

class ProcessingWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProcessingWidget(QWidget *parent = nullptr);
    ~ProcessingWidget() override;

    // 获取按钮接口
    QPushButton* getSelectButton() const { return btnSelect; }
    QPushButton* getSelectFolderButton() const { return btnSelectFolder; }
    QPushButton* getSaveButton() const { return btnSave; }
    QPushButton* getShowOriginalButton() const { return btnShowOriginal; }
    QPushButton* getFlipHButton() const { return btnFlipH; }
    QPushButton* getFlipVButton() const { return btnFlipV; }
    QPushButton* getMeanFilterButton() const { return btnMeanFilter; }
    QPushButton* getGaussianFilterButton() const { return btnGaussianFilter; }
    QPushButton* getMedianFilterButton() const { return btnMedianFilter; }
    QPushButton* getHistEqualButton() const { return btnHistEqual; }

    // 获取滑块接口
    QSlider* getBrightnessSlider() const { return sliderBrightness; }
    QSlider* getThresholdSlider() const { return sliderThreshold; }
    QSlider* getContrastSlider() const { return sliderContrast; }
    QSlider* getSaturationSlider() const { return sliderSaturation; }
    QSlider* getRSlider() const { return sliderR; }
    QSlider* getGSlider() const { return sliderG; }
    QSlider* getBSlider() const { return sliderB; }
    QSlider* getGammaSlider() const { return sliderGamma; }
    QSlider* getOffsetSlider() const { return sliderOffset; }
    QCheckBox* getRgbToGrayCheckBox() const { return m_rgbToGray; }

    // 获取复选框状态
    bool getSubtractFiltered() const { return m_subtractFiltered->isChecked(); }

    // 显示图片
    void displayImage(const QImage &image);

signals:
    void mouseClicked(const QPoint& pos, int grayValue, int r, int g, int b);
    void imageStatsUpdated(double meanValue);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void onImageProcessed(const QImage &processedImage);
    void onImageStatsUpdated(double meanValue);

private:
    void setupUi();
    void updateImageStats();

    // 缩放相关
    double m_zoomFactor = 1.0;
    const double ZOOM_FACTOR_STEP = 0.1;
    const double MIN_ZOOM = 0.1;
    const double MAX_ZOOM = 5.0;

    // 左侧按钮
    QPushButton *btnSelect;
    QPushButton *btnSelectFolder;
    QPushButton *btnSave;
    QPushButton *btnShowOriginal;
    QPushButton *btnFlipH;
    QPushButton *btnFlipV;
    QPushButton *btnMeanFilter;
    QPushButton *btnGaussianFilter;
    QPushButton *btnMedianFilter;
    QPushButton *btnHistEqual;
    QCheckBox *m_subtractFiltered;  // 控制是否从原始图像中减去滤波结果
    QCheckBox *m_rgbToGray;        // RGB转灰度复选框

    // 中间
    QTabWidget    *tabWidget;
    QGraphicsView *graphicsView;
    QLabel        *imageLabel;

    // 右侧
    QGroupBox *gbBasic;
    QSlider   *sliderBrightness;
    QSlider   *sliderThreshold;
    QSlider   *sliderContrast;
    QSlider   *sliderSaturation;
    QGroupBox *gbColor;
    QSlider   *sliderR;
    QSlider   *sliderG;
    QSlider   *sliderB;
    QSlider   *sliderGamma;
    QSlider   *sliderOffset;

    // 图片相关
    ImageProcessor *imageProcessor;
    QImage m_currentImage;
    ImageProcessorThread *m_processorThread;
};

#endif // PROCESSINGWIDGET_H
