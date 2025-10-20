# Octopus: Efficient Hypergraph Pattern Mining with Practical Processing-in-Memory Architecture
## Project Overview

**Octopus** is an efficient hypergraph pattern mining (HPM) framework for the UPMEM **Processing-in-Memory (PIM)** architecture.

The goal of Octopus is to accelerate HPM tasks on large-scale hypergraphs using UPMEM DPUs. It integrates several system-level optimizations tailored for near-data processing architecture.

---

## 🚀 Key Features

- Fast hypergraph pattern mining for large-scale hypergraphs.
- Schedule-based compact data partitioning for collectively facilitate compact data organization.
- Load-aware balanced task assignment for balanced execution across thousands of DPUs.
- Hyperedge-level candidate generation for reduced computation.
- Asynchronous loader–worker pipeline using WRAM FIFO.

---

## 📁 Directory Structure

```
Octopus-AE/
├── data/         # Hypergraph data  
├── dpu/          # DPU-side programs (C for UPMEM)
├── host/         # Host-side logic (C)
├── include/      # Shared headers
├── makefile      # Compilation rules
└── README.md     # Project description
```

---

## 🛠 Requirements

- **Linux environment**
- **UPMEM SDK v2025.1.0.**
- **GNU Make, C compiler (e.g., `gcc`)**

## ⚙️ Build and Run Instructions

To run HPM on a hypergraph, use:
