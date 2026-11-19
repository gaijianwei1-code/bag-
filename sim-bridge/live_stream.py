"""T2.3 直播流接入：容器内起 live_bridge.py → 逐行转 timeline → 写进 _runs。

live_bridge 是现成的（sim/scripts/live_bridge.py，5 Hz state + 抽稀点云 + TF 合成），
我们只做三件事：起它、读它的 JSON Lines、用 live_adapter 转成 timeline schema。
"""
import json
import subprocess
import threading

CONTAINER = "roboneo-sim"
WS_IN_CONTAINER = "/root/catkin_ws"
BRIDGE_SCRIPT = f"{WS_IN_CONTAINER}/sim/scripts/live_bridge.py"

# 需要转发的 timeline 消息类型（T0.1 schema + path/obstacles 扩展）
FRAME_TYPES = {"frame", "front_hits", "status", "path", "obstacles"}


class LiveStream:
    """一次 run 的直播流。start() 后在后台线程持续产出行，调用方消费 messages。"""

    def __init__(self, run_id, duration, line="B", front_dist=3.0, half_angle=0.5,
                 on_message=None):
        self.run_id = run_id
        self.duration = duration
        self.line = line
        self.front_dist = front_dist
        self.half_angle = half_angle
        self.on_message = on_message
        self._proc = None
        self._thread = None
        self._stop = threading.Event()
        self._adapter = None
        self._stderr = []

    def start(self):
        from live_adapter import TimelineAdapter
        self._adapter = TimelineAdapter(self.front_dist, self.half_angle)
        cmd = (f"source /opt/ros/noetic/setup.bash >/dev/null 2>&1; "
               f"python3 -u {BRIDGE_SCRIPT} --duration {self.duration} "
               f"--line {self.line} --run-id {self.run_id}")
        self._proc = subprocess.Popen(
            ["docker", "exec", CONTAINER, "bash", "-lc", cmd],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=1)
        self._thread = threading.Thread(target=self._pump, daemon=True)
        self._thread.start()
        return self

    def _pump(self):
        """读 stdout 逐行解析；非 JSON 行（ROS 日志）丢弃。"""
        if self._proc is None:
            return
        for line in self._proc.stdout:
            if self._stop.is_set():
                break
            line = line.strip()
            if not line or not line.startswith("{"):
                continue
            try:
                obj = json.loads(line)
            except ValueError:
                continue
            for msg in self._adapter.feed(obj):
                if msg.get("type") in FRAME_TYPES and self.on_message:
                    try:
                        self.on_message(msg)
                    except Exception:  # noqa: BLE001  消费方异常不拖垮流
                        pass
        # 收尾：把 stderr 留档便于排错
        try:
            self._stderr = (self._proc.stderr.read() or "").splitlines()[-20:]
        except (ValueError, OSError):
            pass

    def stop(self, timeout=10):
        self._stop.set()
        if self._proc and self._proc.poll() is None:
            # docker exec 的子进程随容器内进程退出；这里先温和终止宿主机端
            self._proc.terminate()
            try:
                self._proc.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                self._proc.kill()
        if self._thread:
            self._thread.join(timeout=timeout)

    def close(self):
        self.stop()

    def stderr_tail(self):
        return self._stderr

    def __enter__(self):
        return self.start()

    def __exit__(self, *exc):
        self.stop()
        return False


def wait_for_ros(timeout_s=60):
    """等容器内 roscore 就绪（回放启动后才有）。返回 bool。"""
    import time
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        p = subprocess.run(
            ["docker", "exec", CONTAINER, "bash", "-lc",
             "source /opt/ros/noetic/setup.bash >/dev/null 2>&1; "
             "timeout 3 rosnode list 2>/dev/null | head -1"],
            capture_output=True, text=True)
        if p.returncode == 0 and p.stdout.strip():
            return True
        time.sleep(2.0)
    return False
