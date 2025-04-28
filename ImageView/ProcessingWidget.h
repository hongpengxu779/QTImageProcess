#ifndef PROCESSINGWIDGET_H
#define PROCESSINGWIDGET_H

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QCheckBox>
#include <QSpinBox>
#include <QApplication>
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
class QSpinBox;
class QButtonGroup;
class QToolButton;
class ROIOverlay;  // 添加ROIOverlay前置声明

// 定义ROI选择模式枚举
enum class ROISelectionMode {
    None,
    Rectangle,
    Circle,
    Arbitrary
};

// 新增：多圆ROI状态枚举
enum class MultiCircleState {
    None,           // 未开始选择
    FirstCircle,    // 已选择第一个圆
    SecondCircle,   // 已选择第二个圆
    RingROI         // 已生成环形ROI
};

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
    QPushButton* getApplyROIButton() const { return btnApplyROI; }
    QToolButton* getRectangleROIButton() const { return btnRectangleSelection; }

    // 获取滑块接口
    QSlider* getBrightnessSlider() const { return sliderBrightness; }
    QSlider* getGammaSlider() const { return sliderGamma; }
    QSlider* getOffsetSlider() const { return sliderOffset; }
    
    QCheckBox* getRgbToGrayCheckBox() const { return m_rgbToGray; }
    QCheckBox* getShowHistogramCheckbox() const { return m_showHistogram; }
    QSpinBox* getKernelSizeSpinBox() const { return spinKernelSize; }

    // 获取复选框状态
    bool getSubtractFiltered() const { return m_subtractFiltered ? m_subtractFiltered->isChecked() : false; }
    bool getShowHistogram() const { return m_showHistogram ? m_showHistogram->isChecked() : false; }
    int getKernelSize() const { return spinKernelSize ? spinKernelSize->value() : 3; }

    // 显示图片
    void displayImage(const QImage &image);
    
    // 重置标签显示
    void resetValueLabels();
    
    // ROI选择模式控制
    void setROIMode(bool enableROI);
    bool isROIMode() const { return m_isROIMode; }
    
    // 获取当前选择的矩形ROI (以实际图像像素坐标表示)
    QRect getRectangleROI() const { return m_imageRectangleROI; }
    QPoint getCircleCenter() const { return m_imageCircleCenter; }
    int getCircleRadius() const { return m_imageCircleRadius; }
    QPolygon getArbitraryROI() const { return m_imageArbitraryROI; }

    // 新增：获取多圆ROI相关信息
    QPoint getFirstCircleCenter() const { return m_imageCircleCenter; }
    int getFirstCircleRadius() const { return m_imageCircleRadius; }
    QPoint getSecondCircleCenter() const { return m_imageSecondCircleCenter; }
    int getSecondCircleRadius() const { return m_imageSecondCircleRadius; }
    MultiCircleState getMultiCircleState() const { return m_multiCircleState; }
    
    // 新增：获取环形ROI中的像素值
    QVector<int> getRingROIPixelValues() const;

signals:
    void mouseClicked(const QPoint& pos, int grayValue, int r, int g, int b);
    void mouseMoved(const QPoint& pos, int grayValue);
    void imageStatsUpdated(double meanValue);
    void showHistogramRequested(bool show); // Signal to show histogram dialog
    void kernelSizeChanged(int size); // Signal for kernel size change
    void roiSelected(const QRect& rect); // Signal for rectangle ROI
    void roiSelected(const QPoint& center, int radius); // Signal for circle ROI
    void roiSelected(const QPolygon& polygon); // Signal for arbitrary ROI
    
    // 新增：环形ROI选择完成信号
    void ringROISelected(const QPoint& firstCenter, int firstRadius, 
                         const QPoint& secondCenter, int secondRadius);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onImageProcessed(const QImage &processedImage);
    void onImageStatsUpdated(double meanValue);
    void updateKValueLabel(int value);
    void updateBValueLabel(int value);
    void updateGammaValueLabel(int value);
    void onROISelectionModeChanged(int id);
    void clearROISelection();

private:
    void setupUi();
    void setupROISelectionControls();
    void updateImageStats();
    double calculateMeanValue(const QImage &image);
    
    // 坐标转换方法
    QPoint mapToImageCoordinates(const QPoint& labelPos);
    QPoint mapFromImageCoordinates(const QPoint& imagePos);
    QRect getScaledImageRect();
    QRect mapToImageRect(const QRect& uiRect);
    QRect mapFromImageRect(const QRect& imageRect);
    void updateROIDisplay();
    
    // 新增：计算环形ROI区域
    void calculateRingROI();
    
    // 新增：判断点是否在环形区域内
    bool isPointInRingROI(const QPoint& point) const;

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
    QSpinBox *spinKernelSize;      // 卷积核大小控制

    // ROI选择相关控件
    QGroupBox *gbROISelection;     // ROI选择组
    QButtonGroup *roiSelectionGroup; // ROI选择按钮组
    QToolButton *btnRectangleSelection; // 矩形选择按钮
    QToolButton *btnCircleSelection; // 圆形选择按钮
    QToolButton *btnArbitrarySelection; // 任意形状选择按钮
    QToolButton *btnClearSelection; // 清除选择按钮
    QPushButton *btnApplyROI; // 应用ROI按钮
    QPushButton *btnRectangleROI; // 矩形ROI按钮
    ROIOverlay *m_roiOverlay;      // ROI覆盖层
    
    // ROI选择状态 (UI坐标)
    ROISelectionMode m_currentROIMode = ROISelectionMode::None;
    bool m_selectionInProgress = false;
    QPoint m_selectionStart;
    QPoint m_selectionCurrent;
    QVector<QPoint> m_arbitraryPoints;
    QRect m_rectangleROI;
    QPoint m_circleCenter;
    int m_circleRadius = 0;
    QPolygon m_arbitraryROI;
    
    // 新增：多圆ROI相关变量
    MultiCircleState m_multiCircleState = MultiCircleState::None;
    QPoint m_secondCircleCenter;
    int m_secondCircleRadius = 0;
    
    // ROI选择状态 (图像像素坐标)
    QRect m_imageRectangleROI;     // 以图像像素为单位的矩形ROI
    QPoint m_imageCircleCenter;    // 以图像像素为单位的圆形ROI中心
    int m_imageCircleRadius = 0;   // 以图像像素为单位的圆形ROI半径
    QPolygon m_imageArbitraryROI;  // 以图像像素为单位的任意形状ROI
    
    // 新增：第二个圆和环形ROI（图像坐标）
    QPoint m_imageSecondCircleCenter;  // 第二个圆的中心
    int m_imageSecondCircleRadius = 0; // 第二个圆的半径
    
    // 控制鼠标响应的全局变量
    bool m_isROIMode = false; // false表示显示坐标模式，true表示ROI选择模式

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
};

#endif // PROCESSINGWIDGET_H
