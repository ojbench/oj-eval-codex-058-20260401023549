// Implementation for Problem 058 - Pokedex
#include <bits/stdc++.h>
using namespace std;

// Exception hierarchy
class BasicException {
protected:
    std::string msg;
public:
    explicit BasicException(const char *_message) : msg(_message ? _message : "") {}
    virtual const char *what() const { return msg.c_str(); }
    virtual ~BasicException() = default;
};

class ArgumentException: public BasicException {
public:
    explicit ArgumentException(const char *_message) : BasicException(_message) {}
};

class IteratorException: public BasicException {
public:
    explicit IteratorException(const char *_message) : BasicException(_message) {}
};

struct Pokemon {
    char name[12];
    int id;
    // Store types as vector of indices for fast compute
    vector<int> type_ids;
    string types_str;
};

class Pokedex {
private:
    string fileName;
    // map keyed by id for deterministic iterator order (ascending id)
    map<int, Pokemon> by_id;

    // Allowed types and mapping
    vector<string> type_list;
    unordered_map<string,int> type_index;
    // attack multiplier table [atk][def]
    vector<vector<float>> mult;

    void init_types() {
        // 7-type simplified set: fire, water, grass, electric, ground, flying, dragon
        type_list = {"fire","water","grass","electric","ground","flying","dragon"};
        type_index.clear();
        for (int i=0;i<(int)type_list.size();++i) type_index[type_list[i]] = i;
        mult.assign(type_list.size(), vector<float>(type_list.size(), 1.0f));
        auto idx = [&](const string &s)->int { auto it=type_index.find(s); return it==type_index.end()?-1:it->second; };
        auto setm = [&](const string &atk, const string &def, float v){
            int a=idx(atk), d=idx(def); if(a>=0&&d>=0) mult[a][d]=v; };

        // Fire interactions
        setm("fire","grass",2.0f);
        setm("fire","water",0.5f);
        setm("fire","fire",0.5f);
        setm("fire","dragon",0.5f);
        // Water interactions
        setm("water","fire",2.0f);
        setm("water","ground",2.0f);
        setm("water","water",0.5f);
        setm("water","grass",0.5f);
        setm("water","dragon",0.5f);
        // Grass interactions
        setm("grass","water",2.0f);
        setm("grass","ground",2.0f);
        setm("grass","fire",0.5f);
        setm("grass","grass",0.5f);
        setm("grass","flying",0.5f);
        setm("grass","dragon",0.5f);
        // Electric interactions
        setm("electric","water",2.0f);
        setm("electric","flying",2.0f);
        setm("electric","grass",0.5f);
        setm("electric","electric",0.5f);
        setm("electric","dragon",0.5f);
        setm("electric","ground",0.0f); // immunity as specified
        // Ground interactions
        setm("ground","fire",2.0f);
        setm("ground","electric",2.0f);
        setm("ground","grass",0.5f);
        setm("ground","flying",0.0f); // immunity as specified
        // Flying interactions
        setm("flying","grass",2.0f);
        setm("flying","electric",0.5f);
        // Dragon interactions
        setm("dragon","dragon",2.0f);
    }

    static bool is_alpha_str(const char *s) {
        if (!s || !*s) return false;
        for (const char *p=s; *p; ++p) {
            if (!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z'))) return false;
        }
        return true;
    }

    vector<int> parse_types_or_throw(const char *types, string &types_joined) const {
        if (!types) throw ArgumentException("Argument Error: PM Type Invalid ()");
        string t(types);
        vector<int> res;
        vector<string> parts;
        {
            string cur;
            for (char c: t) {
                if (c=='#') { parts.push_back(cur); cur.clear(); }
                else cur.push_back(c);
            }
            parts.push_back(cur);
        }
        if (parts.empty()) throw ArgumentException("Argument Error: PM Type Invalid ()");
        // Validate each type and build joined string without spaces
        unordered_set<int> seen;
        vector<string> cleaned;
        for (auto &p: parts) {
            if (p.empty()) {
                throw ArgumentException("Argument Error: PM Type Invalid ()");
            }
            auto it = type_index.find(p);
            if (it == type_index.end()) {
                string m = string("Argument Error: PM Type Invalid (") + p + ")";
                throw ArgumentException(m.c_str());
            }
            if (!seen.insert(it->second).second) {
                // allow duplicates silently or treat invalid? We'll allow duplicates by ignoring
            }
            res.push_back(it->second);
            cleaned.push_back(p);
        }
        if ((int)res.size() < 1 || (int)res.size() > 7) {
            // generic invalid type count - report first
            string p = parts.front();
            string m = string("Argument Error: PM Type Invalid (") + p + ")";
            throw ArgumentException(m.c_str());
        }
        // Recompose a canonical joined string preserving order
        types_joined.clear();
        for (size_t i=0;i<cleaned.size();++i) {
            if (i) types_joined.push_back('#');
            types_joined += cleaned[i];
        }
        return res;
    }

    void load_file() {
        by_id.clear();
        if (fileName.empty()) return;
        ifstream fin(fileName);
        if (!fin.good()) return; // no file yet
        // Format: each line: id name types_str
        string name, types;
        long long idll;
        while (fin >> idll >> name >> types) {
            if (idll <= 0 || idll > INT_MAX) continue;
            Pokemon p{};
            p.id = (int)idll;
            memset(p.name, 0, sizeof(p.name));
            strncpy(p.name, name.c_str(), sizeof(p.name)-1);
            p.types_str = types;
            // Map types to ids if possible
            p.type_ids.clear();
            string tmp;
            for (size_t i=0;i<=types.size();++i) {
                if (i==types.size() || types[i]=='#') {
                    if (!tmp.empty()) {
                        auto it = type_index.find(tmp);
                        if (it != type_index.end()) p.type_ids.push_back(it->second);
                    }
                    tmp.clear();
                } else tmp.push_back(types[i]);
            }
            by_id[p.id] = p;
        }
    }

    void save_file() const {
        if (fileName.empty()) return;
        ofstream fout(fileName, ios::trunc);
        if (!fout.good()) return;
        for (auto &kv : by_id) {
            fout << kv.second.id << ' ' << kv.second.name << ' ' << kv.second.types_str << '\n';
        }
    }

public:
    explicit Pokedex(const char *_fileName) {
        init_types();
        if (_fileName) fileName = _fileName; else fileName.clear();
        load_file();
    }

    ~Pokedex() {
        save_file();
    }

    bool pokeAdd(const char *name, int id, const char *types) {
        // Validate arguments first; throw on invalid
        if (!is_alpha_str(name) || strlen(name) > 10) {
            string m = string("Argument Error: PM Name Invalid (") + (name?name:"") + ")";
            throw ArgumentException(m.c_str());
        }
        if (id <= 0) {
            string m = string("Argument Error: PM ID Invalid (") + to_string(id) + ")";
            throw ArgumentException(m.c_str());
        }
        string types_joined;
        vector<int> tids = parse_types_or_throw(types, types_joined);

        if (by_id.find(id) != by_id.end()) return false;
        Pokemon p{};
        memset(p.name, 0, sizeof(p.name));
        strncpy(p.name, name, sizeof(p.name)-1);
        p.id = id;
        p.type_ids = tids;
        p.types_str = types_joined;
        by_id[id] = p;
        return true;
    }

    bool pokeDel(int id) {
        auto it = by_id.find(id);
        if (it == by_id.end()) return false;
        by_id.erase(it);
        return true;
    }

    std::string pokeFind(int id) const {
        auto it = by_id.find(id);
        if (it == by_id.end()) return string("None");
        return string(it->second.name);
    }

    std::string typeFind(const char *types) const {
        string joined;
        vector<int> q = parse_types_or_throw(types, joined);
        vector<pair<int,string>> hits; // id, name
        for (auto &kv : by_id) {
            const Pokemon &p = kv.second;
            // Check if p.type_ids includes all q
            unordered_set<int> s(p.type_ids.begin(), p.type_ids.end());
            bool ok = true;
            for (int t : q) if (!s.count(t)) { ok = false; break; }
            if (ok) hits.push_back({p.id, string(p.name)});
        }
        if (hits.empty()) return string("None");
        sort(hits.begin(), hits.end());
        // First line: count, then names line by line
        string out;
        out += to_string((int)hits.size());
        for (auto &x : hits) {
            out.push_back('\n');
            out += x.second;
        }
        return out;
    }

    float attack(const char *type, int id) const {
        auto it = by_id.find(id);
        if (it == by_id.end()) return -1.0f;
        string tname(type?type:"");
        auto itx = type_index.find(tname);
        if (itx == type_index.end()) {
            // invalid type argument: per spec, here type is guaranteed single and valid; but be safe
            return -1.0f;
        }
        float res = 1.0f;
        int atk = itx->second;
        for (int def : it->second.type_ids) {
            res *= mult[atk][def];
        }
        return res;
    }

    int catchTry() const {
        if (by_id.empty()) return 0;
        // Start with smallest id owned
        // BFS-like process: acquire if any owned type can deal >=2x to target
        // We assume you can choose any owned pokemon and any one of its types as attack type
        set<int> owned;
        owned.insert(by_id.begin()->first);
        bool changed = true;
        while (changed) {
            changed = false;
            for (auto &kv : by_id) {
                int pid = kv.first;
                if (owned.count(pid)) continue;
                const Pokemon &target = kv.second;
                bool can = false;
                for (int oid : owned) {
                    const Pokemon &op = by_id.at(oid);
                    for (int atk_type : op.type_ids) {
                        float multv = 1.0f;
                        for (int def : target.type_ids) multv *= mult[atk_type][def];
                        if (multv >= 2.0f - 1e-6) { can = true; break; }
                    }
                    if (can) break;
                }
                if (can) {
                    owned.insert(pid);
                    changed = true;
                }
            }
        }
        return (int)owned.size();
    }

    struct iterator {
        const Pokedex *parent = nullptr;
        map<int,Pokemon>::iterator it;
        bool valid = false;

        iterator() = default;
        iterator(const Pokedex *p, map<int,Pokemon>::iterator i, bool v) : parent(p), it(i), valid(v) {}

        void ensure_deref() const {
            if (!valid || parent==nullptr) {
                throw IteratorException("Iterator Error: Invalid Iterator");
            }
            if (it == const_cast<map<int,Pokemon>&>(parent->by_id).end()) {
                throw IteratorException("Iterator Error: Dereference End");
            }
        }

        iterator &operator++() {
            if (!valid || parent==nullptr) throw IteratorException("Iterator Error: Invalid Iterator");
            if (it == const_cast<map<int,Pokemon>&>(parent->by_id).end()) {
                throw IteratorException("Iterator Error: Out Of Range");
            }
            ++it; return *this;
        }
        iterator &operator--() {
            if (!valid || parent==nullptr) throw IteratorException("Iterator Error: Invalid Iterator");
            auto beginIt = const_cast<map<int,Pokemon>&>(parent->by_id).begin();
            if (it == beginIt) {
                throw IteratorException("Iterator Error: Out Of Range");
            }
            --it; return *this;
        }
        iterator operator++(int) { iterator tmp=*this; ++(*this); return tmp; }
        iterator operator--(int) { iterator tmp=*this; --(*this); return tmp; }
        iterator & operator = (const iterator &rhs) {
            parent = rhs.parent; it = rhs.it; valid = rhs.valid; return *this;
        }
        bool operator == (const iterator &rhs) const {
            return parent==rhs.parent && (!valid && !rhs.valid ? true : it==rhs.it);
        }
        bool operator != (const iterator &rhs) const { return !(*this==rhs); }
        Pokemon & operator*() const { ensure_deref(); return it->second; }
        Pokemon *operator->() const { ensure_deref(); return &it->second; }
    };

    iterator begin() {
        return iterator(this, by_id.begin(), true);
    }

    iterator end() {
        return iterator(this, by_id.end(), true);
    }
};

