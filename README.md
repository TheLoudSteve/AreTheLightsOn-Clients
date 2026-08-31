# Are The Lights On clients

Open-source monitor clients for [Are The Lights On](https://arethelightson.com), a community project for detecting local power outages and restoration.

| Client | Status | Purpose |
| --- | --- | --- |
| [Docker](docker/) | Available | Run an outbound-only monitor on utility-powered hardware. |
| ESP32 | Planned | A small dedicated utility-power monitor. |

Every client sends a private, authenticated heartbeat to the service. Contributors' device locations and identities are never publicly exposed; public maps show only coarse coverage areas.

## Safety requirement

A monitor is useful only when it loses power with the utility. Do not install a client on UPS-, generator-, battery-, or cloud-backed hardware. You must attest to this during registration.

## Shared protocol

See [the heartbeat protocol](docs/protocol.md). Each client may release independently while keeping that contract consistent.
