import axios from "axios";

// For local development vite is configured with proxy so will replace this with local server IP
// When deployed caddy will use a reverse proxy to call the actual service
const BASE_URL = "/arq-gateway/api";

export const httpClient = axios.create({
    baseURL: BASE_URL,
    timeout: 10000, // 10 seconds
    headers: {
        "Content-Type": "application/json",
    },
});

// ------ REQUEST INTERCEPTOR ------

httpClient.interceptors.request.use(
    (config) => {
        // Will add auth tokens here in the future
        console.log(`[ARQ.Gateway] ${config.method?.toUpperCase()} ${config.url}`);
        return config;
    }
);

// ------ RESPONSE INTERCEPTOR ------

httpClient.interceptors.response.use(
    (response) => {
        return response;
    },
    (error) => {
        if (error.response) {
            // The gateway responded with a status code outside the 2xx range
            console.error(`[ARQ.Gateway] REST API Error ${error.response.status}:`, error.message);
        } else if (error.request) {
            // The request was made but no response was received (network failure/C# server down)
            console.error("[ARQ.Gateway] Network Error - No response received");
        }

        return Promise.reject(error);
    }
);

// ------ UTILITIES ------

export function parseApiError(error: unknown): string {
    if (axios.isAxiosError(error)) {
        if (error.response) {
            const status = error.response.status;
            
            // Try to extract a meaningful message from the C# Gateway's JSON response
            const serverMessage = error.response.data?.detail 
                               || error.response.data?.title 
                               || error.response.data?.message;

            return serverMessage 
                ? `HTTP ${status}: ${serverMessage}` 
                : `HTTP ${status}: Endpoint not found or rejected.`;
                
        } else if (error.request) {
            // The request left the browser, but the server never replied
            return "Server unreachable. Connection refused.";
        }
    }

    return error instanceof Error ? error.message : "An unknown error occurred.";
}