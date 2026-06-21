import { defineConfig, loadEnv } from 'vite'
import react from '@vitejs/plugin-react'
import tailwindcss from '@tailwindcss/vite'
import path from "path"

// https://vite.dev/config/
export default defineConfig(({ mode }) => {
  // Load environment variables from .env files based on the current mode
  const env = loadEnv(mode, process.cwd(), '');

  const apiTarget = env.VITE_API_TARGET || 'http://localhost:5147';

  console.log(`Proxying /arq-gateway calls to: ${apiTarget} (Mode: ${mode})`);

  return {
    plugins: [react(), tailwindcss()],
    resolve: {
      alias: {
        "@": path.resolve(__dirname, "./src"),
      },
    },
    server: {
      port: 3000,
      proxy: {
        '/arq-gateway': {
          target: apiTarget,
          changeOrigin: true,
          secure: false,
          // Strips '/arq-gateway' so the C# app just sees the route
          rewrite: (path) => path.replace(/^\/arq-gateway/, ''),
        },
      },
    },
  }
})
