import { defineConfig } from 'vitest/config';
import react from '@vitejs/plugin-react';

export default defineConfig({
    plugins: [react()],
    server: {
        // §9: the UI must also run in a plain browser against a running server,
        // not only inside the Tauri shell. Keep this port stable so the dev
        // proxy configuration in the server does not have to guess.
        port: 5173,
        strictPort: true,
    },
    build: {
        // The Tauri shell loads this build output; Tauri targets a fixed,
        // modern webview, so there is no reason to ship legacy transpilation.
        target: 'es2022',
        sourcemap: true,
    },
    test: {
        globals: true,
        environment: 'jsdom',
        include: ['src/**/*.test.{ts,tsx}'],
    },
});
