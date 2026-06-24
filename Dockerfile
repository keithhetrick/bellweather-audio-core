FROM ubuntu:24.04@sha256:786a8b558f7be160c6c8c4a54f9a57274f3b4fb1491cf65146521ae77ff1dc54 AS clang-fetch

ARG BWS_IMAGE_REVISION=unknown
ARG BWS_IMAGE_CREATED=unknown
ARG BWS_IMAGE_ARCH=unknown

LABEL org.opencontainers.image.title="Bellweather Audio Core clang proof" \
      org.opencontainers.image.description="Bellweather audio core clang proof image" \
      org.opencontainers.image.source="https://github.com/keithhetrick/bellweather-audio-core" \
      org.opencontainers.image.revision="${BWS_IMAGE_REVISION}" \
      org.opencontainers.image.created="${BWS_IMAGE_CREATED}" \
      org.opencontainers.image.vendor="Bellweather Studios" \
      com.bellweatherstudios.repo="bellweather-audio-core" \
      com.bellweatherstudios.image.role="oss-proof" \
      com.bellweatherstudios.lifecycle="proof" \
      com.bellweatherstudios.cleanup="eligible-after-14d" \
      com.bellweatherstudios.arch="${BWS_IMAGE_ARCH}"

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN export DEBIAN_FRONTEND=noninteractive \
    && apt-get update >/tmp/apt.log 2>&1 \
    && apt-get install -y --no-install-recommends \
        cmake ninja-build clang git python3 ca-certificates >>/tmp/apt.log 2>&1 \
    && rm -rf /var/lib/apt/lists/* /tmp/apt.log \
    || { cat /tmp/apt.log; exit 1; }

COPY . /src
WORKDIR /src

RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTING=ON \
        -DBWS_BUILD_BAROMETER_PLUGIN=OFF \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_CXX_COMPILER=clang++ \
        2>&1 | tee /tmp/configure.log \
    && ! grep -E "warning:|CMake Warning|Manually-specified variables were not used|Performing Test .* - Failed" /tmp/configure.log \
    && cmake --build build 2>&1 | tee /tmp/build.log \
    && ! grep -E "warning:|CMake Warning" /tmp/build.log \
    && ctest --test-dir build -L public-surface --output-on-failure \
    && ctest --test-dir build -R bw_dsp_metering --output-on-failure \
    && ctest --test-dir build -L framework-boundary --output-on-failure \
    && ctest --test-dir build -L bws-library --output-on-failure

CMD ["bash", "-c", "set -euo pipefail; echo '=== Public surface compile/link smoke ==='; ctest --test-dir build -L public-surface --output-on-failure; echo; echo '=== BS.1770 / EBU R128 conformance suite ==='; ctest --test-dir build -R bw_dsp_metering --output-on-failure; echo; echo '=== JUCE separation check ==='; ctest --test-dir build -L framework-boundary --output-on-failure; echo; echo '=== Library behavioral suites ==='; ctest --test-dir build -L bws-library --output-on-failure; echo; echo '============================================================'; echo '  Bellweather Audio Core: public surface + conformance + JUCE separation + library suites PASSED'; echo '============================================================'"]

FROM ubuntu:24.04@sha256:786a8b558f7be160c6c8c4a54f9a57274f3b4fb1491cf65146521ae77ff1dc54 AS gcc-fetch

ARG BWS_IMAGE_REVISION=unknown
ARG BWS_IMAGE_CREATED=unknown
ARG BWS_IMAGE_ARCH=unknown

LABEL org.opencontainers.image.title="Bellweather Audio Core gcc proof" \
      org.opencontainers.image.description="Bellweather audio core gcc proof image" \
      org.opencontainers.image.source="https://github.com/keithhetrick/bellweather-audio-core" \
      org.opencontainers.image.revision="${BWS_IMAGE_REVISION}" \
      org.opencontainers.image.created="${BWS_IMAGE_CREATED}" \
      org.opencontainers.image.vendor="Bellweather Studios" \
      com.bellweatherstudios.repo="bellweather-audio-core" \
      com.bellweatherstudios.image.role="oss-proof" \
      com.bellweatherstudios.lifecycle="proof" \
      com.bellweatherstudios.cleanup="eligible-after-14d" \
      com.bellweatherstudios.arch="${BWS_IMAGE_ARCH}"

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN export DEBIAN_FRONTEND=noninteractive \
    && apt-get update >/tmp/apt.log 2>&1 \
    && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git python3 ca-certificates >>/tmp/apt.log 2>&1 \
    && rm -rf /var/lib/apt/lists/* /tmp/apt.log \
    || { cat /tmp/apt.log; exit 1; }

COPY . /src
WORKDIR /src

RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTING=ON \
        -DBWS_BUILD_BAROMETER_PLUGIN=OFF \
        -DCMAKE_C_COMPILER=gcc \
        -DCMAKE_CXX_COMPILER=g++ \
        2>&1 | tee /tmp/configure.log \
    && ! grep -E "warning:|CMake Warning|Manually-specified variables were not used|Performing Test .* - Failed" /tmp/configure.log \
    && cmake --build build 2>&1 | tee /tmp/build.log \
    && ! grep -E "warning:|CMake Warning" /tmp/build.log \
    && ctest --test-dir build -L public-surface --output-on-failure \
    && ctest --test-dir build -R bw_dsp_metering --output-on-failure \
    && ctest --test-dir build -L framework-boundary --output-on-failure \
    && ctest --test-dir build -L bws-library --output-on-failure

CMD ["bash", "-c", "set -euo pipefail; echo '=== Public surface compile/link smoke ==='; ctest --test-dir build -L public-surface --output-on-failure; echo; echo '=== BS.1770 / EBU R128 conformance suite ==='; ctest --test-dir build -R bw_dsp_metering --output-on-failure; echo; echo '=== JUCE separation check ==='; ctest --test-dir build -L framework-boundary --output-on-failure; echo; echo '=== Library behavioral suites ==='; ctest --test-dir build -L bws-library --output-on-failure; echo; echo '============================================================'; echo '  Bellweather Audio Core: public surface + conformance + JUCE separation + library suites PASSED'; echo '============================================================'"]

FROM ubuntu:24.04@sha256:786a8b558f7be160c6c8c4a54f9a57274f3b4fb1491cf65146521ae77ff1dc54 AS gcc-system-catch2

ARG BWS_IMAGE_REVISION=unknown
ARG BWS_IMAGE_CREATED=unknown
ARG BWS_IMAGE_ARCH=unknown

LABEL org.opencontainers.image.title="Bellweather Audio Core system Catch2 proof" \
      org.opencontainers.image.description="Bellweather audio core system Catch2 proof image" \
      org.opencontainers.image.source="https://github.com/keithhetrick/bellweather-audio-core" \
      org.opencontainers.image.revision="${BWS_IMAGE_REVISION}" \
      org.opencontainers.image.created="${BWS_IMAGE_CREATED}" \
      org.opencontainers.image.vendor="Bellweather Studios" \
      com.bellweatherstudios.repo="bellweather-audio-core" \
      com.bellweatherstudios.image.role="oss-proof" \
      com.bellweatherstudios.lifecycle="proof" \
      com.bellweatherstudios.cleanup="eligible-after-14d" \
      com.bellweatherstudios.arch="${BWS_IMAGE_ARCH}"

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN export DEBIAN_FRONTEND=noninteractive \
    && apt-get update >/tmp/apt.log 2>&1 \
    && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git python3 ca-certificates catch2 >>/tmp/apt.log 2>&1 \
    && rm -rf /var/lib/apt/lists/* /tmp/apt.log \
    || { cat /tmp/apt.log; exit 1; }

COPY . /src
WORKDIR /src

RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTING=ON \
        -DBWS_BUILD_BAROMETER_PLUGIN=OFF \
        -DBWS_FETCH_TEST_DEPS=OFF \
        -DCMAKE_C_COMPILER=gcc \
        -DCMAKE_CXX_COMPILER=g++ \
        2>&1 | tee /tmp/configure.log \
    && ! grep -E "warning:|CMake Warning|Manually-specified variables were not used|Performing Test .* - Failed" /tmp/configure.log \
    && cmake --build build 2>&1 | tee /tmp/build.log \
    && ! grep -E "warning:|CMake Warning" /tmp/build.log \
    && ctest --test-dir build -L public-surface --output-on-failure \
    && ctest --test-dir build -R bw_dsp_metering --output-on-failure \
    && ctest --test-dir build -L framework-boundary --output-on-failure \
    && ctest --test-dir build -L bws-library --output-on-failure

CMD ["bash", "-c", "set -euo pipefail; echo '=== Public surface compile/link smoke ==='; ctest --test-dir build -L public-surface --output-on-failure; echo; echo '=== BS.1770 / EBU R128 conformance suite ==='; ctest --test-dir build -R bw_dsp_metering --output-on-failure; echo; echo '=== JUCE separation check ==='; ctest --test-dir build -L framework-boundary --output-on-failure; echo; echo '=== Library behavioral suites ==='; ctest --test-dir build -L bws-library --output-on-failure; echo; echo '============================================================'; echo '  Bellweather Audio Core: public surface + conformance + JUCE separation + library suites PASSED'; echo '============================================================'"]

FROM ubuntu:24.04@sha256:786a8b558f7be160c6c8c4a54f9a57274f3b4fb1491cf65146521ae77ff1dc54 AS barometer-clang

ARG BWS_IMAGE_REVISION=unknown
ARG BWS_IMAGE_CREATED=unknown
ARG BWS_IMAGE_ARCH=unknown

LABEL org.opencontainers.image.title="Bellweather Audio Core Barometer proof" \
      org.opencontainers.image.description="Bellweather audio core Barometer proof image" \
      org.opencontainers.image.source="https://github.com/keithhetrick/bellweather-audio-core" \
      org.opencontainers.image.revision="${BWS_IMAGE_REVISION}" \
      org.opencontainers.image.created="${BWS_IMAGE_CREATED}" \
      org.opencontainers.image.vendor="Bellweather Studios" \
      com.bellweatherstudios.repo="bellweather-audio-core" \
      com.bellweatherstudios.image.role="oss-proof" \
      com.bellweatherstudios.lifecycle="proof" \
      com.bellweatherstudios.cleanup="eligible-after-14d" \
      com.bellweatherstudios.arch="${BWS_IMAGE_ARCH}"

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN export DEBIAN_FRONTEND=noninteractive \
    && apt-get update >/tmp/apt.log 2>&1 \
    && apt-get install -y --no-install-recommends \
        cmake ninja-build clang git python3 python3-fonttools pkg-config ca-certificates \
        libgtk-3-dev libwebkit2gtk-4.1-dev \
        libasound2-dev libjack-jackd2-dev libfreetype-dev libfontconfig1-dev \
        libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev \
        libxrandr-dev libxrender-dev libglu1-mesa-dev mesa-common-dev \
        libcurl4-openssl-dev >>/tmp/apt.log 2>&1 \
    && rm -rf /var/lib/apt/lists/* /tmp/apt.log \
    || { cat /tmp/apt.log; exit 1; }

COPY . /src
WORKDIR /src

RUN cmake -S . -B build-barometer -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DBUILD_TESTING=OFF \
        -DBWS_BUILD_BAROMETER_PLUGIN=ON \
        -DBWS_JUCE_FETCH=ON \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_CXX_COMPILER=clang++ \
        2>&1 | tee /tmp/configure.log \
    && ! grep -E "warning:|CMake Warning|Manually-specified variables were not used|Performing Test .* - Failed" /tmp/configure.log \
    && cmake --build build-barometer --target BwsBarometer_VST3 --parallel 2 2>&1 \
        | tee /tmp/build.log \
        | sed -E '/^removing moduleinfo[.]json$/d;/^creating .*Barometer[.]vst3$/d' \
    && ! grep -E "warning:|CMake Warning" /tmp/build.log

CMD ["bash", "-c", "set -euo pipefail; find build-barometer/plugins/production/BwsBarometer -type d -name 'Barometer.vst3' | grep -q .; echo 'Barometer VST3 build PASSED'"]

# Keep quickstart as the final stage: `docker build .` uses the last stage.
FROM clang-fetch AS quickstart

CMD ["bash", "-c", "set -euo pipefail; echo '=== Public surface compile/link smoke ==='; ctest --test-dir build -L public-surface --output-on-failure; echo; echo '=== BS.1770 / EBU R128 conformance suite ==='; ctest --test-dir build -R bw_dsp_metering --output-on-failure; echo; echo '=== JUCE separation check ==='; ctest --test-dir build -L framework-boundary --output-on-failure; echo; echo '=== Library behavioral suites ==='; ctest --test-dir build -L bws-library --output-on-failure; echo; echo '============================================================'; echo '  Bellweather Audio Core: public surface + conformance + JUCE separation + library suites PASSED'; echo '============================================================'"]
