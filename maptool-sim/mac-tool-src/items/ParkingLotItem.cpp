#include "ParkingLotItem.h"

ParkingLotPointItem::ParkingLotPointItem(QAbstractGraphicsShapeItem* parent, QPointF p, int type)
    : MPointItem(parent, p),
        pointType(type)
{
    this->setFlag(QGraphicsItem::ItemIsMovable);
}

void ParkingLotPointItem::setIsWidthPoint(bool type)
{
    pointType = 2;
}
void ParkingLotPointItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    MPointItem::mouseMoveEvent(event);
    QPointF pos(event->scenePos().x(), event->scenePos().y());
    emit pointMoved(this, pos);
//    if ( is_widthPoint && isSelected() && event->buttons() & Qt::LeftButton )
//    {
//        QPointF p(event->scenePos().x(), event->scenePos().y());
//    }
}

ParkingLotItem::ParkingLotItem()
{
    m_complete = false;
    nameLabel = new QGraphicsTextItem(this); // 将文本绑定到 ParkingLotItem 上
    nameLabel->setDefaultTextColor(Qt::blue);
    nameLabel->setFont(QFont("Arial", 10));
    nameLabel->setPlainText("停车区");
    nameLabel->setZValue(1);  // 确保显示在上层
}

ParkingLotItem::~ParkingLotItem()
{
}

void ParkingLotItem::setStartPoint(QPointF pos)
{
    qDebug()<< "ParkingLotItem::setStartPoint receive message:" << pos;
    if (startPoint == nullptr)
    {
        startPoint = new ParkingLotPointItem(this, pos, 0);
        connect(startPoint, &ParkingLotPointItem::pointMoved, this, &ParkingLotItem::selfUpdate);
    }
    else
    {
        startPoint->setPoint(pos);
    }
    updateNameLabelPosition();
}

void ParkingLotItem::setEndPoint(QPointF pos)
{
    qDebug()<< "ParkingLotItem::setEndPoint receive message:" << pos;
    if (endPoint == nullptr)
    {
        endPoint = new ParkingLotPointItem(this, pos, 1);
        connect(endPoint, &ParkingLotPointItem::pointMoved, this, &ParkingLotItem::selfUpdate);
    }
    else
    {
        endPoint->setPoint(pos);
    }
    m_polygon.clear();
    m_polygon << startPoint->getPoint() << endPoint->getPoint();
    this->update();
}

double distanceFromPointToLine(const QPointF& a, const QPointF& b, const QPointF& c) {
    // 计算直线 AB 的参数
    double x1 = a.x(), y1 = a.y();
    double x2 = b.x(), y2 = b.y();
    double x3 = c.x(), y3 = c.y();

    // 使用点到直线的距离公式
    double numerator = (y2 - y1) * x3 - (x2 - x1) * y3 + x2 * y1 - y2 * x1;
    double denominator = std::sqrt((y2 - y1) * (y2 - y1) + (x2 - x1) * (x2 - x1));

    return numerator / denominator;
}

void ParkingLotItem::setWidth(QPointF pos)
{
    qDebug()<< "ParkingLotItem::setWidth receive message:" << pos;
    QPointF startP = startPoint->getPoint(), endP = endPoint->getPoint();
    QPointF longEdgeVector = endP - startP;
    qreal longEdgeLength = std::sqrt(longEdgeVector.x() * longEdgeVector.x() + longEdgeVector.y() * longEdgeVector.y());
    QPointF longEdgeUnitVector = longEdgeVector / longEdgeLength;
    m_width = distanceFromPointToLine(startP, endP, pos);
    QPointF widthVector = QPointF( longEdgeUnitVector.y(), - longEdgeUnitVector.x() ) * m_width;
    if (widthPoint == nullptr)
    {
        widthPoint = new ParkingLotPointItem(this, startP + widthVector, 2);
        connect(widthPoint, &ParkingLotPointItem::pointMoved, this, &ParkingLotItem::selfUpdate);
        m_complete = true;
    }
    else
    {
        widthPoint->setPoint(startP + widthVector);
    }
    m_polygon.clear();
    m_polygon << startP << startP + widthVector << endP + widthVector << endP;
    this->update();
}

void ParkingLotItem::selfUpdate(ParkingLotPointItem* pointItem, QPointF pos)
{
    if(pointItem->pointType == 0 || pointItem->pointType == 1)
    {
        this->setWidth(widthPoint->getPoint());
    }
    else
    {
        this->setWidth(pos);
    }
    updateNameLabelPosition();
}
void ParkingLotItem::updateNameLabelPosition()
{
    if (nameLabel && startPoint)
    {
        QPointF labelPos = startPoint->pos() + QPointF(0, -20);
        nameLabel->setPos(labelPos);
    }
}

QRectF ParkingLotItem::boundingRect() const
{
    return m_polygon.boundingRect();
}

void ParkingLotItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(200,50,50,40));

    painter->drawPolygon(m_polygon);
}
