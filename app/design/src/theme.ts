// Theme contract — CLAUDE.md §9.
//
// Colours, fonts and layout density are *data*, so a modder can ship a skin
// without touching source. This file defines the shape and how a theme becomes
// CSS custom properties; the values live in themes/*.json.
//
// No component may hardcode a colour or a spacing value. The ESLint config
// cannot catch that, so it is a review item — see the /ui command.

export interface AttributeRampStop {
    /**
     * Lower bound of this band as a fraction of the attribute's range, 0..1.
     *
     * Deliberately not a raw attribute value: the 1-20 scale is a game rule and
     * lives in the database (§3). The server sends the value and its bounds;
     * the UI only normalises and looks up a colour.
     */
    readonly atLeast: number;
    readonly color: string;
}

export interface Theme {
    readonly id: string;
    readonly name: string;
    readonly colorScheme: 'dark' | 'light';
    readonly color: Readonly<Record<string, string>>;
    readonly attributeRamp: readonly AttributeRampStop[];
    readonly space: Readonly<Record<string, number>>;
    readonly font: Readonly<Record<string, string>>;
    readonly fontSize: Readonly<Record<string, number>>;
    readonly density: Readonly<Record<string, number>>;
    readonly radius: Readonly<Record<string, number>>;
    readonly motion: Readonly<Record<string, number>>;
}

/**
 * Picks the ramp colour for a normalised value in 0..1.
 *
 * Formatting, not deciding (§3): the caller has already been told the value and
 * its bounds by the server.
 */
export function attributeColor(theme: Theme, normalised: number): string {
    const clamped = Math.min(1, Math.max(0, normalised));

    // Walk down so the highest matching band wins.
    for (let i = theme.attributeRamp.length - 1; i >= 0; i -= 1) {
        const stop = theme.attributeRamp[i];
        if (stop !== undefined && clamped >= stop.atLeast) {
            return stop.color;
        }
    }

    return theme.color['textMuted'] ?? '#8b949e';
}

/**
 * Flattens a theme into CSS custom properties, applied once at the root.
 *
 * Numeric scales become px strings here so that no component has to remember
 * which tokens carry units.
 */
export function themeToCssVariables(theme: Theme): Record<string, string> {
    const vars: Record<string, string> = {};

    for (const [key, value] of Object.entries(theme.color)) {
        vars[`--color-${key}`] = value;
    }
    for (const [key, value] of Object.entries(theme.font)) {
        vars[`--font-${key}`] = value;
    }
    for (const [group, scale] of [
        ['space', theme.space],
        ['size', theme.fontSize],
        ['density', theme.density],
        ['radius', theme.radius],
    ] as const) {
        for (const [key, value] of Object.entries(scale)) {
            vars[`--${group}-${key}`] = `${value}px`;
        }
    }
    for (const [key, value] of Object.entries(theme.motion)) {
        vars[`--motion-${key}`] = `${value}ms`;
    }

    return vars;
}
