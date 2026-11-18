#include "graphicsview.h"

#include <QMouseEvent>
#include <QScrollBar>
#include <QCursor>

//重写QGraphicsView实现图像的缩放

GraphicsView::GraphicsView(QWidget *parent) : QGraphicsView(parent)
{
}

GraphicsView::~GraphicsView()
{
}

//使得鼠标能够实现放大缩小
/*
void GraphicsView::wheelEvent(QWheelEvent *e)
{
    int distance = e->delta()/66;
    double val;

    if (distance != 0)
    {
        val = pow(1.2, distance);
        this->scale(val, val);
    }
}
*/
//231015-腾飞
void GraphicsView::wheelEvent(QWheelEvent *e)
{
    QPointF cursorPoint = e->position();
    QPointF scenePos = this->mapToScene(QPoint(cursorPoint.x(), cursorPoint.y()));
    QPointF centerscenePos = mapToScene(this->width() / 2, this->height() / 2);
    //    qreal viewWidth = this->viewport()->width();
    //    qreal viewHeight = this->viewport()->height();
    //    qreal hScale = cursorPoint.x() / viewWidth;
    //    qreal vScale = cursorPoint.y() / viewHeight;
    int distance = e->angleDelta().y()/66;
    double val;

    if (distance != 0)
    {
        val = pow(1.2, distance);
        this->scale(val, val);
        centerOn((centerscenePos.x()-scenePos.x())/val+scenePos.x(),(centerscenePos.y()-scenePos.y())/val+scenePos.y());
    }

}

//========== 鼠标中键按住拖动 = 平移地图 ==========
//macOS 上不少鼠标/触控板没有中键，额外提供「Option + 左键拖动」作为等效平移方式
static bool isPanTrigger(QMouseEvent *e)
{
    if (e->button() == Qt::MiddleButton)
        return true;
#if defined(Q_OS_MACOS)
    if (e->button() == Qt::LeftButton && (e->modifiers() & Qt::AltModifier))
        return true;
#endif
    return false;
}

void GraphicsView::mousePressEvent(QMouseEvent *e)
{
    if (isPanTrigger(e))
    {
        m_middlePanning = true;
        m_lastPos = e->pos();
        viewport()->setCursor(Qt::ClosedHandCursor);
        e->accept();
        return;                     //不再传递给 scene/item，避免误触发选点等操作
    }
    QGraphicsView::mousePressEvent(e);
}

void GraphicsView::mouseMoveEvent(QMouseEvent *e)
{
    if (m_middlePanning)
    {
        QPoint delta = e->pos() - m_lastPos;
        m_lastPos = e->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        e->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(e);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_middlePanning &&
        (e->button() == Qt::MiddleButton || e->button() == Qt::LeftButton))
    {
        m_middlePanning = false;
        viewport()->setCursor(Qt::ArrowCursor);
        e->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(e);
}
