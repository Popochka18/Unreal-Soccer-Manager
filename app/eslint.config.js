// ESLint flat config — CLAUDE.md §9.
//
// The rules below that are set to "error" are not style preferences; they are
// the §3 hard-NOs expressed in a form CI can enforce.

import js from '@eslint/js';
import tseslint from 'typescript-eslint';

export default tseslint.config(
  {
    ignores: [
      '**/dist/**',
      '**/node_modules/**',
      '**/*.config.js',
      '**/playwright-report/**',
      'ipc/src/generated.ts',
      // Cargo build output. The Tauri crate is Rust and is deliberately outside
      // the pnpm workspace (ADR-0002), but `cargo build` emits generated .js
      // under target/, which the type-aware parser then rejects as being
      // outside every tsconfig. CI never sees this because the `app` job does
      // not build the crate — only a developer who has run `cargo build` does.
      'tauri/target/**',
    ],
  },

  js.configs.recommended,
  ...tseslint.configs.recommendedTypeChecked,

  {
    languageOptions: {
      parserOptions: {
        projectService: true,
        tsconfigRootDir: import.meta.dirname,
      },
    },
    rules: {
      // §3: no `any`, no `@ts-ignore`, no swallowed errors.
      '@typescript-eslint/no-explicit-any': 'error',
      '@typescript-eslint/ban-ts-comment': [
        'error',
        { 'ts-ignore': true, 'ts-expect-error': 'allow-with-description' },
      ],
      'no-empty': ['error', { allowEmptyCatch: false }],
      '@typescript-eslint/no-unused-vars': [
        'error',
        { argsIgnorePattern: '^_', varsIgnorePattern: '^_' },
      ],
      '@typescript-eslint/no-floating-promises': 'error',
      '@typescript-eslint/no-misused-promises': 'error',
      'eqeqeq': ['error', 'always'],
    },
  },

  // §9: /app/ui may not fetch() anything itself. All server traffic goes
  // through the generated client in /app/ipc, which is the only place that
  // knows the contract version.
  {
    files: ['ui/**/*.{ts,tsx}'],
    rules: {
      'no-restricted-globals': [
        'error',
        { name: 'fetch', message: 'Use the generated client from @pitchforge/ipc (§9).' },
      ],
      'no-restricted-properties': [
        'error',
        {
          object: 'window',
          property: 'fetch',
          message: 'Use the generated client from @pitchforge/ipc (§9).',
        },
      ],
    },
  },
);
