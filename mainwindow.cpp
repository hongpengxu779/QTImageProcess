#include "mainwindow.h"
#include "ImageView/ProcessingWidget.h"
#include "HistogramDialog.h"
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QStatusBar>
#include <QDebug>
#include <QTimer>
#include <QSignalBlocker>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_processingWidget(nullptr)
    , imageProcessor(nullptr)
    , m_statusLabel(nullptr)
    , m_pixelInfoLabel(nullptr)
    , m_meanValueLabel(nullptr)
    , m_histogramDialog(nullptr)
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

        // Create histogram dialog (not shown by default)
        m_histogramDialog = new HistogramDialog(this);
        if (!m_histogramDialog) {
            throw std::runtime_error("Failed to create HistogramDialog");
        }
        qDebug() << "HistogramDialog created";

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
    delete m_histogramDialog;
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

    // 连接直方图复选框信号
    if (m_processingWidget->getShowHistogramCheckbox()) {
        connect(m_processingWidget->getShowHistogramCheckbox(), &QCheckBox::stateChanged, 
                this, [this](int state) { onShowHistogramChanged(state == Qt::Checked); });
    }
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
    
    // 更新直方图
    updateHistogramDialog();
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
    
    // 如果直方图对话框已打开，则更新直方图
    updateHistogramDialog();
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
    
    // 如果直方图对话框已打开，则更新直方图
    updateHistogramDialog();
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
    
    // 如果直方图对话框已打开，则更新直方图
    updateHistogramDialog();
}

void MainWindow::onRgbToGrayChanged(bool checked)
{
    try {
        if (!imageProcessor) {
            qDebug() << "错误: imageProcessor 为空";
            return;
        }

        // 检查是否有图像加载
        if (imageProcessor->getOriginalImage().isNull()) {
            qDebug() << "错误: 没有加载图像";
            QMessageBox::warning(this, tr("警告"), tr("请先加载图像再执行此操作"));
            
            // 如果没有图像，取消选中复选框
            if (m_processingWidget && m_processingWidget->getRgbToGrayCheckBox()) {
                QSignalBlocker blocker(m_processingWidget->getRgbToGrayCheckBox()); // 阻止信号循环
                m_processingWidget->getRgbToGrayCheckBox()->setChecked(false);
            }
            return;
        }
        
        qDebug() << "RGB 转灰度状态改变: " << (checked ? "选中" : "取消选中");
        
        // 使用 try-catch 分别处理两种转换方式
        try {
            if (checked) {
                // 转换为灰度图
                qDebug() << "转换图像为灰度...";
                imageProcessor->convertToGrayscale();
                qDebug() << "保存灰度图像状态...";
                imageProcessor->saveGrayscaleImage();
            } else {
                // 恢复到原始图像
                qDebug() << "恢复到原始图像...";
                imageProcessor->resetToOriginal();
            }
            
            // 显示处理后的图像
            qDebug() << "更新显示...";
            m_processingWidget->displayImage(imageProcessor->getProcessedImage());
            
            // 更新直方图
            qDebug() << "更新直方图...";
            updateHistogramDialog();
            
            qDebug() << "RGB 转灰度操作完成";
        } catch (const std::exception& e) {
            qDebug() << "在图像转换过程中出错:" << e.what();
            QMessageBox::warning(this, tr("错误"), tr("图像处理失败: %1").arg(e.what()));
            
            // 在失败时恢复复选框状态
            if (m_processingWidget && m_processingWidget->getRgbToGrayCheckBox()) {
                QSignalBlocker blocker(m_processingWidget->getRgbToGrayCheckBox()); // 阻止信号循环
                m_processingWidget->getRgbToGrayCheckBox()->setChecked(!checked);
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "onRgbToGrayChanged 发生异常:" << e.what();
        QMessageBox::critical(this, tr("严重错误"), tr("处理过程中发生未知错误: %1").arg(e.what()));
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

void MainWindow::onShowHistogramChanged(bool show)
{
    try {
        if (show) {
            if (!m_histogramDialog) {
                m_histogramDialog = new HistogramDialog(this);
            }
            
            // 先更新直方图数据，再显示对话框
            if (imageProcessor && !imageProcessor->getProcessedImage().isNull()) {
                // 使用当前处理后的图像
                QImage currentImage = imageProcessor->getProcessedImage();
                
                // 如果RGB转灰度被勾选，则先转换成灰度图
                if (m_processingWidget && m_processingWidget->getRgbToGrayCheckBox() && 
                    m_processingWidget->getRgbToGrayCheckBox()->isChecked()) {
                    // 确保使用灰度图像版本
                    QImage grayImage = currentImage.convertToFormat(QImage::Format_Grayscale8);
                    m_histogramDialog->updateHistogram(grayImage);
                    qDebug() << "显示灰度直方图：使用灰度图像版本";
                } else {
                    // 使用原始图像
                    m_histogramDialog->updateHistogram(currentImage);
                    qDebug() << "显示灰度直方图：使用原始图像";
                }
            } else {
                qDebug() << "无法更新直方图：图像为空";
            }
            
            // 显示对话框
            m_histogramDialog->show();
            m_histogramDialog->raise(); // 确保窗口在前台显示
            m_histogramDialog->activateWindow();
        } else if (m_histogramDialog) {
            m_histogramDialog->hide();
        }
    } catch (const std::exception& e) {
        qDebug() << "显示直方图对话框时出错:" << e.what();
    }
}

void MainWindow::updateHistogramDialog()
{
    try {
        if (m_histogramDialog && m_histogramDialog->isVisible() && imageProcessor) {
            QImage currentImage = imageProcessor->getProcessedImage();
            
            if (!currentImage.isNull()) {
                // 使用灰度图像版本还是原图取决于RGB转灰度是否被勾选
                if (m_processingWidget && m_processingWidget->getRgbToGrayCheckBox() && 
                    m_processingWidget->getRgbToGrayCheckBox()->isChecked()) {
                    // 转换为灰度图像
                    QImage grayImage = currentImage.convertToFormat(QImage::Format_Grayscale8);
                    m_histogramDialog->updateHistogram(grayImage);
                    qDebug() << "更新直方图：使用灰度图像";
                } else {
                    // 使用当前图像
                    m_histogramDialog->updateHistogram(currentImage);
                    qDebug() << "更新直方图：使用原始图像";
                }
            } else {
                qDebug() << "无法更新直方图：当前图像为空";
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "更新直方图对话框时出错:" << e.what();
    }
}


