# Tools used

| Tool and version | Used by | Used for | Files affected | What we changed or verified |
|------------------|---------|----------|----------------|-----------------------------|
| ChatGPT (AI assistant; 17 Nov 2026) | Thakur Tejasva Singh | Focused revision of the Analyst's predicted-cost report | `REPORT.md`, `TOOLS.md` | Compared the report with the shared interface and available branches; separated rebuilt-cell, read, write, and combine counters; aligned the prefix example to 1-based ranges. The prefix-baseline decrease bound remains open until its implementation is available. |
| Claude (Anthropic), Claude Sonnet 5.5; 19 Nov 2026 | Koushik Reddy K | Adversary module: generators, measurement tools, replay harness, D5 demo | `adversary/*` | Verified against INTERFACE.md, tested counters, verified oracle parity |

## Representative prompts

- 17 Nov 2026: Review the repository and `main` before revising `REPORT.md`. Treat teammate feedback as a suggestion; model prefix-max decreases with rebuilt cells separately from reads, writes, and combines, and use the project's 1-based inclusive indexing.
- 19 Nov 2026: Write the Adversary module for Project 5 against the repository's real INTERFACE.md, app/sensor_demo.cpp, Structure B and the oracle; measure Structure B's actual update and query cost; produce reproducible seeded generators.