# Using the Project Rules in a New AI Conversation

The authoritative instructions for RFLink Universal are in the repository root: [AGENTS.md](../AGENTS.md). The current implementation facts and limits are in [docs/V0_7_STATUS.md](V0_7_STATUS.md).

## What loads automatically?

| Tool | What to expect |
| --- | --- |
| Codex working in this repository | Uses applicable `AGENTS.md` files as repository instructions |
| GitHub Copilot in repository context | Uses `.github/copilot-instructions.md` when that Copilot feature supports repository instructions |
| A normal ChatGPT conversation | **Does not automatically read GitHub files** just because the repository exists |
| A ChatGPT Project | Project instructions and attached/connected sources can be available across its chats |

## Recommended one-time setup for ChatGPT

Keep your RFLink work in a single ChatGPT Project (for example, **Eigen rfid board**). Put the following short instruction in the **Project instructions**:

> For every RFLink Universal development request, first read the current `AGENTS.md` and `docs/V0_7_STATUS.md` from `juulius/rflink-universal` using the connected GitHub source, where available. Follow their engineering, UX, security, versioning and documentation rules. All repository files, UI copy, code comments and documentation must be in English. Never claim a full ESP-IDF build, a hardware test or a stable release unless it actually occurred. If the repository cannot be read, say so instead of guessing.

This is an instruction to **check the current source**; it does not guarantee that every chat can access GitHub. If GitHub is not connected or the project context is unavailable, provide the rules file or reconnect the repository.

## Best practice for each new task

1. Read the latest repo instructions and status rather than relying on an outdated chat summary.
2. Check which firmware and Builder source is actually present in GitHub.
3. Make focused changes and update the corresponding documentation.
4. Report tests separately: source inspection, host tests, official ESP-IDF build and physical hardware validation.

Do not store private GitHub tokens, passwords or device identifiers in these instructions.
