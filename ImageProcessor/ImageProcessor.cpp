#include "ImageProcessor.h"
#include <QImage>
#include <QColor>
#include <cmath>
#include <algorithm>
#include <vector>
#include <opencv2/opencv.hpp>
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

    cv::Mat mat = QImageToMat(processedImage);
    cv::Mat originalMat = QImageToMat(originalImage);
    cv::Mat filteredMat;
    cv::blur(mat, filteredMat, cv::Size(kernelSize, kernelSize));

    if (subtractFromOriginal) {
        cv::subtract(originalMat, filteredMat, filteredMat);
    }

    processedImage = MatToQImage(filteredMat);
    emit imageProcessed();
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
    // value范围是-100到100，转换为k和b
    double k = 1.0 + value / 100.0;  // k范围是0到2
    double b = value;  // b范围是-100到100

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
    if (processedImage.isNull()) {
        emit error(tr("没有可处理的图像"));
        return;
    }

    cv::Mat mat = QImageToMat(processedImage);
    cv::Mat gray;

    if (mat.channels() == 3) {
        cv::cvtColor(mat, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = mat;
    }

    processedImage = MatToQImage(gray);
    emit imageProcessed();
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
    cv::Mat mat;
    switch (image.format()) {
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32:
        case QImage::Format_ARGB32_Premultiplied:
            mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
            cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
            break;
        case QImage::Format_RGB888:
            mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
            cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
            break;
        case QImage::Format_Grayscale8:
            mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.constBits(), image.bytesPerLine());
            break;
        default:
            mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
            cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
    }
    return mat;
}

// cv::Mat 转 QImage
QImage ImageProcessor::MatToQImage(const cv::Mat &mat)
{
    if (mat.type() == CV_8UC3) {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
    } else if (mat.type() == CV_8UC1) {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
    }
    return QImage();
}
