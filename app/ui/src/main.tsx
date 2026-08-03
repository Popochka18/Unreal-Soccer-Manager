import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import { defaultTheme, themeToCssVariables } from '@pitchforge/design';

import { App } from './App.js';
import './index.css';

const root = document.getElementById('root');
if (root === null) {
    throw new Error('index.html is missing #root');
}

// Tokens are applied once at the root as CSS custom properties. Components read
// them via var(); none of them hardcode a colour or a spacing value (§9).
for (const [name, value] of Object.entries(themeToCssVariables(defaultTheme))) {
    document.documentElement.style.setProperty(name, value);
}
document.documentElement.style.colorScheme = defaultTheme.colorScheme;

createRoot(root).render(
    <StrictMode>
        <App />
    </StrictMode>,
);
