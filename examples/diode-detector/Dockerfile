FROM ubuntu:24.04@sha256:786a8b558f7be160c6c8c4a54f9a57274f3b4fb1491cf65146521ae77ff1dc54
SHELL ["/bin/bash", "-euo", "pipefail", "-c"]
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential=12.10ubuntu1 cmake=3.28.3-1build7 ninja-build=1.11.1-2 \
    git=1:2.43.0-1ubuntu7.3 ca-certificates=20260601~24.04.1 curl=8.5.0-2ubuntu10.15 \
    python3=3.12.3-0ubuntu2.1 python3-venv=3.12.3-0ubuntu2.1 \
    bison=2:3.8.2+dfsg-1ubuntu0.24.04.1 flex=2.6.4-8.2build1 \
    libreadline-dev=8.2-4build1 && rm -rf /var/lib/apt/lists/*
WORKDIR /tmp/ngspice-47
RUN curl -fL --retry 3 https://downloads.sourceforge.net/project/ngspice/ng-spice-rework/47/ngspice-47.tar.gz -o /tmp/ngspice.tar.gz \
    && echo '894e649651f1838a14095e5a5439e7d3aa63e87ede14d283173fda4fcdef675f  /tmp/ngspice.tar.gz' | sha256sum -c - \
    && tar -xzf /tmp/ngspice.tar.gz -C /tmp \
    && ./configure --with-x=no --disable-debug --disable-openmp \
    && make -j4 && make install && rm -rf /tmp/ngspice*
WORKDIR /tmp
RUN git clone https://github.com/catchorg/Catch2.git /tmp/catch2 \
    && git -C /tmp/catch2 checkout 31588bb4f56b638dd5afc28d3ebff9b9dcefb88d \
    && cmake -S /tmp/catch2 -B /tmp/catch-build -G Ninja -DCATCH_BUILD_TESTING=OFF \
    && cmake --build /tmp/catch-build --parallel 4 && cmake --install /tmp/catch-build \
    && rm -rf /tmp/catch2 /tmp/catch-build
COPY requirements.lock /tmp/requirements.lock
RUN python3 -m venv /opt/circuit-python \
    && /opt/circuit-python/bin/pip install --require-hashes --only-binary=:all: -r /tmp/requirements.lock
ENV PATH="/opt/circuit-python/bin:${PATH}" PYTHONDONTWRITEBYTECODE=1
WORKDIR /source
ENTRYPOINT ["python", "/source/examples/diode-detector/reproduce.py"]
