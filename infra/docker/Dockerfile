# ==========================================
# Builder
# ==========================================
FROM ubuntu:24.04 AS builder

# Install Build Dependencies
# - build-essential: gcc, g++, make
# - cmake: build system
# - openjdk: aeron build needs java
# - python: needed for swig python
# - other utility programs
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3.12 \
    python3.12-dev \
    git \
    curl \
    zip \
    unzip \
    wget \
    software-properties-common \
    && rm -rf /var/lib/apt/lists/*

# Setup Build Directory
WORKDIR /src
COPY . .

RUN infra/docker/install_gcc.sh
RUN infra/docker/install_ninja.sh

# Configure & Build
RUN scripts/configure.sh

# Build using all available cores
RUN cmake --build .build --config Release -j $(nproc)

# Install
RUN cmake --install .build --config Release --prefix /app/ARQ

# ==========================================
# Runtime
# ==========================================
FROM ubuntu:24.04 AS runtime

# Install Runtime Dependencies
# - ca-certificates for ssl
# - libssl for crypto
RUN apt-get update && apt-get install -y \
    ca-certificates \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Create App Directory
WORKDIR /app/ARQ

# Copy artifacts from the builder stage
COPY --from=builder /app/ARQ .

ENV LD_LIBRARY_PATH="/app/ARQ/lib"
ENV PATH="/app/ARQ/bin:$PATH"