#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QtMath>

//重写QGraphicsView实现图像的缩放
class GraphicsView : public QGraphicsView
{
public:
    GraphicsView(QWidget* parent = nullptr);
    ~GraphicsView();

protected:
    virtual void wheelEvent(QWheelEvent *e);

    //鼠标中键按住拖动 = 平移地图
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);

private:
    bool m_middlePanning = false;   //是否正在用中键平移
    QPoint m_lastPos;               //上一次鼠标位置
};

#endif // GRAPHICSVIEW_H
