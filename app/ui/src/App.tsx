import { CONTRACT_VERSION } from '@pitchforge/ipc';
import { defaultTheme } from '@pitchforge/design';

/**
 * M0 placeholder.
 *
 * It exists to prove the three workspace packages resolve and compose, and it
 * renders nothing the game needs. The eleven real screens land at M6 (§9);
 * build them with the /ui command, not by growing this file.
 */
export function App(): React.JSX.Element {
    return (
        <main style={{ padding: 'var(--space-7)' }}>
            <h1 style={{ fontSize: 'var(--size-xxl)', margin: 0 }}>PitchForge</h1>
            <p style={{ color: 'var(--color-textMuted)', marginTop: 'var(--space-4)' }}>
                M0 skeleton. No server, no world, no match engine wired up yet.
            </p>
            <dl style={{ marginTop: 'var(--space-7)', display: 'grid', gap: 'var(--space-3)' }}>
                <div>
                    <dt style={{ color: 'var(--color-textFaint)', fontSize: 'var(--size-xs)' }}>
                        IPC contract
                    </dt>
                    <dd className="numeric" style={{ margin: 0 }}>
                        v{CONTRACT_VERSION}
                    </dd>
                </div>
                <div>
                    <dt style={{ color: 'var(--color-textFaint)', fontSize: 'var(--size-xs)' }}>
                        Theme
                    </dt>
                    <dd style={{ margin: 0 }}>{defaultTheme.name}</dd>
                </div>
            </dl>
        </main>
    );
}
