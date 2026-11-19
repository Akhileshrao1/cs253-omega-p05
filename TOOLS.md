# Tools used

| Tool and version | Used by | Used for | Files affected | What we changed or verified |
|------------------|---------|----------|----------------|-----------------------------|
| ChatGPT (OpenAI, 19 Nov 2026) | Hanchate Akhilesh Rao | Drafted and typeset the D3 range GCD proof | `docs/derivations/D3_250001031.pdf` | Checked the proof against both accumulators in `query_internal`. Ran the GCD and interface smoke tests and confirmed the README demo output. |
| ChatGPT (AI assistant; 17 Nov 2026) | Thakur Tejasva Singh | Focused revision of the Analyst's predicted-cost report | `REPORT.md`, `TOOLS.md` | Compared the report with the shared interface and available branches; separated rebuilt-cell, read, write, and combine counters; aligned the prefix example to 1-based ranges. The prefix-baseline decrease bound remains open until its implementation is available. |

## Representative prompts

- 18 Nov 2026 (paraphrased): Show where `log(g_i/g_j)` comes from so it can be explained during the D3 presentation.
- 19 Nov 2026 (paraphrased): Divide the Architect's remaining work into parts and complete the first deliverable.

- 17 Nov 2026: Review the repository and `main` before revising `REPORT.md`. Treat teammate feedback as a suggestion; model prefix-max decreases with rebuilt cells separately from reads, writes, and combines, and use the project's 1-based inclusive indexing.
