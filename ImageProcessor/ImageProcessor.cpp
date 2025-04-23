#include "ImageProcessor.h"
#include <QImage>
#include <QColor>
#include <cmath>
#include <algorithm>
#include <vector>
#include <opencv2/opencv.hpp>
#include <QDebug>

ImageProcessor::ImageProcessor(QObject *parent)
    : QObject(parent)
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

    if (kernelSize % 2 == 0) {
        emit error(tr("核大小必须是奇数"));
        return;
    }

    if (kernelSize < 3 || kernelSize > 31) {
        emit error(tr("核大小必须在3到31之间"));
        return;
    }

    try {
        qDebug() << "Converting QImage to Mat...";
        cv::Mat mat = QImageToMat(processedImage);
        if (mat.empty()) {
            emit error(tr("图像转换失败"));
            return;
        }
        qDebug() << "Mat size:" << mat.rows << "x" << mat.cols << "channels:" << mat.channels();

        // 检查图像尺寸是否合理
        if (mat.rows <= 0 || mat.cols <= 0) {
            emit error(tr("图像尺寸无效"));
            return;
        }

        // 检查图像通道数
        if (mat.channels() != 3) {
            emit error(tr("图像必须是3通道RGB图像"));
            return;
        }

        cv::Mat filteredMat;
        qDebug() << "Applying mean filter with kernel size:" << kernelSize;
        cv::blur(mat, filteredMat, cv::Size(kernelSize, kernelSize));

        if (subtractFromOriginal) {
            qDebug() << "Converting original image to Mat...";
            cv::Mat originalMat = QImageToMat(originalImage);
            if (originalMat.empty()) {
                emit error(tr("原始图像转换失败"));
                return;
            }

            // 检查原始图像尺寸是否匹配
            if (originalMat.rows != mat.rows || originalMat.cols != mat.cols) {
                emit error(tr("原始图像尺寸不匹配"));
                return;
            }

            // 检查原始图像通道数
            if (originalMat.channels() != 3) {
                emit error(tr("原始图像必须是3通道RGB图像"));
                return;
            }

            qDebug() << "Subtracting filtered image from original...";
            cv::subtract(originalMat, filteredMat, filteredMat);
        }

        qDebug() << "Converting Mat back to QImage...";
        QImage newImage = MatToQImage(filteredMat);
        if (newImage.isNull()) {
            emit error(tr("结果图像转换失败"));
            return;
        }
        qDebug() << "New image size:" << newImage.width() << "x" << newImage.height();

        // 检查结果图像是否有效
        if (newImage.width() <= 0 || newImage.height() <= 0) {
            emit error(tr("结果图像尺寸无效"));
            return;
        }

        processedImage = newImage;
        emit imageProcessed();
    } catch (const cv::Exception& e) {
        qDebug() << "OpenCV error:" << e.what();
        emit error(tr("OpenCV错误: %1").arg(e.what()));
    } catch (const std::exception& e) {
        qDebug() << "General error:" << e.what();
        emit error(tr("处理错误: %1").arg(e.what()));
    } catch (...) {
        qDebug() << "Unknown error occurred";
        emit error(tr("未知错误"));
    }
}

void ImageProcessor::applyGaussianFilter(int kernelSize, double sigma, bool subtractFromOriginal)
{
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        return;
    }

    if (kernelSize % 2 == 0) {
        emit error(tr("核大小必须是奇数"));
        return;
    }

    cv::Mat mat = QImageToMat(processedImage);
    cv::Mat originalMat = QImageToMat(originalImage);
    cv::Mat filteredMat;
    cv::GaussianBlur(mat, filteredMat, cv::Size(kernelSize, kernelSize), sigma);

    if (subtractFromOriginal) {
        cv::subtract(originalMat, filteredMat, filteredMat);
    }

    processedImage = MatToQImage(filteredMat);
    emit imageProcessed();
}

void ImageProcessor::applyMedianFilter(int kernelSize, bool subtractFromOriginal)
{
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        return;
    }

    if (kernelSize % 2 == 0) {
        emit error(tr("核大小必须是奇数"));
        return;
    }

    cv::Mat mat = QImageToMat(processedImage);
    cv::Mat originalMat = QImageToMat(originalImage);
    cv::Mat filteredMat;
    cv::medianBlur(mat, filteredMat, kernelSize);

    if (subtractFromOriginal) {
        cv::subtract(originalMat, filteredMat, filteredMat);
    }

    processedImage = MatToQImage(filteredMat);
    emit imageProcessed();
}

void ImageProcessor::adjustBrightness(int value)
{
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        return;
    }

    cv::Mat mat = QImageToMat(processedImage);
    cv::Mat result;

    // 线性变换 y = kx + b
    // value范围是-100到100，转换为k
    double k = 1.0;  // 默认系数为1.0
    
    // 根据value调整k值
    if (value >= 0) {
        k = 1.0 + value / 100.0;  // k范围是1.0到2.0
    } else {
        k = 1.0 + value / 100.0;  // k范围是0.0到1.0
    }

    double b = 0.0;  // b将从外部传入

    mat.convertTo(result, -1, k, b);
    processedImage = MatToQImage(result);
    emit imageProcessed();
}

void ImageProcessor::adjustGammaContrast(double gamma, int offset)
{
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        return;
    }

    cv::Mat mat = QImageToMat(processedImage);
    cv::Mat result;

    // 归一化到0-1范围
    mat.convertTo(result, CV_32F, 1.0/255.0);

    // 应用gamma校正
    cv::pow(result, 1.0/gamma, result);

    // 添加偏移量
    result = result + offset/255.0;

    // 裁剪到0-1范围
    cv::threshold(result, result, 1.0, 1.0, cv::THRESH_TRUNC);
    cv::threshold(result, result, 0.0, 0.0, cv::THRESH_TOZERO);

    // 转换回0-255范围
    result.convertTo(result, CV_8U, 255.0);

    processedImage = MatToQImage(result);
    emit imageProcessed();
}

void ImageProcessor::convertToGrayscale()
{
    try {
        if (processedImage.isNull()) {
            qDebug() << "转换失败: 当前图像为空";
            emit error(tr("没有可处理的图像"));
            return;
        }

        qDebug() << "开始转换为灰度图，图像大小: " << processedImage.width() << "x" << processedImage.height();

        // 如果已经是灰度图像，不需要再转换
        if (processedImage.format() == QImage::Format_Grayscale8) {
            qDebug() << "图像已经是灰度格式，无需转换";
            emit imageProcessed();
            return;
        }

        cv::Mat mat = QImageToMat(processedImage);
        if (mat.empty()) {
            qDebug() << "转换为灰度图失败: 无法将图像转换为 Mat 格式";
            emit error(tr("图像格式转换失败"));
            return;
        }

        cv::Mat gray;
        // 检查通道数，只有彩色图像需要转换
        if (mat.channels() == 3 || mat.channels() == 4) {
            qDebug() << "将 " << mat.channels() << " 通道图像转换为灰度";
            cv::cvtColor(mat, gray, cv::COLOR_BGR2GRAY);
        } else if (mat.channels() == 1) {
            // 已经是单通道图像，直接拷贝
            gray = mat.clone();
            qDebug() << "图像已经是单通道，直接拷贝";
        } else {
            qDebug() << "不支持的通道数: " << mat.channels();
            emit error(tr("不支持的图像格式，通道数: %1").arg(mat.channels()));
            return;
        }

        if (gray.empty()) {
            qDebug() << "转换为灰度图失败: 处理后的 Mat 为空";
            emit error(tr("灰度图像处理失败"));
            return;
        }

        QImage grayImage = MatToQImage(gray);
        if (grayImage.isNull()) {
            qDebug() << "转换为灰度图失败: MatToQImage 返回空图像";
            emit error(tr("灰度图像转换失败"));
            return;
        }

        processedImage = grayImage;
        qDebug() << "转换为灰度图成功";
        emit imageProcessed();
    } catch (const cv::Exception& e) {
        qDebug() << "OpenCV 错误 (convertToGrayscale): " << e.what();
        emit error(tr("OpenCV 处理错误: %1").arg(e.what()));
    } catch (const std::exception& e) {
        qDebug() << "转换为灰度图异常: " << e.what();
        emit error(tr("灰度转换错误: %1").arg(e.what()));
    } catch (...) {
        qDebug() << "转换为灰度图时发生未知异常";
        emit error(tr("灰度转换发生未知错误"));
    }
}

void ImageProcessor::applyHistogramEqualization()
{
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        return;
    }

    cv::Mat mat = QImageToMat(processedImage);
    cv::Mat result;

    if (mat.channels() == 3) {
        // 如果是彩色图像，先转换为灰度图
        cv::cvtColor(mat, result, cv::COLOR_BGR2GRAY);
    } else {
        result = mat;
    }

    // 应用直方图均衡化
    cv::equalizeHist(result, result);

    processedImage = MatToQImage(result);
    emit imageProcessed();
}

void ImageProcessor::applyHistogramStretching()
{
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        return;
    }

    try {
        cv::Mat mat = QImageToMat(processedImage);
        if (mat.empty()) {
            emit error(tr("图像转换失败"));
            return;
        }

        cv::Mat result;
        if (mat.channels() == 3) {
            // 如果是彩色图像，转换为灰度图
            cv::cvtColor(mat, result, cv::COLOR_BGR2GRAY);
        } else if (mat.channels() == 1) {
            result = mat.clone();
        } else {
            emit error(tr("不支持的图像格式"));
            return;
        }

        // 找到最小和最大像素值
        double minVal, maxVal;
        cv::minMaxLoc(result, &minVal, &maxVal);

        // 如果图像已经是全范围，则不需要拉伸
        if (minVal == 0 && maxVal == 255) {
            return;
        }

        // 确保分母不为零
        if (maxVal == minVal) {
            emit error(tr("图像灰度值范围太小"));
            return;
        }

        // 应用线性拉伸
        double scale = 255.0 / (maxVal - minVal);
        double shift = -minVal * scale;
        result.convertTo(result, CV_8U, scale, shift);

        // 确保结果图像有效
        if (result.empty()) {
            emit error(tr("图像处理失败"));
            return;
        }

        QImage newImage = MatToQImage(result);
        if (newImage.isNull()) {
            emit error(tr("结果图像转换失败"));
            return;
        }

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
        qDebug() << "QImage is null";
        return cv::Mat();
    }

    cv::Mat mat;
    try {
        switch (image.format()) {
            case QImage::Format_RGB32:
            case QImage::Format_ARGB32:
            case QImage::Format_ARGB32_Premultiplied: {
                // 创建深拷贝以避免内存问题
                QImage copy = image.copy();
                mat = cv::Mat(copy.height(), copy.width(), CV_8UC4, copy.bits(), copy.bytesPerLine());
                cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
                break;
            }
            case QImage::Format_RGB888: {
                // 创建深拷贝以避免内存问题
                QImage copy = image.copy();
                mat = cv::Mat(copy.height(), copy.width(), CV_8UC3, copy.bits(), copy.bytesPerLine());
                cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
                break;
            }
            case QImage::Format_Grayscale8: {
                // 创建深拷贝以避免内存问题
                QImage copy = image.copy();
                mat = cv::Mat(copy.height(), copy.width(), CV_8UC1, copy.bits(), copy.bytesPerLine());
                break;
            }
            default: {
                // 转换为RGB32格式并创建深拷贝
                QImage converted = image.convertToFormat(QImage::Format_RGB32);
                QImage copy = converted.copy();
                mat = cv::Mat(copy.height(), copy.width(), CV_8UC4, copy.bits(), copy.bytesPerLine());
                cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
                break;
            }
        }
    } catch (const cv::Exception& e) {
        qDebug() << "OpenCV error in QImageToMat:" << e.what();
        return cv::Mat();
    } catch (const std::exception& e) {
        qDebug() << "Error in QImageToMat:" << e.what();
        return cv::Mat();
    }

    if (mat.empty()) {
        qDebug() << "Failed to convert QImage to Mat";
    }
    return mat;
}

// cv::Mat 转 QImage
QImage ImageProcessor::MatToQImage(const cv::Mat &mat)
{
    if (mat.empty()) {
        qDebug() << "Mat is empty";
        return QImage();
    }

    try {
        if (mat.type() == CV_8UC3) {
            cv::Mat rgb;
            cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
            // 创建深拷贝以确保数据安全
            cv::Mat rgbCopy = rgb.clone();
            QImage result(rgbCopy.data, rgbCopy.cols, rgbCopy.rows, rgbCopy.step, QImage::Format_RGB888);
            return result.copy();
        } else if (mat.type() == CV_8UC1) {
            // 创建深拷贝以确保数据安全
            cv::Mat grayCopy = mat.clone();
            QImage result(grayCopy.data, grayCopy.cols, grayCopy.rows, grayCopy.step, QImage::Format_Grayscale8);
            return result.copy();
        } else {
            qDebug() << "Unsupported Mat type:" << mat.type();
            return QImage();
        }
    } catch (const cv::Exception& e) {
        qDebug() << "OpenCV error in MatToQImage:" << e.what();
        return QImage();
    } catch (const std::exception& e) {
        qDebug() << "Error in MatToQImage:" << e.what();
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

    cv::Mat mat = QImageToMat(processedImage);
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

    mat.convertTo(result, -1, k, b);
    processedImage = MatToQImage(result);
    emit imageProcessed();
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
