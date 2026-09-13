"""Shared rendering helpers for analyzer before/after behavioral evidence."""

from __future__ import annotations

from collections.abc import Iterable


EvidenceSnapshot = dict[str, list[tuple[str, str]]]


def markdown_text(value: object) -> str:
    """Escape row text so evidence values remain valid single-line Markdown cells."""
    return str(value).replace("|", "\\|").replace("\r", "").replace("\n", "<br>")


def _ordered_rows(rows: Iterable[tuple[str, str]]) -> list[tuple[str, str]]:
    """Normalize keyed snapshot rows into deterministic report order."""
    return sorted((str(key), markdown_text(value)) for key, value in rows)


def render_evidence(before: EvidenceSnapshot, after: EvidenceSnapshot) -> list[str]:
    """Render target-phase snapshots and exact keyed deltas for analyzer reports.

    A row key identifies the same target-specific object across phases.  Equal keys
    with different values are reported as changes; keys present in only one phase
    are reported as additions or removals.  This avoids mistaking a final-state dump
    for evidence of what the target analyzer changed.
    """
    categories = list(dict.fromkeys((*before.keys(), *after.keys())))
    lines = ["## Before target analysis", ""]
    _render_snapshot(lines, before, categories)
    lines.extend(["", "## After target analysis", ""])
    _render_snapshot(lines, after, categories)
    lines.extend(["", "## Delta", ""])
    any_changes = False
    for category in categories:
        delta = _delta_rows(before.get(category, []), after.get(category, []))
        lines.extend([f"### {category}", ""])
        if not delta:
            lines.append("No changes observed.")
        else:
            any_changes = True
            lines.extend(["| Change | Key | Before | After |", "| --- | --- | --- | --- |"])
            lines.extend(
                f"| `{change}` | `{key}` | `{old_value}` | `{new_value}` |"
                for change, key, old_value, new_value in delta
            )
        lines.append("")
    if not any_changes:
        lines.append("No changes observed.")
    return lines


def _render_snapshot(lines: list[str], snapshot: EvidenceSnapshot, categories: list[str]) -> None:
    """Append each category as a keyed state table, including empty states."""
    for category in categories:
        lines.extend([f"### {category}", "", "| Key | State |", "| --- | --- |"])
        rows = _ordered_rows(snapshot.get(category, []))
        if rows:
            lines.extend(f"| `{key}` | `{value}` |" for key, value in rows)
        else:
            lines.append("| `(none)` | `No rows observed` |")
        lines.append("")


def _delta_rows(before: Iterable[tuple[str, str]], after: Iterable[tuple[str, str]]) -> list[tuple[str, str, str, str]]:
    """Return deterministic additions, removals, and changes keyed by object identity."""
    before_by_key = dict(_ordered_rows(before))
    after_by_key = dict(_ordered_rows(after))
    rows = []
    for key in sorted(set(before_by_key) | set(after_by_key)):
        if key not in before_by_key:
            rows.append(("Added", key, "", after_by_key[key]))
        elif key not in after_by_key:
            rows.append(("Removed", key, before_by_key[key], ""))
        elif before_by_key[key] != after_by_key[key]:
            rows.append(("Changed", key, before_by_key[key], after_by_key[key]))
    return rows
