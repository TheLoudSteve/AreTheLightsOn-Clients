# Are The Lights On Docker monitor

This container sends one authenticated, outbound HTTPS heartbeat per minute to Are The Lights On. It has no inbound ports.

## Before you run it

This is only a valid outage monitor when the host itself loses power when utility power is lost. Do not run it on a UPS-backed server, generator-backed server, battery-backed NAS, cloud VM, or other redundant-power setup. You attest to this during device registration.

## Install

1. In [Are The Lights On](https://arethelightson.com/contribute), select **Docker container**, choose the physical location of the host, and complete the utility-power attestation.
2. Copy the generated configuration into a new `.env` file next to `docker-compose.yml`. Treat it like a password; it contains the device secret.
3. Start the monitor:

   ```sh
   docker compose up -d
   ```

4. Confirm the monitor status on the contribution dashboard. It must first complete 15 uninterrupted minute heartbeats, then a 24-hour reliability warm-up.

## Update

```sh
docker compose pull
docker compose up -d
```

## Security model

- The image runs as a non-root user, uses a read-only root filesystem, drops Linux capabilities, and accepts no inbound traffic.
- The device secret is sent only as a Bearer credential to the HTTPS heartbeat endpoint.
- The secret is never written to logs. Keep `.env` private and do not commit it.
- Generating a new setup configuration in the dashboard immediately invalidates the prior secret.

## Local verification

```sh
python3 -m unittest docker/test_monitor.py
docker build -t arethelightson-client-docker ./docker
```
