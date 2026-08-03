//! PitchForge desktop shell.
//!
//! CLAUDE.md §9: the Rust side is a launcher and a bridge. It owns a window and
//! nothing else.
//!
//! It must never acquire: game state, a SQLite handle, a rule, a formula, or a
//! type that describes a footballer. The headless server owns all of that and is
//! reached over loopback HTTP/WebSocket by the generated client in `/app/ipc`.
//! If a change here needs to know what a player is, it belongs in the server.

pub fn run() {
    tauri::Builder::default()
        .run(tauri::generate_context!())
        .expect("error while running the PitchForge shell");
}
