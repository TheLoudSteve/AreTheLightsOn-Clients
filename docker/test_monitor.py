import importlib.util
import json
from pathlib import Path
import unittest


SPEC = importlib.util.spec_from_file_location("monitor", Path(__file__).with_name("monitor.py"))
assert SPEC and SPEC.loader
monitor = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(monitor)


class Response:
    status = 202

    def __enter__(self):
        return self

    def __exit__(self, *_):
        return False


class MonitorTests(unittest.TestCase):
    def test_payload_uses_the_standard_heartbeat_contract(self):
        payload = json.loads(monitor.heartbeat_payload("node_abc", monitor.time.monotonic() - 90))
        self.assertEqual(payload["device_id"], "node_abc")
        self.assertGreaterEqual(payload["uptime_seconds"], 89)
        self.assertEqual(payload["firmware_version"], "docker/0.1.0")
        self.assertNotIn("wifi_rssi", payload)

    def test_heartbeat_is_authorized_and_json(self):
        requests = []

        def opener(request, timeout):
            requests.append((request, timeout))
            return Response()

        monitor.send_heartbeat("https://example.com/v1/heartbeats", "private-secret", b"{}", opener)
        request, timeout = requests[0]
        self.assertEqual(timeout, 10)
        self.assertEqual(request.get_header("Authorization"), "Bearer private-secret")
        self.assertEqual(request.get_header("Content-type"), "application/json")

    def test_rejects_non_https_endpoints(self):
        import os
        old = os.environ.get("ATLO_HEARTBEAT_URL")
        os.environ.update({"ATLO_HEARTBEAT_URL": "http://example.com", "ATLO_DEVICE_ID": "node_abc", "ATLO_DEVICE_SECRET": "secret"})
        with self.assertRaisesRegex(ValueError, "https"):
            monitor.settings()
        if old is None:
            del os.environ["ATLO_HEARTBEAT_URL"]
        else:
            os.environ["ATLO_HEARTBEAT_URL"] = old


if __name__ == "__main__":
    unittest.main()
