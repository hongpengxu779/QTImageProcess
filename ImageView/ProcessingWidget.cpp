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
#include <QFileDialog>
#include <QDir>
#include <QImageReader>
#include <QFileInfo>
#include <QImageWriter> // Added for save format checking

// 辅助函数声明
void logRectInfo(const QString& prefix, const QRect& rect);

// 新增：辅助函数，将QImage::Format转换为可读的字符串
QString getQImageFormatName(QImage::Format format) {
    switch (format) {
        case QImage::Format_Invalid: return "Invalid";
        case QImage::Format_Mono: return "Mono";
        case QImage::Format_MonoLSB: return "MonoLSB";
        case QImage::Format_Indexed8: return "Indexed8";
        case QImage::Format_RGB32: return "RGB32";
        case QImage::Format_ARGB32: return "ARGB32";
        case QImage::Format_ARGB32_Premultiplied: return "ARGB32_Premultiplied";
        case QImage::Format_RGB16: return "RGB16";
        case QImage::Format_ARGB8565_Premultiplied: return "ARGB8565_Premultiplied";
        case QImage::Format_RGB666: return "RGB666";
        case QImage::Format_ARGB6666_Premultiplied: return "ARGB6666_Premultiplied";
        case QImage::Format_RGB555: return "RGB555";
        case QImage::Format_ARGB8555_Premultiplied: return "ARGB8555_Premultiplied";
        case QImage::Format_RGB888: return "RGB888";
        case QImage::Format_RGB444: return "RGB444";
        case QImage::Format_ARGB4444_Premultiplied: return "ARGB4444_Premultiplied";
        case QImage::Format_RGBX8888: return "RGBX8888";
        case QImage::Format_RGBA8888: return "RGBA8888";
        case QImage::Format_RGBA8888_Premultiplied: return "RGBA8888_Premultiplied";
        case QImage::Format_BGR30: return "BGR30";
        case QImage::Format_A2BGR30_Premultiplied: return "A2BGR30_Premultiplied";
        case QImage::Format_RGB30: return "RGB30";
        case QImage::Format_A2RGB30_Premultiplied: return "A2RGB30_Premultiplied";
        case QImage::Format_Alpha8: return "Alpha8";
        case QImage::Format_Grayscale8: return "Grayscale8";
        case QImage::Format_Grayscale16: return "Grayscale16";
        case QImage::Format_RGBX64: return "RGBX64";
        case QImage::Format_RGBA64: return "RGBA64";
        case QImage::Format_RGBA64_Premultiplied: return "RGBA64_Premultiplied";
        default: return "Unknown";
    }
}

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
        // 修改条件：在选择第二个圆时(FirstCircle state + inProgress) 或 选择完成后(SecondCircle/RingROI state) 都绘制
        if (m_secondCircleRadius > 0 && 
            (m_multiCircleState >= MultiCircleState::SecondCircle || 
             (m_multiCircleState == MultiCircleState::FirstCircle && m_selectionInProgress))) {
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
    , btnPrevImage(nullptr)
    , btnNextImage(nullptr)
    , m_currentImageIndex(-1)
    , m_lastSaveFolder(QDir::currentPath()) // Initialize with current path
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
        imageLabel->setMinimumSize(600, 900); // Increased height from 500 to 700
        imageLabel->setStyleSheet("QLabel { background-color : white; border: 1px solid gray; }");
        imageLabel->setMouseTracking(true);

        // --- Add Navigation Buttons ---
        btnPrevImage = new QPushButton("<");
        btnNextImage = new QPushButton(">");
        btnPrevImage->setFixedSize(30, 60); // Adjust size as needed
        btnNextImage->setFixedSize(30, 60); // Adjust size as needed
        btnPrevImage->setToolTip(tr("上一张图片"));
        btnNextImage->setToolTip(tr("下一张图片"));

        auto *imageLayout = new QHBoxLayout(); // Layout for buttons and image
        imageLayout->addWidget(btnPrevImage);
        imageLayout->addWidget(imageLabel, 1); // Give image label stretch factor
        imageLayout->addWidget(btnNextImage);
        vCenter->addLayout(imageLayout); // Add this layout instead of just imageLabel

        // Initially disable navigation buttons
        updateNavigationButtonsState();
        // --- End Navigation Buttons ---

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

        // --- Connect Folder Selection and Navigation Signals ---
        connect(btnSelectFolder, &QPushButton::clicked, this, &ProcessingWidget::onSelectFolderClicked);
        connect(btnPrevImage, &QPushButton::clicked, this, &ProcessingWidget::onPrevImageClicked);
        connect(btnNextImage, &QPushButton::clicked, this, &ProcessingWidget::onNextImageClicked);
        // --- End Signal Connections ---

        // --- Connect Select and Save Buttons ---
        // 注释掉这一行，防止重复触发，让MainWindow处理选择图像事件
        // connect(btnSelect, &QPushButton::clicked, this, &ProcessingWidget::onSelectClicked);
        connect(btnSave, &QPushButton::clicked, this, &ProcessingWidget::onSaveClicked);
        // --- End Select and Save Buttons ---

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
    static bool isProcessing = false;
    if (isProcessing) return;
    isProcessing = true;
    
    try {
        if (image.isNull()) {
            qDebug() << "Warning: Attempted to display null image";
            return;
        }

        if (!imageLabel) {
            qDebug() << "Error: imageLabel is null";
            return;
        }

        // 记录源图像信息
        qDebug() << "源图像信息 - 大小:" << image.size() 
                << "格式:" << image.format() 
                << "(" << getQImageFormatName(image.format()) << ")"
                << "深度:" << image.depth() << "位"
                << "是否空:" << image.isNull();

        // 创建一个安全的深拷贝
        QImage safeImage;
        try {
            safeImage = image.copy();
            
            // 对于索引色图像，转换为RGB32格式以确保安全
            if (safeImage.format() == QImage::Format_Indexed8 || 
                safeImage.format() == QImage::Format_Mono ||
                safeImage.format() == QImage::Format_MonoLSB) {
                qDebug() << "将索引色或单色图像转换为RGB32格式";
                safeImage = safeImage.convertToFormat(QImage::Format_RGB32);
            }
            
            qDebug() << "创建安全拷贝 - 大小:" << safeImage.size() 
                    << "格式:" << safeImage.format() 
                    << "(" << getQImageFormatName(safeImage.format()) << ")";
        }
        catch (const std::exception& e) {
            qDebug() << "创建图像拷贝时出错:" << e.what();
            return;
        }
        catch (...) {
            qDebug() << "创建图像拷贝时出现未知错误";
            return;
        }

        // 设置当前图像
        m_currentImage = safeImage;
        
        // 发射图像变化信号
        emit imageChanged(m_currentImage);

        // 创建QPixmap并缩放
        QPixmap pixmap;
        try {
            pixmap = QPixmap::fromImage(safeImage);
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
        }
        catch (const std::exception& e) {
            qDebug() << "创建或缩放QPixmap时出错:" << e.what();
            return;
        }
        catch (...) {
            qDebug() << "创建或缩放QPixmap时出现未知错误";
            return;
        }

        // 设置到标签
        try {
            imageLabel->setPixmap(pixmap);
        }
        catch (const std::exception& e) {
            qDebug() << "设置QPixmap到标签时出错:" << e.what();
            return;
        }
        catch (...) {
            qDebug() << "设置QPixmap到标签时出现未知错误";
            return;
        }

        qDebug() << "图像显示成功完成";

        // 触发图像统计信息更新
        try {
            emit imageStatsUpdated(calculateMeanValue(m_currentImage));
        }
        catch (const std::exception& e) {
            qDebug() << "计算图像统计信息时出错:" << e.what();
        }
        catch (...) {
            qDebug() << "计算图像统计信息时出现未知错误";
        }
        
        // 更新ROI显示
        updateROIDisplay();
    } catch (const std::exception& e) {
        qDebug() << "Error displaying image:" << e.what();
    }
    catch (...) {
        qDebug() << "Unknown error during image display";
    }
    
    isProcessing = false;
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
        // 确保图像已加载且imageLabel存在
        if (m_currentImage.isNull() || !imageLabel) {
            return;
        }

        // 获取鼠标位置相对于图像标签的坐标
        QPoint pos = imageLabel->mapFrom(this, event->pos());
        
        // 检查鼠标是否在图像标签范围内
        if (!imageLabel->rect().contains(pos)) {
            return;
        }

        // 转换为图像坐标
        QPoint imagePos = mapToImageCoordinates(pos);
        
        qDebug() << "鼠标按下，图像坐标:" << imagePos;

        // 处理左键点击
        if (event->button() == Qt::LeftButton) {
            // 首先检查是否是在已完成的环形ROI上点击
            if (m_multiCircleState == MultiCircleState::RingROI) {
                // 计算点到第一个圆心的距离
                double dx1 = imagePos.x() - m_imageCircleCenter.x();
                double dy1 = imagePos.y() - m_imageCircleCenter.y();
                double dist1 = sqrt(dx1*dx1 + dy1*dy1);
                
                // 如果点击在第一个圆内，移动第一个圆
                if (dist1 <= m_imageCircleRadius) {
                    m_movingCircle = true;
                    m_currentCircle = 1;
                    setCursor(Qt::SizeAllCursor);
                    qDebug() << "开始移动第一个圆";
                    return;
                }
                
                // 检查点是否在环形区域内（两个圆的对称差）
                if (isPointInRingROI(imagePos)) {
                    m_movingCircle = true;
                    m_currentCircle = 2;
                    setCursor(Qt::SizeAllCursor);
                    qDebug() << "开始移动第二个圆";
                    return;
                }
            }
            
            // 如果不是移动ROI，执行常规ROI选择逻辑
            switch (m_currentROIMode) {
                case ROISelectionMode::Rectangle:
                    // 开始矩形选择
                    m_selectionStart = pos;
                    m_selectionCurrent = m_selectionStart;
                    m_selectionInProgress = true;
                    setCursor(Qt::CrossCursor);
                    break;
                    
                case ROISelectionMode::Circle:
                    // 开始圆形选择或处理多圆状态
                    if (m_multiCircleState == MultiCircleState::None) {
                        // 第一个圆的选择开始
                        m_circleCenter = pos;
                        m_imageCircleCenter = mapToImageCoordinates(pos);
                        m_circleRadius = 0;
                        m_selectionInProgress = true;
                        m_multiCircleState = MultiCircleState::FirstCircle;
                    } else if (m_multiCircleState == MultiCircleState::FirstCircle) {
                        // 开始选择第二个圆 - 立即更新状态为SecondCircle
                        m_secondCircleCenter = pos;
                        m_imageSecondCircleCenter = mapToImageCoordinates(pos);
                        m_secondCircleRadius = 0;
                        m_selectionInProgress = true;
                        m_multiCircleState = MultiCircleState::SecondCircle; // 立即更新状态
                    }
                    break;
                    
                case ROISelectionMode::Arbitrary:
                    // 对于任意形状，添加点
                    if (!m_selectionInProgress) {
                        // 开始新的任意形状选择
                        m_arbitraryPoints.clear();
                        m_selectionInProgress = true;
                    }
                    // 添加新的点到任意形状中
                    m_arbitraryPoints.append(pos);
                    break;
                    
                default:
                    break;
            }
            
            // 更新ROI覆盖层
            updateROIDisplay();
        } else if (event->button() == Qt::RightButton) {
            // 右键用于取消选择
            if (m_selectionInProgress) {
                // 取消当前选择
                m_selectionInProgress = false;
                m_movingCircle = false;
                m_currentCircle = 0;
                setCursor(Qt::ArrowCursor);
                qDebug() << "ROI选择已取消";
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "Error in mousePressEvent:" << e.what();
    }
}

// 实现处理ROI移动的函数
void ProcessingWidget::handleRoiMovement(const QPointF& imagePos)
{
    // 检查点击位置是否在圆形ROI内部或接近其边缘
    const int circleCenterHandleRadius = 10; // 圆心附近判定半径
    
    // 检查第一个圆
    if (m_circleRadius > 0) {
        // 计算点击位置到圆心的距离
        double dx = imagePos.x() - m_imageCircleCenter.x();
        double dy = imagePos.y() - m_imageCircleCenter.y();
        double distance = sqrt(dx*dx + dy*dy);
        
        // 如果点击在圆心附近或圆内，选择第一个圆
        if (distance <= circleCenterHandleRadius || distance <= m_imageCircleRadius) {
            m_movingCircle = true;
            m_currentCircle = 1; // 第一个圆
            setCursor(Qt::SizeAllCursor);
            qDebug() << "开始移动第一个圆形ROI";
            return;
        }
    }
    
    // 检查第二个圆
    if (m_secondCircleRadius > 0) {
        // 计算点击位置到圆心的距离
        double dx = imagePos.x() - m_imageSecondCircleCenter.x();
        double dy = imagePos.y() - m_imageSecondCircleCenter.y();
        double distance = sqrt(dx*dx + dy*dy);
        
        // 如果点击在圆心附近或圆内，选择第二个圆
        if (distance <= circleCenterHandleRadius || distance <= m_imageSecondCircleRadius) {
            m_movingCircle = true;
            m_currentCircle = 2; // 第二个圆
            setCursor(Qt::SizeAllCursor);
            qDebug() << "开始移动第二个圆形ROI";
            return;
        }
    }
    
    // 如果点击不在任何圆上，重置状态
    m_movingCircle = false;
    m_currentCircle = 0;
    setCursor(Qt::ArrowCursor);
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

// 实现calculateImageDistance函数 - 将UI距离转换为图像距离
int ProcessingWidget::calculateImageDistance(int uiDistance)
{
    if (m_currentImage.isNull() || !imageLabel) {
        return 0;
    }
    
    // 获取图像在标签中的实际显示区域
    QRect displayRect = getScaledImageRect();
    
    // 计算UI距离和图像距离的比例
    double ratioX = static_cast<double>(m_currentImage.width()) / displayRect.width();
    
    // 将UI距离转换为图像距离
    int imageDistance = qRound(uiDistance * ratioX);
    
    return imageDistance;
}

// 添加一个方法来强制更新ROI显示
void ProcessingWidget::updateROIDisplay()
{
    if (m_roiOverlay && !m_currentImage.isNull() && imageLabel) {
        // 获取实际图像显示区域
        QRect actualImageRect = getScaledImageRect();
        
        // 确保覆盖层大小与imageLabel一致
        m_roiOverlay->setGeometry(0, 0, imageLabel->width(), imageLabel->height());
        
        // 更新ROI数据 - 始终传递所有当前的ROI信息，包括第一个和第二个圆
        m_roiOverlay->setROIData(m_rectangleROI, m_circleCenter, m_circleRadius,
                               m_arbitraryPoints, m_selectionInProgress,
                               m_imageRectangleROI, 
                               m_currentImage.width(), m_currentImage.height(),
                               actualImageRect, m_imageCircleRadius,
                               m_secondCircleCenter, m_secondCircleRadius,
                               m_imageSecondCircleRadius, m_multiCircleState);
        
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
    
    // 环形ROI计算完成后，启用应用按钮
    if (btnApplyROI) {
        btnApplyROI->setEnabled(true);
        qDebug() << "环形ROI计算完成，启用应用按钮";
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

// --- Implementation for Folder Selection and Navigation ---

void ProcessingWidget::onSelectFolderClicked()
{
    try {
        QString dirPath = QFileDialog::getExistingDirectory(this, tr("选择图片文件夹"),
                                                        QString(), // Start directory (empty means default)
                                                        QFileDialog::ShowDirsOnly
                                                        | QFileDialog::DontResolveSymlinks);

        if (dirPath.isEmpty()) {
            return; // User cancelled
        }

        QDir directory(dirPath);
        qDebug() << "选择文件夹:" << dirPath;
        
        // 收集支持的图像格式
        QStringList nameFilters;
        QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
        qDebug() << "支持的图像格式:" << supportedFormats;
        
        for (const QByteArray &format : supportedFormats) {
            nameFilters << "*." + QString::fromLatin1(format);
        }

        // 查找文件夹中的图像文件
        QStringList foundFiles = directory.entryList(nameFilters, QDir::Files | QDir::Readable);
        qDebug() << "找到" << foundFiles.size() << "个图像文件";

        if (foundFiles.isEmpty()) {
            m_currentImageIndex = -1;
            QMessageBox::information(this, tr("无图像"), tr("在选定文件夹中未找到支持的图像文件。"));
            // Optionally clear the display or show a placeholder
            if (imageLabel) {
                imageLabel->clear(); 
            }
            m_currentImage = QImage(); // Clear current image data
        } else {
            // 清除旧的图像文件列表
            m_imageFiles.clear();
            
            // 转换为完整路径
            for (const QString& file : foundFiles) {
                m_imageFiles.append(directory.filePath(file));
            }
            
            qDebug() << "准备加载第一张图像:" << m_imageFiles.first();
            m_currentImageIndex = 0;
            
            // 使用增强版的displayImageAtIndex加载第一张图像
            displayImageAtIndex(m_currentImageIndex);
            qDebug() << "已加载" << m_imageFiles.size() << "张图像，来自" << dirPath;
        }
        
        // 更新导航按钮状态
        updateNavigationButtonsState();
        
        // 更新最后使用的文件夹
        m_lastSaveFolder = dirPath;
    }
    catch (const std::exception& e) {
        qWarning() << "选择文件夹过程中出现异常:" << e.what();
        QMessageBox::warning(this, tr("错误"), tr("选择文件夹时出错: %1").arg(e.what()));
    }
    catch (...) {
        qWarning() << "选择文件夹过程中出现未知异常";
        QMessageBox::warning(this, tr("错误"), tr("选择文件夹时出现未知错误"));
    }
}

void ProcessingWidget::displayImageAtIndex(int index)
{
    try {
        if (index < 0 || index >= m_imageFiles.size()) {
            qDebug() << "Error: Invalid image index" << index;
            return;
        }

        QString imagePath = m_imageFiles.at(index);
        qDebug() << "加载图像文件:" << imagePath << "索引:" << index << "/" << (m_imageFiles.size()-1);
        
        // 使用QImageReader加载图像，更安全
        QImageReader reader(imagePath);
        
        // 检查图像格式
        QByteArray format = reader.format();
        if (!format.isEmpty()) {
            qDebug() << "图像格式:" << format;
        }
        
        // 设置错误处理
        reader.setDecideFormatFromContent(true);
        
        QImage newImage;
        bool success = false;
        
        try {
            // 尝试读取图像
            success = reader.read(&newImage);
            
            if (!success) {
                qWarning() << "Error loading image:" << imagePath 
                         << "Error:" << reader.errorString();
                QMessageBox::warning(this, tr("加载失败"), 
                                    tr("无法加载图像文件: %1\n错误: %2")
                                    .arg(QFileInfo(imagePath).fileName())
                                    .arg(reader.errorString()));
                return;
            }
            
            // 检查加载的图像是否有效
            if (newImage.isNull()) {
                qWarning() << "加载的图像为空";
                QMessageBox::warning(this, tr("加载失败"), 
                                    tr("加载的图像为空: %1").arg(QFileInfo(imagePath).fileName()));
                return;
            }
            
            // 显示加载的图像信息
            qDebug() << "成功加载图像 - 大小:" << newImage.size() 
                    << "格式:" << newImage.format() 
                    << "(" << getQImageFormatName(newImage.format()) << ")"
                    << "深度:" << newImage.depth() << "位";
            
            // 使用安全的displayImage方法显示图像
            displayImage(newImage);
            
            // 更新导航按钮状态
            updateNavigationButtonsState();
        }
        catch (const std::exception& e) {
            qWarning() << "加载图像时出现异常:" << e.what();
            QMessageBox::warning(this, tr("加载失败"), 
                               tr("加载图像时出现异常: %1").arg(e.what()));
        }
        catch (...) {
            qWarning() << "加载图像时出现未知异常";
            QMessageBox::warning(this, tr("加载失败"), tr("加载图像时出现未知错误"));
        }
    }
    catch (const std::exception& e) {
        qWarning() << "displayImageAtIndex函数异常:" << e.what();
    }
    catch (...) {
        qWarning() << "displayImageAtIndex函数未知异常";
    }
}

void ProcessingWidget::onPrevImageClicked()
{
    if (m_imageFiles.size() <= 1) return; // Nothing to navigate

    m_currentImageIndex--;
    if (m_currentImageIndex < 0) {
        m_currentImageIndex = m_imageFiles.size() - 1; // Wrap around to the end
    }
    displayImageAtIndex(m_currentImageIndex);
}

void ProcessingWidget::onNextImageClicked()
{
    if (m_imageFiles.size() <= 1) return; // Nothing to navigate

    m_currentImageIndex++;
    if (m_currentImageIndex >= m_imageFiles.size()) {
        m_currentImageIndex = 0; // Wrap around to the beginning
    }
    displayImageAtIndex(m_currentImageIndex);
}

void ProcessingWidget::updateNavigationButtonsState()
{
    bool enable = m_imageFiles.size() > 1;
    if (btnPrevImage) {
        btnPrevImage->setEnabled(enable);
    }
    if (btnNextImage) {
        btnNextImage->setEnabled(enable);
    }
}

// --- New Slot for Saving Image ---
void ProcessingWidget::onSaveClicked()
{
    if (m_currentImage.isNull()) {
        QMessageBox::warning(this, tr("无法保存"), tr("没有可保存的图像。"));
        return;
    }

    QString defaultFileName;
    QString currentFileDir = m_lastSaveFolder; // Default to last saved/opened folder

    // Try to get filename and directory from the currently loaded file
    if (m_currentImageIndex >= 0 && m_currentImageIndex < m_imageFiles.size()) {
        QFileInfo fileInfo(m_imageFiles[m_currentImageIndex]);
        QString baseName = fileInfo.completeBaseName();
        QString suffix = fileInfo.suffix();

        // Suggest a modified name like "original_processed.png"
        defaultFileName = baseName + "_processed." + suffix;
        // Use the directory of the current file if m_lastSaveFolder hasn't been set otherwise yet
        if (QDir(m_lastSaveFolder) == QDir(QDir::currentPath())) { // Check if still default
             currentFileDir = fileInfo.absolutePath();
        }

        // Ensure suggested suffix is valid for saving, default to png otherwise
        if (!QImageWriter::supportedImageFormats().contains(suffix.toUtf8()) || suffix.isEmpty()) {
             defaultFileName = baseName + "_processed.png";
        }
    } else {
        defaultFileName = "processed_image.png"; // Fallback if no file source known
    }

    // Construct the initial path for the dialog
    QString initialPath = QDir(currentFileDir).filePath(defaultFileName);

    QString saveFilePath = QFileDialog::getSaveFileName(
        this,
        tr("保存处理后的图像"),
        initialPath, // Use the constructed initial path
        tr("PNG (*.png);;JPEG (*.jpg *.jpeg);;Bitmap (*.bmp)") // Provide common formats
    );

    if (!saveFilePath.isEmpty()) {
        // Ensure the file has a valid extension based on the filter or default to png
        QFileInfo fi(saveFilePath);
        QString suffix = fi.suffix().toLower();

        // Basic check if suffix is valid or force png
         if (suffix.isEmpty() || !QStringList({"png", "jpg", "jpeg", "bmp"}).contains(suffix)) {
            saveFilePath += ".png";
            qWarning() << "Adding default .png suffix as none/invalid was provided:" << saveFilePath;
         }

        if (m_currentImage.save(saveFilePath)) {
            QMessageBox::information(this, tr("保存成功"), tr("图像已保存至: %1").arg(saveFilePath));
            // Update the last used directory to where the file was just saved
            m_lastSaveFolder = QFileInfo(saveFilePath).absolutePath();
        } else {
            QMessageBox::critical(this, tr("保存失败"), tr("无法将图像保存至: %1").arg(saveFilePath));
        }
    }
}


// --- End New Slots ---

// --- New Slot for Single Image Selection ---
void ProcessingWidget::onSelectClicked()
{
    // Start browsing from the last used folder
    QString filePath = QFileDialog::getOpenFileName(this, tr("选择图像"), m_lastSaveFolder, tr("Images (*.png *.jpg *.bmp *.jpeg *.gif)"));
    if (!filePath.isEmpty()) {
        // 使用QImageReader安全加载图像
        QImageReader reader(filePath);
        reader.setDecideFormatFromContent(true);
        
        QImage newImage;
        if (reader.read(&newImage)) {
            // Store the path of the single selected image in m_imageFiles for consistency
            m_imageFiles.clear();
            m_imageFiles.append(filePath);
            m_currentImageIndex = 0;

            displayImage(newImage);
            // Update the last used folder based on this selection
            m_lastSaveFolder = QFileInfo(filePath).absolutePath();
            updateNavigationButtonsState(); // Update nav buttons (likely disabling them)
        } else {
            QMessageBox::warning(this, tr("加载失败"), 
                               tr("无法加载图像文件: %1\n错误: %2")
                               .arg(QFileInfo(filePath).fileName())
                               .arg(reader.errorString()));
        }
    }
}

void ProcessingWidget::mouseMoveEvent(QMouseEvent *event)
{
    try {
        if (m_currentImage.isNull() || !imageLabel) {
            return;
        }
        
        // 获取鼠标位置相对于图像标签的坐标
        QPoint pos = imageLabel->mapFrom(this, event->pos());
        
        // 检查鼠标是否在图像标签范围内
        if (!imageLabel->rect().contains(pos)) {
            return;
        }
        
        // 转换为图像坐标
        QPoint imagePos = mapToImageCoordinates(pos);
        
        // 如果在移动圆形ROI
        if (m_movingCircle) {
            // 根据当前选中的圆进行移动
            if (m_currentCircle == 1) {
                // 移动第一个圆
                m_circleCenter = pos;
                m_imageCircleCenter = imagePos;
                qDebug() << "移动第一个圆到 UI坐标:" << pos << " 图像坐标:" << imagePos;
                updateROIDisplay();
            } else if (m_currentCircle == 2) {
                // 移动第二个圆
                m_secondCircleCenter = pos;
                m_imageSecondCircleCenter = imagePos;
                qDebug() << "移动第二个圆到 UI坐标:" << pos << " 图像坐标:" << imagePos;
                updateROIDisplay();
            }
            return;
        }
        
        // 根据当前ROI选择模式处理鼠标移动
        if (m_selectionInProgress) {
            switch (m_currentROIMode) {
                case ROISelectionMode::Rectangle:
                    // 更新矩形选择
                    m_selectionCurrent = pos;
                    m_rectangleROI = QRect(m_selectionStart, m_selectionCurrent).normalized();
                    // 转换为图像坐标
                    m_imageRectangleROI = mapToImageRect(m_rectangleROI);
                    break;
                    
                case ROISelectionMode::Circle: {
                    // 更新圆形选择
                    // 计算半径 - 鼠标当前位置到圆心的距离
                    int dx = pos.x() - m_circleCenter.x();
                    int dy = pos.y() - m_circleCenter.y();
                    int radius = qRound(sqrt(dx*dx + dy*dy));
                    
                    if (m_multiCircleState == MultiCircleState::FirstCircle) {
                        // 更新第一个圆
                        m_circleRadius = radius;
                        m_imageCircleRadius = calculateImageDistance(radius);
                        // 确保图像坐标中心也被正确设置
                        if (m_imageCircleCenter.isNull()) {
                            m_imageCircleCenter = mapToImageCoordinates(m_circleCenter);
                        }
                    } else if (m_multiCircleState == MultiCircleState::SecondCircle) {
                        // 更新第二个圆
                        m_secondCircleRadius = radius;
                        m_imageSecondCircleRadius = calculateImageDistance(radius);
                        // 确保图像坐标中心也被正确设置
                        if (m_imageSecondCircleCenter.isNull()) {
                            m_imageSecondCircleCenter = mapToImageCoordinates(m_secondCircleCenter);
                        }
                    }
                    break;
                }
                    
                default:
                    break;
            }
            
            // 更新ROI显示
            updateROIDisplay();
        } else {
            // 发送鼠标移动信号，包含像素信息
            QColor pixelColor = m_currentImage.pixelColor(imagePos);
            int r = pixelColor.red();
            int g = pixelColor.green();
            int b = pixelColor.blue();
            int grayValue = qGray(r, g, b);
            emit mouseMoved(imagePos, grayValue, r, g, b);
        }
    } catch (const std::exception& e) {
        qDebug() << "Error in mouseMoveEvent:" << e.what();
    }
}

void ProcessingWidget::mouseReleaseEvent(QMouseEvent *event)
{
    try {
        if (m_currentImage.isNull() || !imageLabel) {
            return;
        }

        // 如果正在移动圆形ROI
        if (m_movingCircle) {
            m_movingCircle = false;
            m_currentCircle = 0;
            setCursor(Qt::ArrowCursor);
            
            // 如果是环形ROI，需要更新环形区域
            if (m_multiCircleState == MultiCircleState::RingROI) {
                calculateRingROI();
                // 如果应用按钮存在，启用它
                if (btnApplyROI) {
                    btnApplyROI->setEnabled(true);
                }
            }
            return;
        }
        
        // 如果是右键点击并且正在选择任意形状ROI
        if (event->button() == Qt::RightButton && m_selectionInProgress && 
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
            return;
        }
        
        // 处理左键释放 - 完成ROI选择
        if (event->button() == Qt::LeftButton && m_selectionInProgress) {
            // 根据当前ROI模式完成选择
            switch (m_currentROIMode) {
                case ROISelectionMode::Rectangle:
                    // 完成矩形选择
                    m_selectionInProgress = false;
                    
                    // 确保矩形有效
                    if (m_rectangleROI.width() > 0 && m_rectangleROI.height() > 0) {
                        // 发送ROI选择信号
                        emit roiSelected(m_imageRectangleROI);
                        
                        // 启用应用按钮
                        if (btnApplyROI) {
                            btnApplyROI->setEnabled(true);
                        }
                    }
                    break;
                    
                case ROISelectionMode::Circle:
                    // 完成圆形选择
                    if (m_multiCircleState == MultiCircleState::FirstCircle) {
                        // 完成第一个圆形选择
                        m_selectionInProgress = false;
                        
                        // 创建圆形ROI
                        m_roiCircle1 = QRect(m_circleCenter.x() - m_circleRadius, 
                                          m_circleCenter.y() - m_circleRadius,
                                          m_circleRadius * 2, m_circleRadius * 2);
                        
                        qDebug() << "第一个圆形ROI选择完成";
                        qDebug() << "UI中心: " << m_circleCenter << " 半径: " << m_circleRadius;
                        qDebug() << "图像中心: " << m_imageCircleCenter << " 半径: " << m_imageCircleRadius;
                        
                    } else if (m_multiCircleState == MultiCircleState::SecondCircle) {
                        // 完成第二个圆形选择
                        m_selectionInProgress = false;
                        
                        // 创建第二个圆形ROI
                        m_roiCircle2 = QRect(m_secondCircleCenter.x() - m_secondCircleRadius, 
                                          m_secondCircleCenter.y() - m_secondCircleRadius,
                                          m_secondCircleRadius * 2, m_secondCircleRadius * 2);
                        
                        // 第二个圆选择完成，状态已经在点击时更新，此处不需要再次设置
                        // m_multiCircleState = MultiCircleState::SecondCircle;
                        
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
                            // 清除第二个圆的数据
                            m_secondCircleCenter = QPoint();
                            m_secondCircleRadius = 0;
                            m_imageSecondCircleCenter = QPoint();
                            m_imageSecondCircleRadius = 0;
                            m_roiCircle2 = QRect();
                        } else {
                            // 两个圆都选择完成，发送环形ROI选择完成信号
                            emit ringROISelected(m_imageCircleCenter, m_imageCircleRadius,
                                              m_imageSecondCircleCenter, m_imageSecondCircleRadius);
                            
                            // 此时才启用应用按钮
                            if (btnApplyROI) {
                                btnApplyROI->setEnabled(true);
                                qDebug() << "环形ROI创建完成，应用按钮已启用";
                            }
                            
                            // 提示用户环形ROI已创建完成
                            QMessageBox::information(this, tr("环形ROI创建完成"), 
                                                   tr("环形ROI创建完成，包含 %1 个像素点，您可以点击'应用ROI'按钮进行操作")
                                                   .arg(pixels.size()));
                        }
                    }
                    break;
                    
                default:
                    break;
            }
            
            // 更新ROI覆盖层
            updateROIDisplay();
        }
        
        // 只有当没有完成有效选择时，才禁用应用按钮
        if (btnApplyROI && !m_selectionInProgress) {
            // 根据环形ROI是否有效来决定是否启用应用按钮
            if (m_multiCircleState == MultiCircleState::RingROI && !m_imageCircleCenter.isNull() && !m_imageSecondCircleCenter.isNull()) {
                btnApplyROI->setEnabled(true);
            } else if (!m_rectangleROI.isNull() && m_rectangleROI.width() > 0 && m_rectangleROI.height() > 0) {
                btnApplyROI->setEnabled(true);
            } else if (m_arbitraryPoints.size() > 2) {
                btnApplyROI->setEnabled(true);
            } else {
                btnApplyROI->setEnabled(false);
            }
        }
        
        // 重置鼠标形状
        setCursor(Qt::ArrowCursor);
        
        // 更新覆盖层
        updateROIDisplay();
    } catch (const std::exception& e) {
        qDebug() << "Error in mouseReleaseEvent:" << e.what();
    }
}

// Add this implementation for wheelEvent
void ProcessingWidget::wheelEvent(QWheelEvent *event)
{
    if (m_currentImage.isNull() || !imageLabel) {
        return;
    }

    // Handle zooming with Ctrl+Mouse Wheel
    if (event->modifiers() & Qt::ControlModifier) {
        double numDegrees = event->angleDelta().y() / 8.0;
        double numSteps = numDegrees / 15.0;
        
        // Calculate new zoom factor
        double newZoomFactor = m_zoomFactor + numSteps * ZOOM_FACTOR_STEP;
        
        // Clamp zoom factor to min/max limits
        newZoomFactor = qBound(MIN_ZOOM, newZoomFactor, MAX_ZOOM);
        
        if (newZoomFactor != m_zoomFactor) {
            m_zoomFactor = newZoomFactor;
            
            // Update image display with new zoom factor
            if (!m_currentImage.isNull() && imageLabel) {
                QPixmap pixmap = QPixmap::fromImage(m_currentImage);
                QSize scaledSize = pixmap.size() * m_zoomFactor;
                
                // Maintain aspect ratio
                pixmap = pixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                imageLabel->setPixmap(pixmap);
                
                // Update ROI display with new zoom
                updateROIDisplay();
            }
            
            qDebug() << "Zoom factor changed to:" << m_zoomFactor;
        }
        
        event->accept();
    } else {
        // Pass other wheel events to parent
        QWidget::wheelEvent(event);
    }
}

// Add this implementation for updateKValueLabel
void ProcessingWidget::updateKValueLabel(int value)
{
    if (!lblKValue) {
        return;
    }
    
    // Calculate the actual brightness multiplier value (range from 0.1 to 2.0)
    double k = 1.0 + (value / 100.0);
    
    // Update the label
    lblKValue->setText(QString("k = %1").arg(k, 0, 'f', 2));
}

// Add this implementation for updateBValueLabel
void ProcessingWidget::updateBValueLabel(int value)
{
    if (!lblBValue) {
        return;
    }
    
    // Convert slider value to actual offset (-100 to 100)
    int b = value;
    
    // Update the label
    lblBValue->setText(QString("b = %1").arg(b));
}

// Add this implementation for updateGammaValueLabel
void ProcessingWidget::updateGammaValueLabel(int value)
{
    if (!lblGammaValue) {
        return;
    }
    
    // Calculate the actual gamma value (range from 0.1 to 2.0)
    double gamma = value / 10.0;
    
    // Update the label
    lblGammaValue->setText(QString("γ = %1").arg(gamma, 0, 'f', 2));
}


