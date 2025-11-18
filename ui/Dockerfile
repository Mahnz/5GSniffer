FROM python:3.11-slim

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1

WORKDIR /app
COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

COPY app ./app
COPY static ./static

ENV ZMQ_ENDPOINT="tcp://127.0.0.1:5557"
ENV RNTI_TTL_SECONDS="30"
ENV WS_BROADCAST_INTERVAL_MS="500"
ENV HOST="0.0.0.0"
ENV PORT="8088"

EXPOSE 8088

CMD ["python", "-m", "app.main"]

