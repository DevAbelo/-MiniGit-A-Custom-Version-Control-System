# MiniGit Version Control System

**Project File:** `minigit.cpp`  
# 👥 Prepared by:
## 1,Abel Debalke----ATE/0668/15
## 2,Ephratha Samuel----ATE/2652/15
## 3,Kaleab Adane----ATE/5365/13
## 4,Natnael T/Michael----ATE/5455/15
## 5,Yaphet Benyam Ayalew----ATE/8841/13
## 6,Helina Tesfaye----ATE/1540/15

## 📝 Project Description
A lightweight Git-like version control system implemented from scratch in C++. MiniGit tracks file changes, manages commits, handles branching/merging, and maintains revision history - all without external libraries.

## 🛠️ Features Implemented
| Feature          | Command               | Description                          |
|------------------|-----------------------|--------------------------------------|
| Repository Init  | `init`                | Creates .minigit directory structure |
| File Staging     | `add <file>`          | Stages files for commit              |
| Committing       | `commit -m "msg"`     | Saves snapshots with messages        |
| Branching        | `branch <name>`       | Creates new branches                 |
| Switching        | `checkout <target>`   | Changes branches/commits             |
| Merging          | `merge <branch>`      | Combines branches with conflict detection |
| History View     | `log`                 | Shows commit timeline                |
| Diff Tool        | `diff <commit1> <commit2>` | Compares file versions          |

## 🚀 Getting Started

### Prerequisites
- GCC/G++ (MinGW on Windows)
- C++17 compatible compiler
- Filesystem library support

### Compilation
```bash
g++ minigit.cpp -o minigit -std=c++17
