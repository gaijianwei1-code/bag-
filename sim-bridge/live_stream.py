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
# b_line_run.py 用 `roscore -p 11323` 起自己的 master，不是默认 11311。
# live_bridge 必须指向同一个端口，否则永远 "Unable to register with master"
ROS_MASTER_URI = "http://localhost:11323"

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
        self._proc_obs = None
        self._thread = None
        self._thread_obs = None
        self._stop = threading.Event()
        self._adapter = None
        self._stderr = []

    def start(self):
        from live_adapter import TimelineAdapter
        self._adapter = TimelineAdapter(self.front_dist, self.half_angle)
        src = ("source /opt/ros/noetic/setup.bash >/dev/null 2>&1; "
               "source /root/catkin_ws/devel/setup.bash >/dev/null 2>&1; "
               f"export ROS_MASTER_URI={ROS_MASTER_URI}; ")

        # 主进程：live_bridge（位姿/点云/path/obstacles）
        cmd = (src + f"python3 -u {BRIDGE_SCRIPT} --duration {self.duration} "
               f"--line {self.line} --run-id {self.run_id}")
        self._proc = subprocess.Popen(
            ["docker", "exec", CONTAINER, "bash", "-lc", cmd],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=1)

        # T3.1 伴随进程：lookahead 观测（补 lookahead_dist / carrot）
        # 与主进程共用同一 ROS master；无 master 时它自己会报错退出，不影响主流程
        obs = (src + f"python3 -u {WS_IN_CONTAINER}/sim/scripts/lookahead_observer.py")
        self._proc_obs = subprocess.Popen(
            ["docker", "exec", CONTAINER, "bash", "-lc", obs],
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, bufsize=1)

        self._thread = threading.Thread(target=self._pump, daemon=True)
        self._thread.start()
        self._thread_obs = threading.Thread(target=self._pump_obs, daemon=True)
        self._thread_obs.start()
        return self

    def _pump_obs(self):
        """读 lookahead 观测进程的 JSON 行，交给同一适配器。"""
        if self._proc_obs is None:
            return
        try:
            for line in self._proc_obs.stdout:
                if self._stop.is_set():
                    break
                line = line.strip()
                if not line.startswith("{"):
                    continue
                try:
                    obj = json.loads(line)
                except ValueError:
                    continue
                for msg in self._adapter.feed(obj):
                    if msg.get("type") in FRAME_TYPES and self.on_message:
                        try:
                            self.on_message(msg)
                        except Exception:  # noqa: BLE001
                            pass
        except (ValueError, OSError):
            pass

    def _pump(self):
        """读 stdout 逐行解析；非 JSON 行（ROS 日志）丢弃。

        stderr 用独立线程持续收集——否则 pipe 满会阻塞子进程，且崩溃原因丢失。
        """
        if self._proc is None:
            return

        def drain_stderr():
            try:
                for line in self._proc.stderr:
                    line = line.rstrip()
                    if line:
                        self._stderr.append(line)
                        del self._stderr[:-40]   # 只留最后 40 行
            except (ValueError, OSError):
                pass

        err_thread = threading.Thread(target=drain_stderr, daemon=True)
        err_thread.start()

        for line in self._proc.stdout:
            if self._stop.is_set():
                break
            line = line.strip()
            if not line:
                continue
            # 关键：rospy 的日志（"Unable to register with master..."）也走 stdout，
            # 不是 stderr。必须留档，否则"为什么没有帧"无法诊断。
            if not line.startswith("{"):
                self._stderr.append(line)
                del self._stderr[:-40]
                continue
            try:
                obj = json.loads(line)
            except ValueError:
                self._stderr.append(f"[bad json] {line[:200]}")
                del self._stderr[:-40]
                continue
            for msg in self._adapter.feed(obj):
                if msg.get("type") in FRAME_TYPES and self.on_message:
                    try:
                        self.on_message(msg)
                    except Exception:  # noqa: BLE001  消费方异常不拖垮流
                        pass
        err_thread.join(timeout=2)

    def stop(self, timeout=10):
        self._stop.set()
        for p in (self._proc, self._proc_obs):
            if p and p.poll() is None:
                p.terminate()
                try:
                    p.wait(timeout=timeout)
                except subprocess.TimeoutExpired:
                    p.kill()
        for th in (self._thread, self._thread_obs):
            if th:
                th.join(timeout=timeout)

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
    """等容器内 ROS master 就绪。用法：在回放启动后调用。返回 bool。

    用 rosparam list 探活（比 rosnode list 快且不依赖节点已注册）。
    """
    import time
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        try:
            p = subprocess.run(
                ["docker", "exec", CONTAINER, "bash", "-lc",
                 "source /opt/ros/noetic/setup.bash >/dev/null 2>&1; "
                 f"export ROS_MASTER_URI={ROS_MASTER_URI}; "
                 "rosparam list /rosdistro 2>/dev/null | head -1"],
                capture_output=True, text=True, timeout=15)
        except subprocess.TimeoutExpired:
            time.sleep(1.0)
            continue
        if p.stdout.strip():
            return True
        time.sleep(1.0)
    return False
