#!/usr/bin/env python3
"""诊断：为什么 collect_t32 的 carrot_seen=False，但 rostopic echo 能看到 seq。"""
import json
import math
import sys

import rospy
from geometry_msgs.msg import PointStamped, PoseStamped

n_pose = [0]
n_carrot = [0]
carrot_msgs = []


def pose_cb(m):
    n_pose[0] += 1


def carrot_cb(m):
    n_carrot[0] += 1
    if len(carrot_msgs) < 3:
        carrot_msgs.append([round(m.point.x, 3), round(m.point.y, 3)])


def main():
    duration = float(sys.argv[1]) if len(sys.argv) > 1 else 20.0
    rospy.init_node("t32_diag", anonymous=True)
    rospy.Subscriber("/odom/current_pose", PoseStamped, pose_cb, queue_size=5)
    rospy.Subscriber("/sweep_base/PursuitLocalPlanner/lookahead_point",
                     PointStamped, carrot_cb, queue_size=5)
    rate = rospy.Rate(2.0)
    t0 = rospy.Time.now()
    while not rospy.is_shutdown() and (rospy.Time.now() - t0).to_sec() < duration:
        rate.sleep()
    out = {
        "n_pose": n_pose[0],
        "n_carrot": n_carrot[0],
        "carrot_samples": carrot_msgs,
        "sim_time": round(rospy.Time.now().to_sec(), 2),
        "topics": sorted([t for t in rospy.get_published_topics() if "lookahead" in t[0]]),
    }
    sys.stdout.write("DIAG " + json.dumps(out) + "\n")
    sys.stdout.flush()
    return 0


if __name__ == "__main__":
    sys.exit(main())
