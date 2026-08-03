export type { Theme, AttributeRampStop } from './theme.js';
export { attributeColor, themeToCssVariables } from './theme.js';
export { defaultTheme, parseTheme } from './defaultTheme.js';

// The dense table primitive, the attribute cell and the comparison primitive
// land at M6 (§9). They go here, and every screen uses them rather than rolling
// its own — that is what keeps 5 000-row tables at 60 fps.
