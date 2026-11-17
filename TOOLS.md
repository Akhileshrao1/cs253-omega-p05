# Tools used

| Tool and version | Used by | Used for | Files affected | What we changed or verified |
|------------------|---------|----------|----------------|-----------------------------|
| ChatGPT (AI assistant; 17 Nov 2026) | Thakur Tejasva Singh | Focused revision of the Analyst's predicted-cost report | `REPORT.md`, `TOOLS.md` | Compared the report with the shared interface and available branches; separated rebuilt-cell, read, write, and combine counters; aligned the prefix example to 1-based ranges. The prefix-baseline decrease bound remains open until its implementation is available. |

## Representative prompts

- 17 Nov 2026: Review the repository and `main` before revising `REPORT.md`. Treat teammate feedback as a suggestion; model prefix-max decreases with rebuilt cells separately from reads, writes, and combines, and use the project's 1-based inclusive indexing.
