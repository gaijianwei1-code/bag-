#!/usr/bin/env python3
"""T3.1 lookahead 观测节点：把 carrot 反算成 lookahead 距离，以 JSON Lines 输出。

不改上游源码（项目约束）。pursuit_local_planner 已在发布 `lookahead_point`
（carrot 点，base_link 系），本节点订阅它并输出：
    {"type":"lookahead","dist":1.766,"x":1.70,"y":0.48}   10 Hz
    {"type":"carrot","xy":[map_x,map_y]}                  位姿变换后（若有 TF）

与 live_bridge.py 同样输出 JSON Lines 到 stdout，非 JSON 行由宿主机丢弃。
"""
import json
import math
import sys
import time

import rospy
import tf2_ros
from geometry_msgs.msg import PointStamped
from std_msgs.msg import Float64

EMIT_HZ = 10.0

# 真实话题名（2026-11-19 实测）：private_nh.advertise 加在 sweep_base/PursuitLocalPlanner 下，
# 不是 /lookahead_point。用绝对名避免本节点的命名空间影响。
TOPIC_CARROT = "/sweep_base/PursuitLocalPlanner/lookahead_point"
TOPIC_THETA = "/sweep_base/PursuitLocalPlanner/theta"


def emit(obj):
    sys.stdout.write(json.dumps(obj, separators=(",", ":")) + "\n")
    sys.stdout.flush()


class Observer:
    def __init__(self):
        self.buf = None
        self.count = 0
        self.theta = None
        self.map_xy = None
        self.tf_buf = tf2_ros.Buffer()
        tf2_ros.TransformListener(self.tf_buf)
        rospy.Subscriber(TOPIC_CARROT, PointStamped, self.cb, queue_size=5)
        rospy.Subscriber(TOPIC_THETA, Float64, self.theta_cb, queue_size=5)

    def theta_cb(self, msg):
        self.theta = msg.data

    def cb(self, msg):
        x, y = msg.point.x, msg.point.y   # PointStamped 用 .point
        d = math.hypot(x, y)
        self.buf = {"type": "lookahead", "dist": round(d, 4),
                    "x": round(x, 4), "y": round(y, 4)}
        if self.theta is not None:
            self.buf["theta"] = round(self.theta, 5)
        self.count += 1

        # 顺带把 carrot 转到 map 系（供 UI 画前视目标点）
        try:
            tr = self.tf_buf.lookup_transform("map", msg.header.frame_id or "base_link",
                                              msg.header.stamp,
                                              rospy.Duration(0.3))
            t = tr.transform.translation
            c, s = 1.0, 0.0
            q = tr.transform.rotation
            c = 1 - 2 * (q.y * q.y + q.z * q.z)
            s = 2 * (q.x * q.y + q.w * q.z)
            mx = t.x + c * x - s * y
            my = t.y + s * x + c * y
            self.map_xy = [round(mx, 4), round(my, 4)]
        except Exception:  # noqa: BLE001  TF 未就绪时跳过，不影响主功能
            pass


def main():
    for _ in range(10):
        try:
            rospy.init_node("lookahead_observer", anonymous=True)
            break
        except rospy.exceptions.ROSInitException:
            time.sleep(1.0)
    else:
        emit({"type": "status", "state": "error", "reason": "no ROS master"})
        return 1

    obs = Observer()
    last_n = -1
    period = 1.0 / EMIT_HZ
    # 用 wall clock：sim time 未初始化时 rospy.Rate/time 会抛异常
    while not rospy.is_shutdown():
        if obs.buf is not None:
            emit(obs.buf)
            obs.buf = None
        if obs.map_xy is not None:
            emit({"type": "carrot", "xy": obs.map_xy})
            obs.map_xy = None
            if obs.count % 50 == 0 and obs.count != last_n:
                last_n = obs.count
        time.sleep(period)
    return 0


if __name__ == "__main__":
    sys.exit(main())
