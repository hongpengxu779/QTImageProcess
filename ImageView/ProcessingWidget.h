#ifndef PROCESSINGWIDGET_H
#define PROCESSINGWIDGET_H

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QCheckBox>
#include "../ImageProcessor/ImageProcessor.h"
// 注释掉不存在的头文件
// #include "ImageProcessorThread.h"

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
    QSlider* getGammaSlider() const { return sliderGamma; }
    QSlider* getOffsetSlider() const { return sliderOffset; }
    
    QCheckBox* getRgbToGrayCheckBox() const { return m_rgbToGray; }
    QCheckBox* getShowHistogramCheckbox() const { return m_showHistogram; }

    // 获取复选框状态
    bool getSubtractFiltered() const { return m_subtractFiltered ? m_subtractFiltered->isChecked() : false; }
    bool getShowHistogram() const { return m_showHistogram ? m_showHistogram->isChecked() : false; }

    // 显示图片
    void displayImage(const QImage &image);
    
    // 重置标签显示
    void resetValueLabels();

signals:
    void mouseClicked(const QPoint& pos, int grayValue, int r, int g, int b);
    void mouseMoved(const QPoint& pos, int grayValue);
    void imageStatsUpdated(double meanValue);
    void showHistogramRequested(bool show); // Signal to show histogram dialog

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void onImageProcessed(const QImage &processedImage);
    void onImageStatsUpdated(double meanValue);
    void updateKValueLabel(int value);
    void updateBValueLabel(int value);
    void updateGammaValueLabel(int value);

private:
    void setupUi();
    void updateImageStats();
    double calculateMeanValue(const QImage &image);  // 新添加的方法声明

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
    QCheckBox *m_showHistogram;    // 显示灰度直方图复选框

    // 中间
    QTabWidget    *tabWidget;
    QGraphicsView *graphicsView;
    QLabel        *imageLabel;

    // 右侧
    QGroupBox *gbBasic;
    QSlider   *sliderBrightness;
    QGroupBox *gbColor;
    QSlider   *sliderGamma;
    QSlider   *sliderOffset;
    QLabel    *lblKValue;         // 显示k值的标签
    QLabel    *lblBValue;         // 显示b值的标签
    QLabel    *lblGammaValue;     // 显示gamma值的标签

    // 图片相关
    ImageProcessor *imageProcessor;
    QImage m_currentImage;
    // 移除不存在的类型
    // ImageProcessorThread *m_processorThread;
};

#endif // PROCESSINGWIDGET_H
