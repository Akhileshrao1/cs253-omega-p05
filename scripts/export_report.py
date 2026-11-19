"""Export REPORT.md and the standalone D6 derivation with Pandoc and Chromium."""

import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
STYLE = """
@page {
  size: A4;
  margin: 17mm 17mm 18mm;
  @bottom-left { content: "CS 253 / MA 253  ·  Team Omega"; color: #516273; font-size: 8pt; }
  @bottom-right { content: counter(page); color: #516273; font-size: 8pt; }
}
body { margin: 0; color: #172938; font: 10pt/1.38 "Segoe UI", Arial, sans-serif; }
h1, h2, h3 { color: #0f5060; line-height: 1.15; break-after: avoid; }
h1 { font-size: 24pt; letter-spacing: -0.35pt; margin: 0 0 9pt; }
h2 { font-size: 16pt; margin: 17pt 0 6pt; }
h3 { font-size: 12pt; margin: 13pt 0 5pt; }
p, li { widows: 2; orphans: 2; }
p { margin: 5pt 0 8pt; }
li { margin: 3pt 0; }
a { color: #0d5b71; }
code { font-size: 0.9em; overflow-wrap: anywhere; }
pre { background: #f0f6f7; padding: 7pt; break-inside: avoid; }
table { width: 100%; border-collapse: collapse; font-size: 9pt; margin: 8pt 0 11pt; }
thead { background: #e8f3f4; }
tr { break-inside: avoid; }
th, td { text-align: left; vertical-align: top; border-bottom: 1px solid #d6e2e7; padding: 4.5pt; }
figure { margin: 9pt 0 10pt; break-inside: avoid; }
img { max-width: 100%; object-fit: contain; }
figcaption { color: #395363; font-size: 8.5pt; }
.page-break { break-before: page; }
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--chromium", default="chromium", help="path to Chrome, Chromium or Edge")
    parser.add_argument("--pandoc", default="pandoc")
    args = parser.parse_args()
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    paths = [ROOT / "REPORT.md", ROOT / "docs/derivations/D6_250001072.md"]
    with tempfile.TemporaryDirectory(prefix="report-", dir=build) as temporary:
        work = Path(temporary)
        css = work / "print.css"
        for source in paths:
            style = STYLE
            if source.name == "REPORT.md":
                style += "\nbody { font-size: 10.5pt; line-height: 1.45; }\n"
            css.write_text(style, encoding="utf-8")
            html = work / source.with_suffix(".html").name
            pdf = source.with_suffix(".pdf")
            staged_pdf = work / pdf.name
            subprocess.run([
                args.pandoc, str(source), "--from=gfm+tex_math_dollars", "--standalone",
                "--mathml", "--embed-resources", "--css=" + str(css),
                "--metadata=pagetitle:" + source.stem, "--output=" + str(html),
            ], cwd=ROOT, check=True)
            subprocess.run([
                args.chromium, "--headless", "--disable-background-networking", "--disable-extensions",
                "--no-first-run", "--no-pdf-header-footer", "--user-data-dir=" + str(work / "browser"),
                "--print-to-pdf=" + str(staged_pdf), html.as_uri(),
            ], cwd=ROOT, check=True, timeout=60)
            if not staged_pdf.is_file() or staged_pdf.stat().st_size == 0:
                raise RuntimeError(f"Chromium did not generate {pdf.name}")
            staged_pdf.replace(pdf)
            print(pdf.relative_to(ROOT))


if __name__ == "__main__":
    main()
