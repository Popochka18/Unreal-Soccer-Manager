import rawDefaultTheme from '../themes/default.json' with { type: 'json' };
import type { Theme } from './theme.js';

/**
 * Validates an untrusted theme object.
 *
 * Themes are shippable by modders (§9), so this runs on data we did not write.
 * A bad theme must fail loudly at load with a usable message rather than
 * producing a half-styled UI that looks like a rendering bug.
 */
export function parseTheme(input: unknown): Theme {
    if (typeof input !== 'object' || input === null) {
        throw new TypeError('theme: expected an object');
    }

    const candidate = input as Partial<Record<keyof Theme, unknown>>;

    const requireString = (key: keyof Theme): string => {
        const value = candidate[key];
        if (typeof value !== 'string') {
            throw new TypeError(`theme.${String(key)}: expected a string`);
        }
        return value;
    };

    const requireRecord = <T>(key: keyof Theme, kind: 'string' | 'number'): Readonly<Record<string, T>> => {
        const value = candidate[key];
        if (typeof value !== 'object' || value === null || Array.isArray(value)) {
            throw new TypeError(`theme.${String(key)}: expected an object`);
        }
        for (const [entryKey, entryValue] of Object.entries(value)) {
            if (typeof entryValue !== kind) {
                throw new TypeError(`theme.${String(key)}.${entryKey}: expected a ${kind}`);
            }
        }
        return value as Readonly<Record<string, T>>;
    };

    const colorScheme = requireString('colorScheme');
    if (colorScheme !== 'dark' && colorScheme !== 'light') {
        throw new TypeError(`theme.colorScheme: expected "dark" or "light", got "${colorScheme}"`);
    }

    const ramp = candidate.attributeRamp;
    if (!Array.isArray(ramp) || ramp.length === 0) {
        throw new TypeError('theme.attributeRamp: expected a non-empty array');
    }
    const attributeRamp = ramp.map((stop, index) => {
        if (typeof stop !== 'object' || stop === null) {
            throw new TypeError(`theme.attributeRamp[${index}]: expected an object`);
        }
        const { atLeast, color } = stop as Record<string, unknown>;
        if (typeof atLeast !== 'number' || atLeast < 0 || atLeast > 1) {
            throw new TypeError(`theme.attributeRamp[${index}].atLeast: expected a number in 0..1`);
        }
        if (typeof color !== 'string') {
            throw new TypeError(`theme.attributeRamp[${index}].color: expected a string`);
        }
        return { atLeast, color };
    });

    return {
        id: requireString('id'),
        name: requireString('name'),
        colorScheme,
        color: requireRecord<string>('color', 'string'),
        attributeRamp,
        space: requireRecord<number>('space', 'number'),
        font: requireRecord<string>('font', 'string'),
        fontSize: requireRecord<number>('fontSize', 'number'),
        density: requireRecord<number>('density', 'number'),
        radius: requireRecord<number>('radius', 'number'),
        motion: requireRecord<number>('motion', 'number'),
    };
}

/** The shipped theme. Validated at module load so a broken edit fails fast. */
export const defaultTheme: Theme = parseTheme(rawDefaultTheme);
