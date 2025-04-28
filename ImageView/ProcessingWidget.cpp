#include "ProcessingWidget.h"
#include <QPushButton>
#include <QSlider>
#include <QTabWidget>
#include <QGraphicsView>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QMouseEvent>
#include <QPainter>
#include <QDebug>
#include <QSpinBox>
#include <QButtonGroup>
#include <QToolButton>
#include <QIcon>
#include <QMessageBox>

// 辅助函数声明
void logRectInfo(const QString& prefix, const QRect& rect);

// 创建一个透明的覆盖层来绘制ROI
class ROIOverlay : public QWidget {
public:
    ROIOverlay(QWidget *parent) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents); // 允许鼠标事件穿透到底层控件
        setAttribute(Qt::WA_NoSystemBackground);       // 无系统背景
        setAttribute(Qt::WA_TranslucentBackground);    // 透明背景
    }

    // 更新ROIOverlay的setROIData方法，支持多圆和环形ROI
    void setROIData(const QRect& rect, const QPoint& center, int radius, 
                   const QVector<QPoint>& points, bool inProgress,
                   const QRect& imageRect, int imageWidth, int imageHeight,
                   const QRect& actualImageRect, int imageCircleRadius = 0,
                   const QPoint& secondCenter = QPoint(), int secondRadius = 0,
                   int secondImageRadius = 0, MultiCircleState multiCircleState = MultiCircleState::None) {
        m_rectangleROI = rect;
        m_circleCenter = center;
        m_circleRadius = radius;
        m_arbitraryPoints = points;
        m_selectionInProgress = inProgress;
        m_imageRectangleROI = imageRect;
        m_imageWidth = imageWidth;
        m_imageHeight = imageHeight;
        m_actualImageRect = actualImageRect; // 保存实际图像显示区域
        m_imageCircleRadius = imageCircleRadius; // 保存图像坐标中的圆半径
        
        // 新增: 保存第二个圆数据和多圆状态
        m_secondCircleCenter = secondCenter;
        m_secondCircleRadius = secondRadius;
        m_secondImageCircleRadius = secondImageRadius;
        m_multiCircleState = multiCircleState;
        
        // 计算宽高比例
        if (imageWidth > 0 && imageHeight > 0 && 
            rect.width() > 0 && rect.height() > 0 &&
            imageRect.width() > 0 && imageRect.height() > 0) {
            
            qDebug() << "ROI覆盖层设置:";
            qDebug() << "  UI矩形: " << rect;
            qDebug() << "  图像矩形: " << imageRect;
            qDebug() << "  图像显示区域: " << m_actualImageRect;
            qDebug() << "  比例: UI/图像 = " 
                    << QString::number(static_cast<double>(rect.width()) / imageRect.width(), 'f', 2) << " x " 
                    << QString::number(static_cast<double>(rect.height()) / imageRect.height(), 'f', 2);
        }
        
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QWidget::paintEvent(event);
        
        // 如果实际图像区域为空，不绘制
        if (m_actualImageRect.isEmpty()) {
            return;
        }
        
        // 获取绘图区域
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // 设置裁剪区域，只在实际图像区域内绘制
        painter.setClipRect(m_actualImageRect);
        
        // 设置绘制参数
        QPen pen;
        
        if (m_selectionInProgress) {
            // 选择进行中时使用更醒目的颜色
            pen.setColor(QColor(255, 102, 0)); // 亮橙色
            pen.setWidth(2);
            pen.setStyle(Qt::DashLine);
        } else {
            // 选择完成时使用绿色
            pen.setColor(QColor(0, 180, 0)); // 深绿色
            pen.setWidth(2);
            pen.setStyle(Qt::SolidLine);
        }
        
        painter.setPen(pen);
        
        // 半透明填充
        QBrush brush(QColor(128, 218, 235, 60)); // 半透明浅蓝色
        painter.setBrush(brush);
        
        // 绘制矩形ROI - 确保只绘制与图像区域相交的部分
        if (!m_rectangleROI.isNull()) {
            // 计算与实际图像区域的交集
            QRect drawRect = m_rectangleROI.intersected(m_actualImageRect);
            if (!drawRect.isEmpty()) {
                painter.drawRect(drawRect);
                
                // 在矩形四角绘制小方块手柄
                int handleSize = 6;
                QColor handleColor(255, 255, 255); // 白色手柄
                QPen handlePen(QColor(0, 0, 0)); // 黑色边框
                handlePen.setWidth(1);
                painter.setPen(handlePen);
                painter.setBrush(QBrush(handleColor));
                
                // 左上
                if (m_actualImageRect.contains(drawRect.topLeft()))
                    painter.drawRect(QRect(drawRect.left() - handleSize/2, 
                                         drawRect.top() - handleSize/2, 
                                         handleSize, handleSize));
                // 右上
                if (m_actualImageRect.contains(drawRect.topRight()))
                    painter.drawRect(QRect(drawRect.right() - handleSize/2, 
                                         drawRect.top() - handleSize/2, 
                                         handleSize, handleSize));
                // 左下
                if (m_actualImageRect.contains(drawRect.bottomLeft()))
                    painter.drawRect(QRect(drawRect.left() - handleSize/2, 
                                         drawRect.bottom() - handleSize/2, 
                                         handleSize, handleSize));
                // 右下
                if (m_actualImageRect.contains(drawRect.bottomRight()))
                    painter.drawRect(QRect(drawRect.right() - handleSize/2, 
                                         drawRect.bottom() - handleSize/2, 
                                         handleSize, handleSize));
                
                // 显示ROI尺寸信息
                QFont font = painter.font();
                font.setPointSize(9);
                painter.setFont(font);
                
                QString sizeInfo = QString("%1×%2").arg(m_imageRectangleROI.width()).arg(m_imageRectangleROI.height());
                QPen textPen(Qt::white);
                painter.setPen(textPen);
                
                // 绘制半透明背景
                QRect textRect = painter.fontMetrics().boundingRect(sizeInfo);
                textRect.adjust(-5, -3, 5, 3);
                
                // 确保文本显示在图像区域内
                QPoint textPos = drawRect.bottomLeft();
                if (textPos.y() + textRect.height() + 5 > m_actualImageRect.bottom()) {
                    textPos.setY(drawRect.top() - textRect.height() - 5);
                } else {
                    textPos.setY(textPos.y() + 5);
                }
                
                if (textPos.x() + textRect.width() > m_actualImageRect.right()) {
                    textPos.setX(m_actualImageRect.right() - textRect.width() - 5);
                }
                
                textRect.moveTopLeft(textPos);
                
                // 确保文本区域在图像内
                textRect = textRect.intersected(m_actualImageRect);
                if (!textRect.isEmpty()) {
                    painter.fillRect(textRect, QColor(0, 0, 0, 128));
                    painter.drawText(textRect, Qt::AlignCenter, sizeInfo);
                }
            }
        }
        
        // 绘制圆形ROI - 修复圆形绘制问题
        if (m_circleRadius > 0) {
            // 使用不同的颜色绘制第一个圆形
            QPen circlePen = m_multiCircleState >= MultiCircleState::FirstCircle ? 
                            QPen(QColor(0, 128, 255)) : pen; // 蓝色表示第一个圆
            circlePen.setWidth(2);
            painter.setPen(circlePen);
            
            QBrush circleBrush = m_multiCircleState >= MultiCircleState::FirstCircle ? 
                               QBrush(QColor(0, 128, 255, 40)) : brush; // 半透明蓝色
            painter.setBrush(circleBrush);
            
            // 检查圆心是否在图像区域内
            if (m_actualImageRect.contains(m_circleCenter)) {
                // 计算圆与图像区域相交的部分
                int clippedRadius = m_circleRadius;
                
                // 边界裁剪计算
                clippedRadius = qMin(clippedRadius, m_circleCenter.x() - m_actualImageRect.left());
                clippedRadius = qMin(clippedRadius, m_actualImageRect.right() - m_circleCenter.x());
                clippedRadius = qMin(clippedRadius, m_circleCenter.y() - m_actualImageRect.top());
                clippedRadius = qMin(clippedRadius, m_actualImageRect.bottom() - m_circleCenter.y());
                
                // 绘制圆形
                if (clippedRadius > 0) {
                    painter.drawEllipse(m_circleCenter, clippedRadius, clippedRadius);
                    
                    // 在圆心绘制十字线
                    QPen centerPen(QColor(255, 255, 255)); // 白色中心点
                    centerPen.setWidth(1);
                    painter.setPen(centerPen);
                    int crossSize = 6;
                    painter.drawLine(m_circleCenter.x() - crossSize, m_circleCenter.y(),
                                  m_circleCenter.x() + crossSize, m_circleCenter.y());
                    painter.drawLine(m_circleCenter.x(), m_circleCenter.y() - crossSize,
                                  m_circleCenter.x(), m_circleCenter.y() + crossSize);
                    
                    // 显示圆形ROI信息
                    QFont font = painter.font();
                    font.setPointSize(9);
                    painter.setFont(font);
                    
                    QString sizeInfo = QString("R=%1").arg(m_imageCircleRadius);
                    if (m_multiCircleState >= MultiCircleState::FirstCircle) {
                        sizeInfo = QString("R1=%1").arg(m_imageCircleRadius);
                    }
                    QPen textPen(Qt::white);
                    painter.setPen(textPen);
                    
                    // 绘制半透明背景
                    QRect textRect = painter.fontMetrics().boundingRect(sizeInfo);
                    textRect.adjust(-5, -3, 5, 3);
                    
                    // 确保文本显示在图像区域内 - 在圆下方
                    QPoint textPos = QPoint(m_circleCenter.x() - textRect.width()/2, 
                                          m_circleCenter.y() + clippedRadius + 5);
                    
                    if (textPos.y() + textRect.height() > m_actualImageRect.bottom()) {
                        textPos.setY(m_circleCenter.y() - clippedRadius - textRect.height() - 5);
                    }
                    
                    if (textPos.x() < m_actualImageRect.left()) {
                        textPos.setX(m_actualImageRect.left());
                    }
                    if (textPos.x() + textRect.width() > m_actualImageRect.right()) {
                        textPos.setX(m_actualImageRect.right() - textRect.width());
                    }
                    
                    textRect.moveTopLeft(textPos);
                    
                    // 确保文本区域在图像内
                    textRect = textRect.intersected(m_actualImageRect);
                    if (!textRect.isEmpty()) {
                        painter.fillRect(textRect, QColor(0, 0, 0, 128));
                        painter.drawText(textRect, Qt::AlignCenter, sizeInfo);
                    }
                }
            }
        }
        
        // 新增：绘制第二个圆形ROI
        if (m_multiCircleState >= MultiCircleState::SecondCircle && m_secondCircleRadius > 0) {
            // 使用不同的颜色绘制第二个圆形
            QPen secondCirclePen(QColor(255, 128, 0)); // 橙色表示第二个圆
            secondCirclePen.setWidth(2);
            painter.setPen(secondCirclePen);
            
            QBrush secondCircleBrush(QColor(255, 128, 0, 40)); // 半透明橙色
            painter.setBrush(secondCircleBrush);
            
            // 检查圆心是否在图像区域内
            if (m_actualImageRect.contains(m_secondCircleCenter)) {
                // 计算圆与图像区域相交的部分
                int clippedRadius = m_secondCircleRadius;
                
                // 边界裁剪计算
                clippedRadius = qMin(clippedRadius, m_secondCircleCenter.x() - m_actualImageRect.left());
                clippedRadius = qMin(clippedRadius, m_actualImageRect.right() - m_secondCircleCenter.x());
                clippedRadius = qMin(clippedRadius, m_secondCircleCenter.y() - m_actualImageRect.top());
                clippedRadius = qMin(clippedRadius, m_actualImageRect.bottom() - m_secondCircleCenter.y());
                
                // 绘制圆形
                if (clippedRadius > 0) {
                    painter.drawEllipse(m_secondCircleCenter, clippedRadius, clippedRadius);
                    
                    // 在圆心绘制十字线
                    QPen centerPen(QColor(255, 255, 255)); // 白色中心点
                    centerPen.setWidth(1);
                    painter.setPen(centerPen);
                    int crossSize = 6;
                    painter.drawLine(m_secondCircleCenter.x() - crossSize, m_secondCircleCenter.y(),
                                  m_secondCircleCenter.x() + crossSize, m_secondCircleCenter.y());
                    painter.drawLine(m_secondCircleCenter.x(), m_secondCircleCenter.y() - crossSize,
                                  m_secondCircleCenter.x(), m_secondCircleCenter.y() + crossSize);
                    
                    // 显示圆形ROI信息
                    QFont font = painter.font();
                    font.setPointSize(9);
                    painter.setFont(font);
                    
                    QString sizeInfo = QString("R2=%1").arg(m_secondImageCircleRadius);
                    
                    QPen textPen(Qt::white);
                    painter.setPen(textPen);
                    
                    // 绘制半透明背景
                    QRect textRect = painter.fontMetrics().boundingRect(sizeInfo);
                    textRect.adjust(-5, -3, 5, 3);
                    
                    // 确保文本显示在图像区域内 - 在圆下方
                    QPoint textPos = QPoint(m_secondCircleCenter.x() - textRect.width()/2, 
                                          m_secondCircleCenter.y() + clippedRadius + 5);
                    
                    if (textPos.y() + textRect.height() > m_actualImageRect.bottom()) {
                        textPos.setY(m_secondCircleCenter.y() - clippedRadius - textRect.height() - 5);
                    }
                    
                    if (textPos.x() < m_actualImageRect.left()) {
                        textPos.setX(m_actualImageRect.left());
                    }
                    if (textPos.x() + textRect.width() > m_actualImageRect.right()) {
                        textPos.setX(m_actualImageRect.right() - textRect.width());
                    }
                    
                    textRect.moveTopLeft(textPos);
                    
                    // 确保文本区域在图像内
                    textRect = textRect.intersected(m_actualImageRect);
                    if (!textRect.isEmpty()) {
                        painter.fillRect(textRect, QColor(0, 0, 0, 128));
                        painter.drawText(textRect, Qt::AlignCenter, sizeInfo);
                    }
                }
            }
            
            // 如果两个圆都已经选择完成，绘制环形区域指示
            if (m_multiCircleState == MultiCircleState::RingROI) {
                // 使用不同的颜色表示环形区域
                QPen ringPen(QColor(255, 0, 128)); // 粉红色表示环形区域
                ringPen.setWidth(2);
                ringPen.setStyle(Qt::DotLine);
                painter.setPen(ringPen);
                
                QBrush ringBrush(QColor(255, 0, 128, 30)); // 半透明粉红色
                painter.setBrush(ringBrush);
                
                // 为环形区域添加标记 - 在两圆之间绘制连接线
                painter.drawLine(m_circleCenter, m_secondCircleCenter);
                
                // 计算环形区域的中点位置
                QPoint midPoint((m_circleCenter.x() + m_secondCircleCenter.x()) / 2,
                              (m_circleCenter.y() + m_secondCircleCenter.y()) / 2);
                
                // 计算环形区域的描述（根据两个圆的位置和大小关系）
                QString ringDescription;
                
                // 判断两个圆的位置关系
                double centerDistance = sqrt(
                    pow(m_secondCircleCenter.x() - m_circleCenter.x(), 2) +
                    pow(m_secondCircleCenter.y() - m_circleCenter.y(), 2)
                );
                
                if (centerDistance < m_circleRadius + m_secondCircleRadius) {
                    // 两个圆相交
                    ringDescription = "相交环形ROI";
                } else {
                    // 两个圆分离
                    ringDescription = "分离环形ROI";
                }
                
                // 绘制环形区域说明
                QRect textRect = painter.fontMetrics().boundingRect(ringDescription);
                textRect.adjust(-5, -3, 5, 3);
                textRect.moveCenter(midPoint);
                
                // 确保文本区域在图像内
                textRect = textRect.intersected(m_actualImageRect);
                if (!textRect.isEmpty()) {
                    painter.fillRect(textRect, QColor(0, 0, 0, 128));
                    painter.setPen(Qt::white);
                    painter.drawText(textRect, Qt::AlignCenter, ringDescription);
                }
            }
        }
        
        // 绘制任意形状ROI
        if (m_arbitraryPoints.size() > 1) {
            // 设置多边形边线样式
            QPen polyPen(QColor(255, 165, 0)); // 橙色
            polyPen.setWidth(2);
            painter.setPen(polyPen);
            
            // 创建裁剪后的点集
            QVector<QPoint> clippedPoints;
            for (const QPoint &pt : m_arbitraryPoints) {
                if (m_actualImageRect.contains(pt)) {
                    clippedPoints.append(pt);
                }
            }
            
            // 绘制已经点击的点之间的线
            for (int i = 0; i < clippedPoints.size() - 1; ++i) {
                painter.drawLine(clippedPoints[i], clippedPoints[i+1]);
            }
            
            // 如果选择尚未完成，绘制临时线段
            if (m_selectionInProgress && clippedPoints.size() >= 2) {
                painter.drawLine(clippedPoints.last(), clippedPoints.first());
            }
            
            // 绘制点
            QPen pointPen(QColor(0, 0, 255)); // 蓝色点
            pointPen.setWidth(2);
            painter.setPen(pointPen);
            
            // 绘制控制点
            for (const QPoint &pt : clippedPoints) {
                painter.setBrush(QBrush(QColor(255, 255, 255))); // 白色填充
                painter.drawEllipse(pt, 4, 4);
            }
        }
    }

private:
    QRect m_rectangleROI;
    QPoint m_circleCenter;
    int m_circleRadius = 0;
    QVector<QPoint> m_arbitraryPoints;
    bool m_selectionInProgress = false;
    QRect m_imageRectangleROI;
    int m_imageWidth = 0;  
    int m_imageHeight = 0;
    QRect m_actualImageRect; // 实际图像显示区域
    int m_imageCircleRadius = 0; // 图像坐标中的圆半径
    
    // 新增：第二个圆和多圆状态
    QPoint m_secondCircleCenter;
    int m_secondCircleRadius = 0;
    int m_secondImageCircleRadius = 0;
    MultiCircleState m_multiCircleState = MultiCircleState::None;
};

ProcessingWidget::ProcessingWidget(QWidget *parent)
    : QWidget(parent)
    , btnSelect(nullptr)
    , btnSelectFolder(nullptr)
    , btnSave(nullptr)
    , btnShowOriginal(nullptr)
    , btnFlipH(nullptr)
    , btnFlipV(nullptr)
    , btnMeanFilter(nullptr)
    , btnGaussianFilter(nullptr)
    , btnMedianFilter(nullptr)
    , btnHistEqual(nullptr)
    , tabWidget(nullptr)
    , graphicsView(nullptr)
    , imageLabel(nullptr)
    , gbBasic(nullptr)
    , sliderBrightness(nullptr)
    , gbColor(nullptr)
    , m_subtractFiltered(nullptr)
    , m_rgbToGray(nullptr)
    , m_showHistogram(nullptr)
    , sliderGamma(nullptr)
    , sliderOffset(nullptr)
    , lblKValue(nullptr)
    , lblBValue(nullptr)
    , lblGammaValue(nullptr)
    , gbROISelection(nullptr)
    , roiSelectionGroup(nullptr)
    , btnRectangleSelection(nullptr)
    , btnCircleSelection(nullptr)
    , btnArbitrarySelection(nullptr)
    , btnClearSelection(nullptr)
    , btnApplyROI(nullptr)
    , m_zoomFactor(1.0)
    , ZOOM_FACTOR_STEP(0.1)
    , MAX_ZOOM(2.0)
    , MIN_ZOOM(0.5)
    , spinKernelSize(nullptr)
    , m_currentROIMode(ROISelectionMode::None)
    , m_selectionInProgress(false)
    , m_arbitraryPoints()
    , m_rectangleROI()
    , m_circleCenter()
    , m_circleRadius(0)
    , m_arbitraryROI()
    , m_imageRectangleROI()
    , m_imageCircleCenter()
    , m_imageCircleRadius(0)
    , m_imageArbitraryROI()
    , m_roiOverlay(nullptr)
{
    try {
        qDebug() << "Initializing ProcessingWidget...";

        setMouseTracking(true);

        // 设置UI
        setupUi();
        
        if (!imageLabel) {
            throw std::runtime_error("Failed to create imageLabel");
        }
        
        // 确保imageLabel可以接收鼠标事件
        imageLabel->setMouseTracking(true);
        
        // 创建ROI覆盖层
        m_roiOverlay = new ROIOverlay(imageLabel);
        m_roiOverlay->setGeometry(0, 0, imageLabel->width(), imageLabel->height());
        m_roiOverlay->show();
        
        qDebug() << "UI setup completed";

        qDebug() << "ProcessingWidget initialization completed successfully";
    } catch (const std::exception& e) {
        qCritical() << "Error in ProcessingWidget constructor:" << e.what();
        throw; // 重新抛出异常以便在MainWindow中捕获
    }
}

ProcessingWidget::~ProcessingWidget()
{
    try {
        // 清理资源
    } catch (const std::exception& e) {
        qDebug() << "Error in ProcessingWidget destructor:" << e.what();
    }
}

void ProcessingWidget::setupUi()
{
    try {
        qDebug() << "开始设置UI...";
        auto *hMain = new QHBoxLayout(this);
        hMain->setContentsMargins(10,10,10,10);
        hMain->setSpacing(15);

        // 左侧布局
        QWidget *leftW = new QWidget;
        auto *vLeft = new QVBoxLayout(leftW);
        vLeft->setSpacing(12);
        leftW->setMinimumWidth(200);

        // 创建功能分组框
        QGroupBox *gbFile = new QGroupBox(tr("文件操作"));
        QGroupBox *gbFlip = new QGroupBox(tr("图像翻转"));
        QGroupBox *gbFilter = new QGroupBox(tr("图像滤波"));
        
        // 注意：ROI选择控件将在右侧布局中添加，不在左侧添加
        qDebug() << "准备创建ROI选择控件...";
        setupROISelectionControls();
        qDebug() << "ROI选择控件创建完成, gbROISelection=" << (gbROISelection ? "有效" : "无效");

        // 文件操作组
        auto *vFile = new QVBoxLayout(gbFile);
        btnSelect = new QPushButton(tr("选择图像"));
        btnSelectFolder = new QPushButton(tr("选择文件夹"));
        btnSave = new QPushButton(tr("保存图片"));
        btnShowOriginal = new QPushButton(tr("显示原图"));

        QSize buttonSize(120, 35);
        btnSelect->setFixedSize(buttonSize);
        btnSelectFolder->setFixedSize(buttonSize);
        btnSave->setFixedSize(buttonSize);
        btnShowOriginal->setFixedSize(buttonSize);

        vFile->addWidget(btnSelect);
        vFile->addWidget(btnSelectFolder);
        vFile->addWidget(btnSave);
        vFile->addWidget(btnShowOriginal);
        vFile->addStretch();

        // 图像翻转组
        auto *vFlip = new QVBoxLayout(gbFlip);
        btnFlipH = new QPushButton(tr("水平翻转"));
        btnFlipV = new QPushButton(tr("垂直翻转"));

        btnFlipH->setFixedSize(buttonSize);
        btnFlipV->setFixedSize(buttonSize);

        vFlip->addWidget(btnFlipH);
        vFlip->addWidget(btnFlipV);
        vFlip->addStretch();

        // 图像滤波组
        auto *vFilter = new QVBoxLayout(gbFilter);
        btnMeanFilter = new QPushButton(tr("均值滤波"));
        btnGaussianFilter = new QPushButton(tr("高斯滤波"));
        btnMedianFilter = new QPushButton(tr("中值滤波"));
        btnHistEqual = new QPushButton(tr("直方图均衡"));

        btnMeanFilter->setFixedSize(buttonSize);
        btnGaussianFilter->setFixedSize(buttonSize);
        btnMedianFilter->setFixedSize(buttonSize);
        btnHistEqual->setFixedSize(buttonSize);

        // 创建卷积核大小控制
        auto *kernelLayout = new QHBoxLayout();
        QLabel *kernelLabel = new QLabel(tr("卷积核大小:"));
        spinKernelSize = new QSpinBox();
        spinKernelSize->setMinimum(3);
        spinKernelSize->setMaximum(31);
        spinKernelSize->setSingleStep(2);
        spinKernelSize->setValue(3);
        spinKernelSize->setToolTip(tr("设置滤波的卷积核大小 (3-31, 仅奇数)"));
        
        // 确保只能设置奇数值
        connect(spinKernelSize, QOverload<int>::of(&QSpinBox::valueChanged), 
                [this](int value) {
                    // 如果是偶数，调整到最近的奇数
                    if (value % 2 == 0) {
                        if (value > spinKernelSize->value())
                            spinKernelSize->setValue(value + 1);
                        else
                            spinKernelSize->setValue(value - 1);
                    } else {
                        emit kernelSizeChanged(value);
                    }
                });
        
        kernelLayout->addWidget(kernelLabel);
        kernelLayout->addWidget(spinKernelSize);

        vFilter->addLayout(kernelLayout);

        // 添加复选框
        m_subtractFiltered = new QCheckBox(tr("从原图中减去"));
        m_subtractFiltered->setChecked(false);
        vFilter->addWidget(m_subtractFiltered);

        vFilter->addWidget(btnMeanFilter);
        vFilter->addWidget(btnGaussianFilter);
        vFilter->addWidget(btnMedianFilter);
        vFilter->addWidget(btnHistEqual);
        vFilter->addStretch();

        // 将分组框添加到左侧布局 - 注意这里只添加三个分组框，不包含ROI选择框
        vLeft->addWidget(gbFile);
        vLeft->addWidget(gbFlip);
        vLeft->addWidget(gbFilter);
        vLeft->addStretch();

        // 中间布局
        QWidget *centerW = new QWidget;
        auto *vCenter = new QVBoxLayout(centerW);
        imageLabel = new QLabel;
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setMinimumSize(600, 500);
        imageLabel->setStyleSheet("QLabel { background-color : white; border: 1px solid gray; }");
        imageLabel->setMouseTracking(true);
        vCenter->addWidget(imageLabel);

        // 右侧布局
        QWidget *rightW = new QWidget;
        auto *vRight = new QVBoxLayout(rightW);
        vRight->setSpacing(20);
        rightW->setMinimumWidth(250);

        // 基础调整组 - 包含线性变换、Gamma校正和RGB转灰度
        gbBasic = new QGroupBox(tr("基础调整"));
        gbBasic->setMinimumHeight(400);
        auto *vBasic = new QVBoxLayout(gbBasic);
        vBasic->setSpacing(15);
        vBasic->setContentsMargins(10, 20, 10, 10);

        // 添加RGB转灰度复选框
        m_rgbToGray = new QCheckBox(tr("RGB转灰度图"));
        m_rgbToGray->setChecked(false);
        m_rgbToGray->setStyleSheet("QCheckBox { font-weight: bold; }");
        vBasic->addWidget(m_rgbToGray);

        // 添加间距
        vBasic->addSpacing(10);

        // 添加线性变换标签
        QLabel *lblLinearTransform = new QLabel(tr("<b>线性变换 (y = kx + b)</b>"));
        vBasic->addWidget(lblLinearTransform);

        // 创建系数k的滑动条和标签
        QWidget *kWidget = new QWidget;
        auto *kLayout = new QHBoxLayout(kWidget);
        kLayout->setContentsMargins(0, 0, 0, 0);
        kLayout->setSpacing(0);

        QLabel *lblK = new QLabel(tr("系数 k:"));
        lblK->setMinimumWidth(50);

        sliderBrightness = new QSlider(Qt::Horizontal);
        sliderBrightness->setRange(-100, 100);
        sliderBrightness->setValue(0);
        sliderBrightness->setMinimumHeight(30);
        sliderBrightness->setTickPosition(QSlider::TicksBelow);
        sliderBrightness->setTickInterval(20);

        lblKValue = new QLabel("k = 1.00");
        lblKValue->setMinimumWidth(70);

        kLayout->addWidget(lblK);
        kLayout->addWidget(sliderBrightness);
        kLayout->addWidget(lblKValue);

        vBasic->addWidget(kWidget);

        // 创建偏移量b的滑动条和标签
        QWidget *bWidget = new QWidget;
        auto *bLayout = new QHBoxLayout(bWidget);
        bLayout->setContentsMargins(0, 0, 0, 0);
        bLayout->setSpacing(0);

        QLabel *lblB = new QLabel(tr("偏移量 b:"));
        lblB->setMinimumWidth(50);

        sliderOffset = new QSlider(Qt::Horizontal);
        sliderOffset->setRange(-100, 100);
        sliderOffset->setValue(0);
        sliderOffset->setMinimumHeight(30);
        sliderOffset->setTickPosition(QSlider::TicksBelow);
        sliderOffset->setTickInterval(20);

        lblBValue = new QLabel("b = 0");
        lblBValue->setMinimumWidth(70);

        bLayout->addWidget(lblB);
        bLayout->addWidget(sliderOffset);
        bLayout->addWidget(lblBValue);

        vBasic->addWidget(bWidget);

        // 添加间距
        vBasic->addSpacing(10);

        // 添加Gamma校正标签
        QLabel *lblGammaCorrection = new QLabel(tr("<b>Gamma校正</b>"));
        vBasic->addWidget(lblGammaCorrection);

        // 创建Gamma的滑动条和标签
        QWidget *gammaWidget = new QWidget;
        auto *gammaLayout = new QHBoxLayout(gammaWidget);
        gammaLayout->setContentsMargins(0, 0, 0, 0);
        gammaLayout->setSpacing(0);

        QLabel *lblGamma = new QLabel(tr("Gamma值:"));
        lblGamma->setMinimumWidth(70);

        sliderGamma = new QSlider(Qt::Horizontal);
        sliderGamma->setRange(1, 100);
        sliderGamma->setValue(10);
        sliderGamma->setMinimumHeight(30);
        sliderGamma->setTickPosition(QSlider::TicksBelow);
        sliderGamma->setTickInterval(10);

        lblGammaValue = new QLabel("γ = 1.00");
        lblGammaValue->setMinimumWidth(80);
        lblGammaValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        gammaLayout->addWidget(lblGamma);
        gammaLayout->addSpacing(5);
        gammaLayout->addWidget(sliderGamma);
        gammaLayout->addWidget(lblGammaValue);

        vBasic->addWidget(gammaWidget);

        // 添加弹性空间
        vBasic->addStretch();

        vRight->addWidget(gbBasic);

        // 色彩调整组
        gbColor = new QGroupBox(tr("灰度直方图调整"));
        gbColor->setMinimumHeight(150);
        auto *vColor = new QVBoxLayout(gbColor);
        vColor->setSpacing(15);

        // Add histogram display checkbox
        m_showHistogram = new QCheckBox(tr("显示灰度直方图"));
        m_showHistogram->setChecked(false);
        m_showHistogram->setStyleSheet("QCheckBox { font-weight: bold; }");
        vColor->addWidget(m_showHistogram);

        // Connect the checkbox state change signal
        connect(m_showHistogram, &QCheckBox::stateChanged, [this](int state) {
            emit showHistogramRequested(state == Qt::Checked);
        });

        // 添加一些说明文字
        QLabel *histogramInfoLabel = new QLabel(tr("注意：显示直方图将会打开一个新窗口，显示当前图像的灰度分布情况。"));
        histogramInfoLabel->setWordWrap(true);
        histogramInfoLabel->setStyleSheet("QLabel { color: #666; }");
        vColor->addWidget(histogramInfoLabel);

        // 添加弹性空间
        vColor->addStretch();

        // 将灰度直方图组添加到右侧布局
        vRight->addWidget(gbColor);
        
        // 添加ROI选择分组框到右侧布局 - 确保一定添加到右侧
        if (gbROISelection) {
            qDebug() << "正在将ROI选择控件添加到右侧布局...";
            vRight->addWidget(gbROISelection);
            qDebug() << "ROI选择控件已添加到右侧布局";
        } else {
            qDebug() << "错误: ROI选择控件为空，无法添加到右侧布局!";
        }
        
        vRight->addStretch(1);

        // 连接滑块值改变信号到更新标签函数
        connect(sliderBrightness, &QSlider::valueChanged, this, &ProcessingWidget::updateKValueLabel);
        connect(sliderOffset, &QSlider::valueChanged, this, &ProcessingWidget::updateBValueLabel);
        connect(sliderGamma, &QSlider::valueChanged, this, &ProcessingWidget::updateGammaValueLabel);

        // 初始化标签显示
        updateKValueLabel(sliderBrightness->value());
        updateBValueLabel(sliderOffset->value());
        updateGammaValueLabel(sliderGamma->value());

        hMain->addWidget(leftW);
        hMain->addWidget(centerW, 1);
        hMain->addWidget(rightW);
    } catch (const std::exception& e) {
        qDebug() << "Error in setupUi:" << e.what();
    }
}

void ProcessingWidget::setupROISelectionControls()
{
    try {
        qDebug() << "开始创建ROI选择控件...";
        
        // 创建ROI选择分组框
        gbROISelection = new QGroupBox(tr("ROI选择"));
        if (!gbROISelection) {
            qDebug() << "错误: 无法创建ROI选择分组框!";
            return;
        }
        qDebug() << "ROI选择分组框创建成功";
        
        auto *vROI = new QVBoxLayout(gbROISelection);
        vROI->setSpacing(10);
        
        // 创建按钮组
        roiSelectionGroup = new QButtonGroup(this);
        roiSelectionGroup->setExclusive(true); // 单选模式
        
        // 创建工具按钮行布局
        auto *hToolButtons = new QHBoxLayout();
        
        // 矩形选择按钮
        btnRectangleSelection = new QToolButton();
        btnRectangleSelection->setText(tr("矩形"));
        btnRectangleSelection->setToolTip(tr("矩形区域选择"));
        btnRectangleSelection->setCheckable(true);
        btnRectangleSelection->setAutoExclusive(true);
        btnRectangleSelection->setFixedSize(40, 40);
        
        // 圆形选择按钮
        btnCircleSelection = new QToolButton();
        btnCircleSelection->setText(tr("圆形"));
        btnCircleSelection->setToolTip(tr("圆形区域选择"));
        btnCircleSelection->setCheckable(true);
        btnCircleSelection->setAutoExclusive(true);
        btnCircleSelection->setFixedSize(40, 40);
        
        // 任意形状选择按钮
        btnArbitrarySelection = new QToolButton();
        btnArbitrarySelection->setText(tr("任意"));
        btnArbitrarySelection->setToolTip(tr("任意形状区域选择"));
        btnArbitrarySelection->setCheckable(true);
        btnArbitrarySelection->setAutoExclusive(true);
        btnArbitrarySelection->setFixedSize(40, 40);
        
        // 清除选择按钮
        btnClearSelection = new QToolButton();
        btnClearSelection->setText(tr("清除"));
        btnClearSelection->setToolTip(tr("清除ROI选择"));
        btnClearSelection->setFixedSize(40, 40);
        
        // 添加按钮到按钮组
        roiSelectionGroup->addButton(btnRectangleSelection, static_cast<int>(ROISelectionMode::Rectangle));
        roiSelectionGroup->addButton(btnCircleSelection, static_cast<int>(ROISelectionMode::Circle));
        roiSelectionGroup->addButton(btnArbitrarySelection, static_cast<int>(ROISelectionMode::Arbitrary));
        
        // 添加按钮到布局
        hToolButtons->addWidget(btnRectangleSelection);
        hToolButtons->addWidget(btnCircleSelection);
        hToolButtons->addWidget(btnArbitrarySelection);
        hToolButtons->addWidget(btnClearSelection);
        hToolButtons->addStretch();
        
        // 应用ROI按钮
        btnApplyROI = new QPushButton(tr("应用ROI"));
        btnApplyROI->setEnabled(false); // 初始禁用
        
        // 添加工具按钮布局和应用按钮到ROI分组框
        vROI->addLayout(hToolButtons);
        vROI->addWidget(btnApplyROI);
        
        // 连接信号 - 修复QButtonGroup::buttonClicked连接
        connect(roiSelectionGroup, &QButtonGroup::idClicked, 
                this, &ProcessingWidget::onROISelectionModeChanged);
        connect(btnClearSelection, &QToolButton::clicked, this, &ProcessingWidget::clearROISelection);
        
        // 连接应用ROI按钮信号
        connect(btnApplyROI, &QPushButton::clicked, this, [this]() {
            switch (m_currentROIMode) {
                case ROISelectionMode::Rectangle:
                    if (!m_rectangleROI.isNull()) {
                        emit roiSelected(m_imageRectangleROI);
                    }
                    break;
                case ROISelectionMode::Circle:
                    // 处理不同的圆形ROI状态
                    if (m_multiCircleState == MultiCircleState::RingROI) {
                        // 如果是环形ROI，发送环形ROI信号
                        emit ringROISelected(m_imageCircleCenter, m_imageCircleRadius,
                                           m_imageSecondCircleCenter, m_imageSecondCircleRadius);
                    }
                    else if (m_multiCircleState == MultiCircleState::SecondCircle) {
                        // 如果已选择第二个圆但尚未计算环形，计算并发送环形ROI
                        calculateRingROI();
                        emit ringROISelected(m_imageCircleCenter, m_imageCircleRadius,
                                           m_imageSecondCircleCenter, m_imageSecondCircleRadius);
                    }
                    else if (m_multiCircleState == MultiCircleState::FirstCircle) {
                        // 如果只有第一个圆，提示用户选择第二个圆
                        qDebug() << "已选择第一个圆，请继续选择第二个圆以创建环形ROI";
                        // 也可以显示一个消息框提示用户
                    }
                    else if (m_circleRadius > 0) {
                        // 兼容单圆模式
                        emit roiSelected(m_imageCircleCenter, m_imageCircleRadius);
                    }
                    break;
                case ROISelectionMode::Arbitrary:
                    if (m_arbitraryPoints.size() > 2) {
                        emit roiSelected(QPolygon(m_imageArbitraryROI));
                    }
                    break;
                default:
                    break;
            }
        });
        
        // 设置说明文本
        QLabel *lblROIInstructions = new QLabel(tr("选择ROI模式后，在图像上拖动鼠标进行选择"));
        lblROIInstructions->setWordWrap(true);
        lblROIInstructions->setStyleSheet("QLabel { color: #666; font-size: 9pt; }");
        vROI->addWidget(lblROIInstructions);
        
        qDebug() << "ROI选择控件创建完成";
        
    } catch (const std::exception& e) {
        qDebug() << "Error in setupROISelectionControls:" << e.what();
    }
}

void ProcessingWidget::displayImage(const QImage &image)
{
    try {
        if (image.isNull()) {
            qDebug() << "Warning: Attempted to display null image";
            return;
        }

        if (!imageLabel) {
            qDebug() << "Error: imageLabel is null";
            return;
        }

        m_currentImage = image;

        QPixmap pixmap = QPixmap::fromImage(image);
        if (pixmap.isNull()) {
            qDebug() << "Error: Failed to create pixmap from image";
            return;
        }

        qDebug() << "Scaling pixmap to label size:" << imageLabel->size();
        pixmap = pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        if (pixmap.isNull()) {
            qDebug() << "Error: Failed to scale pixmap";
            return;
        }

        imageLabel->setPixmap(pixmap);
        qDebug() << "Image display updated successfully";

        // 触发图像统计信息更新
        emit imageStatsUpdated(calculateMeanValue(image));
    } catch (const std::exception& e) {
        qDebug() << "Error displaying image:" << e.what();
    }
}

// 添加一个辅助函数来计算图像的均值
double ProcessingWidget::calculateMeanValue(const QImage &image)
{
    try {
        if (image.isNull()) {
            return 0.0;
        }

        double sum = 0.0;
        int count = 0;

        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                QColor pixel = image.pixelColor(x, y);
                int gray = qGray(pixel.rgb());
                sum += gray;
                count++;
            }
        }

        return count > 0 ? sum / count : 0.0;
    } catch (const std::exception& e) {
        qDebug() << "Error calculating mean value:" << e.what();
        return 0.0;
    }
}

void ProcessingWidget::onImageProcessed(const QImage &processedImage)
{
    try {
        if (processedImage.isNull()) {
            qDebug() << "Warning: Received null processed image";
            return;
        }

        if (!imageLabel) {
            qDebug() << "Error: imageLabel is null";
            return;
        }

        qDebug() << "Processing image size:" << processedImage.width() << "x" << processedImage.height();
        m_currentImage = processedImage;  // 保存当前图像

        QPixmap pixmap = QPixmap::fromImage(processedImage);
        if (pixmap.isNull()) {
            qDebug() << "Error: Failed to create pixmap from image";
            return;
        }

        qDebug() << "Scaling pixmap to label size:" << imageLabel->size();
        pixmap = pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        if (pixmap.isNull()) {
            qDebug() << "Error: Failed to scale pixmap";
            return;
        }

        imageLabel->setPixmap(pixmap);
        qDebug() << "Image display updated successfully";
    } catch (const std::exception& e) {
        qDebug() << "Error in onImageProcessed:" << e.what();
    }
}

void ProcessingWidget::onImageStatsUpdated(double meanValue)
{
    try {
        emit imageStatsUpdated(meanValue);
    } catch (const std::exception& e) {
        qDebug() << "Error in onImageStatsUpdated:" << e.what();
    }
}

void ProcessingWidget::onROISelectionModeChanged(int id)
{
    m_currentROIMode = static_cast<ROISelectionMode>(id);
    m_selectionInProgress = false;
    m_arbitraryPoints.clear();
    
    // 设置鼠标形状
    switch (m_currentROIMode) {
        case ROISelectionMode::Rectangle:
            setCursor(Qt::CrossCursor);
            break;
        case ROISelectionMode::Circle:
            setCursor(Qt::CrossCursor);
            break;
        case ROISelectionMode::Arbitrary:
            setCursor(Qt::PointingHandCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
            break;
    }
    
    // 清除之前的ROI选择
    if (id != -1) {  // 如果是有效的模式切换（不是仅取消选中）
        m_rectangleROI = QRect();
        m_circleCenter = QPoint();
        m_circleRadius = 0;
        m_arbitraryROI = QPolygon();
        m_imageRectangleROI = QRect();
        m_imageCircleCenter = QPoint();
        m_imageCircleRadius = 0;
        m_imageArbitraryROI = QPolygon();
        
        // 禁用应用按钮
        if (btnApplyROI) {
            btnApplyROI->setEnabled(false);
        }
    }
    
    // 更新覆盖层
    updateROIDisplay();
}

void ProcessingWidget::clearROISelection()
{
    // 清除所有选择
    m_currentROIMode = ROISelectionMode::None;
    m_selectionInProgress = false;
    m_arbitraryPoints.clear();
    m_rectangleROI = QRect();
    m_circleCenter = QPoint();
    m_circleRadius = 0;
    m_arbitraryROI = QPolygon();
    
    // 清除图像坐标的ROI
    m_imageRectangleROI = QRect();
    m_imageCircleCenter = QPoint();
    m_imageCircleRadius = 0;
    m_imageArbitraryROI = QPolygon();
    
    // 新增：清除多圆ROI相关数据
    m_multiCircleState = MultiCircleState::None;
    m_secondCircleCenter = QPoint();
    m_secondCircleRadius = 0;
    m_imageSecondCircleCenter = QPoint();
    m_imageSecondCircleRadius = 0;
    
    // 取消任何按钮的选中状态
    if (roiSelectionGroup) {
        QAbstractButton *checkedButton = roiSelectionGroup->checkedButton();
        if (checkedButton) {
            checkedButton->setChecked(false);
        }
    }
    
    // 禁用应用按钮
    if (btnApplyROI) {
        btnApplyROI->setEnabled(false);
    }
    
    // 重置鼠标形状
    setCursor(Qt::ArrowCursor);
    
    // 更新覆盖层
    updateROIDisplay();
}

void ProcessingWidget::mousePressEvent(QMouseEvent *event)
{
    try {
        // 如果没有图像，不处理
        if (m_currentImage.isNull() || !imageLabel) {
            return;
        }
        
        // 获取图像在标签中的实际显示区域
        QRect actualImageRect = getScaledImageRect();
        
        // 获取鼠标位置相对于imageLabel的坐标
        QPoint labelPos = imageLabel->mapFrom(this, event->position().toPoint());
        
        // 检查鼠标是否在imageLabel区域内
        if (!imageLabel->rect().contains(labelPos)) {
            return;
        }
        
        // 检查点击是否在实际图像区域内
        if (!actualImageRect.contains(labelPos)) {
            qDebug() << "鼠标点击在图像显示区域外，忽略";
            return;
        }
        
        // 后续处理逻辑
        if (m_currentROIMode == ROISelectionMode::None) {
            // 常规模式 - 显示像素信息
            if (event->button() == Qt::LeftButton) {
                // 将UI坐标映射到图像坐标
                QPoint imagePos = mapToImageCoordinates(labelPos);

                // 检查坐标是否有效
                if (imagePos.x() >= 0 && imagePos.y() >= 0) {
                    // 获取像素信息
                    QColor pixelColor = m_currentImage.pixelColor(imagePos);
                    int r = pixelColor.red();
                    int g = pixelColor.green();
                    int b = pixelColor.blue();
                    int grayValue = qGray(r, g, b);

                    // 发送信号
                    emit mouseClicked(imagePos, grayValue, r, g, b);
                }
            }
        } else {
            // ROI选择模式
            if (event->button() == Qt::LeftButton) {
                // 记录起始点(相对于imageLabel的坐标)
                m_selectionStart = labelPos;
                m_selectionCurrent = labelPos;
                m_selectionInProgress = true;
                
                switch (m_currentROIMode) {
                    case ROISelectionMode::Rectangle:
                        // 初始化矩形选择区域
                        m_rectangleROI = QRect(m_selectionStart, m_selectionStart);
                        break;
                        
                    case ROISelectionMode::Circle:
                        // 初始化圆形选择区域 - 根据当前多圆状态决定是处理第一个圆还是第二个圆
                        if (m_multiCircleState == MultiCircleState::FirstCircle) {
                            qDebug() << "开始选择第二个圆";
                            // 当开始选择第二个圆时，不要使用m_circleCenter，而是直接使用m_secondCircleCenter
                            m_secondCircleCenter = m_selectionStart;
                            m_secondCircleRadius = 0;
                            m_imageSecondCircleCenter = mapToImageCoordinates(m_selectionStart);
                            m_imageSecondCircleRadius = 0;
                        } else {
                            qDebug() << "开始选择第一个圆";
                            // 第一次选择圆或重新开始选择
                            m_circleCenter = m_selectionStart;
                            m_circleRadius = 0;
                            m_imageCircleCenter = mapToImageCoordinates(m_selectionStart);
                            m_imageCircleRadius = 0;
                        }
                        break;
                        
                    case ROISelectionMode::Arbitrary:
                        if (!m_selectionInProgress) {
                            // 开始新的任意形状选择
                            m_arbitraryPoints.clear();
                        }
                        m_arbitraryPoints.append(m_selectionStart);
                        break;
                        
                    default:
                        break;
                }
                
                // 确保立即更新显示
                updateROIDisplay();
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "Error in mousePressEvent:" << e.what();
    }
}

void ProcessingWidget::wheelEvent(QWheelEvent *event)
{
    if (m_currentImage.isNull() || !imageLabel) {
        return;
    }
    
    // 保存缩放前的鼠标位置对应的图像坐标
    QPoint labelPos = imageLabel->mapFrom(this, event->position().toPoint());
    QPoint imagePos = mapToImageCoordinates(labelPos);
    
    // 应用缩放
    if (event->angleDelta().y() > 0) {
        // 放大
        m_zoomFactor = qMin(m_zoomFactor + ZOOM_FACTOR_STEP, MAX_ZOOM);
    } else {
        // 缩小
        m_zoomFactor = qMax(m_zoomFactor - ZOOM_FACTOR_STEP, MIN_ZOOM);
    }
    
    // 更新图像显示
    if (!m_currentImage.isNull()) {
        QImage scaledImage = m_currentImage.scaled(
            qRound(m_currentImage.width() * m_zoomFactor),
            qRound(m_currentImage.height() * m_zoomFactor),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        imageLabel->setPixmap(QPixmap::fromImage(scaledImage));
        
        // 更新ROI显示层的大小和位置
        updateROIDisplay();
    }
    
    // 记录缩放信息
    qDebug() << "图像缩放: " << m_zoomFactor;
}

// 更新k值标签
void ProcessingWidget::updateKValueLabel(int value)
{
    double k = 1.0;
    if (value >= 0) {
        k = 1.0 + value / 100.0;  // k范围是1.0到2.0
    } else {
        k = 1.0 + value / 100.0;  // k范围是0.0到1.0
    }
    lblKValue->setText(QString("k = %1").arg(k, 0, 'f', 2));
}

// 更新b值标签
void ProcessingWidget::updateBValueLabel(int value)
{
    lblBValue->setText(QString("b = %1").arg(value));
}

// 更新Gamma值标签
void ProcessingWidget::updateGammaValueLabel(int value)
{
    double gamma = value / 10.0;
    lblGammaValue->setText(QString("γ = %1").arg(gamma, 0, 'f', 2));
}

// 重置标签显示
void ProcessingWidget::resetValueLabels()
{
    // 重置k值标签为默认值
    lblKValue->setText("k = 1.00");

    // 重置b值标签为默认值
    lblBValue->setText("b = 0");

    // 重置Gamma值标签为默认值
    lblGammaValue->setText("γ = 1.00");
}

void ProcessingWidget::mouseMoveEvent(QMouseEvent *event)
{
    try {
        // 如果没有图像，不处理
        if (m_currentImage.isNull() || !imageLabel) {
            return;
        }
        
        // 获取图像在标签中的实际显示区域
        QRect actualImageRect = getScaledImageRect();
        
        // 获取鼠标位置相对于imageLabel的坐标
        QPoint labelPos = imageLabel->mapFrom(this, event->position().toPoint());
        
        // 检查鼠标是否在imageLabel区域内
        if (!imageLabel->rect().contains(labelPos)) {
            return;
        }
        
        // 非ROI选择模式 - 显示像素信息
        if (m_currentROIMode == ROISelectionMode::None || !m_selectionInProgress) {
            // 检查鼠标是否在实际图像区域内
            if (actualImageRect.contains(labelPos)) {
                // 将UI坐标映射到图像坐标
                QPoint imagePos = mapToImageCoordinates(labelPos);
                
                // 检查坐标是否有效
                if (imagePos.x() >= 0 && imagePos.y() >= 0 && 
                    imagePos.x() < m_currentImage.width() && 
                    imagePos.y() < m_currentImage.height()) {
                    // 获取像素灰度值
                    QColor pixelColor = m_currentImage.pixelColor(imagePos);
                    int grayValue = qGray(pixelColor.red(), pixelColor.green(), pixelColor.blue());

                    // 发送坐标和灰度值
                    emit mouseMoved(imagePos, grayValue);
                }
            }
            return;
        }

        // ROI选择模式下的处理
        if (m_selectionInProgress) {
            // 限制鼠标位置在实际图像区域内
            QPoint constrainedPos;
            constrainedPos.setX(qBound(actualImageRect.left(), labelPos.x(), actualImageRect.right()));
            constrainedPos.setY(qBound(actualImageRect.top(), labelPos.y(), actualImageRect.bottom()));
            
            m_selectionCurrent = constrainedPos;
            
            // 更新ROI数据
            if (m_currentROIMode == ROISelectionMode::Rectangle) {
                // 更新矩形选择 (UI坐标)
                m_rectangleROI = QRect(m_selectionStart, m_selectionCurrent).normalized();
                
                // 打印调试信息
                qDebug() << "---------- 鼠标移动更新ROI ----------";
                qDebug() << "选择起点:" << m_selectionStart;
                qDebug() << "选择当前点:" << m_selectionCurrent;
                qDebug() << "实际图像区域:" << actualImageRect;
                
                // 更新对应的图像坐标 - 直接转换矩形的左上角和右下角点
                QPoint topLeft = mapToImageCoordinates(m_rectangleROI.topLeft());
                QPoint bottomRight = mapToImageCoordinates(m_rectangleROI.bottomRight());
                
                // 确保有效的图像坐标
                if (topLeft.x() >= 0 && topLeft.y() >= 0 && 
                    bottomRight.x() >= 0 && bottomRight.y() >= 0) {
                    m_imageRectangleROI = QRect(topLeft, bottomRight);
                    
                    // 打印最终结果
                    qDebug() << "图像宽高:" << m_currentImage.width() << "x" << m_currentImage.height();
                    qDebug() << "矩形ROI(UI):" << m_rectangleROI;
                    qDebug() << "矩形ROI(图像):" << m_imageRectangleROI;
                }
                qDebug() << "-------------------------------";
            } else if (m_currentROIMode == ROISelectionMode::Circle) {
                // 根据当前状态决定更新第一个圆还是第二个圆
                if (m_multiCircleState == MultiCircleState::FirstCircle) {
                    // 更新第二个圆
                    int dx = m_selectionCurrent.x() - m_selectionStart.x();
                    int dy = m_selectionCurrent.y() - m_selectionStart.y();
                    m_secondCircleRadius = static_cast<int>(sqrt(dx*dx + dy*dy));
                    
                    // 更新图像坐标
                    QPoint edgePoint(
                        m_secondCircleCenter.x() + qRound(m_secondCircleRadius * cos(0)),
                        m_secondCircleCenter.y() + qRound(m_secondCircleRadius * sin(0))
                    );
                    
                    if (actualImageRect.contains(edgePoint)) {
                        QPoint imageEdgePoint = mapToImageCoordinates(edgePoint);
                        
                        if (imageEdgePoint.x() >= 0 && imageEdgePoint.y() >= 0) {
                            m_imageSecondCircleRadius = static_cast<int>(
                                sqrt(pow(imageEdgePoint.x() - m_imageSecondCircleCenter.x(), 2) + 
                                     pow(imageEdgePoint.y() - m_imageSecondCircleCenter.y(), 2))
                            );
                            
                            qDebug() << "第二个圆 - 中心(UI): " << m_secondCircleCenter 
                                     << " 半径: " << m_secondCircleRadius;
                            qDebug() << "第二个圆 - 中心(图像): " << m_imageSecondCircleCenter 
                                     << " 半径: " << m_imageSecondCircleRadius;
                        }
                    }
                } else {
                    // 更新第一个圆
                    int dx = m_selectionCurrent.x() - m_selectionStart.x();
                    int dy = m_selectionCurrent.y() - m_selectionStart.y();
                    m_circleRadius = static_cast<int>(sqrt(dx*dx + dy*dy));
                    
                    // 更新对应的图像坐标
                    m_imageCircleCenter = mapToImageCoordinates(m_circleCenter);
                    
                    // 计算图像坐标中的半径
                    if (m_imageCircleCenter.x() >= 0 && m_imageCircleCenter.y() >= 0) {
                        // 取圆上的水平右侧点
                        double angle = 0;
                        QPoint edgePoint(
                            m_circleCenter.x() + qRound(m_circleRadius * cos(angle)),
                            m_circleCenter.y() + qRound(m_circleRadius * sin(angle))
                        );
                        
                        // 确保边缘点在图像区域内
                        if (actualImageRect.contains(edgePoint)) {
                            QPoint imageEdgePoint = mapToImageCoordinates(edgePoint);
                            
                            // 计算图像坐标中的半径（两点间距离）
                            if (imageEdgePoint.x() >= 0 && imageEdgePoint.y() >= 0) {
                                m_imageCircleRadius = static_cast<int>(
                                    sqrt(pow(imageEdgePoint.x() - m_imageCircleCenter.x(), 2) + 
                                        pow(imageEdgePoint.y() - m_imageCircleCenter.y(), 2))
                                );
                                
                                qDebug() << "第一个圆 - 中心(UI): " << m_circleCenter << " 半径: " << m_circleRadius;
                                qDebug() << "第一个圆 - 中心(图像): " << m_imageCircleCenter << " 半径: " << m_imageCircleRadius;
                            }
                        }
                    }
                }
            }
            
            // 更新ROI覆盖层
            updateROIDisplay();
        }
    } catch (const std::exception& e) {
        qDebug() << "Error in mouseMoveEvent:" << e.what();
    }
}

void ProcessingWidget::mouseReleaseEvent(QMouseEvent *event)
{
    try {
        if (event->button() == Qt::LeftButton && m_selectionInProgress) {
            switch (m_currentROIMode) {
                case ROISelectionMode::Rectangle:
                    // 完成矩形选择
                    m_selectionInProgress = false;
                    
                    // 确保至少有一定大小的矩形
                    if (m_rectangleROI.width() > 5 && m_rectangleROI.height() > 5) {
                        // 确保直接使用已计算好的m_imageRectangleROI
                        if (!m_imageRectangleROI.isNull()) {
                            // 发送ROI选择信号 (传递图像坐标)
                            emit roiSelected(m_imageRectangleROI);
                            
                            // 启用应用按钮
                            if (btnApplyROI) {
                                btnApplyROI->setEnabled(true);
                            }
                            
                            // 打印调试信息
                            qDebug() << "矩形ROI选择完成";
                            qDebug() << "UI坐标: " << m_rectangleROI;
                            qDebug() << "图像坐标: " << m_imageRectangleROI;
                        }
                    }
                    break;
                    
                case ROISelectionMode::Circle:
                    // 完成圆形选择
                    m_selectionInProgress = false;
                    
                    // 确保至少有一定半径的圆
                    if (m_circleRadius > 5) {
                        // 确保m_imageCircleCenter和m_imageCircleRadius有效
                        if (m_imageCircleCenter.x() >= 0 && m_imageCircleRadius > 0) {
                            
                            // 根据多圆选择状态处理
                            if (m_multiCircleState == MultiCircleState::None) {
                                // 第一个圆选择完成，保存第一个圆的信息
                                QPoint firstCircleCenter = m_circleCenter;
                                int firstCircleRadius = m_circleRadius;
                                QPoint firstImageCircleCenter = m_imageCircleCenter;
                                int firstImageCircleRadius = m_imageCircleRadius;
                                
                                // 转换到多圆状态
                                m_multiCircleState = MultiCircleState::FirstCircle;
                                
                                qDebug() << "第一个圆形ROI选择完成";
                                qDebug() << "UI中心: " << firstCircleCenter << " 半径: " << firstCircleRadius;
                                qDebug() << "图像中心: " << firstImageCircleCenter << " 半径: " << firstImageCircleRadius;
                                
                                // 需要立即进入选择第二个圆的状态，不要发送信号
                                // 提示用户继续选择第二个圆
                                QMessageBox::information(this, tr("第一个圆选择完成"), 
                                                       tr("请继续选择第二个圆以创建环形ROI"));
                                
                                // 重置选择状态，但保持ROI模式，以便用户可以继续选择第二个圆
                                m_selectionInProgress = false;
                                
                                // 不启用应用按钮，直到选择完第二个圆
                                if (btnApplyROI) {
                                    btnApplyROI->setEnabled(false);
                                }
                            } 
                            else if (m_multiCircleState == MultiCircleState::FirstCircle) {
                                // 第二个圆选择完成，已经在鼠标移动事件中更新了m_secondCircleCenter等变量
                                // 打印调试信息用于诊断问题
                                qDebug() << "检查两个圆是否相同:";
                                qDebug() << "第一个圆 - 中心:" << m_imageCircleCenter << "半径:" << m_imageCircleRadius;
                                qDebug() << "第二个圆 - 中心:" << m_imageSecondCircleCenter << "半径:" << m_imageSecondCircleRadius;
                                
                                // 判断圆心是否相同需要更精确的比较
                                double centerDistance = sqrt(
                                    pow(m_imageSecondCircleCenter.x() - m_imageCircleCenter.x(), 2) +
                                    pow(m_imageSecondCircleCenter.y() - m_imageCircleCenter.y(), 2)
                                );
                                
                                // 只有当圆心距离接近0且半径几乎相同时才认为是相同的圆
                                if (centerDistance < 1.0 && abs(m_imageSecondCircleRadius - m_imageCircleRadius) < 1) {
                                    // 两个圆完全相同，提示用户重新选择
                                    QMessageBox::warning(this, tr("无效的选择"), 
                                                       tr("两个圆的中心和半径完全相同，请重新选择第二个圆"),
                                                       QMessageBox::Ok);
                                    
                                    // 重置第二个圆的选择，保持在FirstCircle状态
                                    m_selectionInProgress = false;
                                    // 清除第二个圆的数据
                                    m_secondCircleCenter = QPoint();
                                    m_secondCircleRadius = 0;
                                    m_imageSecondCircleCenter = QPoint();
                                    m_imageSecondCircleRadius = 0;
                                    return;
                                }
                                
                                // 第二个圆选择完成，更新状态
                                m_multiCircleState = MultiCircleState::SecondCircle;
                                
                                qDebug() << "第二个圆形ROI选择完成";
                                qDebug() << "UI中心: " << m_secondCircleCenter << " 半径: " << m_secondCircleRadius;
                                qDebug() << "图像中心: " << m_imageSecondCircleCenter << " 半径: " << m_imageSecondCircleRadius;
                                
                                // 计算环形ROI
                                calculateRingROI();
                                
                                // 获取环形ROI中的像素
                                QVector<int> pixels = getRingROIPixelValues();
                                
                                // 检查是否有有效像素
                                if (pixels.isEmpty()) {
                                    // 环形区域中没有有效像素
                                    QMessageBox::warning(this, tr("无效的环形区域"), 
                                                       tr("环形区域中没有找到有效像素，请重新选择两个圆"),
                                                       QMessageBox::Ok);
                                    
                                    // 重置到第一个圆的状态，让用户重新选择第二个圆
                                    m_multiCircleState = MultiCircleState::FirstCircle;
                                    m_selectionInProgress = false;
                                    // 清除第二个圆的数据
                                    m_secondCircleCenter = QPoint();
                                    m_secondCircleRadius = 0;
                                    m_imageSecondCircleCenter = QPoint();
                                    m_imageSecondCircleRadius = 0;
                                    return;
                                }
                                
                                // 两个圆都选择完成，发送环形ROI选择完成信号
                                emit ringROISelected(m_imageCircleCenter, m_imageCircleRadius,
                                                  m_imageSecondCircleCenter, m_imageSecondCircleRadius);
                                
                                // 此时才启用应用按钮
                                if (btnApplyROI) {
                                    btnApplyROI->setEnabled(true);
                                }
                                
                                // 提示用户环形ROI已创建完成
                                QMessageBox::information(this, tr("环形ROI创建完成"), 
                                                       tr("环形ROI创建完成，包含 %1 个像素点，您可以点击'应用ROI'按钮进行操作")
                                                       .arg(pixels.size()));
                            }
                        }
                    }
                    break;
                    
                case ROISelectionMode::Arbitrary:
                    // 对于任意形状，此处不做更改
                    break;
                    
                default:
                    break;
            }
            
            // 更新ROI覆盖层
            updateROIDisplay();
        } else if (event->button() == Qt::RightButton && m_selectionInProgress && 
                  m_currentROIMode == ROISelectionMode::Arbitrary && m_arbitraryPoints.size() > 2) {
            // 右键单击完成任意形状选择
            m_selectionInProgress = false;
            m_arbitraryROI = QPolygon(m_arbitraryPoints);
            
            // 转换为图像坐标
            QPolygon imagePolygon;
            for (const QPoint& pt : m_arbitraryPoints) {
                imagePolygon << mapToImageCoordinates(pt);
            }
            
            // 过滤掉无效坐标
            QPolygon validImagePolygon;
            for (const QPoint& pt : imagePolygon) {
                if (pt.x() >= 0 && pt.y() >= 0) {
                    validImagePolygon << pt;
                }
            }
            
            if (validImagePolygon.size() > 2) {
                m_imageArbitraryROI = validImagePolygon;
                
                // 发送ROI选择信号
                emit roiSelected(m_imageArbitraryROI);
                
                // 启用应用按钮
                if (btnApplyROI) {
                    btnApplyROI->setEnabled(true);
                }
                
                // 打印调试信息
                qDebug() << "任意形状ROI选择完成，包含 " << m_imageArbitraryROI.size() << " 个点";
            }
            
            // 更新ROI覆盖层
            updateROIDisplay();
        }
    } catch (const std::exception& e) {
        qDebug() << "Error in mouseReleaseEvent:" << e.what();
    }
}

void ProcessingWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    
    // 如果没有图像，不绘制ROI
    if (m_currentImage.isNull() || !imageLabel || !imageLabel->pixmap()) {
        return;
    }
    
    // 无论是否在ROI模式，只要有ROI数据就绘制它们
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 设置绘制参数 - 使用更明显的颜色和样式
    QPen pen;
    
    if (m_selectionInProgress) {
        // 选择进行中时使用更醒目的颜色
        pen.setColor(QColor(255, 102, 0)); // 亮橙色
        pen.setWidth(2);
        pen.setStyle(Qt::DashLine);
    } else {
        // 选择完成时使用绿色
        pen.setColor(QColor(0, 180, 0)); // 深绿色
        pen.setWidth(2);
        pen.setStyle(Qt::SolidLine);
    }
    
    painter.setPen(pen);
    
    // 半透明填充
    QBrush brush(QColor(128, 218, 235, 60)); // 半透明浅蓝色
    painter.setBrush(brush);
    
    // 绘制ROI，去掉当前模式检查，任何有效的ROI都会被绘制
    if (!m_rectangleROI.isNull()) {
        painter.drawRect(m_rectangleROI);
        
        // 在矩形四角绘制小方块手柄
        int handleSize = 6;
        QColor handleColor(255, 255, 255); // 白色手柄
        QPen handlePen(QColor(0, 0, 0)); // 黑色边框
        handlePen.setWidth(1);
        painter.setPen(handlePen);
        painter.setBrush(QBrush(handleColor));
        
        // 左上
        painter.drawRect(QRect(m_rectangleROI.left() - handleSize/2, 
                             m_rectangleROI.top() - handleSize/2, 
                             handleSize, handleSize));
        // 右上
        painter.drawRect(QRect(m_rectangleROI.right() - handleSize/2, 
                             m_rectangleROI.top() - handleSize/2, 
                             handleSize, handleSize));
        // 左下
        painter.drawRect(QRect(m_rectangleROI.left() - handleSize/2, 
                             m_rectangleROI.bottom() - handleSize/2, 
                             handleSize, handleSize));
        // 右下
        painter.drawRect(QRect(m_rectangleROI.right() - handleSize/2, 
                             m_rectangleROI.bottom() - handleSize/2, 
                             handleSize, handleSize));
    }
    
    if (m_circleRadius > 0) {
        // 恢复ROI的绘制颜色和样式
        painter.setPen(pen);
        painter.setBrush(brush);
        
        // 绘制圆形
        painter.drawEllipse(m_circleCenter, m_circleRadius, m_circleRadius);
        
        // 在圆心绘制十字线
        QPen centerPen(QColor(255, 255, 255)); // 白色中心点
        centerPen.setWidth(1);
        painter.setPen(centerPen);
        int crossSize = 6;
        painter.drawLine(m_circleCenter.x() - crossSize, m_circleCenter.y(),
                       m_circleCenter.x() + crossSize, m_circleCenter.y());
        painter.drawLine(m_circleCenter.x(), m_circleCenter.y() - crossSize,
                       m_circleCenter.x(), m_circleCenter.y() + crossSize);
    }
    
    if (m_arbitraryPoints.size() > 1) {
        // 设置多边形边线样式
        QPen polyPen(QColor(255, 165, 0)); // 橙色
        polyPen.setWidth(2);
        painter.setPen(polyPen);
        
        // 绘制已经点击的点之间的线
        for (int i = 0; i < m_arbitraryPoints.size() - 1; ++i) {
            painter.drawLine(m_arbitraryPoints[i], m_arbitraryPoints[i+1]);
        }
        
        // 如果选择尚未完成，绘制临时线段
        if (m_selectionInProgress && !m_arbitraryPoints.isEmpty()) {
            painter.drawLine(m_arbitraryPoints.last(), m_arbitraryPoints.first());
        }
        
        // 绘制点
        QPen pointPen(QColor(0, 0, 255)); // 蓝色点
        pointPen.setWidth(2);
        painter.setPen(pointPen);
        
        // 绘制控制点
        for (const QPoint &pt : m_arbitraryPoints) {
            painter.setBrush(QBrush(QColor(255, 255, 255))); // 白色填充
            painter.drawEllipse(pt, 4, 4);
        }
    }
}

// 修正 getScaledImageRect 方法，确保精确计算图像显示区域
QRect ProcessingWidget::getScaledImageRect()
{
    if (m_currentImage.isNull() || !imageLabel) {
        return QRect();
    }
    
    // 获取图像和标签尺寸
    QSize labelSize = imageLabel->size();
    QSize imageSize = m_currentImage.size();
    
    // 处理缩放系数
    imageSize = QSize(
        qRound(imageSize.width() * m_zoomFactor),
        qRound(imageSize.height() * m_zoomFactor)
    );
    
    // 计算缩放后的图像尺寸（保持纵横比）
    QSize scaledSize;
    double aspectRatio = (double)imageSize.width() / imageSize.height();
    
    if (labelSize.width() / aspectRatio <= labelSize.height()) {
        // 宽度限制
        scaledSize.setWidth(labelSize.width());
        scaledSize.setHeight(qRound(labelSize.width() / aspectRatio));
    } else {
        // 高度限制
        scaledSize.setHeight(labelSize.height());
        scaledSize.setWidth(qRound(labelSize.height() * aspectRatio));
    }
    
    // 计算图像在标签中的位置（居中显示）
    int offsetX = (labelSize.width() - scaledSize.width()) / 2;
    int offsetY = (labelSize.height() - scaledSize.height()) / 2;
    
    // 确保偏移量不为负
    offsetX = qMax(0, offsetX);
    offsetY = qMax(0, offsetY);
    
    qDebug() << "图像显示区域计算:";
    qDebug() << "  标签尺寸: " << labelSize;
    qDebug() << "  原始图像尺寸: " << m_currentImage.size();
    qDebug() << "  缩放后图像尺寸: " << imageSize;
    qDebug() << "  实际显示尺寸: " << scaledSize;
    qDebug() << "  偏移: " << QPoint(offsetX, offsetY);
    
    return QRect(offsetX, offsetY, scaledSize.width(), scaledSize.height());
}

// 重新实现 mapToImageCoordinates 方法，更精确地处理坐标转换
QPoint ProcessingWidget::mapToImageCoordinates(const QPoint& labelPos)
{
    if (m_currentImage.isNull() || !imageLabel) {
        return QPoint(-1, -1);
    }

    // 获取图像在标签中的实际显示区域
    QRect displayRect = getScaledImageRect();
    
    // 检查点是否在显示区域内
    if (!displayRect.contains(labelPos)) {
        return QPoint(-1, -1);
    }
    
    // 计算点在显示区域内的相对位置（比例）
    double relativeX = static_cast<double>(labelPos.x() - displayRect.left()) / displayRect.width();
    double relativeY = static_cast<double>(labelPos.y() - displayRect.top()) / displayRect.height();
    
    // 将相对位置转换为图像坐标
    int imageX = qRound(relativeX * m_currentImage.width());
    int imageY = qRound(relativeY * m_currentImage.height());
    
    // 确保坐标在图像范围内
    imageX = qBound(0, imageX, m_currentImage.width() - 1);
    imageY = qBound(0, imageY, m_currentImage.height() - 1);
    
    // 输出调试信息
    qDebug() << "坐标转换:";
    qDebug() << "  Label位置: " << labelPos;
    qDebug() << "  显示区域: " << displayRect;
    qDebug() << "  相对位置: " << QPointF(relativeX, relativeY);
    qDebug() << "  图像坐标: " << QPoint(imageX, imageY);
    
    return QPoint(imageX, imageY);
}

// 重新实现 mapFromImageCoordinates 方法，确保与 mapToImageCoordinates 保持一致
QPoint ProcessingWidget::mapFromImageCoordinates(const QPoint& imagePos)
{
    if (m_currentImage.isNull() || !imageLabel || 
        imagePos.x() < 0 || imagePos.y() < 0 || 
        imagePos.x() >= m_currentImage.width() || 
        imagePos.y() >= m_currentImage.height()) {
        return QPoint(-1, -1);
    }
    
    // 获取图像在标签中的实际显示区域
    QRect displayRect = getScaledImageRect();
    
    // 计算图像坐标在图像内的相对位置（比例）
    double relativeX = static_cast<double>(imagePos.x()) / m_currentImage.width();
    double relativeY = static_cast<double>(imagePos.y()) / m_currentImage.height();
    
    // 将相对位置转换为显示区域内的坐标
    int labelX = qRound(displayRect.left() + relativeX * displayRect.width());
    int labelY = qRound(displayRect.top() + relativeY * displayRect.height());
    
    // 确保坐标在标签区域内
    labelX = qBound(displayRect.left(), labelX, displayRect.right());
    labelY = qBound(displayRect.top(), labelY, displayRect.bottom());
    
    return QPoint(labelX, labelY);
}

// 实现logRectInfo函数
// 添加调试日志输出
void logRectInfo(const QString& prefix, const QRect& rect)
{
    qDebug() << prefix << "x:" << rect.x() << "y:" << rect.y() 
             << "width:" << rect.width() << "height:" << rect.height();
}

// 修改mapToImageRect函数，使用更精确的坐标转换
QRect ProcessingWidget::mapToImageRect(const QRect& uiRect)
{
    if (m_currentImage.isNull() || !imageLabel) {
        return QRect();
    }
    
    // 图像显示区域
    QRect actualImageRect = getScaledImageRect();
    
    // 确保UI矩形与图像区域相交
    QRect clippedRect = uiRect.intersected(actualImageRect);
    if (clippedRect.isEmpty()) {
        return QRect();
    }
    
    // 计算相对于图像显示区域的偏移和比例
    double leftRatio = (double)(clippedRect.left() - actualImageRect.left()) / actualImageRect.width();
    double topRatio = (double)(clippedRect.top() - actualImageRect.top()) / actualImageRect.height();
    double rightRatio = (double)(clippedRect.right() - actualImageRect.left()) / actualImageRect.width();
    double bottomRatio = (double)(clippedRect.bottom() - actualImageRect.top()) / actualImageRect.height();
    
    // 转换为图像坐标
    int imgLeft = qRound(leftRatio * m_currentImage.width());
    int imgTop = qRound(topRatio * m_currentImage.height());
    int imgRight = qRound(rightRatio * m_currentImage.width());
    int imgBottom = qRound(bottomRatio * m_currentImage.height());
    
    // 构造图像矩形并确保在图像范围内
    QRect imageRect(
        qBound(0, imgLeft, m_currentImage.width() - 1),
        qBound(0, imgTop, m_currentImage.height() - 1),
        qBound(1, imgRight - imgLeft + 1, m_currentImage.width()),
        qBound(1, imgBottom - imgTop + 1, m_currentImage.height())
    );
    
    // 调试输出
    logRectInfo("UI矩形(原始):", uiRect);
    logRectInfo("UI矩形(裁剪):", clippedRect);
    logRectInfo("图像矩形:", imageRect);
    qDebug() << "比例: " << leftRatio << topRatio << rightRatio << bottomRatio;
    
    return imageRect;
}

QRect ProcessingWidget::mapFromImageRect(const QRect& imageRect)
{
    if (m_currentImage.isNull() || !imageLabel) {
        return QRect();
    }
    
    // 图像显示区域
    QRect actualImageRect = getScaledImageRect();
    
    // 确保图像矩形在图像范围内
    QRect clippedImageRect = imageRect.intersected(QRect(0, 0, m_currentImage.width(), m_currentImage.height()));
    if (clippedImageRect.isEmpty()) {
        return QRect();
    }
    
    // 计算比例
    double leftRatio = (double)clippedImageRect.left() / m_currentImage.width();
    double topRatio = (double)clippedImageRect.top() / m_currentImage.height();
    double rightRatio = (double)clippedImageRect.right() / m_currentImage.width();
    double bottomRatio = (double)clippedImageRect.bottom() / m_currentImage.height();
    
    // 转换为UI坐标
    int uiLeft = qRound(actualImageRect.left() + leftRatio * actualImageRect.width());
    int uiTop = qRound(actualImageRect.top() + topRatio * actualImageRect.height());
    int uiRight = qRound(actualImageRect.left() + rightRatio * actualImageRect.width());
    int uiBottom = qRound(actualImageRect.top() + bottomRatio * actualImageRect.height());
    
    return QRect(uiLeft, uiTop, uiRight - uiLeft + 1, uiBottom - uiTop + 1);
}

// 添加一个方法来强制更新ROI显示
void ProcessingWidget::updateROIDisplay()
{
    if (m_roiOverlay && !m_currentImage.isNull() && imageLabel) {
        // 获取实际图像显示区域
        QRect actualImageRect = getScaledImageRect();
        
        // 确保覆盖层大小与imageLabel一致
        m_roiOverlay->setGeometry(0, 0, imageLabel->width(), imageLabel->height());
        
        // 更新ROI数据 - 在选择第二个圆时，传递正确的信息
        if (m_multiCircleState == MultiCircleState::FirstCircle) {
            // 如果正在选择第二个圆，则传递第一个圆和正在选择的第二个圆的信息
            m_roiOverlay->setROIData(m_rectangleROI, m_circleCenter, m_circleRadius,
                                   m_arbitraryPoints, m_selectionInProgress,
                                   m_imageRectangleROI, 
                                   m_currentImage.width(), m_currentImage.height(),
                                   actualImageRect, m_imageCircleRadius,
                                   m_secondCircleCenter, m_secondCircleRadius,
                                   m_imageSecondCircleRadius, m_multiCircleState);
        } else {
            // 否则，传递当前的ROI信息
            m_roiOverlay->setROIData(m_rectangleROI, m_circleCenter, m_circleRadius,
                                   m_arbitraryPoints, m_selectionInProgress,
                                   m_imageRectangleROI, 
                                   m_currentImage.width(), m_currentImage.height(),
                                   actualImageRect, m_imageCircleRadius,
                                   m_secondCircleCenter, m_secondCircleRadius,
                                   m_imageSecondCircleRadius, m_multiCircleState);
        }
        m_roiOverlay->update();
    }
}

void ProcessingWidget::setROIMode(bool enableROI)
{
    m_isROIMode = enableROI; 
    
    // 如果关闭ROI模式，确保清除任何进行中的选择
    if (!enableROI) {
        m_selectionInProgress = false;
    }
    
    // 立即更新UI
    update();
    QApplication::processEvents();
}

void ProcessingWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_roiOverlay && imageLabel) {
        // 使覆盖层的大小与imageLabel一致
        m_roiOverlay->setGeometry(0, 0, imageLabel->width(), imageLabel->height());
        
        // 更新ROI显示
        updateROIDisplay();
    }
}

// 实现计算环形ROI区域的方法
void ProcessingWidget::calculateRingROI()
{
    // 只有当两个圆都有效时才进行计算
    if (m_imageCircleRadius <= 0 || m_imageSecondCircleRadius <= 0) {
        qDebug() << "环形ROI计算失败：圆半径无效";
        return;
    }
    
    // 设置状态为已完成环形ROI计算
    m_multiCircleState = MultiCircleState::RingROI;
    
    // 对于环形ROI，不再需要固定内外圆的概念
    // 我们允许两个圆之间有任意位置关系，只要能构成有效区域
    
    qDebug() << "环形ROI计算完成:";
    qDebug() << "第一个圆 - 中心:" << m_imageCircleCenter << "半径:" << m_imageCircleRadius;
    qDebug() << "第二个圆 - 中心:" << m_imageSecondCircleCenter << "半径:" << m_imageSecondCircleRadius;
    
    // 获取环形ROI中的像素值进行测试
    QVector<int> pixelValues = getRingROIPixelValues();
    qDebug() << "环形ROI包含" << pixelValues.size() << "个像素点";
    
    // 如果像素数量不太多，可以输出一些示例像素值
    if (!pixelValues.isEmpty()) {
        int sampleSize = qMin(10, pixelValues.size());
        QStringList sampleValues;
        for (int i = 0; i < sampleSize; ++i) {
            sampleValues << QString::number(pixelValues[i]);
        }
        qDebug() << "像素值样本:" << sampleValues.join(", ");
    }
}

// 判断点是否在环形区域内 - 不再局限于内圆和外圆的关系
bool ProcessingWidget::isPointInRingROI(const QPoint& point) const
{
    if (m_imageCircleRadius <= 0 || m_imageSecondCircleRadius <= 0) {
        return false;
    }
    
    // 计算点到两个圆心的距离
    double dx1 = point.x() - m_imageCircleCenter.x();
    double dy1 = point.y() - m_imageCircleCenter.y();
    double dist1 = sqrt(dx1*dx1 + dy1*dy1);
    
    double dx2 = point.x() - m_imageSecondCircleCenter.x();
    double dy2 = point.y() - m_imageSecondCircleCenter.y();
    double dist2 = sqrt(dx2*dx2 + dy2*dy2);
    
    // 计算点到圆心的距离与圆半径的关系
    bool inCircle1 = (dist1 <= m_imageCircleRadius);
    bool inCircle2 = (dist2 <= m_imageSecondCircleRadius);
    
    // 环形区域的定义：点在第二个圆内但不在第一个圆内（或相反）
    // 即点只属于其中一个圆，这样定义的环形区域更灵活
    return (inCircle1 && !inCircle2) || (inCircle2 && !inCircle1);
}

// 实现获取环形ROI中像素值的方法
QVector<int> ProcessingWidget::getRingROIPixelValues() const
{
    QVector<int> pixelValues;
    
    // 检查图像和环形ROI是否有效
    if (m_currentImage.isNull() || 
        m_imageCircleRadius <= 0 || 
        m_imageSecondCircleRadius <= 0 ||
        m_multiCircleState != MultiCircleState::RingROI) {
        return pixelValues;
    }
    
    // 确定环形区域的边界盒
    int left = qMin(m_imageCircleCenter.x() - m_imageCircleRadius,
                 m_imageSecondCircleCenter.x() - m_imageSecondCircleRadius);
    int top = qMin(m_imageCircleCenter.y() - m_imageCircleRadius,
                m_imageSecondCircleCenter.y() - m_imageSecondCircleRadius);
    int right = qMax(m_imageCircleCenter.x() + m_imageCircleRadius,
                 m_imageSecondCircleCenter.x() + m_imageSecondCircleRadius);
    int bottom = qMax(m_imageCircleCenter.y() + m_imageCircleRadius,
                 m_imageSecondCircleCenter.y() + m_imageSecondCircleRadius);
    
    // 确保边界盒在图像范围内
    left = qMax(0, left);
    top = qMax(0, top);
    right = qMin(m_currentImage.width() - 1, right);
    bottom = qMin(m_currentImage.height() - 1, bottom);
    
    // 遍历边界盒中的每个像素
    for (int y = top; y <= bottom; ++y) {
        for (int x = left; x <= right; ++x) {
            QPoint pixelPos(x, y);
            
            // 检查像素是否在环形区域内
            if (isPointInRingROI(pixelPos)) {
                // 获取像素灰度值
                QColor pixelColor = m_currentImage.pixelColor(x, y);
                int grayValue = qGray(pixelColor.red(), pixelColor.green(), pixelColor.blue());
                pixelValues.append(grayValue);
            }
        }
    }
    
    return pixelValues;
}


