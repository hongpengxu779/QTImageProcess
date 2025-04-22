#include "mainwindow.h"
#include "ImageView/ProcessingWidget.h"
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QStatusBar>
#include <QDebug>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_processingWidget(nullptr)
    , imageProcessor(nullptr)
    , m_statusLabel(nullptr)
    , m_pixelInfoLabel(nullptr)
    , m_meanValueLabel(nullptr)
{
    // Initialize member variables
    m_processingWidget = new ProcessingWidget(this);
    imageProcessor = new ImageProcessor(this);
    m_statusLabel = new QLabel(this);
    m_pixelInfoLabel = new QLabel(this);
    m_meanValueLabel = new QLabel(this);

    setupUI();
    setupConnections();
    createMenuBar();
    setupStatusBar();
}

MainWindow::~MainWindow()
{
    delete m_processingWidget;
    delete imageProcessor;
    delete m_statusLabel;
    delete m_pixelInfoLabel;
    delete m_meanValueLabel;
}

void MainWindow::setupUI()
{
    setCentralWidget(m_processingWidget);
    resize(1024, 768);
}

void MainWindow::setupConnections()
{
    if (!imageProcessor || !m_processingWidget) {
        return;
    }

    // 连接按钮信号
    connect(m_processingWidget->getSelectButton(), &QPushButton::clicked, this, &MainWindow::onSelectImage);
    connect(m_processingWidget->getSelectFolderButton(), &QPushButton::clicked, this, &MainWindow::onSelectFolder);
    connect(m_processingWidget->getSaveButton(), &QPushButton::clicked, this, &MainWindow::onSaveImage);
    connect(m_processingWidget->getShowOriginalButton(), &QPushButton::clicked, this, &MainWindow::onShowOriginal);
    connect(m_processingWidget->getFlipHButton(), &QPushButton::clicked, this, &MainWindow::onFlipHorizontal);
    connect(m_processingWidget->getFlipVButton(), &QPushButton::clicked, this, &MainWindow::onFlipVertical);
    connect(m_processingWidget->getMeanFilterButton(), &QPushButton::clicked, this, &MainWindow::onMeanFilter);
    connect(m_processingWidget->getGaussianFilterButton(), &QPushButton::clicked, this, &MainWindow::onGaussianFilter);
    connect(m_processingWidget->getMedianFilterButton(), &QPushButton::clicked, this, &MainWindow::onMedianFilter);

    // 连接滑块信号
    connect(m_processingWidget->getBrightnessSlider(), &QSlider::valueChanged, this, &MainWindow::onBrightnessChanged);
    connect(m_processingWidget->getGammaSlider(), &QSlider::valueChanged, this, &MainWindow::onGammaChanged);
    connect(m_processingWidget->getOffsetSlider(), &QSlider::valueChanged, this, &MainWindow::onOffsetChanged);

    // 连接复选框信号
    connect(m_processingWidget->getRgbToGrayCheckBox(), &QCheckBox::toggled, this, &MainWindow::onRgbToGrayChanged);

    // 连接直方图均衡化按钮
    connect(m_processingWidget->getHistEqualButton(), &QPushButton::clicked, this, &MainWindow::onHistEqualClicked);

    // 连接其他信号
    connect(m_processingWidget, &ProcessingWidget::mouseClicked, this, &MainWindow::onMouseClicked);
    connect(m_processingWidget, &ProcessingWidget::imageStatsUpdated, this, &MainWindow::onImageStatsUpdated);
    connect(imageProcessor, &ImageProcessor::imageLoaded, this, &MainWindow::onImageLoaded);
    connect(imageProcessor, &ImageProcessor::error, this, &MainWindow::onError);
    connect(imageProcessor, &ImageProcessor::imageProcessed, this, &MainWindow::onImageProcessed);
}

void MainWindow::createMenuBar()
{
    menuBar()->addMenu(tr("文件(&F)"));
    menuBar()->addMenu(tr("滤镜(&L)"));
    menuBar()->addMenu(tr("关于(&A)"));
    menuBar()->addMenu(tr("帮助(&H)"));
    menuBar()->addMenu(tr("工具(&T)"));
    addToolBar(tr("工具栏"));
}

void MainWindow::setupStatusBar()
{
    if (!m_statusLabel || !m_pixelInfoLabel || !m_meanValueLabel) {
        qDebug() << "Status bar labels not initialized";
        return;
    }

    QStatusBar *statusBar = this->statusBar();

    // 设置标签样式
    m_statusLabel->setMinimumWidth(100);
    m_pixelInfoLabel->setMinimumWidth(300);  // 增加宽度以适应更多信息
    m_meanValueLabel->setMinimumWidth(150);

    // 添加标签到状态栏
    statusBar->addWidget(m_statusLabel);
    statusBar->addWidget(m_pixelInfoLabel);
    statusBar->addWidget(m_meanValueLabel);

    // 初始化显示
    m_statusLabel->setText(tr("就绪"));
    m_pixelInfoLabel->setText(tr("点击图像显示坐标和RGB值"));
    m_meanValueLabel->setText(tr("图像均值: 0.00"));
}

void MainWindow::onSelectImage()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("选择图片"), "",
        tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"));

    if (!fileName.isEmpty()) {
        imageProcessor->loadImage(fileName);
    }
}

void MainWindow::onImageLoaded(bool success)
{
    if (success) {
        m_processingWidget->displayImage(imageProcessor->getProcessedImage());
    } else {
        QMessageBox::warning(this, tr("错误"), tr("无法加载图片！"));
    }
}

void MainWindow::onResetToOriginal()
{
    imageProcessor->resetToOriginal();
}

void MainWindow::onImageProcessed()
{
    m_processingWidget->displayImage(imageProcessor->getProcessedImage());
}

void MainWindow::onError(const QString &errorMessage)
{
    QMessageBox::warning(this, tr("错误"), errorMessage);
}

void MainWindow::onSelectFolder()
{
    // Implementation needed
}

void MainWindow::onSaveImage()
{
    // Implementation needed
}

void MainWindow::onFlipHorizontal()
{
    imageProcessor->flipHorizontal();
}

void MainWindow::onFlipVertical()
{
    imageProcessor->flipVertical();
}

void MainWindow::onMeanFilter()
{
    imageProcessor->applyMeanFilter(3, m_processingWidget->getSubtractFiltered());  // 使用3x3的均值滤波
}

void MainWindow::onGaussianFilter()
{
    imageProcessor->applyGaussianFilter(3, 1.0, m_processingWidget->getSubtractFiltered());  // 使用3x3的高斯滤波，sigma=1.0
}

void MainWindow::onMedianFilter()
{
    imageProcessor->applyMedianFilter(3, m_processingWidget->getSubtractFiltered());  // 使用3x3的中值滤波
}

void MainWindow::onBrightnessChanged(int value)
{
    imageProcessor->adjustBrightness(value);
    m_processingWidget->displayImage(imageProcessor->getProcessedImage());
}

void MainWindow::onThresholdChanged(int value)
{
    // Implementation needed
}

void MainWindow::onContrastChanged(int value)
{
    // Implementation needed
}

void MainWindow::onSaturationChanged(int value)
{
    // Implementation needed
}

void MainWindow::onRChanged(int value)
{
    // Implementation needed
}

void MainWindow::onGChanged(int value)
{
    // Implementation needed
}

void MainWindow::onBChanged(int value)
{
    // Implementation needed
}

void MainWindow::onShowOriginal()
{
    if (!imageProcessor->getOriginalImage().isNull()) {
        imageProcessor->resetToOriginal();  // 重置处理图像为原图
        m_processingWidget->displayImage(imageProcessor->getProcessedImage());
    } else {
        QMessageBox::warning(this, tr("错误"), tr("没有原始图像！"));
    }
}

void MainWindow::onMouseClicked(const QPoint &pos, int grayValue, int r, int g, int b)
{
    updateStatusBar(pos, grayValue, r, g, b);
}

void MainWindow::onImageStatsUpdated(double meanValue)
{
    updateImageStats(meanValue);
}

void MainWindow::updateStatusBar(const QPoint &pos, int grayValue, int r, int g, int b)
{
    QString info = QString("Position: (%1, %2) | Gray: %3 | RGB: (%4, %5, %6)")
        .arg(pos.x())
        .arg(pos.y())
        .arg(grayValue)
        .arg(r)
        .arg(g)
        .arg(b);
    m_pixelInfoLabel->setText(info);
}

void MainWindow::updateImageStats(double meanValue)
{
    QString stats = QString("Mean Value: %1").arg(meanValue, 0, 'f', 2);
    m_meanValueLabel->setText(stats);
}

void MainWindow::onGammaChanged(int value)
{
    // 将滑块值转换为gamma值，范围从0.1到10
    double gamma = value / 10.0;
    int offset = m_processingWidget->getOffsetSlider()->value();
    imageProcessor->adjustGammaContrast(gamma, offset);
    m_processingWidget->displayImage(imageProcessor->getProcessedImage());
}

void MainWindow::onOffsetChanged(int value)
{
    int gamma = m_processingWidget->getGammaSlider()->value();
    imageProcessor->adjustGammaContrast(gamma / 10.0, value);
    m_processingWidget->displayImage(imageProcessor->getProcessedImage());
}

void MainWindow::onRgbToGrayChanged(bool checked)
{
    if (checked) {
        imageProcessor->convertToGrayscale();
        m_processingWidget->displayImage(imageProcessor->getProcessedImage());
    } else {
        imageProcessor->resetToOriginal();
        m_processingWidget->displayImage(imageProcessor->getProcessedImage());
    }
}

void MainWindow::onHistEqualClicked()
{
    imageProcessor->applyHistogramEqualization();
    m_processingWidget->displayImage(imageProcessor->getProcessedImage());
}

void MainWindow::onHistogramCalculated(const QVector<int> &histogram)
{
    // TODO: Implement histogram visualization
    // This could update a histogram widget or display the data in some way
    Q_UNUSED(histogram);
}

void MainWindow::onHistogramEqualization()
{
    if (!imageProcessor->getOriginalImage().isNull()) {
        imageProcessor->applyHistogramEqualization();
        m_processingWidget->displayImage(imageProcessor->getProcessedImage());
    } else {
        QMessageBox::warning(this, tr("错误"), tr("没有原始图像！"));
    }
}

void MainWindow::onHistogramStretching()
{
    if (!imageProcessor->getOriginalImage().isNull()) {
        imageProcessor->applyHistogramStretching();
        m_processingWidget->displayImage(imageProcessor->getProcessedImage());
    } else {
        QMessageBox::warning(this, tr("错误"), tr("没有原始图像！"));
    }
}


