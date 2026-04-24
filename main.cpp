#include <queue>
#include <string>
#include <unordered_set>
#include <vector>
#include <iostream>

namespace Grammar {
class NFA;
NFA MakeStar(const char &character);
NFA MakePlus(const char &character);
NFA MakeQuestion(const char &character);
NFA Concatenate(const NFA &nfa1, const NFA &nfa2);
NFA Union(const NFA &nfa1, const NFA &nfa2);
NFA MakeSimple(const char &character);

enum class TransitionType { Epsilon, a, b };

struct Transition {
  TransitionType type;
  int to;
  Transition(TransitionType type, int to) : type(type), to(to) {}
};

class NFA {
private:
  int start;
  std::unordered_set<int> ends;
  std::vector<std::vector<Transition>> transitions;

public:
  NFA() = default;
  ~NFA() = default;

  std::unordered_set<int> GetEpsilonClosure(std::unordered_set<int> states) const {
    std::unordered_set<int> closure;
    std::queue<int> queue;
    for (const auto &state : states) {
      if (closure.find(state) != closure.end()) continue;
      queue.push(state);
      closure.insert(state);
    }
    while (!queue.empty()) {
      int current = queue.front();
      queue.pop();
      if (current < 0 || current >= (int)transitions.size()) continue;
      for (const auto &transition : transitions[current]) {
        if (transition.type == TransitionType::Epsilon) {
          if (closure.find(transition.to) == closure.end()) {
            queue.push(transition.to);
            closure.insert(transition.to);
          }
        }
      }
    }
    return closure;
  }

  std::unordered_set<int> Advance(std::unordered_set<int> current_states, char character) const {
    std::unordered_set<int> next_states;
    // epsilon-closure of current states
    auto closure = GetEpsilonClosure(current_states);
    // move on character
    TransitionType need = (character == 'a') ? TransitionType::a : TransitionType::b;
    for (int s : closure) {
      if (s < 0 || s >= (int)transitions.size()) continue;
      for (const auto &tr : transitions[s]) {
        if (tr.type == need) next_states.insert(tr.to);
      }
    }
    // epsilon-closure of destinations
    return GetEpsilonClosure(next_states);
  }

  bool IsAccepted(int state) const { return ends.find(state) != ends.end(); }
  int GetStart() const { return start; }

  friend NFA MakeStar(const char &character);
  friend NFA MakePlus(const char &character);
  friend NFA MakeQuestion(const char &character);
  friend NFA MakeSimple(const char &character);
  friend NFA Concatenate(const NFA &nfa1, const NFA &nfa2);
  friend NFA Union(const NFA &nfa1, const NFA &nfa2);
};

class RegexChecker {
private:
  NFA nfa;

  static bool is_postfix(char c) { return c == '*' || c == '+' || c == '?'; }

  static NFA buildTerm(const std::string &term) {
    bool has = false;
    NFA cur;
    for (size_t i = 0; i < term.size(); ++i) {
      char c = term[i];
      if (c != 'a' && c != 'b') continue;
      char op = 0;
      if (i + 1 < term.size() && is_postfix(term[i + 1])) {
        op = term[i + 1];
        ++i;
      }
      NFA piece;
      if (op == '*') piece = MakeStar(c);
      else if (op == '+') piece = MakePlus(c);
      else if (op == '?') piece = MakeQuestion(c);
      else piece = MakeSimple(c);
      if (!has) {
        cur = piece;
        has = true;
      } else {
        cur = Concatenate(cur, piece);
      }
    }
    // If term is empty, we skip it by returning a simple placeholder; caller will ignore empty parts
    if (!has) return MakeSimple('a');
    return cur;
  }

public:
  bool Check(const std::string &str) const {
    std::unordered_set<int> states; states.insert(nfa.GetStart());
    for (char ch : str) {
      if (ch != 'a' && ch != 'b') return false;
      states = nfa.Advance(states, ch);
      if (states.empty()) return false;
    }
    for (int s : states) if (nfa.IsAccepted(s)) return true;
    return false;
  }

  RegexChecker(const std::string &regex) {
    // split by '|'
    std::vector<NFA> parts;
    std::string cur;
    for (char c : regex) {
      if (c == '|') { parts.push_back(buildTerm(cur)); cur.clear(); }
      else cur.push_back(c);
    }
    parts.push_back(buildTerm(cur));
    if (parts.empty()) {
      // default empty regex: match nothing -> create NFA with one state and no end
      NFA tmp = MakeSimple('a');
      nfa = Union(tmp, tmp); // ensures structure, though it matches 'a'
      // But parts will never be empty since we push back at least one term
    }
    NFA res = parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
      res = Union(res, parts[i]);
    }
    nfa = res;
  }
};

NFA MakeStar(const char &character) {
  NFA nfa;
  nfa.start = 0;
  nfa.ends.insert(0);
  nfa.transitions.assign(1, {});
  if (character == 'a') nfa.transitions[0].push_back({TransitionType::a, 0});
  else nfa.transitions[0].push_back({TransitionType::b, 0});
  return nfa;
}

NFA MakePlus(const char &character) {
  NFA nfa;
  nfa.start = 0;
  nfa.ends.insert(1);
  nfa.transitions.assign(2, {});
  if (character == 'a') {
    nfa.transitions[0].push_back({TransitionType::a, 1});
    nfa.transitions[1].push_back({TransitionType::a, 1});
  } else {
    nfa.transitions[0].push_back({TransitionType::b, 1});
    nfa.transitions[1].push_back({TransitionType::b, 1});
  }
  return nfa;
}

NFA MakeQuestion(const char &character) {
  NFA nfa;
  nfa.start = 0;
  nfa.ends.insert(0);
  nfa.ends.insert(1);
  nfa.transitions.assign(2, {});
  if (character == 'a') nfa.transitions[0].push_back({TransitionType::a, 1});
  else nfa.transitions[0].push_back({TransitionType::b, 1});
  return nfa;
}

NFA Concatenate(const NFA &nfa1, const NFA &nfa2) {
  NFA res;
  int offset = (int)nfa1.transitions.size();
  res.start = nfa1.start;
  res.transitions = nfa1.transitions;
  res.transitions.resize(offset + (int)nfa2.transitions.size());
  // copy nfa2 transitions
  for (size_t i = 0; i < nfa2.transitions.size(); ++i) {
    for (const auto &tr : nfa2.transitions[i]) {
      res.transitions[offset + (int)i].push_back({tr.type, tr.to + offset});
    }
  }
  // connect ends of nfa1 to start of nfa2 via epsilon
  int target = nfa2.start + offset;
  for (int e : nfa1.ends) {
    if (e >= 0 && e < (int)res.transitions.size())
      res.transitions[e].push_back({TransitionType::Epsilon, target});
  }
  // ends are ends of nfa2 (offset)
  for (int e : nfa2.ends) res.ends.insert(e + offset);
  return res;
}

NFA Union(const NFA &nfa1, const NFA &nfa2) {
  NFA res;
  int size1 = (int)nfa1.transitions.size();
  int size2 = (int)nfa2.transitions.size();
  res.start = 0;
  res.transitions.assign(1 + size1 + size2, {});
  // epsilon from new start to both starts
  res.transitions[0].push_back({TransitionType::Epsilon, 1 + nfa1.start});
  res.transitions[0].push_back({TransitionType::Epsilon, 1 + size1 + nfa2.start});
  // copy nfa1
  for (int i = 0; i < size1; ++i) {
    for (const auto &tr : nfa1.transitions[i]) {
      res.transitions[1 + i].push_back({tr.type, 1 + tr.to});
    }
  }
  // copy nfa2
  for (int i = 0; i < size2; ++i) {
    for (const auto &tr : nfa2.transitions[i]) {
      res.transitions[1 + size1 + i].push_back({tr.type, 1 + size1 + tr.to});
    }
  }
  // ends are union of both ends (offset accordingly)
  for (int e : nfa1.ends) res.ends.insert(1 + e);
  for (int e : nfa2.ends) res.ends.insert(1 + size1 + e);
  return res;
}

NFA MakeSimple(const char &character) {
  NFA nfa;
  nfa.start = 0;
  nfa.ends.insert(1);
  nfa.transitions.assign(2, {});
  if (character == 'a') nfa.transitions[0].push_back({TransitionType::a, 1});
  else nfa.transitions[0].push_back({TransitionType::b, 1});
  return nfa;
}

} // namespace Grammar

int main() {
  using namespace std;
  using namespace Grammar;
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  vector<string> lines;
  string line;
  while (getline(cin, line)) {
    // trim whitespace
    size_t start = line.find_first_not_of(" \t\r\n");
    if (start == string::npos) continue;
    size_t end = line.find_last_not_of(" \t\r\n");
    lines.push_back(line.substr(start, end - start + 1));
  }
  if (lines.empty()) return 0;
  string regex = lines[0];
  vector<string> inputs;
  if (lines.size() >= 2) {
    bool numeric = true;
    for (char ch : lines[1]) if (!(ch >= '0' && ch <= '9')) { numeric = false; break; }
    if (numeric) {
      int n = stoi(lines[1]);
      for (size_t i = 0; i < (size_t)n && 2 + i < lines.size(); ++i) inputs.push_back(lines[2 + i]);
    } else {
      for (size_t i = 1; i < lines.size(); ++i) inputs.push_back(lines[i]);
    }
  }
  RegexChecker checker(regex);
  for (const auto &s : inputs) {
    cout << (checker.Check(s) ? "YES" : "NO") << '\n';
  }
  return 0;
}
