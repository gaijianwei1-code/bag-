#ifndef PARKINGLOTITEM_H
#define PARKINGLOTITEM_H

#include <QGraphicsItem>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QPointer>
#include <QPolygonF>
#include <cmath>
#include "mapSelectItem.h"

class ParkingLotPointItem : public MPointItem
{
    Q_OBJECT
public:
    ParkingLotPointItem(QAbstractGraphicsShapeItem* parent, QPointF p, int type);
    void setIsWidthPoint(bool);
    int pointType;  //0-firstPoint, 1-secondPoint, 2-widthPoint

signals:
    void pointMoved(ParkingLotPointItem*, QPointF);

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

private:
};

class ParkingLotItem : public QObject, public QAbstractGraphicsShapeItem
{
    Q_OBJECT
public:
    ParkingLotItem();
    ~ParkingLotItem();
    inline QPointF getStartPoint() { return startPoint->getPoint(); }
    inline QPointF getEndPoint() { return endPoint->getPoint(); }
    inline qreal getWidth() { return m_width; }
    inline bool isComplete() { return m_complete; }
    void setName(const QString& newName) {
        name = newName;
        if (nameLabel)
            nameLabel->setPlainText(newName);
    }

    QString getName() const { return name; }

public slots:
    void setStartPoint(QPointF);
    void setEndPoint(QPointF);
    void setWidth(QPointF);
    void selfUpdate(ParkingLotPointItem*, QPointF);

protected:
    virtual QRectF boundingRect() const override;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
    void updateNameLabelPosition();

private:
    QPolygonF m_polygon;
    QPointer<ParkingLotPointItem> startPoint = nullptr;
    QPointer<ParkingLotPointItem> endPoint = nullptr;
    QPointer<ParkingLotPointItem> widthPoint = nullptr;
    qreal m_width;
    bool m_complete;
    QGraphicsTextItem* nameLabel = nullptr;  // 显示名称的图元
    QString name;
};

#endif // PARKINGLOTITEM_H
