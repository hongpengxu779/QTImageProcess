#include "HistogramDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QDebug>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QFrame>
#include <QFont>
#include <QPen>
#include <QBrush>
#include <QTimer>

HistogramDialog::HistogramDialog(QWidget *parent)
    : QDialog(parent)
    , m_histogram(256, 0)
    , m_histogramLabel(nullptr)
    , m_statsLabel(nullptr)
    , m_histogramFrame(nullptr)
    , m_currentImage()
{
    setWindowTitle(tr("灰度直方图"));
    setMinimumSize(500, 400);
    setupUi();
}

HistogramDialog::~HistogramDialog()
{
}

void HistogramDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Create stats label
    m_statsLabel = new QLabel(this);
    m_statsLabel->setAlignment(Qt::AlignCenter);
    m_statsLabel->setMinimumHeight(30);
    m_statsLabel->setText(tr("灰度值统计: 暂无数据"));
    mainLayout->addWidget(m_statsLabel);

    // Create frame for histogram
    m_histogramFrame = new QFrame(this);
    m_histogramFrame->setFrameShape(QFrame::StyledPanel);
    m_histogramFrame->setFrameShadow(QFrame::Sunken);
    m_histogramFrame->setMinimumHeight(300);
    
    // Create histogram label
    m_histogramLabel = new QLabel(m_histogramFrame);
    m_histogramLabel->setAlignment(Qt::AlignCenter);
    
    // Layout for frame
    auto *frameLayout = new QVBoxLayout(m_histogramFrame);
    frameLayout->setContentsMargins(1, 1, 1, 1);
    frameLayout->addWidget(m_histogramLabel);
    
    mainLayout->addWidget(m_histogramFrame, 1);

    // Add axis labels
    QLabel *xAxisLabel = new QLabel(tr("灰度值 (0-255)"), this);
    xAxisLabel->setAlignment(Qt::AlignCenter);
    QLabel *yAxisLabel = new QLabel(tr("像素数"), this);
    yAxisLabel->setAlignment(Qt::AlignCenter);
    
    mainLayout->addWidget(xAxisLabel);
    
    // Create an empty histogram initially
    renderHistogram();
}

void HistogramDialog::calculateHistogram(const QImage& image)
{
    if (image.isNull()) {
        qDebug() << "计算直方图失败：图像为空";
        return;
    }

    qDebug() << "开始计算直方图，图像大小：" << image.width() << "x" << image.height() 
             << "，格式：" << image.format();

    // Reset histogram
    m_histogram.fill(0, 256);
    
    // Calculate grayscale histogram
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QColor color = image.pixelColor(x, y);
            int gray = qGray(color.rgb());
            if (gray >= 0 && gray < 256) {
                m_histogram[gray]++;
            }
        }
    }
    
    // Find maximum value for scaling
    int maxCount = 0;
    int totalPixels = 0;
    double sum = 0;
    
    for (int i = 0; i < 256; i++) {
        maxCount = qMax(maxCount, m_histogram[i]);
        totalPixels += m_histogram[i];
        sum += i * m_histogram[i];
    }
    
    // Calculate mean value
    double meanValue = totalPixels > 0 ? sum / totalPixels : 0;
    
    // 打印直方图中非零值的数量，便于调试
    int nonZeroValues = 0;
    for (int i = 0; i < 256; i++) {
        if (m_histogram[i] > 0) {
            nonZeroValues++;
        }
    }
    
    // Update stats label
    m_statsLabel->setText(tr("灰度统计: 平均值 = %1, 总像素 = %2, 非零值 = %3")
                           .arg(meanValue, 0, 'f', 2)
                           .arg(totalPixels)
                           .arg(nonZeroValues));
    
    qDebug() << "直方图计算完成: 平均值 =" << meanValue 
             << "总像素 =" << totalPixels
             << "最大值 =" << maxCount
             << "非零值个数 =" << nonZeroValues;
}

void HistogramDialog::renderHistogram()
{
    // Get frame size for rendering
    QSize frameSize = m_histogramFrame->size();
    frameSize.setHeight(frameSize.height() - 2); // Account for frame border
    frameSize.setWidth(frameSize.width() - 2);
    
    if (frameSize.width() <= 0 || frameSize.height() <= 0) {
        frameSize = QSize(480, 300); // Default size if frame size is invalid
    }
    
    // Create pixmap for drawing
    m_histogramPixmap = QPixmap(frameSize);
    m_histogramPixmap.fill(Qt::white);
    
    QPainter painter(&m_histogramPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Drawing constants
    const int marginLeft = 40;  // 增加左边距以容纳Y轴标签
    const int marginRight = 10;
    const int marginTop = 20;   // 增加上边距以容纳标题
    const int marginBottom = 30;
    const int graphWidth = frameSize.width() - marginLeft - marginRight;
    const int graphHeight = frameSize.height() - marginTop - marginBottom;
    
    // Draw title
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(frameSize.width()/2 - 50, 15, tr("灰度直方图"));
    painter.setFont(QFont("Arial", 8));
    
    // Draw coordinate system
    painter.setPen(QPen(Qt::black, 1));
    painter.drawLine(marginLeft, graphHeight + marginTop, marginLeft + graphWidth, graphHeight + marginTop); // X-axis
    painter.drawLine(marginLeft, marginTop, marginLeft, graphHeight + marginTop); // Y-axis
    
    // 添加坐标轴标题
    painter.drawText(marginLeft + graphWidth/2 - 20, graphHeight + marginTop + 25, tr("灰度值"));
    painter.save();
    painter.translate(10, marginTop + graphHeight/2);
    painter.rotate(-90);
    painter.drawText(0, 0, tr("像素数量"));
    painter.restore();
    
    // Find maximum value
    int maxCount = 1; // Prevent division by zero
    for (int i = 0; i < 256; ++i) {
        maxCount = qMax(maxCount, m_histogram[i]);
    }
    
    qDebug() << "渲染直方图：最大计数值 =" << maxCount 
             << "图形宽度 =" << graphWidth 
             << "图形高度 =" << graphHeight;
    
    // 如果maxCount太小，说明直方图基本为空，显示警告信息
    if (maxCount <= 1) {
        painter.setPen(Qt::red);
        painter.drawText(marginLeft + 20, marginTop + graphHeight/2, 
                         tr("无直方图数据或图像为空"));
        
        // 即使没有数据，也绘制坐标轴刻度
        // Draw axis labels (every 32 steps)
        painter.setPen(QPen(Qt::black, 1));
        for (int i = 0; i <= 256; i += 32) {
            int x = marginLeft + (i * graphWidth) / 256;
            painter.drawLine(x, graphHeight + marginTop, x, graphHeight + marginTop + 5);
            painter.drawText(x - 10, graphHeight + marginTop + 20, QString::number(i));
        }
        
        // Draw y-axis labels
        painter.drawText(marginLeft - 25, marginTop + 10, "0");
        painter.drawText(marginLeft - 25, marginTop + graphHeight/2, "0");
        painter.drawText(marginLeft - 25, marginTop + graphHeight, "0");
        
        m_histogramLabel->setPixmap(m_histogramPixmap);
        return;
    }
    
    // Draw histogram bars
    const double barWidth = static_cast<double>(graphWidth) / 256;
    
    for (int i = 0; i < 256; ++i) {
        // Calculate bar height (scaled to fit graph)
        double barHeight = (static_cast<double>(m_histogram[i]) / maxCount) * graphHeight;
        
        if (barHeight < 1 && m_histogram[i] > 0) {
            barHeight = 1; // 确保有数据的柱子至少有1个像素高
        }
        
        // Draw bar
        QRectF barRect(
            marginLeft + i * barWidth,
            marginTop + graphHeight - barHeight,
            barWidth,
            barHeight
        );
        
        // Use gray level as color with blue tint for better visibility
        int gray = i;
        QColor barColor = QColor(gray/2, gray/2, gray);
        painter.setPen(QPen(Qt::darkGray, 0.5));
        painter.setBrush(QBrush(barColor));
        painter.drawRect(barRect);
    }
    
    // Draw axis labels (every 32 steps)
    painter.setPen(QPen(Qt::black, 1));
    for (int i = 0; i <= 256; i += 32) {
        int x = marginLeft + (i * graphWidth) / 256;
        painter.drawLine(x, graphHeight + marginTop, x, graphHeight + marginTop + 5);
        painter.drawText(x - 10, graphHeight + marginTop + 20, QString::number(i));
    }
    
    // Draw y-axis labels (max count, 3/4, half, 1/4, and zero)
    painter.drawText(marginLeft - 35, marginTop + 5, QString::number(maxCount));
    painter.drawText(marginLeft - 35, marginTop + graphHeight*1/4, QString::number(maxCount*3/4));
    painter.drawText(marginLeft - 35, marginTop + graphHeight/2, QString::number(maxCount/2));
    painter.drawText(marginLeft - 35, marginTop + graphHeight*3/4, QString::number(maxCount/4));
    painter.drawText(marginLeft - 35, marginTop + graphHeight, "0");
    
    // Draw horizontal guidelines
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
    painter.drawLine(marginLeft, marginTop + 5, marginLeft + graphWidth, marginTop + 5);
    painter.drawLine(marginLeft, marginTop + graphHeight*1/4, marginLeft + graphWidth, marginTop + graphHeight*1/4);
    painter.drawLine(marginLeft, marginTop + graphHeight/2, marginLeft + graphWidth, marginTop + graphHeight/2);
    painter.drawLine(marginLeft, marginTop + graphHeight*3/4, marginLeft + graphWidth, marginTop + graphHeight*3/4);
    
    // Set the pixmap to the label
    m_histogramLabel->setPixmap(m_histogramPixmap);
}

void HistogramDialog::updateHistogram(const QImage& image)
{
    qDebug() << "更新直方图：收到图像大小 =" << image.width() << "x" << image.height();
    
    // 保存当前图像，以便窗口显示和调整大小时刷新
    m_currentImage = image;
    
    calculateHistogram(image);
    renderHistogram();
    
    // 确保图像标签可见且处于正确尺寸
    if (m_histogramLabel) {
        m_histogramLabel->adjustSize();
        m_histogramLabel->update();
    }
}

void HistogramDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    
    // 如果有存储的图像，显示时重新渲染
    if (!m_currentImage.isNull()) {
        qDebug() << "直方图对话框显示：重新渲染直方图";
        calculateHistogram(m_currentImage);
        renderHistogram();
    } else {
        qDebug() << "直方图对话框显示：没有可用图像数据";
    }
    
    // 使用定时器在显示后稍微延迟渲染，确保窗口完全显示
    QTimer::singleShot(100, this, [this]() {
        renderHistogram();
        if (m_histogramLabel) {
            m_histogramLabel->update();
        }
    });
}

void HistogramDialog::paintEvent(QPaintEvent* event)
{
    QDialog::paintEvent(event);
}

void HistogramDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    renderHistogram(); // Re-render histogram on resize
} 