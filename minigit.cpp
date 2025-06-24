#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <set>
#include <algorithm>

using namespace std;
namespace fs = filesystem;

// Constants
const fs::path MINIGIT_DIR = ".minigit";
const fs::path OBJECTS_DIR = "objects";
const fs::path REFS_DIR = "refs";
const fs::path HEADS_DIR = "heads";
const fs::path HEAD_FILE = "HEAD";
const fs::path INDEX_FILE = "index";

// Utility functions
string calculate_hash(const string& content) {
    hash<string> hasher;
    stringstream ss;
    ss << hex << setw(16) << setfill('0') << hasher(content);
    return ss.str() + ss.str();
}

void write_file(const fs::path& path, const string& content) {
    ofstream file(path);
    if (!file) throw runtime_error("Failed to write: " + path.string());
    file << content;
}

string read_file(const fs::path& path) {
    if (!fs::exists(path)) return "";
    ifstream file(path);
    if (!file) throw runtime_error("Failed to read: " + path.string());
    return string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
}

string get_current_time() {
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    char buf[100];
    strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y", localtime(&t));
    return string(buf);
}

// Index class
class Index {
    map<string, string> entries;

public:
    void load(const fs::path& repo_root) {
        entries.clear();
        stringstream ss(read_file(repo_root / INDEX_FILE));
        for (string line; getline(ss, line); ) {
            stringstream ls(line);
            string hash, file;
            ls >> hash >> file;
            if (!hash.empty() && !file.empty()) entries[file] = hash;
        }
    }

    void save(const fs::path& repo_root) const {
        stringstream ss;
        for (const auto& [f, h] : entries)
            ss << h << " " << f << "\n";
        write_file(repo_root / INDEX_FILE, ss.str());
    }

    void add(const string& f, const string& h) { entries[f] = h; }
    void clear() { entries.clear(); }
    bool empty() const { return entries.empty(); }
    void set_entries(const map<string, string>& e) { entries = e; }
    const map<string, string>& get_entries() const { return entries; }
};
// Commit class
class Commit {
    string hash, message;
    vector<string> parents;
    map<string, string> files;
    long timestamp;

public:
    string serialize() const {
        stringstream ss;
        ss << "tree " << files.size() << "\n";
        for (const auto& [f, h] : files) ss << f << " " << h << "\n";
        for (const auto& p : parents) ss << "parent " << p << "\n";
        ss << "timestamp " << timestamp << "\n\n" << message;
        return ss.str();
    }

    static Commit deserialize(const string& data) {
        Commit c;
        stringstream ss(data), ls;
        for (string line; getline(ss, line) && !line.empty(); ) {
            if (line.rfind("tree ", 0) == 0) {
                int count = stoi(line.substr(5));
                while (count-- > 0 && getline(ss, line)) {
                    ls.clear(); ls.str(line);
                    string f, h; ls >> f >> h;
                    if (!f.empty() && !h.empty()) c.files[f] = h;
                }
            } else if (line.rfind("parent ", 0) == 0) {
                c.parents.push_back(line.substr(7));
            } else if (line.rfind("timestamp ", 0) == 0) {
                c.timestamp = stol(line.substr(10));
            }
        }
        c.message.assign(istreambuf_iterator<char>(ss), {});
        return c;
    }

    void set(const string& m, long t, const map<string, string>& f, const vector<string>& p = {}) {
        message = m;
        timestamp = t;
        files = f;
        parents = p;
    }

    const string& get_hash() const { return hash; }
    const vector<string>& get_parents() const { return parents; }
    const map<string, string>& get_files() const { return files; }
    const string& get_message() const { return message; }
    long get_timestamp() const { return timestamp; }
};

// Repository class
class Repository {
    fs::path root;

public:
    Repository() : root(MINIGIT_DIR) {}

    fs::path objects_path() const { return root / OBJECTS_DIR; }
    fs::path refs_path() const { return root / REFS_DIR; }
    fs::path heads_path() const { return root / REFS_DIR / HEADS_DIR; }
    fs::path head_path() const { return root / HEAD_FILE; }
    fs::path index_path() const { return root / INDEX_FILE; }
    fs::path root_path() const { return root; }

    string get_head_hash() const {
        string h = read_file(head_path());
        return h.rfind("ref: ", 0) == 0 ? read_file(root / h.substr(5)) : h;
    }

    string current_branch() const {
        string h = read_file(head_path());
        return h.rfind("ref: ", 0) == 0 ? fs::path(h.substr(5)).filename().string() : "";
    }

    void update_head(const string& ref) { write_file(head_path(), ref); }
    void update_branch(const string& b, const string& h) { write_file(heads_path() / b, h); }

    vector<string> get_branches() const {
        vector<string> branches;
        if (fs::exists(heads_path())) {
            for (const auto& entry : fs::directory_iterator(heads_path())) {
                branches.push_back(entry.path().filename().string());
            }
        }
        return branches;
    }
};
// Command implementations
namespace Command {
    Repository repo;

    void init() {
        if (fs::exists(MINIGIT_DIR)) {
            cout << "MiniGit repository already exists.\n";
            return;
        }
        fs::create_directory(MINIGIT_DIR);
        fs::create_directory(MINIGIT_DIR / OBJECTS_DIR);
        fs::create_directory(MINIGIT_DIR / REFS_DIR);
        fs::create_directory(MINIGIT_DIR / REFS_DIR / HEADS_DIR);
        write_file(MINIGIT_DIR / HEAD_FILE, "ref: refs/heads/main");
        write_file(MINIGIT_DIR / "refs/heads/main", "");
        write_file(MINIGIT_DIR / INDEX_FILE, "");
        cout << "Initialized empty MiniGit repository in " << fs::absolute(MINIGIT_DIR) << endl;
    }

    void add(const string& filename) {
        if (!fs::exists(repo.head_path())) {
            cerr << "Error: Not a MiniGit repository.\n";
            return;
        }
        fs::path file_path(filename);
        if (!fs::exists(file_path)) {
            cerr << "Error: File '" << filename << "' does not exist.\n";
            return;
        }
        string content = read_file(file_path);
        string hash = calculate_hash(content);
        string rel_path = fs::relative(file_path).string();
        replace(rel_path.begin(), rel_path.end(), '\\', '/');
        if (!fs::exists(repo.objects_path() / hash)) {
            write_file(repo.objects_path() / hash, content);
        }
        Index index;
        index.load(repo.root_path());
        index.add(rel_path, hash);
        index.save(repo.root_path());
        cout << "Staged: " << rel_path << endl;
    }

    void commit(const string& message) {
        Index index;
        index.load(repo.root_path());
        if (index.empty()) {
            cout << "Nothing to commit.\n";
            return;
        }
        Commit commit;
        commit.set(message,
                   chrono::system_clock::now().time_since_epoch().count(),
                   index.get_entries(),
                   repo.get_head_hash().empty() ? vector<string>() : vector<string>{repo.get_head_hash()});
        string data = commit.serialize();
        string hash = calculate_hash(data);
        write_file(repo.objects_path() / hash, data);
        string branch = repo.current_branch();
        if (!branch.empty()) {
            repo.update_branch(branch, hash);
            cout << "Committed to " << branch << " as " << hash << endl;
        } else {
            repo.update_head(hash);
            cout << "Committed in detached HEAD as " << hash << endl;
        }
        index.clear();
        index.save(repo.root_path());
    }

    void log() {
        string current = repo.get_head_hash();
        if (current.empty()) {
            cout << "No commits yet.\n";
            return;
        }
        while (!current.empty()) {
            Commit c = Commit::deserialize(read_file(repo.objects_path() / current));
            cout << "commit " << current << "\n";
            if (c.get_parents().size() > 1) {
                cout << "Merge:";
                for (const auto& p : c.get_parents())
                    cout << " " << p.substr(0, 7);
                cout << "\n";
            }
            cout << "Date: " << get_current_time() << "\n\n" << c.get_message() << "\n\n";
            current = c.get_parents().empty() ? "" : c.get_parents()[0];
        }
    }

    void branch(const string& name) {
        if (fs::exists(repo.heads_path() / name)) {
            cerr << "Error: Branch '" << name << "' already exists.\n";
            return;
        }
        string head = repo.get_head_hash();
        if (head.empty()) {
            cerr << "Error: No commits yet.\n";
            return;
        }
        write_file(repo.heads_path() / name, head);
        cout << "Created branch " << name << " at " << head.substr(0, 7) << endl;
    }

    void checkout(const string& name) {
        string target_hash;
        bool is_branch = false;

        if (fs::exists(repo.heads_path() / name)) {
            target_hash = read_file(repo.heads_path() / name);
            repo.update_head("ref: refs/heads/" + name);
            is_branch = true;
        } else if (fs::exists(repo.objects_path() / name)) {
            target_hash = name;
            repo.update_head(name);
        } else {
            cerr << "Error: No such branch or commit.\n";
            return;
        }

        if (target_hash.empty()) {
            cout << "Switched to new branch " << name << endl;
            return;
        }

        Commit target = Commit::deserialize(read_file(repo.objects_path() / target_hash));
        Index index;
        index.load(repo.root_path());

        for (const auto& [f, _] : index.get_entries()) {
            fs::remove(f);
        }

        for (const auto& [f, h] : target.get_files()) {
            fs::create_directories(fs::path(f).parent_path());
            write_file(f, read_file(repo.objects_path() / h));
        }

        index.set_entries(target.get_files());
        index.save(repo.root_path());
        cout << "Switched to " << (is_branch ? "branch " : "commit ") << name << endl;
    }

    string find_lca(const string& a, const string& b) {
        set<string> ancestors;
        vector<string> queue = {a};
        for (size_t i = 0; i < queue.size(); i++) {
            Commit c = Commit::deserialize(read_file(repo.objects_path() / queue[i]));
            for (const auto& p : c.get_parents()) {
                if (ancestors.insert(p).second) queue.push_back(p);
            }
        }
        queue = {b};
        for (size_t i = 0; i < queue.size(); i++) {
            if (ancestors.count(queue[i])) return queue[i];
            Commit c = Commit::deserialize(read_file(repo.objects_path() / queue[i]));
            for (const auto& p : c.get_parents()) queue.push_back(p);
        }
        return "";
    }
    void diff(const string& commit1, const string& commit2) {
        Commit c1 = Commit::deserialize(read_file(repo.objects_path() / commit1));
        Commit c2 = Commit::deserialize(read_file(repo.objects_path() / commit2));

        cout << "Diff between " << commit1.substr(0, 7) << " and " << commit2.substr(0, 7) << ":\n";

        set<string> all_files;
        for (const auto& [f, _] : c1.get_files()) all_files.insert(f);
        for (const auto& [f, _] : c2.get_files()) all_files.insert(f);

        for (const auto& file : all_files) {
            bool in1 = c1.get_files().count(file);
            bool in2 = c2.get_files().count(file);

            if (!in1) cout << "+++ Added: " << file << "\n";
            else if (!in2) cout << "--- Removed: " << file << "\n";
            else if (c1.get_files().at(file) != c2.get_files().at(file))
                cout << "*** Modified: " << file << "\n";
        }
    }
    void merge(const string& branch_name) {
        if (!fs::exists(repo.heads_path() / branch_name)) {
            cerr << "Error: Branch '" << branch_name << "' doesn't exist.\n";
            return;
        }

        string current_hash = repo.get_head_hash();
        string other_hash = read_file(repo.heads_path() / branch_name);

        if (current_hash == other_hash) {
            cout << "Already up to date.\n";
            return;
        }

        string lca_hash = find_lca(current_hash, other_hash);
        if (lca_hash == current_hash) {
            cout << "Fast-forward merge.\n";
            checkout(branch_name);
            return;
        }
        if (lca_hash == other_hash) {
            cout << "Already up to date.\n";
            return;
        }

        Commit lca = Commit::deserialize(read_file(repo.objects_path() / lca_hash));
        Commit current = Commit::deserialize(read_file(repo.objects_path() / current_hash));
        Commit other = Commit::deserialize(read_file(repo.objects_path() / other_hash));
        map<string, string> merged = current.get_files();
        bool has_conflicts = false;

        set<string> all_files;
        for (const auto& [f, _] : lca.get_files()) all_files.insert(f);
        for (const auto& [f, _] : current.get_files()) all_files.insert(f);
        for (const auto& [f, _] : other.get_files()) all_files.insert(f);

        for (const auto& file : all_files) {
            string lh = lca.get_files().count(file) ? lca.get_files().at(file) : "";
            string ch = current.get_files().count(file) ? current.get_files().at(file) : "";
            string oh = other.get_files().count(file) ? other.get_files().at(file) : "";

            if (ch == oh) continue;
            if (ch != lh && oh != lh) {
                cout << "CONFLICT: " << file << " - both modified\n";
                has_conflicts = true;
            } else if (oh != lh) {
                merged[file] = oh;
            }
        }

        if (has_conflicts) {
            cerr << "Merge conflicts detected. Resolve them and commit.\n";
            return;
        }

        Commit merge_commit;
        merge_commit.set("Merge branch '" + branch_name + "'",
                         chrono::system_clock::now().time_since_epoch().count(),
                         merged,
                         {current_hash, other_hash});
        string data = merge_commit.serialize();
        string hash = calculate_hash(data);
        write_file(repo.objects_path() / hash, data);

        string current_branch = repo.current_branch();
        if (!current_branch.empty()) {
            repo.update_branch(current_branch, hash);
        } else {
            repo.update_head(hash);
        }

        cout << "Merge successful. New commit: " << hash.substr(0, 7) << endl;
    }

}
    int main() {
        try {
            string line;
            cout << "MiniGit CLI. Type 'quit' to exit.\n";
            while (true) {
                cout << "> ";
                getline(cin, line);
                if (line.empty()) continue;

                stringstream ss(line);
                vector<string> args;
                string arg;
                while (ss >> arg) args.push_back(arg);
                if (args.empty()) continue;

                if (args[0] == "quit") break;

                string cmd = args[0];
                if (cmd == "init") Command::init();
                else if (cmd == "add" && args.size() > 1) Command::add(args[1]);
                else if (cmd == "commit" && args.size() > 2 && args[1] == "-m") Command::commit(args[2]);
                else if (cmd == "log") Command::log();
                else if (cmd == "branch" && args.size() > 1) Command::branch(args[1]);
                else if (cmd == "checkout" && args.size() > 1) Command::checkout(args[1]);
                else if (cmd == "merge" && args.size() > 1) Command::merge(args[1]);
                else if (cmd == "diff" && args.size() > 2) Command::diff(args[1], args[2]);
                else cerr << "Invalid command or arguments.\n";
            }
        } catch (const exception& e) {
            cerr << "Error: " << e.what() << endl;
            return 1;
        }
        return 0;
    }


