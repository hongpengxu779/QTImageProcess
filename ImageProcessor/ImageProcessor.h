#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QObject>
#include <QImage>
#include <opencv2/opencv.hpp>

class ImageProcessor : public QObject
{
    Q_OBJECT
public:
    explicit ImageProcessor(QObject *parent = nullptr);
    ~ImageProcessor() override;

    // 图像处理函数
    bool loadImage(const QString &filePath);
    const QImage& getOriginalImage() const;
    const QImage& getProcessedImage() const;
    void resetToOriginal();

    // 图像处理操作
    void flipHorizontal();
    void flipVertical();
    void applyMeanFilter(int kernelSize = 3, bool subtractFromOriginal = false);
    void applyGaussianFilter(int kernelSize = 3, double sigma = 1.0, bool subtractFromOriginal = false);
    void applyMedianFilter(int kernelSize = 3, bool subtractFromOriginal = false);

    // 新增图像处理函数
    void adjustBrightness(int value);  // 亮度调整 y = kx + b
    void applyLinearTransform(int kValue, int bValue);  // 线性变换 y = kx + b
    void adjustGammaContrast(double gamma, int offset);  // Gamma对比度调整
    void convertToGrayscale();  // RGB转灰度
    void applyHistogramEqualization();  // 直方图均衡化
    void applyHistogramStretching();  // 直方图拉伸
    
    // 灰度图像处理相关函数
    void saveGrayscaleImage();  // 保存当前灰度图像状态
    bool isGrayscale() const;   // 判断当前图像是否为灰度图
    void restoreGrayscaleImage();  // 恢复到灰度图像状态

private:
    // QImage 和 cv::Mat 转换函数
    static cv::Mat QImageToMat(const QImage &image);
    static QImage MatToQImage(const cv::Mat &mat);

signals:
    void imageLoaded(bool success);
    void imageProcessed();
    void error(const QString &errorMessage);

private:
    QImage originalImage;
    QImage processedImage;
    QImage grayscaleImage;
};

#endif // IMAGEPROCESSOR_H
