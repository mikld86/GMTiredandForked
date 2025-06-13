import { defineConfig } from 'vite';
import preact from '@preact/preset-vite';
import analyzer, { adapter } from 'vite-bundle-analyzer';

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [preact()],
  build: {
    rollupOptions: {
      treeshake: "smallest"
    }
  },

  server: {
    proxy: {
      '/api': {
        target: 'http://gaggimate.local/',
        changeOrigin: true,
      },
      '/ws': {
        target: 'ws://gaggimate.local',
        ws: true,
      }
    },
  },
});
