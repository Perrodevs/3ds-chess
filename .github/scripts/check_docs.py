"""Check paths and section links in the repository's maintained entry points."""
from pathlib import Path
import re
from urllib.parse import unquote, urlsplit

root = Path(__file__).resolve().parents[2]
pages = (
    "README.md", "CONTRIBUTING.md", "CODE_OF_CONDUCT.md", "SECURITY.md",
    "docs/README_ES.md", "docs/ARCHITECTURE.md", "docs/BUILDING.md",
    "docs/ROADMAP.md", "docs/TROUBLESHOOTING.md", "docs/media/README.md",
    "docs/TESTING.md", "docs/GITHUB.md",
)


def anchors(path):
    counts = {}
    for heading in re.findall(r"^#{1,6} (.+)$", path.read_text(), re.M):
        slug = re.sub(r"[^\w -]", "", heading.lower()).replace(" ", "-")
        count = counts.get(slug, 0)
        yield f"{slug}-{count}" if count else slug
        counts[slug] = count + 1


errors = []
checked = 0
for relative in pages:
    page = root / relative
    content = page.read_text()
    for raw in re.findall(r"\]\(([^)]+)\)", content):
        link = urlsplit(raw)
        if link.scheme or link.netloc:
            continue
        checked += 1
        target = (page.parent / unquote(link.path)).resolve() if link.path else page
        if not target.is_relative_to(root) or not target.exists():
            errors.append(f"{relative}: broken path {raw}")
        elif link.fragment and target.suffix == ".md" and unquote(link.fragment) not in set(anchors(target)):
            errors.append(f"{relative}: missing section {raw}")
if errors:
    raise SystemExit("\n".join(errors))
print(f"Verified {checked} local documentation links across {len(pages)} files")
