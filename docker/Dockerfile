FROM python:3.13-alpine

LABEL org.opencontainers.image.source="https://github.com/TheLoudSteve/AreTheLightsOn-Clients" \
      org.opencontainers.image.description="Are The Lights On outbound Docker monitor"

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1

RUN addgroup -S atlo && adduser -S -G atlo -h /app atlo

WORKDIR /app
COPY monitor.py /app/monitor.py
USER atlo

ENTRYPOINT ["python", "/app/monitor.py"]
