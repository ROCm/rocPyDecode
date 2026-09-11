FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG ROCM_SERIES=10.1
ARG GPU_TARGET
ARG ROCM_BUILD=20260825-32791995050
ARG ROCM_PACKAGE_VERSION=10.1.0~20260825-32791995050
ARG ALLOW_UNSIGNED_ROCM_REPOSITORY=false

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        git \
        gnupg \
        libavcodec-dev \
        libavformat-dev \
        libavutil-dev \
        libdlpack-dev \
        libnuma-dev \
        libswscale-dev \
        ninja-build \
        pkg-config \
        pybind11-dev \
        python3-numpy \
        python3-pytest \
        wget \
    && rm -rf /var/lib/apt/lists/* \
    && mkdir -p /etc/apt/keyrings \
    && wget -qO- https://repo.amd.com/rocm/packages-multi-arch/gpg/rocm.gpg \
        | gpg --dearmor -o /etc/apt/keyrings/amdrocm.gpg

RUN if [ "${ALLOW_UNSIGNED_ROCM_REPOSITORY}" = "true" ]; then \
        repository_options="trusted=yes"; \
    elif [ "${ALLOW_UNSIGNED_ROCM_REPOSITORY}" = "false" ]; then \
        repository_options="signed-by=/etc/apt/keyrings/amdrocm.gpg"; \
    else \
        echo "ALLOW_UNSIGNED_ROCM_REPOSITORY must be true or false" >&2; \
        exit 2; \
    fi \
    && test -n "${GPU_TARGET}" \
    && echo "deb [${repository_options}] https://nightly.repo.amd.com/rocm/core/packages/ubuntu2404/${ROCM_BUILD} stable main" \
        > /etc/apt/sources.list.d/rocm-nightly.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        "amdrocm-core-sdk${ROCM_SERIES}-${GPU_TARGET}=${ROCM_PACKAGE_VERSION}" \
        "amdrocm-decode-test${ROCM_SERIES}=${ROCM_PACKAGE_VERSION}" \
        "amdrocm-jpeg-test${ROCM_SERIES}=${ROCM_PACKAGE_VERSION}" \
    && rm -rf /var/lib/apt/lists/*

ENV ROCM_PATH=/opt/rocm
ENV ROCM_HOME=/opt/rocm
ENV PATH=/opt/rocm/bin:/opt/rocm/lib/llvm/bin:${PATH}
ENV CMAKE_PREFIX_PATH=/opt/rocm/lib/cmake
ENV LD_LIBRARY_PATH=/opt/rocm/lib

RUN hipconfig --full \
    && test -d "${ROCM_PATH}/share/rocdecode/utils" \
    && test -d "${ROCM_PATH}/share/rocdecode/video" \
    && test -d "${ROCM_PATH}/share/rocjpeg/images"

WORKDIR /workspace/rocPyDecode

CMD ["bash"]
