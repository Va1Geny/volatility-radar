# Claude Code

Project instructions are shared with Codex and Cursor in AGENTS.md:

@AGENTS.md

Hooks in `.claude/settings.json`:
- Before searches and reads, a graphify guard steers you to the knowledge graph.
- After edits, `graphify update .` refreshes the graph.

Both hooks need `graphify` on PATH.
