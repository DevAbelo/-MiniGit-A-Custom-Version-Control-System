# 🔧 MiniGit: A Lightweight Version Control System in C++

**MiniGit** is a simplified, educational version of Git, written entirely in C++. It allows local version control features like commits, branches, checkouts, diffs, and merges — all without needing any third-party tools or libraries. It’s a perfect project for learning how Git works under the hood and how data structures like hash maps, trees, and graphs apply to real-world systems.

---

## 📦 Features

- ✅ `init` — Initialize a MiniGit repository
- ✅ `add <file>` — Stage files for the next commit
- ✅ `commit -m <message>` — Create a commit with staged changes
- ✅ `log` — View commit history
- ✅ `branch <name>` — Create a new branch
- ✅ `checkout <branch|commit>` — Switch to another branch or specific commit
- ✅ `merge <branch>` — Merge changes from one branch into the current
- ✅ `diff <commit1> <commit2>` — View differences between two commits

---

## 🗂️ Directory Structure

Once initialized, MiniGit creates a `.minigit` folder that stores all repo metadata and objects:

.minigit/
├── HEAD # Current branch or commit reference
├── index # Staging area (filename → hash)
├── objects/ # Stores all file and commit blobs
└── refs/
└── heads/ # Each file is a branch (name → commit hash)


---

## 💡 How It Works

| Concept       | Implementation                       |
|---------------|--------------------------------------|
| **Hashing**   | `std::hash<string>` used to create file/content IDs |
| **Commits**   | Stored as text files in `.minigit/objects/`, linking to file blobs |
| **Index**     | `index` file acts as a staging area |
| **Branches**  | Text files in `refs/heads/` mapping names to commits |
| **Merges**    | Detects LCA (Lowest Common Ancestor) and performs 3-way merge |
| **Diffing**   | Compares hashes of tracked files between two commits |

---

## 🚀 Usage

1. **Compile**
```bash
g++ -std=c++17 -o minigit minigit.cpp

./minigit init
./minigit add file.txt
./minigit commit -m "First commit"
./minigit log
./minigit branch feature
./minigit checkout feature
./minigit merge main
./minigit diff <commit1> <commit2>
$ ./minigit init
Initialized empty MiniGit repository

$ echo "hello" > file.txt
$ ./minigit add file.txt
Staged: file.txt

$ ./minigit commit -m "Initial commit"
Committed to main as a1b2c3...

$ ./minigit log
commit a1b2c3...
Date:   Tue Jun 24 20:31:12 2025
    Initial commit
