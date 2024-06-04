// ANDREW KIM & ROCCO SCINTO

import { createApp } from 'vue'
import './style.css'
import App from './App.vue'
import '/node_modules/bootstrap/dist/css/bootstrap.min.css';
import PrimeVue from 'primevue/config';
import { definePreset } from 'primevue/themes';
import Aura from 'primevue/themes/aura';
import 'primeicons/primeicons.css'

const app = createApp(App)

const MyPreset = definePreset(Aura, {
    semantic: {
        primary: {
            50: '{zinc.50}',
            100: '{zinc.100}',
            200: '{zinc.200}',
            300: '{zinc.300}',
            400: '{zinc.400}',
            500: '{zinc.500}',
            600: '{zinc.600}',
            700: '{zinc.700}',
            800: '{zinc.800}',
            900: '{zinc.900}',
            950: '{zinc.950}'
        }
    }
});

app.use(PrimeVue, {
    // Default theme configuration
    theme: {
        preset: MyPreset,
        options: {
            prefix: 'p',
            darkModeSelector: 'false',
            cssLayer: false
        }
    }
});

app.mount('#app')
