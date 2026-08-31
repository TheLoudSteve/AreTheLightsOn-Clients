#!/usr/bin/env python3
"""Outbound-only heartbeat monitor for Are The Lights On."""

import json
import logging
import os
import signal
import sys
import time
from typing import Callable
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


INTERVAL_SECONDS = 60
VERSION = "0.1.0"
STOP = False


def required(name: str) -> str:
    value = os.environ.get(name, "").strip()
    if not value:
        raise ValueError(f"{name} is required")
    return value


def settings() -> tuple[str, str, str]:
    endpoint = required("ATLO_HEARTBEAT_URL")
    if not endpoint.startswith("https://"):
        raise ValueError("ATLO_HEARTBEAT_URL must use https")
    return endpoint, required("ATLO_DEVICE_ID"), required("ATLO_DEVICE_SECRET")


def heartbeat_payload(device_id: str, started_at: float) -> bytes:
    return json.dumps({
        "device_id": device_id,
        "uptime_seconds": max(0, int(time.monotonic() - started_at)),
        "firmware_version": f"docker/{VERSION}",
    }).encode("utf-8")


def send_heartbeat(endpoint: str, secret: str, payload: bytes, opener: Callable = urlopen) -> None:
    request = Request(endpoint, data=payload, method="POST", headers={
        "Authorization": f"Bearer {secret}",
        "Content-Type": "application/json",
        "User-Agent": f"arethelightson-client-docker/{VERSION}",
    })
    try:
        with opener(request, timeout=10) as response:
            if not 200 <= response.status < 300:
                raise RuntimeError(f"heartbeat was rejected with HTTP {response.status}")
    except HTTPError as error:
        raise RuntimeError(f"heartbeat was rejected with HTTP {error.code}") from error
    except URLError as error:
        raise RuntimeError(f"heartbeat request failed: {error.reason}") from error


def stop(_: int, __: object) -> None:
    global STOP
    STOP = True


def main() -> int:
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    try:
        endpoint, device_id, secret = settings()
    except ValueError as error:
        logging.error("configuration error: %s", error)
        return 2

    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)
    started_at = time.monotonic()
    logging.info("starting monitor for device %s", device_id)
    while not STOP:
        try:
            send_heartbeat(endpoint, secret, heartbeat_payload(device_id, started_at))
            logging.info("heartbeat accepted")
        except RuntimeError as error:
            logging.warning("%s", error)
        for _ in range(INTERVAL_SECONDS):
            if STOP:
                break
            time.sleep(1)
    logging.info("monitor stopped")
    return 0


if __name__ == "__main__":
    sys.exit(main())
