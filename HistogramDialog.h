#ifndef HISTOGRAMDIALOG_H
#define HISTOGRAMDIALOG_H

#include <QDialog>
#include <QImage>
#include <QVector>
#include <QShowEvent>

class QLabel;
class QFrame;

class HistogramDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HistogramDialog(QWidget *parent = nullptr);
    ~HistogramDialog();

    // Update the histogram with a new image
    void updateHistogram(const QImage& image);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void setupUi();
    void calculateHistogram(const QImage& image);
    void renderHistogram();

    QVector<int> m_histogram;    // Stores the histogram data (256 values for grayscale)
    QPixmap m_histogramPixmap;   // Rendered histogram image
    QLabel* m_histogramLabel;    // Label to display the histogram
    QLabel* m_statsLabel;        // Label to display histogram statistics
    QFrame* m_histogramFrame;    // Frame for the histogram display
    QImage m_currentImage;       // Store current image for refresh
};

#endif // HISTOGRAMDIALOG_H 