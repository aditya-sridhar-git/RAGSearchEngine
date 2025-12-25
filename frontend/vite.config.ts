import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,      // Fixed port - always use this
    strictPort: true // Fail if port is in use (instead of switching to another)
  }
})
