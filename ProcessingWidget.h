#ifndef PROCESSINGWIDGET_H
#define PROCESSINGWIDGET_H

#include <QWidget>
#include <QImage>

class QPushButton;
class QSlider;
class QTabWidget;
class QGraphicsView;
class QLabel;
class QCheckBox;
class QGroupBox;

class ProcessingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProcessingWidget(QWidget *parent = nullptr);
    ~ProcessingWidget();

    // Getters for UI elements
    QPushButton* getSelectButton() const { return btnSelect; }
    QPushButton* getSelectFolderButton() const { return btnSelectFolder; }
    QPushButton* getSaveButton() const { return btnSave; }
    QPushButton* getShowOriginalButton() const { return btnShowOriginal; }
    QPushButton* getFlipHButton() const { return btnFlipH; }
    QPushButton* getFlipVButton() const { return btnFlipV; }
    QPushButton* getMeanFilterButton() const { return btnMeanFilter; }
    QPushButton* getGaussianFilterButton() const { return btnGaussianFilter; }
    QPushButton* getMedianFilterButton() const { return btnMedianFilter; }
    QPushButton* getGrayscaleButton() const { return btnGrayscale; }
    
    QSlider* getBrightnessSlider() const { return sliderBrightness; }
    QSlider* getGammaSlider() const { return sliderGamma; }
    QSlider* getOffsetSlider() const { return sliderOffset; }
    QSlider* getRSlider() const { return sliderR; }
    QSlider* getGSlider() const { return sliderG; }
    QSlider* getBSlider() const { return sliderB; }
    
    QCheckBox* getSubtractFilteredCheckbox() const { return m_subtractFiltered; }
    QCheckBox* getRgbToGrayCheckBox() const { return m_rgbToGray; }
    QCheckBox* getShowHistogramCheckbox() const { return m_showHistogram; }
    
    bool getSubtractFiltered() const { return m_subtractFiltered ? m_subtractFiltered->isChecked() : false; }
    bool getShowHistogram() const { return m_showHistogram ? m_showHistogram->isChecked() : false; }
    
    // Image display function
    void displayImage(const QImage &image);
    QImage getCurrentImage() const { return currentImage; }

signals:
    void imageLoaded(const QImage& image);
    void imageProcessed(const QImage& image);
    void mouseMoved(const QPoint& pos, int grayValue);
    void imageStatsUpdated(double meanValue);
    void showHistogramRequested(bool show); // Signal to show histogram dialog

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();
    void updateImageStats();
    double calculateMeanValue(const QImage &image) const;
    
    // Buttons
    QPushButton *btnSelect;
    QPushButton *btnSelectFolder;
    QPushButton *btnSave;
    QPushButton *btnShowOriginal;
    QPushButton *btnFlipH;
    QPushButton *btnFlipV;
    QPushButton *btnMeanFilter;
    QPushButton *btnGaussianFilter;
    QPushButton *btnMedianFilter;
    QPushButton *btnHistEqual;
    QPushButton *btnGrayscale;
    
    // UI Elements
    QTabWidget *tabWidget;
    QGraphicsView *graphicsView;
    QLabel *imageLabel;
    QGroupBox *gbBasic;
    QSlider *sliderBrightness;
    QSlider *sliderGamma;
    QSlider *sliderOffset;
    QGroupBox *gbColor;
    QSlider *sliderR;
    QSlider *sliderG;
    QSlider *sliderB;
    QCheckBox *m_subtractFiltered;
    QCheckBox *m_rgbToGray;
    QCheckBox *m_showHistogram; // Checkbox for showing the histogram
    
    // Image handling
    QImage currentImage;
    double m_zoomFactor;
    const double ZOOM_FACTOR_STEP;
    const double MAX_ZOOM;
    const double MIN_ZOOM;
};

#endif // PROCESSINGWIDGET_H 