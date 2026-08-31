# Monitor heartbeat protocol

Clients send an outbound `POST` to their private `ATLO_HEARTBEAT_URL` once per minute.

```http
Authorization: Bearer <ATLO_DEVICE_SECRET>
Content-Type: application/json
```

```json
{
  "device_id": "node_example",
  "uptime_seconds": 3600,
  "firmware_version": "docker/0.1.0"
}
```

`wifi_rssi` is optional and should be sent only by clients that can report a real Wi-Fi RSSI value. The server records receipt time and ignores client timestamps.

Clients must not log the Bearer credential or expose inbound ports.
