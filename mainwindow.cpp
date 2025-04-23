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
    try {
        qDebug() << "Initializing MainWindow...";

        // Initialize member variables
        m_processingWidget = new ProcessingWidget(this);
        if (!m_processingWidget) {
            throw std::runtime_error("Failed to create ProcessingWidget");
        }
        qDebug() << "ProcessingWidget created";

        imageProcessor = new ImageProcessor(this);
        if (!imageProcessor) {
            throw std::runtime_error("Failed to create ImageProcessor");
        }
        qDebug() << "ImageProcessor created";

        m_statusLabel = new QLabel(this);
        m_pixelInfoLabel = new QLabel(this);
        m_meanValueLabel = new QLabel(this);

        if (!m_statusLabel || !m_pixelInfoLabel || !m_meanValueLabel) {
            throw std::runtime_error("Failed to create status bar labels");
        }
        qDebug() << "Status bar labels created";

        setupUI();
        qDebug() << "UI setup completed";

        setupConnections();
        qDebug() << "Connections setup completed";

        createMenuBar();
        qDebug() << "Menu bar created";

        setupStatusBar();
        qDebug() << "Status bar setup completed";

        qDebug() << "MainWindow initialization completed successfully";
    } catch (const std::exception& e) {
        qCritical() << "Error in MainWindow constructor:" << e.what();
        throw; // 重新抛出异常以便在main中捕获
    }
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
    if (m_processingWidget->getSelectButton()) {
        connect(m_processingWidget->getSelectButton(), &QPushButton::clicked, this, &MainWindow::onSelectImage);
    }
    if (m_processingWidget->getSelectFolderButton()) {
        connect(m_processingWidget->getSelectFolderButton(), &QPushButton::clicked, this, &MainWindow::onSelectFolder);
    }
    if (m_processingWidget->getSaveButton()) {
        connect(m_processingWidget->getSaveButton(), &QPushButton::clicked, this, &MainWindow::onSaveImage);
    }
    if (m_processingWidget->getShowOriginalButton()) {
        connect(m_processingWidget->getShowOriginalButton(), &QPushButton::clicked, this, &MainWindow::onShowOriginal);
    }
    if (m_processingWidget->getFlipHButton()) {
        connect(m_processingWidget->getFlipHButton(), &QPushButton::clicked, this, &MainWindow::onFlipHorizontal);
    }
    if (m_processingWidget->getFlipVButton()) {
        connect(m_processingWidget->getFlipVButton(), &QPushButton::clicked, this, &MainWindow::onFlipVertical);
    }
    if (m_processingWidget->getMeanFilterButton()) {
        connect(m_processingWidget->getMeanFilterButton(), &QPushButton::clicked, this, &MainWindow::onMeanFilter);
    }
    if (m_processingWidget->getGaussianFilterButton()) {
        connect(m_processingWidget->getGaussianFilterButton(), &QPushButton::clicked, this, &MainWindow::onGaussianFilter);
    }
    if (m_processingWidget->getMedianFilterButton()) {
        connect(m_processingWidget->getMedianFilterButton(), &QPushButton::clicked, this, &MainWindow::onMedianFilter);
    }

    // 连接RGB转灰度复选框
    if (m_processingWidget->getRgbToGrayCheckBox()) {
        connect(m_processingWidget->getRgbToGrayCheckBox(), &QCheckBox::stateChanged, this, &MainWindow::onRgbToGrayChanged);
    }

    // 连接滑块信号
    if (m_processingWidget->getBrightnessSlider()) {
        connect(m_processingWidget->getBrightnessSlider(), &QSlider::valueChanged, this, &MainWindow::onBrightnessChanged);
    }
    if (m_processingWidget->getGammaSlider()) {
        connect(m_processingWidget->getGammaSlider(), &QSlider::valueChanged, this, &MainWindow::onGammaChanged);
    }
    if (m_processingWidget->getOffsetSlider()) {
        connect(m_processingWidget->getOffsetSlider(), &QSlider::valueChanged, this, &MainWindow::onOffsetChanged);
    }

    // 连接图像处理信号
    connect(imageProcessor, &ImageProcessor::imageLoaded, this, &MainWindow::onImageLoaded);
    connect(imageProcessor, &ImageProcessor::imageProcessed, this, &MainWindow::onImageProcessed);
    connect(imageProcessor, &ImageProcessor::error, this, &MainWindow::onError);
    
    // 连接鼠标信号
    connect(m_processingWidget, &ProcessingWidget::mouseClicked, this, &MainWindow::onMouseClicked);
    connect(m_processingWidget, &ProcessingWidget::mouseMoved, this, &MainWindow::onMouseMoved);
    connect(m_processingWidget, &ProcessingWidget::imageStatsUpdated, this, &MainWindow::onImageStatsUpdated);
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
    try {
        if (!imageProcessor) {
            qDebug() << "Error: imageProcessor is null";
            return;
        }
        
        bool subtractFiltered = m_processingWidget ? m_processingWidget->getSubtractFiltered() : false;
        imageProcessor->applyMeanFilter(3, subtractFiltered);  // 使用3x3的均值滤波
    } catch (const std::exception& e) {
        qDebug() << "Error in onMeanFilter:" << e.what();
        QMessageBox::warning(this, tr("错误"), tr("均值滤波处理时出错：%1").arg(e.what()));
    } catch (...) {
        qDebug() << "Unknown error in onMeanFilter";
        QMessageBox::warning(this, tr("错误"), tr("均值滤波处理时出现未知错误"));
    }
}

void MainWindow::onGaussianFilter()
{
    try {
        if (!imageProcessor) {
            qDebug() << "Error: imageProcessor is null";
            return;
        }
        
        bool subtractFiltered = m_processingWidget ? m_processingWidget->getSubtractFiltered() : false;
        imageProcessor->applyGaussianFilter(3, 1.0, subtractFiltered);  // 使用3x3的高斯滤波，sigma=1.0
    } catch (const std::exception& e) {
        qDebug() << "Error in onGaussianFilter:" << e.what();
        QMessageBox::warning(this, tr("错误"), tr("高斯滤波处理时出错：%1").arg(e.what()));
    } catch (...) {
        qDebug() << "Unknown error in onGaussianFilter";
        QMessageBox::warning(this, tr("错误"), tr("高斯滤波处理时出现未知错误"));
    }
}

void MainWindow::onMedianFilter()
{
    try {
        if (!imageProcessor) {
            qDebug() << "Error: imageProcessor is null";
            return;
        }
        
        bool subtractFiltered = m_processingWidget ? m_processingWidget->getSubtractFiltered() : false;
        imageProcessor->applyMedianFilter(3, subtractFiltered);  // 使用3x3的中值滤波
    } catch (const std::exception& e) {
        qDebug() << "Error in onMedianFilter:" << e.what();
        QMessageBox::warning(this, tr("错误"), tr("中值滤波处理时出错：%1").arg(e.what()));
    } catch (...) {
        qDebug() << "Unknown error in onMedianFilter";
        QMessageBox::warning(this, tr("错误"), tr("中值滤波处理时出现未知错误"));
    }
}

void MainWindow::onBrightnessChanged(int value)
{
    // 如果RGB转灰度复选框被勾选，应用变换到原始灰度图
    if (m_processingWidget->getRgbToGrayCheckBox()->isChecked()) {
        // 恢复到原始灰度图
        imageProcessor->restoreGrayscaleImage();
        
        // 获取当前偏移量b的值
        int offsetValue = m_processingWidget->getOffsetSlider()->value();
        
        // 应用线性变换
        imageProcessor->applyLinearTransform(value, offsetValue);
        
        // 获取当前的Gamma值，如果需要，应用Gamma校正
        double gamma = m_processingWidget->getGammaSlider()->value() / 10.0;
        if (fabs(gamma - 1.0) > 0.01) {
            imageProcessor->adjustGammaContrast(gamma, 0);
        }
        
        m_processingWidget->displayImage(imageProcessor->getProcessedImage());
    } else {
        // 正常应用线性变换
        int offsetValue = m_processingWidget->getOffsetSlider()->value();
        imageProcessor->applyLinearTransform(value, offsetValue);
        m_processingWidget->displayImage(imageProcessor->getProcessedImage());
    }
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
    // 如果RGB转灰度复选框被勾选，应用变换到原始灰度图
    if (m_processingWidget->getRgbToGrayCheckBox()->isChecked()) {
        // 恢复到原始灰度图
        imageProcessor->restoreGrayscaleImage();
        
        // 获取当前线性变换参数
        int brightnessValue = m_processingWidget->getBrightnessSlider()->value();
        int offsetValue = m_processingWidget->getOffsetSlider()->value();
        
        // 如果有线性变换，先应用线性变换
        if (brightnessValue != 0 || offsetValue != 0) {
            imageProcessor->applyLinearTransform(brightnessValue, offsetValue);
        }
        
        // 应用Gamma校正
        double gamma = value / 10.0;
        imageProcessor->adjustGammaContrast(gamma, 0);
        
        m_processingWidget->displayImage(imageProcessor->getProcessedImage());
    } else {
        // 正常应用Gamma校正
        double gamma = value / 10.0;
        int offset = m_processingWidget->getOffsetSlider()->value();
        imageProcessor->adjustGammaContrast(gamma, offset);
        m_processingWidget->displayImage(imageProcessor->getProcessedImage());
    }
}

void MainWindow::onOffsetChanged(int value)
{
    // 如果RGB转灰度复选框被勾选，应用变换到原始灰度图
    if (m_processingWidget->getRgbToGrayCheckBox()->isChecked()) {
        // 恢复到原始灰度图
        imageProcessor->restoreGrayscaleImage();
        
        // 获取当前系数k的滑块值
        int brightnessValue = m_processingWidget->getBrightnessSlider()->value();
        
        // 应用线性变换
        imageProcessor->applyLinearTransform(brightnessValue, value);
        
        // 获取当前的Gamma值，如果需要，应用Gamma校正
        double gamma = m_processingWidget->getGammaSlider()->value() / 10.0;
        if (fabs(gamma - 1.0) > 0.01) {
            imageProcessor->adjustGammaContrast(gamma, 0);
        }
        
        m_processingWidget->displayImage(imageProcessor->getProcessedImage());
    } else {
        // 正常应用线性变换
        int brightnessValue = m_processingWidget->getBrightnessSlider()->value();
        imageProcessor->applyLinearTransform(brightnessValue, value);
        m_processingWidget->displayImage(imageProcessor->getProcessedImage());
    }
}

void MainWindow::onRgbToGrayChanged(bool checked)
{
    if (checked) {
        // 先恢复原图
        imageProcessor->resetToOriginal();
        
        // 转为灰度图并保存灰度图状态
        imageProcessor->convertToGrayscale();
        imageProcessor->saveGrayscaleImage();
        
        // 显示纯灰度图，不应用任何变换
        m_processingWidget->displayImage(imageProcessor->getProcessedImage());
        
        // 重置所有滑块到初始值，这样不会自动触发变换
        m_processingWidget->getBrightnessSlider()->blockSignals(true);
        m_processingWidget->getOffsetSlider()->blockSignals(true);
        m_processingWidget->getGammaSlider()->blockSignals(true);
        
        m_processingWidget->getBrightnessSlider()->setValue(0);
        m_processingWidget->getOffsetSlider()->setValue(0);
        m_processingWidget->getGammaSlider()->setValue(10); // Gamma=1.0
        
        m_processingWidget->getBrightnessSlider()->blockSignals(false);
        m_processingWidget->getOffsetSlider()->blockSignals(false);
        m_processingWidget->getGammaSlider()->blockSignals(false);
        
        // 重置值标签显示
        m_processingWidget->resetValueLabels();
    } else {
        // 恢复原图
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

void MainWindow::onConvertToGrayscale()
{
    imageProcessor->convertToGrayscale();
    m_processingWidget->displayImage(imageProcessor->getProcessedImage());
}

// 应用所有当前的变换（在勾选RGB转灰度时使用）
void MainWindow::applyCurrentTransformations()
{
    // 恢复到原始灰度图
    imageProcessor->restoreGrayscaleImage();
    
    // 获取当前的线性变换参数
    int brightnessValue = m_processingWidget->getBrightnessSlider()->value();
    int offsetValue = m_processingWidget->getOffsetSlider()->value();
    
    // 如果需要，应用线性变换
    if (brightnessValue != 0 || offsetValue != 0) {
        imageProcessor->applyLinearTransform(brightnessValue, offsetValue);
    }
    
    // 获取当前的Gamma校正参数
    double gamma = m_processingWidget->getGammaSlider()->value() / 10.0;
    
    // 如果需要，应用Gamma校正（Gamma不等于1.0时才需要）
    if (fabs(gamma - 1.0) > 0.01) {
        imageProcessor->adjustGammaContrast(gamma, 0);
    }
    
    // 显示处理后的图像
    m_processingWidget->displayImage(imageProcessor->getProcessedImage());
}

void MainWindow::onMouseMoved(const QPoint &pos, int grayValue)
{
    if (m_pixelInfoLabel) {
        QColor pixelColor;
        if (!imageProcessor->getProcessedImage().isNull()) {
            pixelColor = imageProcessor->getProcessedImage().pixelColor(pos);
        } else {
            pixelColor = QColor(grayValue, grayValue, grayValue);
        }
        
        QString info = QString("Position: (%1, %2) | Gray: %3 | RGB: (%4, %5, %6)")
            .arg(pos.x())
            .arg(pos.y())
            .arg(grayValue)
            .arg(pixelColor.red())
            .arg(pixelColor.green())
            .arg(pixelColor.blue());
        m_pixelInfoLabel->setText(info);
    }
}


