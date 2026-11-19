#!/usr/bin/env python3
"""在容器内运行：采集位姿 + carrot，输出 JSON Lines。"""
import json
import math
import sys
import time

import rospy
from geometry_msgs.msg import PointStamped, PoseStamped

state = {"pose": None, "carrot": None}
out = []


def pose_cb(m):
    q = m.pose.orientation
    yaw = math.atan2(2 * (q.w * q.z + q.x * q.y), 1 - 2 * (q.y * q.y + q.z * q.z))
    state["pose"] = (m.pose.position.x, m.pose.position.y, yaw)


def carrot_cb(m):
    state["carrot"] = (m.point.x, m.point.y)   # PointStamped 用 .point 不是 .pose


def main():
    duration = float(sys.argv[1]) if len(sys.argv) > 1 else 25.0
    rospy.init_node("t32_collect", anonymous=True)
    rospy.Subscriber("/odom/current_pose", PoseStamped, pose_cb, queue_size=5)
    rospy.Subscriber("/sweep_base/PursuitLocalPlanner/lookahead_point",
                     PointStamped, carrot_cb, queue_size=5)
    # 用 wall clock 控制循环：sim time 可能未初始化，rospy.Rate/time 会抛异常
    t0 = time.time()
    while not rospy.is_shutdown():
        elapsed = time.time() - t0
        if elapsed > duration:
            break
        if state["pose"] and state["carrot"]:
            p, c = state["pose"], state["carrot"]
            out.append({"t": round(elapsed, 2),
                        "x": round(p[0], 4), "y": round(p[1], 4), "yaw": round(p[2], 5),
                        "cx": round(c[0], 4), "cy": round(c[1], 4),
                        "la": round(math.hypot(c[0], c[1]), 4)})
        time.sleep(0.2)
    # 一次性输出，避免逐行被 shell/buffer 影响
    sys.stdout.write("DIAG " + json.dumps({"n": len(out), "pose_seen": state["pose"] is not None,
                                           "carrot_seen": state["carrot"] is not None}) + "\n")
    sys.stdout.write("SAMPLES " + json.dumps(out) + "\n")
    sys.stdout.flush()
    return 0


if __name__ == "__main__":
    sys.exit(main())
