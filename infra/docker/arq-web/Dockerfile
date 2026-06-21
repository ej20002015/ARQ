# ==========================================
# Builder
# ==========================================
FROM node:alpine AS builder

WORKDIR /src

# Grab packages
COPY /ui/arq-web/package*.json ./
RUN npm ci

# Build
COPY ./ui/arq-web .
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