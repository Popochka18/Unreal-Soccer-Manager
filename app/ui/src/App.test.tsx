import { describe, expect, it } from 'vitest';
import { render, screen } from '@testing-library/react';
import { attributeColor, defaultTheme } from '@pitchforge/design';
import { CONTRACT_VERSION } from '@pitchforge/ipc';

import { App } from './App.js';

describe('App', () => {
    it('renders the negotiated contract version', () => {
        render(<App />);
        expect(screen.getByText(`v${CONTRACT_VERSION}`)).toBeDefined();
    });
});

describe('attributeColor', () => {
    it('picks the highest band at or below the value', () => {
        // The ramp is theme data, so assert the selection behaviour rather than
        // specific hex values — a reskin must not break this test.
        const low = attributeColor(defaultTheme, 0);
        const high = attributeColor(defaultTheme, 1);
        expect(low).not.toEqual(high);
        expect(high).toEqual(defaultTheme.attributeRamp.at(-1)?.color);
    });

    it('clamps out-of-range input instead of throwing', () => {
        expect(attributeColor(defaultTheme, -5)).toEqual(attributeColor(defaultTheme, 0));
        expect(attributeColor(defaultTheme, 99)).toEqual(attributeColor(defaultTheme, 1));
    });
});
