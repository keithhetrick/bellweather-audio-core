FROM ubuntu:24.04@sha256:786a8b558f7be160c6c8c4a54f9a57274f3b4fb1491cf65146521ae77ff1dc54
SHELL ["/bin/bash", "-euo", "pipefail", "-c"]
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential=12.10ubuntu1 cmake=3.28.3-1build7 \
    ca-certificates=20260601~24.04.1 \
    python3=3.12.3-0ubuntu2.1 python3-venv=3.12.3-0ubuntu2.1 \
    && rm -rf /var/lib/apt/lists/*
COPY requirements.lock /tmp/requirements.lock
RUN python3 -m venv /opt/model-python \
    && /opt/model-python/bin/pip install --require-hashes --only-binary=:all: -r /tmp/requirements.lock
ENV PATH="/opt/model-python/bin:${PATH}" PYTHONDONTWRITEBYTECODE=1 OPENBLAS_NUM_THREADS=1
WORKDIR /source
ENTRYPOINT ["python"]
