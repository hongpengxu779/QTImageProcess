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
        QGroupBox *gbROI = new QGroupBox(tr("ROI操作"));
        
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
        
        // 添加ROI操作组
        auto *vROI = new QVBoxLayout(gbROI);
        btnRectangleROI = new QPushButton(tr("矩形选择"));
        btnApplyROI = new QPushButton(tr("应用ROI"));
        
        btnRectangleROI->setFixedSize(buttonSize);
        btnApplyROI->setFixedSize(buttonSize);
        
        vROI->addWidget(btnRectangleROI);
        vROI->addWidget(btnApplyROI);
        vROI->addStretch();

        // 将分组框添加到左侧布局
        vLeft->addWidget(gbFile);
        vLeft->addWidget(gbFlip);
        vLeft->addWidget(gbFilter);
        vLeft->addWidget(gbROI);
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

void ProcessingWidget::mousePressEvent(QMouseEvent *event)
{
    try {
        // 如果没有图像或imageLabel不存在，不处理
        if (m_currentImage.isNull() || !imageLabel) {
            return;
        }

        // 获取当前鼠标位置相对于imageLabel的坐标
        QPoint mousePos = imageLabel->mapFrom(this, event->pos());

        // 获取图像标签的尺寸
        QSize labelSize = imageLabel->size();

        // 计算图像缩放尺寸（保持纵横比）
        QSize scaledImageSize;
        double aspectRatio = static_cast<double>(m_currentImage.width()) / m_currentImage.height();

        if (labelSize.width() / aspectRatio <= labelSize.height()) {
            // 宽度限制
            scaledImageSize.setWidth(labelSize.width());
            scaledImageSize.setHeight(static_cast<int>(labelSize.width() / aspectRatio));
        } else {
            // 高度限制
            scaledImageSize.setHeight(labelSize.height());
            scaledImageSize.setWidth(static_cast<int>(labelSize.height() * aspectRatio));
        }

        // 计算图像在标签中的位置（居中显示）
        int offsetX = (labelSize.width() - scaledImageSize.width()) / 2;
        int offsetY = (labelSize.height() - scaledImageSize.height()) / 2;

        // 计算鼠标位置相对于图像左上角的偏移
        int relX = mousePos.x() - offsetX;
        int relY = mousePos.y() - offsetY;

        // 检查鼠标是否在图像范围内
        if (relX >= 0 && relX < scaledImageSize.width() && relY >= 0 && relY < scaledImageSize.height()) {
            // 计算缩放系数
            double scaleX = static_cast<double>(m_currentImage.width()) / scaledImageSize.width();
            double scaleY = static_cast<double>(m_currentImage.height()) / scaledImageSize.height();

            // 计算原始图像中的像素坐标
            int imageX = static_cast<int>(relX * scaleX);
            int imageY = static_cast<int>(relY * scaleY);

            // 确保坐标在图像范围内
            imageX = qBound(0, imageX, m_currentImage.width() - 1);
            imageY = qBound(0, imageY, m_currentImage.height() - 1);

            // 根据当前模式选择不同的响应方式
            if (!m_isROIMode) {
                // 正常坐标显示模式
                if (event->button() == Qt::LeftButton) {
                    // 获取像素信息
                    QColor pixelColor = m_currentImage.pixelColor(imageX, imageY);
                    int r = pixelColor.red();
                    int g = pixelColor.green();
                    int b = pixelColor.blue();
                    int grayValue = qGray(r, g, b);

                    // 发送信号
                    emit mouseClicked(QPoint(imageX, imageY), grayValue, r, g, b);
                }
            } else {
                // ROI选择模式
                if (event->button() == Qt::LeftButton) {
                    // 当前只处理矩形选择
                    m_currentROIMode = ROISelectionMode::Rectangle;
                    
                    // 保存图像坐标系中的起始点
                    m_selectionStart = QPoint(imageX, imageY);
                    m_selectionCurrent = QPoint(imageX, imageY);
                    m_selectionInProgress = true;
                    
                    // 重绘界面
                    update();
                }
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "Error in mousePressEvent:" << e.what();
    }
}

void ProcessingWidget::mouseMoveEvent(QMouseEvent *event)
{
    try {
        // 如果没有图像或imageLabel不存在，不处理
        if (m_currentImage.isNull() || !imageLabel) {
            return;
        }

        // 获取当前鼠标位置相对于imageLabel的坐标
        QPoint mousePos = imageLabel->mapFrom(this, event->pos());

        // 获取图像标签的尺寸
        QSize labelSize = imageLabel->size();

        // 计算图像缩放尺寸（保持纵横比）
        QSize scaledImageSize;
        double aspectRatio = static_cast<double>(m_currentImage.width()) / m_currentImage.height();

        if (labelSize.width() / aspectRatio <= labelSize.height()) {
            // 宽度限制
            scaledImageSize.setWidth(labelSize.width());
            scaledImageSize.setHeight(static_cast<int>(labelSize.width() / aspectRatio));
        } else {
            // 高度限制
            scaledImageSize.setHeight(labelSize.height());
            scaledImageSize.setWidth(static_cast<int>(labelSize.height() * aspectRatio));
        }

        // 计算图像在标签中的位置（居中显示）
        int offsetX = (labelSize.width() - scaledImageSize.width()) / 2;
        int offsetY = (labelSize.height() - scaledImageSize.height()) / 2;

        // 计算鼠标位置相对于图像左上角的偏移
        int relX = mousePos.x() - offsetX;
        int relY = mousePos.y() - offsetY;

        // 检查鼠标是否在图像范围内
        if (relX >= 0 && relX < scaledImageSize.width() && relY >= 0 && relY < scaledImageSize.height()) {
            // 计算缩放系数
            double scaleX = static_cast<double>(m_currentImage.width()) / scaledImageSize.width();
            double scaleY = static_cast<double>(m_currentImage.height()) / scaledImageSize.height();

            // 计算原始图像中的像素坐标
            int imageX = static_cast<int>(relX * scaleX);
            int imageY = static_cast<int>(relY * scaleY);

            // 确保坐标在图像范围内
            imageX = qBound(0, imageX, m_currentImage.width() - 1);
            imageY = qBound(0, imageY, m_currentImage.height() - 1);

            // 根据当前模式选择不同的响应方式
            if (!m_isROIMode) {
                // 正常坐标显示模式
                // 获取像素灰度值
                QColor pixelColor = m_currentImage.pixelColor(imageX, imageY);
                int grayValue = qGray(pixelColor.red(), pixelColor.green(), pixelColor.blue());

                // 发送坐标和灰度值
                emit mouseMoved(QPoint(imageX, imageY), grayValue);
            } else {
                // ROI选择模式
                if (m_selectionInProgress && m_currentROIMode == ROISelectionMode::Rectangle) {
                    // 更新矩形的当前点(图像坐标系)
                    m_selectionCurrent = QPoint(imageX, imageY);
                    
                    // 创建ROI矩形
                    m_rectangleROI = QRect(m_selectionStart, m_selectionCurrent).normalized();
                    
                    // 确保矩形完全在图像内
                    m_rectangleROI = m_rectangleROI.intersected(QRect(0, 0, m_currentImage.width(), m_currentImage.height()));
                    
                    // 重绘
                    update();
                }
            }
        } else if (m_isROIMode && m_selectionInProgress && m_currentROIMode == ROISelectionMode::Rectangle) {
            // 如果鼠标在图像外但正在进行选择，我们需要限制当前点
            // 计算缩放系数
            double scaleX = static_cast<double>(m_currentImage.width()) / scaledImageSize.width();
            double scaleY = static_cast<double>(m_currentImage.height()) / scaledImageSize.height();

            // 限制relX和relY在有效范围内
            relX = qBound(0, relX, scaledImageSize.width() - 1);
            relY = qBound(0, relY, scaledImageSize.height() - 1);
            
            // 转换为图像坐标
            int imageX = static_cast<int>(relX * scaleX);
            int imageY = static_cast<int>(relY * scaleY);
            
            // 确保坐标在图像范围内
            imageX = qBound(0, imageX, m_currentImage.width() - 1);
            imageY = qBound(0, imageY, m_currentImage.height() - 1);
            
            // 更新选择
            m_selectionCurrent = QPoint(imageX, imageY);
            
            // 更新矩形
            m_rectangleROI = QRect(m_selectionStart, m_selectionCurrent).normalized();
            
            // 确保矩形完全在图像内
            m_rectangleROI = m_rectangleROI.intersected(QRect(0, 0, m_currentImage.width(), m_currentImage.height()));
            
            // 重绘
            update();
        }
    } catch (const std::exception& e) {
        qDebug() << "Error in mouseMoveEvent:" << e.what();
    }
}

void ProcessingWidget::mouseReleaseEvent(QMouseEvent *event)
{
    try {
        // 如果不是在ROI选择模式或没有正在进行的选择，直接返回
        if (!m_isROIMode || !m_selectionInProgress) {
            return;
        }

        if (event->button() == Qt::LeftButton) {
            // 只处理矩形选择完成
            if (m_currentROIMode == ROISelectionMode::Rectangle) {
                m_selectionInProgress = false;
                
                // 确保矩形至少有一定大小
                if (m_rectangleROI.width() > 5 && m_rectangleROI.height() > 5) {
                    // 启用应用按钮
                    if (btnApplyROI) {
                        btnApplyROI->setEnabled(true);
                    }
                    
                    // 矩形已经在图像坐标系中，通过intersected确保在图像范围内
                    m_rectangleROI = m_rectangleROI.intersected(QRect(0, 0, m_currentImage.width(), m_currentImage.height()));

                    // 打印调试信息
                    qDebug() << "ROI选择完成: " << m_rectangleROI << ", 图像尺寸: " 
                             << m_currentImage.width() << "x" << m_currentImage.height();
                } else {
                    // 如果选择太小，清除选择
                    m_rectangleROI = QRect();
                    if (btnApplyROI) {
                        btnApplyROI->setEnabled(false);
                    }
                }
                
                // 重绘界面
                update();
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "Error in mouseReleaseEvent:" << e.what();
    }
}

void ProcessingWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    
    // 如果没有图像或者不在ROI选择模式，不绘制ROI
    if (m_currentImage.isNull() || !imageLabel || !imageLabel->pixmap()) {
        return;
    }
    
    // 只在ROI模式下或有选定的ROI时绘制
    if ((m_isROIMode || !m_rectangleROI.isNull()) && m_rectangleROI.isValid()) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // 设置绘制参数
        QPen pen(Qt::red);
        pen.setWidth(2);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        
        QBrush brush(QColor(255, 0, 0, 40)); // 半透明红色
        painter.setBrush(brush);
        
        // 获取imageLabel在小部件中的位置
        QRect labelRect = imageLabel->geometry();
        
        // 需要将图像坐标系的ROI映射回Label坐标系
        if (!m_rectangleROI.isNull()) {
            // 获取图像标签的尺寸
            QSize labelSize = imageLabel->size();
            
            // 计算图像缩放尺寸（保持纵横比）
            QSize scaledImageSize;
            double aspectRatio = static_cast<double>(m_currentImage.width()) / m_currentImage.height();
            
            if (labelSize.width() / aspectRatio <= labelSize.height()) {
                // 宽度限制
                scaledImageSize.setWidth(labelSize.width());
                scaledImageSize.setHeight(static_cast<int>(labelSize.width() / aspectRatio));
            } else {
                // 高度限制
                scaledImageSize.setHeight(labelSize.height());
                scaledImageSize.setWidth(static_cast<int>(labelSize.height() * aspectRatio));
            }
            
            // 计算缩放系数
            double scaleX = static_cast<double>(scaledImageSize.width()) / m_currentImage.width();
            double scaleY = static_cast<double>(scaledImageSize.height()) / m_currentImage.height();
            
            // 计算图像在标签中的位置（居中显示）
            int offsetX = (labelSize.width() - scaledImageSize.width()) / 2;
            int offsetY = (labelSize.height() - scaledImageSize.height()) / 2;
            
            // 将图像坐标ROI转换为Label坐标
            QRect labelROI(
                static_cast<int>(m_rectangleROI.x() * scaleX) + offsetX + labelRect.x(),
                static_cast<int>(m_rectangleROI.y() * scaleY) + offsetY + labelRect.y(),
                static_cast<int>(m_rectangleROI.width() * scaleX),
                static_cast<int>(m_rectangleROI.height() * scaleY)
            );
            
            // 绘制ROI
            painter.drawRect(labelROI);
            
            // 如果不是在选择过程中，可以显示一些选择区域的信息
            if (!m_selectionInProgress) {
                // 显示尺寸信息
                QString sizeInfo = QString("%1 × %2").arg(m_rectangleROI.width()).arg(m_rectangleROI.height());
                painter.setPen(Qt::black);
                painter.drawText(labelROI.adjusted(5, 5, -5, -5), Qt::AlignTop | Qt::AlignLeft, sizeInfo);
            }
        }
    }
} 