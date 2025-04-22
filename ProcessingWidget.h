class ProcessingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProcessingWidget(QWidget *parent = nullptr);
    ~ProcessingWidget();

    // ... existing code ...

signals:
    void imageLoaded(const QImage& image);
    void imageProcessed(const QImage& image);
    void mouseMoved(const QPoint& pos, int grayValue);
    void imageStatsUpdated(double meanValue);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    // ... existing code ...
    void updateImageStats();
}; 