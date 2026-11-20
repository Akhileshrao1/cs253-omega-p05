# Tools used

| Tool and version | Used by | Used for | Files affected | What we changed or verified |
|------------------|---------|----------|----------------|-----------------------------|
| ChatGPT (OpenAI, 19 Nov 2026) | Hanchate Akhilesh Rao | Drafted and typeset the D3 range GCD proof | `docs/derivations/D3_250001031.pdf` | Checked the proof against both accumulators in `query_internal`. Ran the GCD and interface smoke tests and confirmed the README demo output. |
| OpenAI Codex (20 Nov 2026) | Hanchate Akhilesh Rao | Added the Structure B evaluator mode and shared README example | `app/sensor_demo.cpp`, `README.md` | Checked both output formats, invalid inputs, 5,000 seeded commands against a plain array, and a replay with one million sensors and one million commands. |
| ChatGPT (AI assistant; 17 Nov 2026) | Thakur Tejasva Singh | Focused revision of the Analyst's predicted-cost report | `REPORT.md`, `TOOLS.md` | Compared the report with the shared interface and available branches; separated rebuilt-cell, read, write, and combine counters; aligned the prefix example to 1-based ranges. The prefix-baseline decrease bound remains open until its implementation is available. |
| ChatGPT (OpenAI, 20 Nov 2026) | Kasireddy Pranav Karthik | Added seeded Oracle differential testing, assignment read-back checks, exact-reason failure shrinking and D4 proof | `oracle/*`, `docs/derivations/D4_250041020.*` | Added build-failure reporting and documented reproducible quick and million-operation commands. |
| Claude (Anthropic), Claude Sonnet 5.5; 19 Nov 2026 | Koushik Reddy K | Adversary module: generators, measurement tools, replay harness, D5 demo | `adversary/*` | Verified against INTERFACE.md, tested counters, verified oracle parity |
| OpenAI Codex, 19 Nov 2026 | Thakur Tejasva Singh | Benchmark harness, interface-compatible prefix Fenwick, seeded differential checks, figures and report update | `bench/`, `src/structB/prefix_max_fenwick.hpp`, `results/`, `scripts/`, `REPORT.md`, D6 | Codex adapted the D5 rescan strategy, ran the smoke suite and four million-operation differential workloads, executed seven timed trials per configuration, and checked the report against the raw CSVs. |
| GCC 15.2.0, Python 3.14.8, Matplotlib 3.11.2, Pandoc 3.12.1, Chrome 157 | Thakur Tejasva Singh | Compile, execute, summarize, plot and export PDF | `results/`, `REPORT.pdf`, `docs/derivations/D6_250001072.pdf` | Archived compiler flags, machine information and seeds. The attempted WPR/xperf LLC-miss capture failed, so the report makes no hardware-counter claims. |

## Representative prompts

- 18 Nov 2026 (paraphrased): Show where `log(g_i/g_j)` comes from so it can be explained during the D3 presentation.
- 19 Nov 2026 (paraphrased): Divide the Architect's remaining work into parts and complete the first deliverable.
- 20 Nov 2026 (paraphrased): Add Structure B to the existing sensor evaluator and document the commands and example for both modes.
- 20 Nov 2026: Update the Oracle differential harness, preserve exact failure reasons while shrinking, and prepare the D4 derivation.

- 17 Nov 2026: Review the repository and `main` before revising `REPORT.md`. Treat teammate feedback as a suggestion; model prefix-max decreases with rebuilt cells separately from reads, writes, and combines, and use the project's 1-based inclusive indexing.
- 19 Nov 2026: Write the Adversary module for Project 5 against the repository's real INTERFACE.md, app/sensor_demo.cpp, Structure B and the oracle; measure Structure B's actual update and query cost; produce reproducible seeded generators.
- 19 Nov 2026 (paraphrased): Build a reproducible benchmark harness, generate cases, save raw measurements and chart code, and fill the measurement sections of the report.
- 19 Nov 2026 (paraphrased): Put the benchmark work on a separate branch for a team pull request.
