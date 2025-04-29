#include "ImageProcessor.h"
#include <QImage>
#include <QColor>
#include <cmath>
#include <algorithm>
#include <vector>
#include <opencv2/opencv.hpp>
#include <QDebug>

ImageProcessor::ImageProcessor(QObject *parent)
    : QObject(parent), kernelSize(3)  // 默认卷积核大小为3
{
}

ImageProcessor::~ImageProcessor() = default;

bool ImageProcessor::loadImage(const QString &filePath)
{
    if (filePath.isEmpty()) {
        emit error(tr("文件路径为空"));
        emit imageLoaded(false);
        return false;
    }

    QImage image(filePath);
    if (image.isNull()) {
        emit error(tr("无法加载图片：%1").arg(filePath));
        emit imageLoaded(false);
        return false;
    }

    originalImage = image;
    processedImage = image;
    emit imageLoaded(true);
    return true;
}

const QImage& ImageProcessor::getOriginalImage() const
{
    return originalImage;
}

const QImage& ImageProcessor::getProcessedImage() const
{
    return processedImage;
}

void ImageProcessor::setProcessedImage(const QImage &image)
{
    if (!image.isNull()) {
        processedImage = image.copy(); // 创建一个深拷贝
        emit imageProcessed(); // 发送图像已处理信号
    }
}

void ImageProcessor::resetToOriginal()
{
    if (!originalImage.isNull()) {
        processedImage = originalImage;
        emit imageProcessed();
    }
}

void ImageProcessor::flipHorizontal()
{
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        return;
    }
    cv::Mat mat = QImageToMat(processedImage);
    cv::flip(mat, mat, 1);  // 1 表示水平翻转
    processedImage = MatToQImage(mat);
    emit imageProcessed();
}

void ImageProcessor::flipVertical()
{
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        return;
    }
    cv::Mat mat = QImageToMat(processedImage);
    cv::flip(mat, mat, 0);  // 0 表示垂直翻转
    processedImage = MatToQImage(mat);
    emit imageProcessed();
}

void ImageProcessor::applyMeanFilter(int kernelSize, bool subtractFromOriginal)
{
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        return;
    }

    qDebug() << "\n====== MEAN FILTER START ======";
    qDebug() << "Mean Filter - Input image format:" << processedImage.format()
             << "Size:" << processedImage.width() << "x" << processedImage.height()
             << "Depth:" << processedImage.depth()
             << "Is null:" << processedImage.isNull();

    if (kernelSize % 2 == 0) {
        emit error(tr("核大小必须是奇数"));
        qDebug() << "Error: 核大小必须是奇数, kernelSize=" << kernelSize;
        qDebug() << "====== MEAN FILTER ERROR END (INVALID KERNEL SIZE) ======\n";
        return;
    }

    if (kernelSize < 3 || kernelSize > 31) {
        emit error(tr("核大小必须在3到31之间"));
        qDebug() << "Error: 核大小必须在3到31之间, kernelSize=" << kernelSize;
        qDebug() << "====== MEAN FILTER ERROR END (INVALID KERNEL SIZE RANGE) ======\n";
        return;
    }

    try {
        // 使用深拷贝确保安全
        QImage safeImage = processedImage.copy();
        
        qDebug() << "Step 1: Converting QImage to Mat safely...";
        cv::Mat mat;
        
        // 使用更安全的方式进行图像格式转换
        switch (safeImage.format()) {
            case QImage::Format_Grayscale8:
                qDebug() << "Converting grayscale image";
                // 深拷贝图像数据到OpenCV矩阵
                {
                    mat = cv::Mat(safeImage.height(), safeImage.width(), CV_8UC1);
                    // 逐行复制数据而不是直接使用内存映射
                    for (int y = 0; y < safeImage.height(); ++y) {
                        const uchar* scanLine = safeImage.constScanLine(y);
                        uchar* matLine = mat.ptr<uchar>(y);
                        std::memcpy(matLine, scanLine, safeImage.width());
                    }
                }
                break;
                
            case QImage::Format_RGB32:
            case QImage::Format_ARGB32:
            case QImage::Format_ARGB32_Premultiplied:
                qDebug() << "Converting RGB32/ARGB32 image";
                // 转换为BGR格式（OpenCV使用BGR而不是RGB）
                {
                    mat = cv::Mat(safeImage.height(), safeImage.width(), CV_8UC4);
                    // 逐行复制数据
                    for (int y = 0; y < safeImage.height(); ++y) {
                        const uchar* scanLine = safeImage.constScanLine(y);
                        uchar* matLine = mat.ptr<uchar>(y);
                        std::memcpy(matLine, scanLine, safeImage.width() * 4);
                    }
                    // 转换为3通道BGR格式
                    cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
                }
                break;
                
            default:
                qDebug() << "Converting other format to RGB32 first";
                // 转换为标准格式再处理
                safeImage = safeImage.convertToFormat(QImage::Format_RGB32);
                mat = cv::Mat(safeImage.height(), safeImage.width(), CV_8UC4);
                // 逐行复制数据
                for (int y = 0; y < safeImage.height(); ++y) {
                    const uchar* scanLine = safeImage.constScanLine(y);
                    uchar* matLine = mat.ptr<uchar>(y);
                    std::memcpy(matLine, scanLine, safeImage.width() * 4);
                }
                // 转换为3通道BGR格式
                cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
                break;
        }
        
        // 验证转换后的Mat
        if (mat.empty()) {
            throw std::runtime_error("Failed to convert QImage to Mat");
        }
        
        qDebug() << "Step 2: Mat conversion successful. Size:" << mat.rows << "x" << mat.cols 
                 << "channels:" << mat.channels() << "type:" << mat.type();
        
        // 应用均值滤波
        cv::Mat filteredMat;
        qDebug() << "Step 3: Applying mean filter with kernel size " << kernelSize;
        
        cv::Size kernelDim(kernelSize, kernelSize);
        cv::blur(mat, filteredMat, kernelDim);
        
        // 如果请求从原始图像中减去滤波后的图像
        if (subtractFromOriginal) {
            qDebug() << "Step 4: Subtracting filtered image from original";
            
            // 转换原始图像
            QImage safOriginal = originalImage.copy();
            cv::Mat originalMat;
            
            // 确保使用与上面相同的转换方法
            switch (safOriginal.format()) {
                case QImage::Format_Grayscale8:
                    originalMat = cv::Mat(safOriginal.height(), safOriginal.width(), CV_8UC1);
                    for (int y = 0; y < safOriginal.height(); ++y) {
                        const uchar* scanLine = safOriginal.constScanLine(y);
                        uchar* matLine = originalMat.ptr<uchar>(y);
                        std::memcpy(matLine, scanLine, safOriginal.width());
                    }
                    break;
                    
                case QImage::Format_RGB32:
                case QImage::Format_ARGB32:
                case QImage::Format_ARGB32_Premultiplied:
                    originalMat = cv::Mat(safOriginal.height(), safOriginal.width(), CV_8UC4);
                    for (int y = 0; y < safOriginal.height(); ++y) {
                        const uchar* scanLine = safOriginal.constScanLine(y);
                        uchar* matLine = originalMat.ptr<uchar>(y);
                        std::memcpy(matLine, scanLine, safOriginal.width() * 4);
                    }
                    cv::cvtColor(originalMat, originalMat, cv::COLOR_BGRA2BGR);
                    break;
                    
                default:
                    safOriginal = safOriginal.convertToFormat(QImage::Format_RGB32);
                    originalMat = cv::Mat(safOriginal.height(), safOriginal.width(), CV_8UC4);
                    for (int y = 0; y < safOriginal.height(); ++y) {
                        const uchar* scanLine = safOriginal.constScanLine(y);
                        uchar* matLine = originalMat.ptr<uchar>(y);
                        std::memcpy(matLine, scanLine, safOriginal.width() * 4);
                    }
                    cv::cvtColor(originalMat, originalMat, cv::COLOR_BGRA2BGR);
                    break;
            }
            
            // 检查尺寸是否匹配
            if (originalMat.size() != filteredMat.size() || originalMat.channels() != filteredMat.channels()) {
                qDebug() << "Error: Original and filtered images have different dimensions or channels";
                throw std::runtime_error("Image dimension or channel mismatch");
            }
            
            // 执行减法操作
            cv::subtract(originalMat, filteredMat, filteredMat);
        }
        
        // 转换回QImage
        qDebug() << "Step 5: Converting filtered Mat back to QImage";
        QImage result;
        
        // 根据通道数选择正确的转换方法
        if (filteredMat.channels() == 1) {
            // 灰度图像
            result = QImage(filteredMat.cols, filteredMat.rows, QImage::Format_Grayscale8);
            // 逐行复制数据
            for (int y = 0; y < filteredMat.rows; ++y) {
                const uchar* matLine = filteredMat.ptr<uchar>(y);
                uchar* resultLine = result.scanLine(y);
                std::memcpy(resultLine, matLine, filteredMat.cols);
            }
        } else {
            // 彩色图像 - 将BGR转为RGB
            cv::Mat rgbMat;
            cv::cvtColor(filteredMat, rgbMat, cv::COLOR_BGR2RGB);
            
            // 创建QImage并复制数据
            result = QImage(rgbMat.cols, rgbMat.rows, QImage::Format_RGB888);
            for (int y = 0; y < rgbMat.rows; ++y) {
                const uchar* matLine = rgbMat.ptr<uchar>(y);
                uchar* resultLine = result.scanLine(y);
                std::memcpy(resultLine, matLine, rgbMat.cols * 3);
            }
        }
        
        // 验证结果
        if (result.isNull()) {
            throw std::runtime_error("Failed to convert result to QImage");
        }
        
        // 设置处理后的图像并发出信号
        processedImage = result;
        qDebug() << "Mean filter completed successfully";
        qDebug() << "====== MEAN FILTER END ======\n";
        emit imageProcessed();
        
    } catch (const cv::Exception& e) {
        qDebug() << "OpenCV error:" << e.what();
        emit error(tr("OpenCV错误: %1").arg(e.what()));
        qDebug() << "====== MEAN FILTER ERROR END ======\n";
    } catch (const std::exception& e) {
        qDebug() << "Processing error:" << e.what();
        emit error(tr("处理错误: %1").arg(e.what()));
        qDebug() << "====== MEAN FILTER ERROR END ======\n";
    } catch (...) {
        qDebug() << "Unknown error occurred";
        emit error(tr("未知错误"));
        qDebug() << "====== MEAN FILTER ERROR END ======\n";
    }
}

void ImageProcessor::applyGaussianFilter(int kernelSize, double sigma, bool subtractFromOriginal)
{
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        return;
    }

    qDebug() << "\n====== GAUSSIAN FILTER START ======";
    qDebug() << "Gaussian Filter - Input image format:" << processedImage.format()
             << "Size:" << processedImage.width() << "x" << processedImage.height()
             << "Depth:" << processedImage.depth()
             << "Is null:" << processedImage.isNull();

    // 保存原始图像的副本用于处理
    QImage originalCopy = processedImage.copy();

    if (kernelSize % 2 == 0) {
        emit error(tr("核大小必须是奇数"));
        qDebug() << "Error: 核大小必须是奇数, kernelSize=" << kernelSize;
        qDebug() << "====== GAUSSIAN FILTER ERROR END (INVALID KERNEL SIZE) ======\n";
        return;
    }

    if (kernelSize < 3 || kernelSize > 31) {
        emit error(tr("核大小必须在3到31之间"));
        qDebug() << "Error: 核大小必须在3到31之间, kernelSize=" << kernelSize;
        qDebug() << "====== GAUSSIAN FILTER ERROR END (INVALID KERNEL SIZE RANGE) ======\n";
        return;
    }

    try {
        qDebug() << "Step 1: Converting QImage to Mat for Gaussian filter...";
        
        // 判断是否是灰度图
        bool isGrayscaleImage = (processedImage.format() == QImage::Format_Grayscale8);
        qDebug() << "Image is grayscale:" << isGrayscaleImage;
        
        cv::Mat mat;
        try {
            mat = QImageToMat(processedImage);
            if (mat.empty()) {
                qDebug() << "Error: QImageToMat returned empty matrix";
                emit error(tr("图像转换失败"));
                qDebug() << "====== GAUSSIAN FILTER ERROR END (CONVERSION FAILED) ======\n";
                return;
            }
        } catch (const std::exception& e) {
            qDebug() << "Exception during QImageToMat:" << e.what();
            emit error(tr("图像转换异常: %1").arg(e.what()));
            qDebug() << "====== GAUSSIAN FILTER ERROR END (CONVERSION EXCEPTION) ======\n";
            return;
        }
        
        qDebug() << "Step 2: Mat conversion successful. Size:" << mat.rows << "x" << mat.cols 
                 << "channels:" << mat.channels() << "type:" << mat.type() << "depth:" << mat.depth();

        // 检查图像尺寸是否合理
        if (mat.rows <= 0 || mat.cols <= 0) {
            qDebug() << "Error: Invalid image dimensions: " << mat.rows << "x" << mat.cols;
            emit error(tr("图像尺寸无效"));
            qDebug() << "====== GAUSSIAN FILTER ERROR END (INVALID DIMENSIONS) ======\n";
            return;
        }

        cv::Mat filteredMat;
        
        qDebug() << "Step 3: Starting Gaussian filtering operation based on image type";
        try {
            // 无论图像类型如何，我们都使用相同的高斯滤波过程
            cv::Size kernelDim(kernelSize, kernelSize);
            qDebug() << "Using kernel dimensions: " << kernelDim.width << "x" << kernelDim.height
                     << " and sigma=" << sigma;
            cv::GaussianBlur(mat, filteredMat, kernelDim, sigma);
            qDebug() << "Gaussian blur operation completed successfully";
            
            // 验证结果
            if (filteredMat.empty()) {
                qDebug() << "Error: Filtered mat is empty after applying Gaussian filter";
                throw std::runtime_error("Filtered mat is empty after applying Gaussian filter");
            }
            
            qDebug() << "Filtered mat: size=" << filteredMat.rows << "x" << filteredMat.cols
                     << " type=" << filteredMat.type() << " channels=" << filteredMat.channels();
        }
        catch (const cv::Exception& e) {
            qDebug() << "OpenCV error during Gaussian blur operation:" << e.what();
            emit error(tr("高斯滤波错误: %1").arg(e.what()));
            qDebug() << "====== GAUSSIAN FILTER ERROR END (BLUR ERROR) ======\n";
            return;
        }
        catch (const std::exception& e) {
            qDebug() << "Error during Gaussian blur:" << e.what();
            emit error(tr("高斯滤波处理错误: %1").arg(e.what()));
            qDebug() << "====== GAUSSIAN FILTER ERROR END (PROCESSING ERROR) ======\n";
            return;
        }
            
        qDebug() << "Step 4: Gaussian filter applied successfully. Result Mat size:" 
                 << filteredMat.rows << "x" << filteredMat.cols 
                 << "channels:" << filteredMat.channels()
                 << "type:" << filteredMat.type();

        if (subtractFromOriginal) {
            qDebug() << "Step 5: Subtracting filtered image from original (requested)";
            try {
                cv::Mat originalMat = QImageToMat(originalImage);
                if (originalMat.empty()) {
                    qDebug() << "Error: Original image conversion to Mat failed";
                    emit error(tr("原始图像转换失败"));
                    qDebug() << "====== GAUSSIAN FILTER ERROR END (ORIGINAL CONVERSION FAILED) ======\n";
                    return;
                }

                // 检查尺寸是否匹配
                if (originalMat.rows != filteredMat.rows || originalMat.cols != filteredMat.cols ||
                    originalMat.channels() != filteredMat.channels()) {
                    qDebug() << "Error: Dimension or channel mismatch for subtraction";
                    emit error(tr("原始图像与滤波图像尺寸或通道不匹配"));
                    qDebug() << "====== GAUSSIAN FILTER ERROR END (DIMENSION MISMATCH) ======\n";
                    return;
                }

                // 执行减法操作
                cv::subtract(originalMat, filteredMat, filteredMat);
                qDebug() << "Subtraction completed successfully";
            }
            catch (const std::exception& e) {
                qDebug() << "Error during subtraction:" << e.what();
                emit error(tr("图像相减操作错误: %1").arg(e.what()));
                qDebug() << "====== GAUSSIAN FILTER ERROR END (SUBTRACTION ERROR) ======\n";
                return;
            }
        }

        qDebug() << "Step 6: Converting filtered Mat back to QImage...";
        QImage newImage;

        try {
            // 转换回QImage
            newImage = MatToQImage(filteredMat);
            if (newImage.isNull()) {
                qDebug() << "Error: Failed to convert result to QImage";
                throw std::runtime_error("Failed to convert result to QImage");
            }
            
            qDebug() << "Created QImage: size=" << newImage.width() << "x" << newImage.height()
                     << " format=" << newImage.format() << " is null=" << newImage.isNull();
        } catch (const std::exception& e) {
            qDebug() << "Exception during conversion to QImage: " << e.what();
            emit error(tr("转换为QImage时出错: %1").arg(e.what()));
            qDebug() << "====== GAUSSIAN FILTER ERROR END (QIMAGE CONVERSION ERROR) ======\n";
            return;
        }

        // 设置处理后的图像
        processedImage = newImage;
        
        qDebug() << "Step 7: Gaussian filter completed successfully";
        qDebug() << "====== GAUSSIAN FILTER END ======\n";
        
        // 发出图像处理完成的信号
        emit imageProcessed();
    } catch (const cv::Exception& e) {
        qDebug() << "Uncaught OpenCV error in Gaussian filter:" << e.what();
        emit error(tr("OpenCV错误: %1").arg(e.what()));
        qDebug() << "====== GAUSSIAN FILTER ERROR END ======\n";
    } catch (const std::exception& e) {
        qDebug() << "Uncaught general error in Gaussian filter:" << e.what();
        emit error(tr("处理错误: %1").arg(e.what()));
        qDebug() << "====== GAUSSIAN FILTER ERROR END ======\n";
    } catch (...) {
        qDebug() << "Unknown error in Gaussian filter";
        emit error(tr("未知错误"));
        qDebug() << "====== GAUSSIAN FILTER ERROR END ======\n";
    }
}

void ImageProcessor::applyMedianFilter(int kernelSize, bool subtractFromOriginal)
{
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        return;
    }

    qDebug() << "\n====== MEDIAN FILTER START ======";
    // 添加调试信息以检查处理前的图像
    qDebug() << "Median Filter - Input image format:" << processedImage.format()
             << "Size:" << processedImage.width() << "x" << processedImage.height()
             << "Depth:" << processedImage.depth()
             << "Is null:" << processedImage.isNull();

    if (kernelSize % 2 == 0 || kernelSize < 3 || kernelSize > 31) {
        emit error(tr("核大小必须是3到31之间的奇数"));
        qDebug() << "Error: 核大小必须是3到31之间的奇数, kernelSize=" << kernelSize;
        qDebug() << "====== MEDIAN FILTER ERROR END (INVALID KERNEL SIZE) ======\n";
        return;
    }

    try {
        qDebug() << "Step 1: Converting QImage to Mat for median filter...";
        
        // 判断是否是灰度图
        bool isGrayscaleImage = (processedImage.format() == QImage::Format_Grayscale8);
        qDebug() << "Image is grayscale:" << isGrayscaleImage;
        
        cv::Mat mat;
        try {
            mat = QImageToMat(processedImage);
            if (mat.empty()) {
                qDebug() << "Error: QImageToMat returned empty matrix";
                emit error(tr("图像转换失败"));
                qDebug() << "====== MEDIAN FILTER ERROR END (CONVERSION FAILED) ======\n";
                return;
            }
        } catch (const std::exception& e) {
            qDebug() << "Exception during QImageToMat:" << e.what();
            emit error(tr("图像转换异常: %1").arg(e.what()));
            qDebug() << "====== MEDIAN FILTER ERROR END (CONVERSION EXCEPTION) ======\n";
            return;
        }
        
        qDebug() << "Step 2: Mat conversion successful. Size:" << mat.rows << "x" << mat.cols 
                 << "channels:" << mat.channels() << "type:" << mat.type() << "depth:" << mat.depth();

        // 检查图像尺寸是否合理
        if (mat.rows <= 0 || mat.cols <= 0) {
            qDebug() << "Error: Invalid image dimensions: " << mat.rows << "x" << mat.cols;
            emit error(tr("图像尺寸无效"));
            qDebug() << "====== MEDIAN FILTER ERROR END (INVALID DIMENSIONS) ======\n";
            return;
        }

        cv::Mat filteredMat;
        
        qDebug() << "Step 3: Starting median filtering operation based on image type";
        // 检查是否是灰度图像
        if (mat.channels() == 1) {
            qDebug() << "Step 3.1: Processing grayscale image with median filter";
            try {
                qDebug() << "Step 3.1.1: Checking if input matrix needs conversion to CV_8UC1";
                // 确保输入矩阵是CV_8UC1类型
                cv::Mat matCopy;
                if (mat.type() != CV_8UC1) {
                    qDebug() << "Converting mat to CV_8UC1 format from type:" << mat.type();
                    mat.convertTo(matCopy, CV_8UC1);
                    qDebug() << "Conversion successful, new Mat type: " << matCopy.type();
                } else {
                    qDebug() << "Mat already in CV_8UC1 format, creating copy";
                    matCopy = mat.clone();
                }
                
                qDebug() << "Step 3.1.2: Creating output matrix with same dimensions";
                // 创建同样大小和类型的输出矩阵
                try {
                    filteredMat.create(matCopy.rows, matCopy.cols, CV_8UC1);
                    qDebug() << "Output matrix created successfully, size: " << filteredMat.rows << "x" << filteredMat.cols;
                } catch (const std::exception& e) {
                    qDebug() << "Exception creating output matrix: " << e.what();
                    throw;
                }
                
                qDebug() << "Step 3.1.3: Applying median filter operation with kernel size " << kernelSize;
                // 使用安全的方式应用中值滤波
                try {
                    qDebug() << "Using kernel size: " << kernelSize;
                    cv::medianBlur(matCopy, filteredMat, kernelSize);
                    qDebug() << "Median blur operation completed successfully";
                } catch (const cv::Exception& e) {
                    qDebug() << "OpenCV exception during median blur operation: " << e.what();
                    throw;
                } catch (const std::exception& e) {
                    qDebug() << "Standard exception during median blur operation: " << e.what();
                    throw;
                } catch (...) {
                    qDebug() << "Unknown exception during median blur operation";
                    throw std::runtime_error("Unknown error during median blur operation");
                }
                
                // 验证结果并添加更详细的调试信息
                qDebug() << "Step 3.1.4: Validating filtered matrix";
                if (filteredMat.empty()) {
                    qDebug() << "Error: Filtered matrix is empty after median blur operation";
                    throw std::runtime_error("Filtered matrix is empty after median blur operation");
                }
                
                if (filteredMat.rows != matCopy.rows || filteredMat.cols != matCopy.cols) {
                    qDebug() << "Error: Filtered matrix dimensions incorrect: " 
                             << filteredMat.rows << "x" << filteredMat.cols
                             << " Expected: " << matCopy.rows << "x" << matCopy.cols;
                    throw std::runtime_error("Filtered matrix dimensions incorrect");
                }
                
                qDebug() << "Median filter operation completed successfully on grayscale image";
                qDebug() << "Filtered mat: size=" << filteredMat.rows << "x" << filteredMat.cols
                         << " type=" << filteredMat.type() << " channels=" << filteredMat.channels()
                         << " is continuous=" << filteredMat.isContinuous();
                
                // 检查第一个像素值以验证数据有效
                if (!filteredMat.empty() && filteredMat.rows > 0 && filteredMat.cols > 0) {
                    uchar firstPixel = filteredMat.at<uchar>(0, 0);
                    uchar lastPixel = filteredMat.at<uchar>(filteredMat.rows-1, filteredMat.cols-1);
                    qDebug() << "First pixel value:" << static_cast<int>(firstPixel)
                             << " Last pixel value:" << static_cast<int>(lastPixel);
                }
            }
            catch (const cv::Exception& e) {
                qDebug() << "OpenCV error in grayscale median filter:" << e.what();
                emit error(tr("灰度图像中值滤波错误: %1").arg(e.what()));
                qDebug() << "====== MEDIAN FILTER ERROR END (GRAYSCALE FILTER ERROR) ======\n";
                return;
            }
            catch (const std::exception& e) {
                qDebug() << "Error in grayscale median filter:" << e.what();
                emit error(tr("灰度图像处理错误: %1").arg(e.what()));
                qDebug() << "====== MEDIAN FILTER ERROR END (GRAYSCALE PROCESSING ERROR) ======\n";
                return;
            }
            catch (...) {
                qDebug() << "Unknown error in grayscale median filter";
                emit error(tr("灰度图像处理未知错误"));
                qDebug() << "====== MEDIAN FILTER ERROR END (UNKNOWN GRAYSCALE ERROR) ======\n";
                return;
            }
        } else {
            qDebug() << "Step 3.2: Processing color image with median filter";
            try {
                qDebug() << "Step 3.2.1: Applying median blur operation to color image";
                // 彩色图像处理
                try {
                    qDebug() << "Using kernel size: " << kernelSize;
                    cv::medianBlur(mat, filteredMat, kernelSize);
                    qDebug() << "Color median blur operation completed successfully";
                } catch (const cv::Exception& e) {
                    qDebug() << "OpenCV exception during color median blur: " << e.what();
                    throw;
                }
                
                qDebug() << "Step 3.2.2: Validating color filtered matrix";
                if (filteredMat.empty()) {
                    qDebug() << "Error: Filtered mat is empty after applying median filter";
                    throw std::runtime_error("Filtered mat is empty after applying median filter");
                }
                
                qDebug() << "Color filtered mat: size=" << filteredMat.rows << "x" << filteredMat.cols
                         << " type=" << filteredMat.type() << " channels=" << filteredMat.channels();
            }
            catch (const cv::Exception& e) {
                qDebug() << "OpenCV error during color median filter operation:" << e.what();
                emit error(tr("彩色图像中值滤波错误: %1").arg(e.what()));
                qDebug() << "====== MEDIAN FILTER ERROR END (COLOR FILTER ERROR) ======\n";
                return;
            }
            catch (const std::exception& e) {
                qDebug() << "Error during color image processing:" << e.what();
                emit error(tr("彩色图像处理错误: %1").arg(e.what()));
                qDebug() << "====== MEDIAN FILTER ERROR END (COLOR PROCESSING ERROR) ======\n";
                return;
            }
            catch (...) {
                qDebug() << "Unknown error during color processing";
                emit error(tr("彩色图像处理未知错误"));
                qDebug() << "====== MEDIAN FILTER ERROR END (UNKNOWN COLOR ERROR) ======\n";
                return;
            }
        }
            
        qDebug() << "Step 4: Median filter applied successfully. Result Mat size:" 
                 << filteredMat.rows << "x" << filteredMat.cols 
                 << "channels:" << filteredMat.channels()
                 << "type:" << filteredMat.type();

        if (subtractFromOriginal) {
            qDebug() << "Step 5: Subtracting filtered image from original (requested)";
            qDebug() << "Step 5.1: Converting original image to Mat...";
            cv::Mat originalMat;
            try {
                originalMat = QImageToMat(originalImage);
                if (originalMat.empty()) {
                    qDebug() << "Error: Original image conversion to Mat failed";
                    emit error(tr("原始图像转换失败"));
                    qDebug() << "====== MEDIAN FILTER ERROR END (ORIGINAL CONVERSION FAILED) ======\n";
                    return;
                }
                qDebug() << "Original Mat: size=" << originalMat.rows << "x" << originalMat.cols
                         << " channels=" << originalMat.channels() << " type=" << originalMat.type();
            } catch (const std::exception& e) {
                qDebug() << "Exception during original image conversion: " << e.what();
                emit error(tr("原始图像转换异常: %1").arg(e.what()));
                qDebug() << "====== MEDIAN FILTER ERROR END (ORIGINAL CONVERSION EXCEPTION) ======\n";
                return;
            }

            // 检查原始图像尺寸是否匹配
            qDebug() << "Step 5.2: Validating original image dimensions";
            if (originalMat.rows != mat.rows || originalMat.cols != mat.cols) {
                qDebug() << "Error: Original image dimensions do not match: " 
                         << originalMat.rows << "x" << originalMat.cols
                         << " vs " << mat.rows << "x" << mat.cols;
                emit error(tr("原始图像尺寸不匹配"));
                qDebug() << "====== MEDIAN FILTER ERROR END (DIMENSION MISMATCH) ======\n";
                return;
            }

            // 检查原始图像通道数是否与当前图像匹配
            qDebug() << "Step 5.3: Validating channel count";
            if (originalMat.channels() != mat.channels()) {
                qDebug() << "Error: Original image channel count does not match: "
                         << originalMat.channels() << " vs " << mat.channels();
                emit error(tr("原始图像通道数与当前图像不匹配"));
                qDebug() << "====== MEDIAN FILTER ERROR END (CHANNEL COUNT MISMATCH) ======\n";
                return;
            }

            qDebug() << "Step 5.4: Subtracting filtered image from original...";
            try {
                cv::subtract(originalMat, filteredMat, filteredMat);
                qDebug() << "Subtraction completed successfully";
            }
            catch (const cv::Exception& e) {
                qDebug() << "OpenCV error during subtraction:" << e.what();
                emit error(tr("图像相减操作失败: %1").arg(e.what()));
                qDebug() << "====== MEDIAN FILTER ERROR END (SUBTRACTION ERROR) ======\n";
                return;
            }
            catch (const std::exception& e) {
                qDebug() << "Error during subtraction:" << e.what();
                emit error(tr("图像相减操作错误: %1").arg(e.what()));
                qDebug() << "====== MEDIAN FILTER ERROR END (SUBTRACTION EXCEPTION) ======\n";
                return;
            }
        }

        qDebug() << "Step 6: Converting filtered Mat back to QImage...";
        QImage newImage;

        try {
            // 针对灰度图像的特殊处理
            if (filteredMat.channels() == 1) {
                qDebug() << "Step 6.1: Converting single-channel filtered result to QImage";
                newImage = ImageProcessor::createGrayscaleImage(filteredMat);
                qDebug() << "Grayscale QImage created: format=" << newImage.format()
                        << " size=" << newImage.width() << "x" << newImage.height()
                        << " isNull=" << newImage.isNull();
            } else {
                // 针对彩色图像的处理
                qDebug() << "Step 6.2: Converting multi-channel filtered result to QImage";
                newImage = MatToQImage(filteredMat);
                qDebug() << "Color QImage created: format=" << newImage.format()
                        << " size=" << newImage.width() << "x" << newImage.height()
                        << " isNull=" << newImage.isNull();
            }
        } catch (const std::exception& e) {
            qDebug() << "Exception during conversion to QImage: " << e.what();
            emit error(tr("转换为QImage时出错: %1").arg(e.what()));
            qDebug() << "====== MEDIAN FILTER ERROR END (QIMAGE CONVERSION ERROR) ======\n";
            return;
        }

        if (newImage.isNull()) {
            qDebug() << "Error: Final QImage is null after conversion";
            emit error(tr("结果图像转换失败"));
            qDebug() << "====== MEDIAN FILTER ERROR END (NULL RESULT IMAGE) ======\n";
            return;
        }
        
        qDebug() << "Step 6.3: Validating new image dimensions";
        qDebug() << "New image size:" << newImage.width() << "x" << newImage.height();
        processedImage = newImage;
        emit imageProcessed();
    } catch (const cv::Exception& e) {
        emit error(tr("OpenCV错误: %1").arg(e.what()));
    } catch (const std::exception& e) {
        emit error(tr("处理错误: %1").arg(e.what()));
    } catch (...) {
        emit error(tr("未知错误"));
    }
}

// QImage 转 cv::Mat
cv::Mat ImageProcessor::QImageToMat(const QImage &image)
{
    if (image.isNull()) {
        qDebug() << "QImageToMat: QImage is null";
        return cv::Mat();
    }

    qDebug() << "QImageToMat: Converting QImage format:" << image.format() 
             << "Size:" << image.width() << "x" << image.height() 
             << "Depth:" << image.depth()
             << "Bytes per line:" << image.bytesPerLine();

    cv::Mat mat;
    try {
        // 检查图像尺寸是否合理
        if (image.width() <= 0 || image.height() <= 0) {
            qDebug() << "QImageToMat: Invalid image dimensions:" << image.width() << "x" << image.height();
            return cv::Mat();
        }

        // 对于所有类型的图像，我们都创建一个深拷贝，并使用更安全的逐像素复制
        QImage copy = image.copy();

        switch (copy.format()) {
            case QImage::Format_RGB32:
            case QImage::Format_ARGB32:
            case QImage::Format_ARGB32_Premultiplied:
                {
                    qDebug() << "QImageToMat: Processing Format_RGB32/ARGB32";
                    
                    // 创建目标矩阵
                    mat = cv::Mat(copy.height(), copy.width(), CV_8UC3);
                    
                    // 逐像素复制，把ARGB/RGB32转为BGR
                    for (int y = 0; y < copy.height(); ++y) {
                        for (int x = 0; x < copy.width(); ++x) {
                            QRgb pixel = copy.pixel(x, y);
                            mat.at<cv::Vec3b>(y, x)[0] = qBlue(pixel);   // B
                            mat.at<cv::Vec3b>(y, x)[1] = qGreen(pixel);  // G
                            mat.at<cv::Vec3b>(y, x)[2] = qRed(pixel);    // R
                        }
                    }
                    
                    qDebug() << "QImageToMat: Converted ARGB to BGR using safe copy, Mat size:"
                             << mat.cols << "x" << mat.rows 
                             << "Channels:" << mat.channels();
                }
                break;
                
            case QImage::Format_RGB888:
                {
                    qDebug() << "QImageToMat: Processing Format_RGB888";
                    
                    // 创建目标矩阵
                    mat = cv::Mat(copy.height(), copy.width(), CV_8UC3);
                    
                    // 逐行复制，把RGB转为BGR
                    for (int y = 0; y < copy.height(); ++y) {
                        const uchar* scanLine = copy.constScanLine(y);
                        for (int x = 0; x < copy.width(); ++x) {
                            // RGB888格式的顺序是RGBRGBRGB...
                            mat.at<cv::Vec3b>(y, x)[0] = scanLine[x*3+2];  // B
                            mat.at<cv::Vec3b>(y, x)[1] = scanLine[x*3+1];  // G
                            mat.at<cv::Vec3b>(y, x)[2] = scanLine[x*3];    // R
                        }
                    }
                    
                    qDebug() << "QImageToMat: Converted RGB888 to BGR using safe copy, Mat size:"
                             << mat.cols << "x" << mat.rows 
                             << "Channels:" << mat.channels();
                }
                break;
                
            case QImage::Format_Grayscale8:
                {
                    qDebug() << "QImageToMat: Processing Format_Grayscale8";
                    
                    // 创建目标矩阵
                    mat = cv::Mat(copy.height(), copy.width(), CV_8UC1);
                    
                    // 逐行复制
                    for (int y = 0; y < copy.height(); ++y) {
                        const uchar* scanLine = copy.constScanLine(y);
                        uchar* matLine = mat.ptr<uchar>(y);
                        std::memcpy(matLine, scanLine, copy.width());
                    }
                    
                    qDebug() << "QImageToMat: Created grayscale Mat using safe copy, size:"
                             << mat.cols << "x" << mat.rows 
                             << "Channels:" << mat.channels();
                }
                break;
                
            case QImage::Format_Indexed8:
                {
                    qDebug() << "QImageToMat: Processing Format_Indexed8";
                    
                    // 将索引色图像转换为RGB32格式
                    QImage convertedImage = copy.convertToFormat(QImage::Format_RGB32);
                    
                    // 创建目标矩阵
                    mat = cv::Mat(convertedImage.height(), convertedImage.width(), CV_8UC3);
                    
                    // 逐像素复制，把RGB32转为BGR
                    for (int y = 0; y < convertedImage.height(); ++y) {
                        for (int x = 0; x < convertedImage.width(); ++x) {
                            QRgb pixel = convertedImage.pixel(x, y);
                            mat.at<cv::Vec3b>(y, x)[0] = qBlue(pixel);   // B
                            mat.at<cv::Vec3b>(y, x)[1] = qGreen(pixel);  // G
                            mat.at<cv::Vec3b>(y, x)[2] = qRed(pixel);    // R
                        }
                    }
                    
                    qDebug() << "QImageToMat: Converted Indexed8 to BGR using safe copy, Mat size:"
                             << mat.cols << "x" << mat.rows 
                             << "Channels:" << mat.channels();
                }
                break;
                
            default:
                qDebug() << "QImageToMat: Unsupported format, converting to RGB32 first:" << copy.format();
                
                // 转换为RGB32格式再处理
                QImage converted = copy.convertToFormat(QImage::Format_RGB32);
                
                // 创建目标矩阵
                mat = cv::Mat(converted.height(), converted.width(), CV_8UC3);
                
                // 逐像素复制，把RGB32转为BGR
                for (int y = 0; y < converted.height(); ++y) {
                    for (int x = 0; x < converted.width(); ++x) {
                        QRgb pixel = converted.pixel(x, y);
                        mat.at<cv::Vec3b>(y, x)[0] = qBlue(pixel);   // B
                        mat.at<cv::Vec3b>(y, x)[1] = qGreen(pixel);  // G
                        mat.at<cv::Vec3b>(y, x)[2] = qRed(pixel);    // R
                    }
                }
                
                qDebug() << "QImageToMat: Converted to RGB32 then to BGR using safe copy, Mat size:"
                         << mat.cols << "x" << mat.rows 
                         << "Channels:" << mat.channels();
                break;
        }
    } catch (const cv::Exception& e) {
        qDebug() << "OpenCV error in QImageToMat:" << e.what();
        return cv::Mat();
    } catch (const std::exception& e) {
        qDebug() << "Error in QImageToMat:" << e.what();
        return cv::Mat();
    } catch (...) {
        qDebug() << "Unknown error in QImageToMat";
        return cv::Mat();
    }

    if (mat.empty()) {
        qDebug() << "Failed to convert QImage to Mat";
    } else {
        qDebug() << "QImageToMat: Successfully converted image, result Mat:" 
                 << "Size:" << mat.cols << "x" << mat.rows 
                 << "Channels:" << mat.channels() 
                 << "Type:" << mat.type() 
                 << "Depth:" << mat.depth() 
                 << "Is continuous:" << mat.isContinuous();
    }
    return mat;
}

// cv::Mat 转 QImage
QImage ImageProcessor::MatToQImage(const cv::Mat &mat)
{
    qDebug() << "\n====== MAT TO QIMAGE START ======";
    if (mat.empty()) {
        qDebug() << "MatToQImage: Mat is empty";
        qDebug() << "====== MAT TO QIMAGE END (FAILED) ======\n";
        return QImage();
    }

    qDebug() << "MatToQImage: Converting Mat size:" << mat.cols << "x" << mat.rows 
             << "Channels:" << mat.channels() 
             << "Type:" << mat.type() 
             << "Depth:" << mat.depth() 
             << "Step:" << mat.step
             << "Is continuous:" << mat.isContinuous();
    
    // 检查像素值范围，用于诊断
    if (mat.channels() == 1) {
        try {
            double minVal, maxVal;
            cv::minMaxLoc(mat, &minVal, &maxVal);
            qDebug() << "Mat pixel value range: min=" << minVal << " max=" << maxVal;
            
            // 只在确保安全的情况下取样像素值
            if (mat.rows > 0 && mat.cols > 0 && mat.data != nullptr && mat.type() == CV_8UC1) {
                try {
                    // 创建一个Mat的副本来确保内存安全
                    cv::Mat safeMatForSampling = mat.clone();
                    if (!safeMatForSampling.empty()) {
                        uchar firstPixel = safeMatForSampling.at<uchar>(0, 0);
                        
                        // 确保中间索引不会越界
                        int midRow = std::min(safeMatForSampling.rows/2, safeMatForSampling.rows-1);
                        int midCol = std::min(safeMatForSampling.cols/2, safeMatForSampling.cols-1);
                        uchar middlePixel = safeMatForSampling.at<uchar>(midRow, midCol);
                        
                        // 确保最后一个索引不会越界
                        int lastRow = safeMatForSampling.rows-1;
                        int lastCol = safeMatForSampling.cols-1;
                        uchar lastPixel = safeMatForSampling.at<uchar>(lastRow, lastCol);
                        
                        qDebug() << "Sample pixels: first=" << static_cast<int>(firstPixel)
                                << " middle=" << static_cast<int>(middlePixel)
                                << " last=" << static_cast<int>(lastPixel);
                    } else {
                        qDebug() << "Skipping pixel sampling: cloned matrix is empty";
                    }
                } catch (const cv::Exception& e) {
                    qDebug() << "OpenCV exception during pixel sampling: " << e.what() << " - continuing conversion";
                } catch (const std::exception& e) {
                    qDebug() << "Standard exception during pixel sampling: " << e.what() << " - continuing conversion";
                } catch (...) {
                    qDebug() << "Unknown exception during pixel sampling - continuing conversion";
                }
            } else {
                qDebug() << "Skipping pixel sampling due to unsafe conditions";
                if (mat.type() != CV_8UC1) {
                    qDebug() << "Matrix type is not CV_8UC1, it is: " << mat.type();
                }
            }
        } catch (const cv::Exception& e) {
            qDebug() << "OpenCV exception during min/max calculation: " << e.what() << " - continuing conversion";
        } catch (const std::exception& e) {
            qDebug() << "Standard exception during min/max calculation: " << e.what() << " - continuing conversion";
        } catch (...) {
            qDebug() << "Unknown exception during min/max calculation - continuing conversion";
        }
    }

    try {
        if (mat.type() == CV_8UC3) {
            qDebug() << "MatToQImage: Processing CV_8UC3 (BGR) Mat";
            cv::Mat rgb;
            cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
            // 创建深拷贝以确保数据安全
            cv::Mat rgbCopy = rgb.clone();
            QImage result(rgbCopy.data, rgbCopy.cols, rgbCopy.rows, rgbCopy.step, QImage::Format_RGB888);
            QImage copy = result.copy(); // 再次复制确保完全分离数据
            qDebug() << "MatToQImage: Created RGB888 QImage, size:" << copy.width() << "x" << copy.height()
                     << "Format:" << copy.format() << "Depth:" << copy.depth()
                     << "Is null:" << copy.isNull();
            
            // 检查QImage像素值
            if (!copy.isNull() && copy.width() > 0 && copy.height() > 0) {
                QRgb firstPixel = copy.pixel(0, 0);
                QRgb middlePixel = copy.pixel(copy.width()/2, copy.height()/2);
                QRgb lastPixel = copy.pixel(copy.width()-1, copy.height()-1);
                qDebug() << "RGB QImage sample pixels: first=RGB(" 
                         << qRed(firstPixel) << "," << qGreen(firstPixel) << "," << qBlue(firstPixel) << ")"
                         << " middle=RGB(" 
                         << qRed(middlePixel) << "," << qGreen(middlePixel) << "," << qBlue(middlePixel) << ")"
                         << " last=RGB(" 
                         << qRed(lastPixel) << "," << qGreen(lastPixel) << "," << qBlue(lastPixel) << ")";
            }
            
            qDebug() << "====== MAT TO QIMAGE END (SUCCESS) ======\n";
            return copy;
        } else if (mat.type() == CV_8UC1) {
            qDebug() << "MatToQImage: Processing CV_8UC1 (Grayscale) Mat - Using specialized function";
            // 使用专门的灰度图像处理函数
            QImage grayImage = ImageProcessor::createGrayscaleImage(mat);
            if (grayImage.isNull()) {
                qDebug() << "MatToQImage: Specialized grayscale conversion failed, trying fallback method";
                // 如果专门的函数失败，尝试备用方法
                cv::Mat grayCopy = mat.clone();
                QImage result(grayCopy.data, grayCopy.cols, grayCopy.rows, grayCopy.step, QImage::Format_Grayscale8);
                QImage copy = result.copy();
                
                qDebug() << "Fallback grayscale QImage: size=" << copy.width() << "x" << copy.height()
                         << " format=" << copy.format() << " is null=" << copy.isNull();
                
                if (!copy.isNull() && copy.width() > 0 && copy.height() > 0) {
                    int firstPixel = qGray(copy.pixel(0, 0));
                    int middlePixel = qGray(copy.pixel(copy.width()/2, copy.height()/2));
                    int lastPixel = qGray(copy.pixel(copy.width()-1, copy.height()-1));
                    qDebug() << "Fallback QImage sample pixels: first=" << firstPixel
                             << " middle=" << middlePixel
                             << " last=" << lastPixel;
                }
                
                qDebug() << "====== MAT TO QIMAGE END (FALLBACK SUCCESS) ======\n";
                return copy;
            }
            
            qDebug() << "MatToQImage: Created Grayscale QImage, size:" << grayImage.width() << "x" << grayImage.height()
                     << "Format:" << grayImage.format() << "Depth:" << grayImage.depth();
            qDebug() << "====== MAT TO QIMAGE END (SUCCESS) ======\n";
            return grayImage;
        } else {
            qDebug() << "MatToQImage: Unsupported Mat type:" << mat.type() << ", trying to convert";
            
            // 尝试转换为支持的格式
            cv::Mat converted;
            if (mat.channels() == 1) {
                // 如果是单通道但类型不是CV_8UC1
                qDebug() << "MatToQImage: Converting single channel Mat to 8-bit";
                mat.convertTo(converted, CV_8UC1);
                // 使用专门的灰度图像处理函数
                QImage result = ImageProcessor::createGrayscaleImage(converted);
                qDebug() << "====== MAT TO QIMAGE END (CONVERTED SUCCESS) ======\n";
                return result;
            } else if (mat.channels() == 3) {
                // 如果是三通道但类型不是CV_8UC3
                qDebug() << "MatToQImage: Converting 3-channel Mat to 8-bit BGR";
                mat.convertTo(converted, CV_8UC3);
                cv::Mat rgb;
                cv::cvtColor(converted, rgb, cv::COLOR_BGR2RGB);
                cv::Mat rgbCopy = rgb.clone();
                QImage result(rgbCopy.data, rgbCopy.cols, rgbCopy.rows, rgbCopy.step, QImage::Format_RGB888);
                QImage copy = result.copy();
                qDebug() << "MatToQImage: Converted to RGB QImage, size:" << copy.width() << "x" << copy.height()
                         << "Format:" << copy.format() << "Is null:" << copy.isNull();
                qDebug() << "====== MAT TO QIMAGE END (CONVERTED SUCCESS) ======\n";
                return copy;
            } else {
                qDebug() << "MatToQImage: Unsupported Mat type and channel count, returning empty image";
                qDebug() << "====== MAT TO QIMAGE END (FAILED) ======\n";
                return QImage();
            }
        }
    } catch (const cv::Exception& e) {
        qDebug() << "OpenCV error in MatToQImage:" << e.what();
        qDebug() << "====== MAT TO QIMAGE END (ERROR) ======\n";
        return QImage();
    } catch (const std::exception& e) {
        qDebug() << "Error in MatToQImage:" << e.what();
        qDebug() << "====== MAT TO QIMAGE END (ERROR) ======\n";
        return QImage();
    }
}

// 添加新方法：应用线性变换 y = kx + b
void ImageProcessor::applyLinearTransform(int kValue, int bValue)
{
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        return;
    }
    
    qDebug() << "\n====== LINEAR TRANSFORM START ======";
    qDebug() << "Linear Transform - Parameters: k=" << kValue << " b=" << bValue;
    
    try {
        // 如果是灰度图像操作，应始终基于当前的灰度图像进行变换
        // 如果未处于灰度模式，则当前图像可能是彩色的，需要先转换为灰度
        bool currentlyGrayscale = (processedImage.format() == QImage::Format_Grayscale8);
        QImage inputImage;
        
        qDebug() << "Current image is grayscale: " << currentlyGrayscale;
        
        if (!currentlyGrayscale) {
            qDebug() << "Converting to grayscale first";
            
            // 将当前图像转换为灰度图
            cv::Mat colorMat = QImageToMat(processedImage);
            cv::Mat grayMat;
            cv::cvtColor(colorMat, grayMat, cv::COLOR_BGR2GRAY);
            
            // 保存临时灰度图像作为输入
            inputImage = createGrayscaleImage(grayMat);
            
            if (inputImage.isNull()) {
                throw std::runtime_error("Failed to convert image to grayscale");
            }
            
            // 保存灰度图像以便后续使用
            saveGrayscaleImage();
        } else {
            // 已经是灰度图像，直接使用
            inputImage = processedImage;
        }
        
        qDebug() << "Input image format:" << inputImage.format()
                 << " Size:" << inputImage.width() << "x" << inputImage.height();
        
        // 转换为cv::Mat进行处理
        cv::Mat mat = QImageToMat(inputImage);
        if (mat.empty()) {
            throw std::runtime_error("Failed to convert QImage to Mat");
        }
        
        cv::Mat result;
        
        // 计算k值
        double k = 1.0;
        if (kValue >= 0) {
            k = 1.0 + kValue / 100.0;  // k范围是1.0到2.0
        } else {
            k = 1.0 + kValue / 100.0;  // k范围是0.0到1.0
        }
        
        // b值直接使用
        double b = bValue;
        
        qDebug() << "Applied transformation: y = " << k << "x + " << b;
        
        // 应用线性变换
        mat.convertTo(result, -1, k, b);
        
        // 确保结果在0-255范围内
        cv::threshold(result, result, 255, 255, cv::THRESH_TRUNC);
        cv::threshold(result, result, 0, 0, cv::THRESH_TOZERO);
        
        // 转换回QImage
        QImage transformedImage = createGrayscaleImage(result);
        if (transformedImage.isNull()) {
            throw std::runtime_error("Failed to convert result to QImage");
        }
        
        // 更新处理后的图像
        processedImage = transformedImage;
        
        qDebug() << "Linear transform completed successfully";
        qDebug() << "====== LINEAR TRANSFORM END ======\n";
        emit imageProcessed();
    } catch (const cv::Exception& e) {
        qDebug() << "OpenCV error:" << e.what();
        emit error(tr("OpenCV错误: %1").arg(e.what()));
        qDebug() << "====== LINEAR TRANSFORM ERROR END ======\n";
    } catch (const std::exception& e) {
        qDebug() << "Processing error:" << e.what();
        emit error(tr("处理错误: %1").arg(e.what()));
        qDebug() << "====== LINEAR TRANSFORM ERROR END ======\n";
    } catch (...) {
        qDebug() << "Unknown error occurred";
        emit error(tr("未知错误"));
        qDebug() << "====== LINEAR TRANSFORM ERROR END ======\n";
    }
}

// 伽马对比度调整
void ImageProcessor::adjustGammaContrast(double gamma, int contrast)
{
    qDebug() << "\n====== GAMMA CONTRAST START ======";
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        qDebug() << "====== GAMMA CONTRAST ERROR END (NULL IMAGE) ======\n";
        return;
    }
    
    qDebug() << "Applying gamma correction with gamma=" << gamma 
             << " and contrast adjustment with contrast=" << contrast;
    
    try {
        // 如果是灰度图像操作，应始终基于当前的灰度图像进行变换
        // 如果未处于灰度模式，则当前图像可能是彩色的，需要先转换为灰度
        bool currentlyGrayscale = (processedImage.format() == QImage::Format_Grayscale8);
        QImage inputImage;
        
        qDebug() << "Current image is grayscale: " << currentlyGrayscale;
        
        if (!currentlyGrayscale) {
            qDebug() << "Converting to grayscale first";
            
            // 将当前图像转换为灰度图
            cv::Mat colorMat = QImageToMat(processedImage);
            cv::Mat grayMat;
            cv::cvtColor(colorMat, grayMat, cv::COLOR_BGR2GRAY);
            
            // 保存临时灰度图像作为输入
            inputImage = createGrayscaleImage(grayMat);
            
            if (inputImage.isNull()) {
                throw std::runtime_error("Failed to convert image to grayscale");
            }
            
            // 保存灰度图像以便后续使用
            saveGrayscaleImage();
        } else {
            // 已经是灰度图像，直接使用
            inputImage = processedImage;
        }
        
        qDebug() << "Input image format:" << inputImage.format()
                 << " Size:" << inputImage.width() << "x" << inputImage.height();
                 
        cv::Mat mat = QImageToMat(inputImage);
        if (mat.empty()) {
            qDebug() << "Error: QImageToMat returned empty matrix";
            emit error(tr("图像转换失败"));
            qDebug() << "====== GAMMA CONTRAST ERROR END (CONVERSION FAILED) ======\n";
            return;
        }
        
        qDebug() << "Input image format: channels=" << mat.channels() 
                 << " type=" << mat.type() << " depth=" << mat.depth();
        
        // 创建查找表(LUT)进行伽马校正
        cv::Mat lookUpTable(1, 256, CV_8U);
        uchar* p = lookUpTable.ptr();
        
        // 计算对比度系数
        double contrastFactor = 1.0 + contrast / 100.0;
        qDebug() << "Contrast factor:" << contrastFactor;
        
        // 填充查找表，应用伽马校正和对比度调整
        for (int i = 0; i < 256; ++i) {
            // 伽马校正: s = c * r^γ (其中r是输入像素值, s是输出像素值)
            double pixelValue = pow(i / 255.0, 1.0 / gamma) * 255.0;
            
            // 应用对比度调整: s = (s - 128) * contrast_factor + 128
            pixelValue = (pixelValue - 128) * contrastFactor + 128;
            
            // 确保值在0-255范围内
            pixelValue = std::min(255.0, std::max(0.0, pixelValue));
            
            p[i] = cv::saturate_cast<uchar>(pixelValue);
        }
        
        // 应用查找表进行变换
        cv::Mat resultMat;
        cv::LUT(mat, lookUpTable, resultMat);
        
        // 转换回QImage
        QImage result = createGrayscaleImage(resultMat);
        if (result.isNull()) {
            qDebug() << "Error: Failed to convert result Mat to QImage";
            emit error(tr("结果图像转换失败"));
            qDebug() << "====== GAMMA CONTRAST ERROR END (RESULT CONVERSION FAILED) ======\n";
            return;
        }
        
        processedImage = result;
        qDebug() << "Gamma and contrast adjustment applied successfully";
        qDebug() << "====== GAMMA CONTRAST END ======\n";
        emit imageProcessed();
    } catch (const cv::Exception& e) {
        qDebug() << "OpenCV error in gamma/contrast adjustment:" << e.what();
        emit error(tr("OpenCV错误: %1").arg(e.what()));
        qDebug() << "====== GAMMA CONTRAST ERROR END ======\n";
    } catch (const std::exception& e) {
        qDebug() << "Error in gamma/contrast adjustment:" << e.what();
        emit error(tr("处理错误: %1").arg(e.what()));
        qDebug() << "====== GAMMA CONTRAST ERROR END ======\n";
    } catch (...) {
        qDebug() << "Unknown error in gamma/contrast adjustment";
        emit error(tr("未知错误"));
        qDebug() << "====== GAMMA CONTRAST ERROR END ======\n";
    }
}

// 保存当前灰度图像状态
void ImageProcessor::saveGrayscaleImage()
{
    try {
        // 检查当前图像是否有效
        if (processedImage.isNull()) {
            qDebug() << "保存灰度图像失败：当前图像为空";
            return;
        }

        // 如果当前图像是灰度图，将其保存到grayscaleImage
        if (isGrayscale()) {
            grayscaleImage = processedImage.copy(); // 使用深拷贝确保数据独立
            qDebug() << "已保存当前灰度图像";
        } else {
            // 如果不是灰度图，先转换为灰度图再保存
            try {
                cv::Mat mat = QImageToMat(processedImage);
                if (!mat.empty()) {
                    cv::Mat gray;
                    cv::cvtColor(mat, gray, cv::COLOR_BGR2GRAY);
                    grayscaleImage = MatToQImage(gray);
                    qDebug() << "已将彩色图像转换为灰度图像并保存";
                } else {
                    qDebug() << "转换为Mat失败，无法保存灰度图像";
                }
            } catch (const std::exception& e) {
                qDebug() << "保存灰度图像时出错:" << e.what();
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "saveGrayscaleImage异常:" << e.what();
    }
}

// 判断当前图像是否为灰度图
bool ImageProcessor::isGrayscale() const
{
    try {
        // 如果图像是空的，返回false
        if (processedImage.isNull() || processedImage.width() == 0 || processedImage.height() == 0) {
            return false;
        }
        
        // 检查图像格式
        if (processedImage.format() == QImage::Format_Grayscale8) {
            return true;
        }
        
        // 对于其他格式，检查RGB值是否相等
        // 取样一些点（不检查所有像素以提高效率）
        int width = processedImage.width();
        int height = processedImage.height();
        int step = qMax(1, qMin(width, height) / 10); // 采样步长
        
        for (int y = 0; y < height; y += step) {
            for (int x = 0; x < width; x += step) {
                if (x < width && y < height) { // 额外检查坐标范围
                    QRgb pixel = processedImage.pixel(x, y);
                    int r = qRed(pixel);
                    int g = qGreen(pixel);
                    int b = qBlue(pixel);
                    
                    // 如果RGB值不相等，则不是灰度图
                    if (r != g || g != b) {
                        return false;
                    }
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        qDebug() << "isGrayscale异常:" << e.what();
        return false;
    }
}

// 恢复到灰度图像状态
void ImageProcessor::restoreGrayscaleImage()
{
    try {
        if (!grayscaleImage.isNull()) {
            // 使用深拷贝确保数据独立
            processedImage = grayscaleImage.copy();
            emit imageProcessed();
            qDebug() << "已恢复到保存的灰度图像";
        } else {
            // 如果没有保存的灰度图像，从原图转换（避免重复调用自身）
            qDebug() << "没有保存的灰度图像，从原始图像转换";
            if (!originalImage.isNull()) {
                // 直接从原始图像转换为灰度，避免调用convertToGrayscale()
                cv::Mat mat = QImageToMat(originalImage);
                if (!mat.empty()) {
                    cv::Mat gray;
                    cv::cvtColor(mat, gray, cv::COLOR_BGR2GRAY);
                    processedImage = MatToQImage(gray);
                    emit imageProcessed();
                } else {
                    qDebug() << "原始图像转换为Mat失败";
                }
            } else {
                qDebug() << "原始图像为空，无法转换为灰度";
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "restoreGrayscaleImage异常:" << e.what();
    }
}

// 实现调试函数，用于打印图像信息
void ImageProcessor::debugImageInfo() const
{
    qDebug() << "\n===============================================";
    qDebug() << "ImageProcessor Debug Information:";
    qDebug() << "-----------------------------------------------";
    
    if (!originalImage.isNull()) {
        qDebug() << "Original Image:";
        qDebug() << "   Size: " << originalImage.width() << "x" << originalImage.height();
        qDebug() << "   Format: " << originalImage.format();
        qDebug() << "   Depth: " << originalImage.depth();
        qDebug() << "   Bytes per line: " << originalImage.bytesPerLine();
        qDebug() << "   Is Grayscale: " << (originalImage.format() == QImage::Format_Grayscale8);
    } else {
        qDebug() << "Original Image: NULL";
    }
    
    qDebug() << "-----------------------------------------------";
    
    if (!processedImage.isNull()) {
        qDebug() << "Processed Image:";
        qDebug() << "   Size: " << processedImage.width() << "x" << processedImage.height();
        qDebug() << "   Format: " << processedImage.format();
        qDebug() << "   Depth: " << processedImage.depth();
        qDebug() << "   Bytes per line: " << processedImage.bytesPerLine();
        qDebug() << "   Is Grayscale: " << (processedImage.format() == QImage::Format_Grayscale8);
        
        // 尝试转换为Mat并检查通道数
        try {
            cv::Mat mat = QImageToMat(processedImage);
            if (!mat.empty()) {
                qDebug() << "   OpenCV Mat Size: " << mat.cols << "x" << mat.rows;
                qDebug() << "   OpenCV Mat Channels: " << mat.channels();
                qDebug() << "   OpenCV Mat Type: " << mat.type();
                qDebug() << "   OpenCV Mat Depth: " << mat.depth();
                qDebug() << "   OpenCV Mat Is Continuous: " << mat.isContinuous();
            }
        } catch (const std::exception& e) {
            qDebug() << "   Error converting to Mat: " << e.what();
        }
    } else {
        qDebug() << "Processed Image: NULL";
    }
    
    qDebug() << "-----------------------------------------------";
    
    if (!grayscaleImage.isNull()) {
        qDebug() << "Grayscale Image Cache:";
        qDebug() << "   Size: " << grayscaleImage.width() << "x" << grayscaleImage.height();
        qDebug() << "   Format: " << grayscaleImage.format();
        qDebug() << "   Is Grayscale: " << (grayscaleImage.format() == QImage::Format_Grayscale8);
    } else {
        qDebug() << "Grayscale Image Cache: NULL";
    }
    
    qDebug() << "===============================================\n";
}

// 处理单通道灰度图像转换为QImage
QImage ImageProcessor::createGrayscaleImage(const cv::Mat &grayscaleMat)
{
    qDebug() << "\n====== CREATE GRAYSCALE IMAGE START ======";
    qDebug() << "Creating specialized grayscale QImage from Mat";
    qDebug() << "Input Mat: size=" << grayscaleMat.cols << "x" << grayscaleMat.rows
             << " type=" << grayscaleMat.type() << " channels=" << grayscaleMat.channels()
             << " depth=" << grayscaleMat.depth() << " is continuous=" << grayscaleMat.isContinuous();
    
    // 检查Mat是否有效
    if (grayscaleMat.empty()) {
        qDebug() << "ERROR: Input Mat is empty!";
        qDebug() << "====== CREATE GRAYSCALE IMAGE END (FAILED) ======\n";
        return QImage();
    }
    
    // 检查像素值范围，用于诊断
    double minVal, maxVal;
    cv::minMaxLoc(grayscaleMat, &minVal, &maxVal);
    qDebug() << "Mat pixel value range: min=" << minVal << " max=" << maxVal;
    
    // 取样一些像素值
    if (grayscaleMat.rows > 0 && grayscaleMat.cols > 0) {
        uchar firstPixel = grayscaleMat.at<uchar>(0, 0);
        uchar middlePixel = grayscaleMat.at<uchar>(grayscaleMat.rows/2, grayscaleMat.cols/2);
        uchar lastPixel = grayscaleMat.at<uchar>(grayscaleMat.rows-1, grayscaleMat.cols-1);
        qDebug() << "Sample pixels: first=" << static_cast<int>(firstPixel)
                 << " middle=" << static_cast<int>(middlePixel)
                 << " last=" << static_cast<int>(lastPixel);
    }
    
    try {
        // 确保矩阵类型正确
        if (grayscaleMat.type() != CV_8UC1) {
            qDebug() << "Converting matrix type to CV_8UC1 from type:" << grayscaleMat.type();
            cv::Mat convertedMat;
            grayscaleMat.convertTo(convertedMat, CV_8UC1);
            
            // 再次检查像素值范围
            double newMinVal, newMaxVal;
            cv::minMaxLoc(convertedMat, &newMinVal, &newMaxVal);
            qDebug() << "Converted Mat pixel value range: min=" << newMinVal << " max=" << newMaxVal;
            
            // 确保数据连续
            cv::Mat continuousMat;
            if (!convertedMat.isContinuous()) {
                qDebug() << "Making data continuous";
                continuousMat = convertedMat.clone();
            } else {
                continuousMat = convertedMat;
            }
            
            // 创建QImage并复制数据
            QImage result(continuousMat.data, continuousMat.cols, continuousMat.rows, 
                         continuousMat.step, QImage::Format_Grayscale8);
            QImage copy = result.copy(); // 深拷贝
            
            qDebug() << "Created QImage: size=" << copy.width() << "x" << copy.height()
                     << " format=" << copy.format() << " depth=" << copy.depth()
                     << " is null=" << copy.isNull();
                     
            // 检查QImage像素值
            if (!copy.isNull() && copy.width() > 0 && copy.height() > 0) {
                int firstQPixel = qGray(copy.pixel(0, 0));
                int middleQPixel = qGray(copy.pixel(copy.width()/2, copy.height()/2));
                int lastQPixel = qGray(copy.pixel(copy.width()-1, copy.height()-1));
                qDebug() << "QImage sample pixels: first=" << firstQPixel
                         << " middle=" << middleQPixel
                         << " last=" << lastQPixel;
            }
            
            qDebug() << "====== CREATE GRAYSCALE IMAGE END (SUCCESS) ======\n";
            return copy;
        } else {
            // 确保数据连续
            cv::Mat continuousMat;
            if (!grayscaleMat.isContinuous()) {
                qDebug() << "Making data continuous";
                continuousMat = grayscaleMat.clone();
            } else {
                continuousMat = grayscaleMat;
            }
            
            // 创建QImage并复制数据
            QImage result(continuousMat.data, continuousMat.cols, continuousMat.rows, 
                         continuousMat.step, QImage::Format_Grayscale8);
            QImage copy = result.copy(); // 深拷贝
            
            qDebug() << "Created QImage: size=" << copy.width() << "x" << copy.height()
                     << " format=" << copy.format() << " depth=" << copy.depth()
                     << " is null=" << copy.isNull();
                     
            // 检查QImage像素值
            if (!copy.isNull() && copy.width() > 0 && copy.height() > 0) {
                int firstQPixel = qGray(copy.pixel(0, 0));
                int middleQPixel = qGray(copy.pixel(copy.width()/2, copy.height()/2));
                int lastQPixel = qGray(copy.pixel(copy.width()-1, copy.height()-1));
                qDebug() << "QImage sample pixels: first=" << firstQPixel
                         << " middle=" << middleQPixel
                         << " last=" << lastQPixel;
            }
            
            qDebug() << "====== CREATE GRAYSCALE IMAGE END (SUCCESS) ======\n";
            return copy;
        }
    } catch (const std::exception& e) {
        qDebug() << "Error in createGrayscaleImage:" << e.what();
        qDebug() << "====== CREATE GRAYSCALE IMAGE END (ERROR) ======\n";
        return QImage();
    }
}

// 将图像转换为灰度图
void ImageProcessor::convertToGrayscale()
{
    qDebug() << "\n====== CONVERT TO GRAYSCALE START ======";
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        qDebug() << "====== CONVERT TO GRAYSCALE ERROR END (NULL IMAGE) ======\n";
        return;
    }
    
    // 检查图像是否已经是灰度图
    if (processedImage.format() == QImage::Format_Grayscale8) {
        qDebug() << "Image is already grayscale, no conversion needed";
        qDebug() << "====== CONVERT TO GRAYSCALE END (ALREADY GRAYSCALE) ======\n";
        return;
    }
    
    try {
        qDebug() << "Converting image to grayscale...";
        cv::Mat mat = QImageToMat(processedImage);
        
        if (mat.empty()) {
            qDebug() << "Error: QImageToMat returned empty matrix";
            emit error(tr("图像转换失败"));
            qDebug() << "====== CONVERT TO GRAYSCALE ERROR END (CONVERSION FAILED) ======\n";
            return;
        }
        
        cv::Mat grayMat;
        cv::cvtColor(mat, grayMat, cv::COLOR_BGR2GRAY);
        
        // 使用专门的灰度图像处理函数
        processedImage = createGrayscaleImage(grayMat);
        
        if (processedImage.isNull()) {
            qDebug() << "Error: Grayscale conversion produced null image";
            emit error(tr("灰度转换失败"));
            qDebug() << "====== CONVERT TO GRAYSCALE ERROR END (NULL RESULT) ======\n";
            return;
        }
        
        // 保存灰度图像以供后续使用
        saveGrayscaleImage();
        
        qDebug() << "Grayscale conversion completed successfully";
        qDebug() << "====== CONVERT TO GRAYSCALE END ======\n";
        emit imageProcessed();
    } catch (const cv::Exception& e) {
        qDebug() << "OpenCV error in grayscale conversion:" << e.what();
        emit error(tr("OpenCV错误: %1").arg(e.what()));
        qDebug() << "====== CONVERT TO GRAYSCALE ERROR END ======\n";
    } catch (const std::exception& e) {
        qDebug() << "Error in grayscale conversion:" << e.what();
        emit error(tr("处理错误: %1").arg(e.what()));
        qDebug() << "====== CONVERT TO GRAYSCALE ERROR END ======\n";
    } catch (...) {
        qDebug() << "Unknown error in grayscale conversion";
        emit error(tr("未知错误"));
        qDebug() << "====== CONVERT TO GRAYSCALE ERROR END ======\n";
    }
}

// 直方图均衡化
void ImageProcessor::applyHistogramEqualization()
{
    qDebug() << "\n====== HISTOGRAM EQUALIZATION START ======";
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        qDebug() << "====== HISTOGRAM EQUALIZATION ERROR END (NULL IMAGE) ======\n";
        return;
    }
    
    try {
        cv::Mat mat = QImageToMat(processedImage);
        if (mat.empty()) {
            qDebug() << "Error: QImageToMat returned empty matrix";
            emit error(tr("图像转换失败"));
            qDebug() << "====== HISTOGRAM EQUALIZATION ERROR END (CONVERSION FAILED) ======\n";
            return;
        }
        
        cv::Mat result;
        
        // 根据图像通道数选择不同的处理方式
        if (mat.channels() == 1) {
            // 灰度图像直接进行直方图均衡化
            qDebug() << "Equalizing grayscale image histogram";
            cv::equalizeHist(mat, result);
        } else {
            // 彩色图像需要在YUV/Lab空间进行处理
            qDebug() << "Equalizing color image histogram in YUV space";
            cv::Mat yuv;
            cv::cvtColor(mat, yuv, cv::COLOR_BGR2YUV);
            
            // 分离通道
            std::vector<cv::Mat> channels;
            cv::split(yuv, channels);
            
            // 只对亮度通道Y进行均衡化处理
            cv::equalizeHist(channels[0], channels[0]);
            
            // 合并通道
            cv::merge(channels, yuv);
            
            // 转换回BGR空间
            cv::cvtColor(yuv, result, cv::COLOR_YUV2BGR);
        }
        
        // 转换回QImage
        QImage qResult = MatToQImage(result);
        if (qResult.isNull()) {
            qDebug() << "Error: Failed to convert result Mat to QImage";
            emit error(tr("结果图像转换失败"));
            qDebug() << "====== HISTOGRAM EQUALIZATION ERROR END (RESULT CONVERSION FAILED) ======\n";
            return;
        }
        
        processedImage = qResult;
        qDebug() << "Histogram equalization applied successfully";
        qDebug() << "====== HISTOGRAM EQUALIZATION END ======\n";
        emit imageProcessed();
    } catch (const cv::Exception& e) {
        qDebug() << "OpenCV error in histogram equalization:" << e.what();
        emit error(tr("OpenCV错误: %1").arg(e.what()));
        qDebug() << "====== HISTOGRAM EQUALIZATION ERROR END ======\n";
    } catch (const std::exception& e) {
        qDebug() << "Error in histogram equalization:" << e.what();
        emit error(tr("处理错误: %1").arg(e.what()));
        qDebug() << "====== HISTOGRAM EQUALIZATION ERROR END ======\n";
    } catch (...) {
        qDebug() << "Unknown error in histogram equalization";
        emit error(tr("未知错误"));
        qDebug() << "====== HISTOGRAM EQUALIZATION ERROR END ======\n";
    }
}

// 直方图拉伸
void ImageProcessor::applyHistogramStretching()
{
    qDebug() << "\n====== HISTOGRAM STRETCHING START ======";
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        qDebug() << "====== HISTOGRAM STRETCHING ERROR END (NULL IMAGE) ======\n";
        return;
    }
    
    try {
        cv::Mat mat = QImageToMat(processedImage);
        if (mat.empty()) {
            qDebug() << "Error: QImageToMat returned empty matrix";
            emit error(tr("图像转换失败"));
            qDebug() << "====== HISTOGRAM STRETCHING ERROR END (CONVERSION FAILED) ======\n";
            return;
        }
        
        cv::Mat result;
        
        // 根据图像通道数选择不同的处理方式
        if (mat.channels() == 1) {
            // 灰度图像处理
            qDebug() << "Stretching grayscale image histogram";
            // 找到最小值和最大值
            double minVal, maxVal;
            cv::minMaxLoc(mat, &minVal, &maxVal);
            qDebug() << "Original range: min=" << minVal << " max=" << maxVal;
            
            // 如果范围已经是0-255，则不需要进一步处理
            if (minVal == 0 && maxVal == 255) {
                qDebug() << "Image already uses full range (0-255), no stretching needed";
                result = mat.clone();
            } else {
                // 线性拉伸 newPixel = (pixel - min) * 255 / (max - min)
                mat.convertTo(result, -1, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));
                
                // 验证结果
                cv::minMaxLoc(result, &minVal, &maxVal);
                qDebug() << "Stretched range: min=" << minVal << " max=" << maxVal;
            }
        } else {
            // 彩色图像处理
            qDebug() << "Stretching color image histogram";
            // 转换到YUV色彩空间
            cv::Mat yuv;
            cv::cvtColor(mat, yuv, cv::COLOR_BGR2YUV);
            
            // 分离通道
            std::vector<cv::Mat> channels;
            cv::split(yuv, channels);
            
            // 只拉伸亮度通道Y
            double minVal, maxVal;
            cv::minMaxLoc(channels[0], &minVal, &maxVal);
            qDebug() << "Original Y channel range: min=" << minVal << " max=" << maxVal;
            
            // 线性拉伸
            channels[0].convertTo(channels[0], -1, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));
            
            // 验证结果
            cv::minMaxLoc(channels[0], &minVal, &maxVal);
            qDebug() << "Stretched Y channel range: min=" << minVal << " max=" << maxVal;
            
            // 合并通道
            cv::merge(channels, yuv);
            
            // 转换回BGR空间
            cv::cvtColor(yuv, result, cv::COLOR_YUV2BGR);
        }
        
        // 转换回QImage
        QImage qResult = MatToQImage(result);
        if (qResult.isNull()) {
            qDebug() << "Error: Failed to convert result Mat to QImage";
            emit error(tr("结果图像转换失败"));
            qDebug() << "====== HISTOGRAM STRETCHING ERROR END (RESULT CONVERSION FAILED) ======\n";
            return;
        }
        
        processedImage = qResult;
        qDebug() << "Histogram stretching applied successfully";
        qDebug() << "====== HISTOGRAM STRETCHING END ======\n";
        emit imageProcessed();
    } catch (const cv::Exception& e) {
        qDebug() << "OpenCV error in histogram stretching:" << e.what();
        emit error(tr("OpenCV错误: %1").arg(e.what()));
        qDebug() << "====== HISTOGRAM STRETCHING ERROR END ======\n";
    } catch (const std::exception& e) {
        qDebug() << "Error in histogram stretching:" << e.what();
        emit error(tr("处理错误: %1").arg(e.what()));
        qDebug() << "====== HISTOGRAM STRETCHING ERROR END ======\n";
    } catch (...) {
        qDebug() << "Unknown error in histogram stretching";
        emit error(tr("未知错误"));
        qDebug() << "====== HISTOGRAM STRETCHING ERROR END ======\n";
    }
}

// 设置卷积核大小
void ImageProcessor::setKernelSize(int size)
{
    // 验证卷积核大小是否有效
    if (validateKernelSize(size)) {
        kernelSize = size;
        qDebug() << "Kernel size set to" << kernelSize;
        emit kernelSizeChanged(kernelSize);
    } else {
        qDebug() << "Invalid kernel size:" << size;
        emit error(tr("无效的卷积核大小: %1").arg(size));
    }
}

// 获取当前卷积核大小
int ImageProcessor::getKernelSize() const
{
    return kernelSize;
}

// 验证卷积核大小
bool ImageProcessor::validateKernelSize(int size)
{
    // 卷积核大小必须是奇数且在3到31之间
    return (size % 2 == 1) && (size >= 3) && (size <= 31);
}

// 使用当前设置的卷积核大小应用均值滤波
void ImageProcessor::applyCurrentMeanFilter(bool subtractFromOriginal)
{
    qDebug() << "Applying mean filter with current kernel size:" << kernelSize;
    applyMeanFilter(kernelSize, subtractFromOriginal);
}

// 使用当前设置的卷积核大小应用高斯滤波
void ImageProcessor::applyCurrentGaussianFilter(double sigma, bool subtractFromOriginal)
{
    qDebug() << "Applying Gaussian filter with current kernel size:" << kernelSize << "and sigma:" << sigma;
    applyGaussianFilter(kernelSize, sigma, subtractFromOriginal);
}

// 使用当前设置的卷积核大小应用中值滤波
void ImageProcessor::applyCurrentMedianFilter(bool subtractFromOriginal)
{
    qDebug() << "Applying median filter with current kernel size:" << kernelSize;
    applyMedianFilter(kernelSize, subtractFromOriginal);
}

