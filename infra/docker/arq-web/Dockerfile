# ==========================================
# Builder
# ==========================================
FROM node:alpine AS builder

# Setup Build Directory
WORKDIR /src
COPY ./ui/arq-web .

# Build
RUN npm ci
RUN npm run build

# ==========================================
# Runtime
# ==========================================
FROM caddy:alpine AS runtime

EXPOSE 80

WORKDIR /app/arq-web

COPY infra/docker/arq-web/Caddyfile /etc/caddy/Caddyfile

# Copy artifacts from the builder stage
COPY --from=builder /src/dist .