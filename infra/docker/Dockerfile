# ==========================================
# C++ Builder
# ==========================================
FROM ubuntu:24.04 AS cpp-builder

# Install Build Dependencies
# - build-essential: gcc, g++, make
# - cmake: build system
# - openjdk: aeron build needs java
# - python: needed for swig python
# - ccache: enables caching of C++ build files
# - other utility programs
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3.12 \
    python3.12-dev \
    ccache \
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

# Install other dependencies using scripts
RUN bash infra/docker/install_gcc.sh
RUN bash infra/docker/install_ninja.sh

# Tell cmake to use ccache
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache

# Set ccache properties
ENV CCACHE_DIR=/root/.ccache
ENV CCACHE_MAXSIZE=5G

# Configure
RUN scripts/configure.sh

# Build using all available cores and ccache
RUN --mount=type=cache,target=/root/.ccache \
    cmake --build .build --config Release -j $(nproc)

# Install
RUN cmake --install .build --config Release --prefix /app/ARQ

# ==========================================
# C#/dotnet Builder
# ==========================================
FROM ubuntu:24.04 AS dotnet-builder

# Install Build Dependencies
# - dotnet-sdk-10.0 for building dotnet10 projects
RUN apt-get update && apt-get install -y \
    dotnet-sdk-10.0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Right at the start copy ARQ dotnet projects restore them to optimise layering
COPY dotnet/ARQ.Gateway/ARQ.Gateway.csproj ./dotnet/ARQ.Gateway/
COPY dotnet/ARQ.NET/ARQ.NET.csproj ./dotnet/ARQ.NET/
RUN dotnet restore ./dotnet/ARQ.Gateway/ARQ.Gateway.csproj

COPY dotnet/ ./dotnet/

WORKDIR /src/dotnet/ARQ.Gateway
RUN dotnet publish -c Release -o /app/ARQ/dotnet/ARQ.Gateway

# ==========================================
# Runtime
# ==========================================
FROM ubuntu:24.04 AS runtime

# Install Runtime Dependencies
# - ca-certificates for ssl
# - libssl for crypto
# - aspnetcore-runtime-10.0 for running dotnet10 projects
RUN apt-get update && apt-get install -y \
    ca-certificates \
    libssl-dev \
    aspnetcore-runtime-10.0 \
    && rm -rf /var/lib/apt/lists/*

# Create App Directory
WORKDIR /app/ARQ

# Copy cpp artifacts from the builder stage
COPY --from=cpp-builder /app/ARQ .

# Copy C# Gateway artifacts into a subfolder of the ARQ root
COPY --from=dotnet-builder /app/ARQ/dotnet/ARQ.Gateway ./dotnet/ARQ.Gateway

# Core environment variables
ENV ARQ_home="/app/ARQ"
ENV LD_LIBRARY_PATH="/app/ARQ/lib"
ENV PATH="/app/ARQ/bin:$PATH"

# Dotnet specifics
ENV ASPNETCORE_URLS="http://+:8080"
EXPOSE 8080