#include "attributeAddDialog.h"
#include "ui_attributeAddDialog.h"

AttributeAddDialog::AttributeAddDialog(QWidget *parent, attribute *attr) :
    QDialog(parent),
    ui(new Ui::AttributeAddDialog)
{
    ui->setupUi(this);
    if (attr)
    {
        switch (attr->road_id) {
        case 20:
            ui->road_id_20->setChecked(true);
            break;
        case 10:
            ui->road_id_10->setChecked(true);
            break;
        case 4:
            ui->road_id_4->setChecked(true);
            break;
        case 2:
            ui->road_id_2->setChecked(true);
            break;
        case 3:
            ui->road_id_3->setChecked(true);
            break;
        case 1:
            ui->road_id_1->setChecked(true);
            break;
        case 0:
        default:
            ui->road_id_NO->setChecked(true);
            break;
        }
        ui->lineEdit_road_width->setText(QString::number(attr->road_width));
        switch (attr->bypass) {
        case 1:
            ui->bypass_off->setChecked(true);
            break;
        default:
            ui->bypass_on->setChecked(true);
            break;
        }
        if(attr->seismic == 1)
            ui->seismic->setChecked(true);
        // if(attr->turnAround == 1)
        //     ui->turnAround->setChecked(true);
        ui->lineEdit_turnAround->setText(QString::number(attr->turnAround));
        if(attr->trash == 1)
            ui->trash->setChecked(true);
        // if(attr->charge == 1)
        //     ui->charge->setChecked(true);
        ui->lineEdit_charge->setText(QString::number(attr->charge));
        if(attr->back == 1)
            ui->back->setChecked(true);
        if(attr->clean_flag == 1)
            ui->clean_flag->setChecked(true);

        if(attr->parking_area == 1)
            ui->parking_area->setChecked(true);

        if(attr->curve_paths == 1)
            ui->curve_paths->setChecked(true);

        if(attr->ignore_obstacles == 1)
            ui->ignore_obstacles->setChecked(true);

        if(attr->reverse_traj == 1)
            ui->reverse_traj->setChecked(true);

        int i = 0;
        for ( auto& it : attr->self_define )
        {
            if(i == 0)
            {
                ui->att_1->setText(it.first);
                ui->val_1->setText(it.second);
            }
            else if(i == 1)
            {
                ui->att_2->setText(it.first);
                ui->val_2->setText(it.second);
            }
            else if(i == 2)
            {
                ui->att_3->setText(it.first);
                ui->val_3->setText(it.second);
            }
            else
            {
                QMessageBox::critical(this, tr("属性过多"), tr("联系开发人员解决") );
                break;
            }
            i ++;
        }
        //如果att_1没有设置值，设定为network_id
        if ( ui->att_1->text().isEmpty() )
            ui->att_1->setText("network_id");
    }
}

AttributeAddDialog::~AttributeAddDialog()
{
    delete ui;
}

void AttributeAddDialog::done(int r)
{
    // 这里是你的条件
    if (r == QDialog::Accepted &&
        self_buttonBox_accepted()/* 你的判断条件 */) {    //在条件不满足时，不关闭窗口
        // 如果满足你的条件，接受对话框并关闭它
        QDialog::done(r);
    }
    else if (r == QDialog::Rejected) {
        // 如果点击的是Cancel按钮，接受对话框并关闭它
        QDialog::done(r);
    }
    else {
        // 如果不满足你的条件，更新窗口的一些数值并返回，对话框不会关闭
    }
}

bool AttributeAddDialog::self_buttonBox_accepted()
{
    std::string ans;

    if(ui->road_id_20->isChecked())
        ans += "20";
    else if(ui->road_id_10->isChecked())
        ans += "10";
    else if(ui->road_id_4->isChecked())
        ans += '4';
    else if(ui->road_id_3->isChecked())
        ans += '3';
    else if(ui->road_id_2->isChecked())
        ans += '2';
    else if(ui->road_id_1->isChecked())
        ans += '1';
    else if(ui->road_id_NO->isChecked())
        ans += '0';
    else
        ans += '0';

    ans += ',';

    //如果lineEdit_road_width没有设置值，设定为0
    if ( ui->lineEdit_road_width->text().isEmpty() )
        ui->lineEdit_road_width->setText("0");
    if( !ui->lineEdit_road_width->text().isEmpty() )
    {
        bool ok;
        int _road_width = ui->lineEdit_road_width->text().toInt(&ok);
        if (!ok) {
            QMessageBox::critical(this, tr("属性值错误"), tr("采样道路宽度的属性值有误\n各属性值必须为数字") );
            return false;
        }
        ans += std::to_string(_road_width);
    }

    ans += ',';

    if(ui->bypass_on->isChecked())
        ans +='0';
    else if(ui->bypass_off->isChecked())
        ans +='1';
    else
        ans +='0';

    ans += ',';

    if(ui->seismic->isChecked())
        ans +='1';
    else
        ans +='0';

    ans += ',';

    // if(ui->turnAround->isChecked())
    //     ans +='1';
    // else
    //     ans +='0';
    //如果lineEdit_turnAround没有设置值，设定为0
    if ( ui->lineEdit_turnAround->text().isEmpty() )
        ui->lineEdit_turnAround->setText("0");
    if( !ui->lineEdit_turnAround->text().isEmpty() )
    {
        bool ok;
        int _turnAround = ui->lineEdit_turnAround->text().toInt(&ok);
        if (!ok) {
            QMessageBox::critical(this, tr("属性值错误"), tr("掉头点的属性值有误\n各属性值必须为数字") );
            return false;
        }
//        if (_turnAround < 10 && _turnAround > -1)
//            ans += std::to_string(_turnAround);
//        else {
//            QMessageBox::critical(this, tr("属性值错误"), tr("掉头点的属性值请设置0~9范围的数字") );
//            return false;
//        }
        ans += std::to_string(_turnAround);
    }

    ans += ',';

    if(ui->trash->isChecked())
        ans +='1';
    else
        ans +='0';

    ans += ',';

    // if(ui->charge->isChecked())
    //     ans +='1';
    // else
    //     ans +='0';
    //如果lineEdit_charge没有设置值，设定为0
    if ( ui->lineEdit_charge->text().isEmpty() )
        ui->lineEdit_charge->setText("0");
    if( !ui->lineEdit_charge->text().isEmpty() )
    {
        bool ok;
        int _charge = ui->lineEdit_charge->text().toInt(&ok);
        if (!ok) {
            QMessageBox::critical(this, tr("属性值错误"), tr("充电点的属性值有误\n各属性值必须为数字") );
            return false;
        }
//        if (_charge < 10 && _charge > -1)
//            ans += std::to_string(_charge);
//        else {
//            QMessageBox::critical(this, tr("属性值错误"), tr("充电点的属性值请设置0~9范围的数字") );
//            return false;
//        }
        ans += std::to_string(_charge);
    }

    ans += ',';

    if(ui->back->isChecked())
        ans +='1';
    else
        ans +='0';

    ans += ',';

    if(ui->clean_flag->isChecked())
        ans +='1';
    else
        ans +='0';

    ans += ',';

    if(ui->parking_area->isChecked())
        ans +='1';
    else
        ans +='0';

    ans += ',';

    if(ui->curve_paths->isChecked())
        ans +='1';
    else
        ans +='0';

    ans += ',';

    if(ui->ignore_obstacles->isChecked())
        ans +='1';
    else
        ans +='0';

    ans += ',';

    if(ui->reverse_traj->isChecked())
        ans +='1';
    else
        ans +='0';

    ans += ',';

    if(!ui->att_1->text().isEmpty()&&!ui->val_1->text().isEmpty())
    {
        bool ok;
        ui->val_1->text().toInt(&ok);
        if (!ok) {
            QMessageBox::critical(this, tr("属性值错误"), ui->att_1->text() + tr("的属性值有误\n各属性值必须为数字") );
            return false;
        }
        ans = ans + ui->att_1->text().toStdString() + ',' + ui->val_1->text().toStdString() + ',';
    }

    if(!ui->att_2->text().isEmpty()&&!ui->val_2->text().isEmpty())
    {
        bool ok;
        ui->val_1->text().toInt(&ok);
        if (!ok) {
            QMessageBox::critical(this, tr("属性值错误"), ui->att_1->text() + tr("的属性值有误\n各属性值必须为数字") );
            return false;
        }
        ans = ans + ui->att_2->text().toStdString() + ',' + ui->val_2->text().toStdString() + ',';
    }

    if(!ui->att_3->text().isEmpty()&&!ui->val_3->text().isEmpty())
    {
        bool ok;
        ui->val_1->text().toInt(&ok);
        if (!ok) {
            QMessageBox::critical(this, tr("属性值错误"), ui->att_1->text() + tr("的属性值有误\n各属性值必须为数字") );
            return false;
        }
        ans = ans + ui->att_3->text().toStdString() + ',' + ui->val_3->text().toStdString() + ',';
    }

    qDebug() << "AttributeAddDialog::sendData: " << QString::fromStdString(ans);
    emit sendData(ans);
    return true;
}
