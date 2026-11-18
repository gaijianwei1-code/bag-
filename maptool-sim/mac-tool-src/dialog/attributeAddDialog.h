#ifndef ATTRIBUTEADDDIALOG_H
#define ATTRIBUTEADDDIALOG_H

#include <QDialog>
#include <string>
#include <QMessageBox>
#include <QDebug>
#include <set>

#define ATTR_NUM 13
inline const std::set<QString> KEY =
    { "pixel_x", "pixel_y", "x", "y", "yaw", "road_id", "road_width",
    "bypass", "seismic", "turnAround", "trash", "charge", "back", "clean_flag",
    "parking_area", "curve_paths", "ignore_obstacles", "reverse_traj" };

struct attribute{
    int road_id;
    int road_width;
    int bypass;
    int seismic;
    int turnAround;
    int trash;
    int charge;
    int back;
    int clean_flag;
    int parking_area;
    int curve_paths;
    int ignore_obstacles;
    int reverse_traj;
    std::map<QString, QString> self_define;
    attribute(int *arr) :
        road_id(arr[0]), road_width(arr[1]), bypass(arr[2]), seismic(arr[3]), turnAround(arr[4]), trash(arr[5]),
        charge(arr[6]), back(arr[7]), clean_flag(arr[8]), parking_area(arr[9]), curve_paths(arr[10]), ignore_obstacles(arr[11]),
        reverse_traj(arr[12])
    {}
    // 拷贝构造函数实现深拷贝
    attribute(const attribute* other) :
        road_id(other->road_id),
        road_width(other->road_width),
        bypass(other->bypass),
        seismic(other->seismic),
        turnAround(other->turnAround),
        trash(other->trash),
        charge(other->charge),
        back(other->back),
        clean_flag(other->clean_flag),
        parking_area(other->parking_area),
        curve_paths(other->curve_paths),
        ignore_obstacles(other->ignore_obstacles),
        reverse_traj(other->reverse_traj),
        self_define(other->self_define) // std::map 会自动进行深拷贝
    {}
    bool operator==(const attribute& b) const {
        if (road_id == b.road_id &&
            road_width == b.road_width &&
            bypass == b.bypass &&
            seismic == b.seismic &&
            turnAround == b.turnAround &&
            trash == b.trash &&
            charge == b.charge &&
            back == b.back &&
            clean_flag == b.clean_flag &&
            parking_area == b.parking_area &&
            curve_paths == b.curve_paths &&
            ignore_obstacles == b.ignore_obstacles &&
            reverse_traj == b.reverse_traj &&
            self_define == b.self_define )
            return true;
        else
            return false;
    }
    bool operator!=(const attribute& b) const {
        if (*this == b) return false;
        return true;
    }
};

namespace Ui {
class AttributeAddDialog;
}

class AttributeAddDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttributeAddDialog(QWidget *parent = nullptr, attribute *attr = nullptr );
    ~AttributeAddDialog();
    void done(int r) override;
    bool self_buttonBox_accepted();

private slots:

signals:
    void sendData(std::string);

private:
    Ui::AttributeAddDialog *ui;
};

#endif // ATTRIBUTEADDDIALOG_H
