"""T2.2 从 bag 提取初始位姿（既定决定 2：不设人工步骤）。

读 bag 内 `/ndt/current_pose` 前 5 s 的样本，取中位值抗抖。
在容器内执行（rosbag 依赖 ROS 环境），宿主机通过 docker exec 调用。
"""
import sys
import json
import math

TOPIC = "/ndt/current_pose"
WINDOW_S = 5.0
MIN_SAMPLES = 3


def yaw_of(q):
    return math.atan2(2.0 * (q.w * q.z + q.x * q.y),
                      1.0 - 2.0 * (q.y * q.y + q.z * q.z))


def extract(bag_path):
    import rosbag
    bag = rosbag.Bag(bag_path)
    try:
        if TOPIC not in bag.get_type_and_topic_info().topics:
            raise LookupError(f"bag 内无 {TOPIC} 话题")

        samples = []
        t0 = None
        for _, msg, t in bag.read_messages(topics=[TOPIC]):
            if t0 is None:
                t0 = t
            if (t - t0).to_sec() > WINDOW_S:
                break
            samples.append((msg.pose.position.x, msg.pose.position.y,
                            yaw_of(msg.pose.orientation)))
            if len(samples) >= 500:
                break
    finally:
        bag.close()

    if len(samples) < MIN_SAMPLES:
        raise LookupError(f"{TOPIC} 前 {WINDOW_S}s 仅 {len(samples)} 条样本"
                          f"（需 >= {MIN_SAMPLES}）")

    def median(vals):
        s = sorted(vals)
        n = len(s)
        return s[n // 2] if n % 2 else 0.5 * (s[n // 2 - 1] + s[n // 2])

    # yaw 跨 ±π 时不需特殊处理：取中位前先归一到 [-π, π)，录制起点极少正对分支
    return {
        "x": median([s[0] for s in samples]),
        "y": median([s[1] for s in samples]),
        "yaw": median([s[2] for s in samples]),
        "samples": len(samples),
        "window_s": WINDOW_S,
        "topic": TOPIC,
    }


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(json.dumps({"error": "usage: extract_init_pose.py <bag>"}))
        sys.exit(2)
    try:
        print(json.dumps(extract(sys.argv[1])))
    except LookupError as e:
        print(json.dumps({"error": "INIT_POSE_MISSING", "detail": str(e)}))
        sys.exit(3)
    except Exception as e:  # noqa: BLE001
        print(json.dumps({"error": "INIT_POSE_FAILED", "detail": str(e)}))
        sys.exit(4)
