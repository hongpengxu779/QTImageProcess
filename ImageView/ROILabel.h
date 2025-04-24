#ifndef ROILABEL_H
#define ROILABEL_H

#include <QLabel>
#include <QMouseEvent>
#include <QDebug>

// 自定义QLabel子类，用于处理鼠标事件
class ROILabel : public QLabel
{
    Q_OBJECT
public:
    explicit ROILabel(QWidget* parent = nullptr) : QLabel(parent) {
        setMouseTracking(true);  // 启用鼠标跟踪
        setFocusPolicy(Qt::StrongFocus);  // 可以接收键盘焦点
        qDebug() << "ROILabel实例已创建，鼠标追踪已启用";
    }
    
    void enterEvent(QEnterEvent *event) override {
        setCursor(Qt::CrossCursor);
        QLabel::enterEvent(event);
    }
    
    void leaveEvent(QEvent *event) override {
        setCursor(Qt::ArrowCursor);
        QLabel::leaveEvent(event);
    }
    
protected:
    void mousePressEvent(QMouseEvent* event) override {
        qDebug() << "ROILabel: 鼠标按下在位置" << event->pos();
        emit mousePressed(event);
        QLabel::mousePressEvent(event);
    }
    
    void mouseMoveEvent(QMouseEvent* event) override {
        emit mouseMoved(event);
        QLabel::mouseMoveEvent(event);
    }
    
    void mouseReleaseEvent(QMouseEvent* event) override {
        qDebug() << "ROILabel: 鼠标释放在位置" << event->pos();
        emit mouseReleased(event);
        QLabel::mouseReleaseEvent(event);
    }
    
signals:
    void mousePressed(QMouseEvent* event);
    void mouseMoved(QMouseEvent* event);
    void mouseReleased(QMouseEvent* event);
};

#endif // ROILABEL_H 