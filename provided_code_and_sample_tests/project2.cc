#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include "lexer.h"
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

using namespace std;

struct Parsing_Rules {
    string lhs;
    vector<string> rhs;
};

vector<Parsing_Rules> grammarRules;
LexicalAnalyzer lex;
Token currentToken;

void getNextToken() {
    currentToken = lex.GetToken();
}

void eat(TokenType expected) {
    if (currentToken.token_type != expected) {
        cout << "SYNTAX ERROR !!!!!!!!!!!!!!" << endl;
        exit(1);
    }
    getNextToken();
}

vector<string> id_list();
vector< vector<string> > right_hand_side();

vector< vector<string> > right_hand_side() {
    vector< vector<string> > alternatives;
    vector<string> alt = id_list();
    alternatives.push_back(alt);
    while (currentToken.token_type == OR) {
        eat(OR);
        alt = id_list();
        alternatives.push_back(alt);
    }
    return alternatives;
}

vector<string> id_list() {
    vector<string> symbols;
    while (currentToken.token_type == ID) {
        symbols.push_back(currentToken.lexeme);
        eat(ID);
    }
    return symbols;
}

void ReadGrammar() {
    getNextToken();
    while (currentToken.token_type == ID){
        string lhs = currentToken.lexeme;
        eat(ID);
        eat(ARROW);
        vector< vector<string> > alternatives = right_hand_side();
        eat(STAR);
        for (auto &alt : alternatives) {
            Parsing_Rules pr;
            pr.lhs = lhs;
            pr.rhs = alt;
            grammarRules.push_back(pr);
        }
    }

    if (currentToken.token_type != HASH) {
        cout << "SYNTAX ERROR !!!!!!!!!!!!!!" << endl;
        exit(1);
    }
    eat(HASH);
}

//Helper Functions

unordered_set<string> getNonterminals(const vector<Parsing_Rules>& rules) {
    unordered_set<string> nts;
    for (auto &r : rules)
        nts.insert(r.lhs);
    return nts;
}

vector<string> getOverallNTOrder(const vector<Parsing_Rules>& rules, const unordered_set<string>& ntSet) {
    vector<string> order;
    unordered_set<string> seen;
    for (auto &r : rules) {
        if (ntSet.count(r.lhs) && !seen.count(r.lhs)) {
            order.push_back(r.lhs);
            seen.insert(r.lhs);
        }
        for (auto &sym : r.rhs) {
            if (ntSet.count(sym) && !seen.count(sym)) {
                order.push_back(sym);
                seen.insert(sym);
            }
        }
    }
    return order;
}

unordered_set<string> computeNullableSet(const vector<Parsing_Rules>& rules, const unordered_set<string>& ntSet) {
    unordered_set<string> nullable;
    bool change = true;
    while (change) {
        change = false;
        for (auto &r : rules) {
            if (nullable.count(r.lhs))
                continue;
            if (r.rhs.empty()) {
                nullable.insert(r.lhs);
                change = true;
            } else {
                bool allNull = true;
                for (auto &sym : r.rhs) {
                    if (!ntSet.count(sym) || !nullable.count(sym)) {
                        allNull = false;
                        break;
                    }
                }
                if (allNull) {
                    nullable.insert(r.lhs);
                    change = true;
                }
            }
        }
    }
    return nullable;
}

vector<string> getTerminalOrder(const vector<Parsing_Rules>& rules, const unordered_set<string>& ntSet) {
    vector<string> order;
    for (auto &r : rules) {
        for (auto &sym : r.rhs) {
            if (!ntSet.count(sym) && find(order.begin(), order.end(), sym) == order.end())
                order.push_back(sym);
        }
    }
    return order;
}

unordered_map<string, vector<string>> computeFirstSets(
    const vector<Parsing_Rules>& rules,
    const unordered_set<string>& ntSet,
    const unordered_set<string>& nullable,
    const vector<string>& overallNT,
    const vector<string>& termOrder)
{
    unordered_map<string, vector<string>> first;
    for (auto &t : termOrder)
        first[t] = { t };
    for (auto &nt : overallNT)
        first[nt] = vector<string>();

    bool updated = true;
    while (updated) {
        updated = false;
        for (auto &r : rules) {
            vector<string> prodFirst;
            for (auto &sym : r.rhs) {
                for (auto &x : first[sym])
                    if (find(prodFirst.begin(), prodFirst.end(), x) == prodFirst.end())
                        prodFirst.push_back(x);
                if (ntSet.count(sym)) {
                    if (!nullable.count(sym))
                        break;
                } else {
                    break;
                }
            }
            for (auto &x : prodFirst) {
                if (find(first[r.lhs].begin(), first[r.lhs].end(), x) == first[r.lhs].end()) {
                    first[r.lhs].push_back(x);
                    updated = true;
                }
            }
        }
    }
    unordered_map<string,int> tIndex;
    for (int i = 0; i < termOrder.size(); i++)
        tIndex[termOrder[i]] = i;
    for (auto &nt : overallNT) {
        sort(first[nt].begin(), first[nt].end(), [&](const string &a, const string &b) {
            return tIndex[a] < tIndex[b];
        });
    }
    return first;
}

unordered_map<string, vector<string>> computeFollowSets(
    const vector<Parsing_Rules>& rules,
    const unordered_set<string>& ntSet,
    const unordered_map<string, vector<string>>& firstMap,
    const unordered_set<string>& nullable,
    const vector<string>& ntOrder)
{
    unordered_map<string, vector<string>> follow;
    for (auto &n : ntOrder)
        follow[n] = {};
    if (!ntOrder.empty())
        follow[ntOrder[0]].push_back("$");

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto &r : rules) {
            string X = r.lhs;
            for (int i = 0; i < r.rhs.size(); i++) {
                string Y = r.rhs[i];
                if (!ntSet.count(Y))
                    continue;
                vector<string> firstBeta;
                bool betaNullable = true;
                for (int j = i+1; j < r.rhs.size(); j++) {
                    string sym = r.rhs[j];
                    for (auto &t : firstMap.at(sym))
                        if (find(firstBeta.begin(), firstBeta.end(), t) == firstBeta.end())
                            firstBeta.push_back(t);
                    if (ntSet.count(sym)) {
                        if (!nullable.count(sym)) {
                            betaNullable = false;
                            break;
                        }
                    } else {
                        betaNullable = false;
                        break;
                    }
                }
                for (auto &t : firstBeta) {
                    if (find(follow[Y].begin(), follow[Y].end(), t) == follow[Y].end()){
                        follow[Y].push_back(t);
                        changed = true;
                    }
                }
                if (i == r.rhs.size()-1 || betaNullable) {
                    for (auto &t : follow[X]) {
                        if (find(follow[Y].begin(), follow[Y].end(), t) == follow[Y].end()){
                            follow[Y].push_back(t);
                            changed = true;
                        }
                    }
                }
            }
        }
    }
    return follow;
}

// Helper: compute the length of the common prefix between two productions.
int commonPrefixLength(const vector<string>& prod1, const vector<string>& prod2) {
    int len = min(prod1.size(), prod2.size());
    int count = 0;
    for (int i = 0; i < len; i++) {
        if (prod1[i] == prod2[i])
            count++;
        else
            break;
    }
    return count;
}

// Lexicographic comparison of two RHS vectors.
bool lexCompare(const vector<string>& a, const vector<string>& b) {
    int n = min(a.size(), b.size());
    for (int i = 0; i < n; i++) {
        if (a[i] < b[i])
            return true;
        else if (a[i] > b[i])
            return false;
    }
    return a.size() < b.size();
}

void Task1()
{
    unordered_set<string> nonterminalSet;
    for (auto &rule : grammarRules)
        nonterminalSet.insert(rule.lhs);
    vector<string> orderedSymbols;
    for (auto &rule : grammarRules) {
        orderedSymbols.push_back(rule.lhs);
        for (auto &sym : rule.rhs)
            orderedSymbols.push_back(sym);
    }
    vector<string> terminalOrder, nonterminalOrder;
    unordered_set<string> seenTerminals, seenNonterminals;
    for (auto &sym : orderedSymbols) {
        if (nonterminalSet.find(sym) != nonterminalSet.end()) {
            if (seenNonterminals.find(sym) == seenNonterminals.end()) {
                nonterminalOrder.push_back(sym);
                seenNonterminals.insert(sym);
            }
        } else {
            if (seenTerminals.find(sym) == seenTerminals.end()) {
                terminalOrder.push_back(sym);
                seenTerminals.insert(sym);
            }
        }
    }
    for (auto &s : terminalOrder)
        cout << s << " ";
    for (auto &s : nonterminalOrder)
        cout << s << " ";
}

void Task2()
{
    // Finding from LHS
    unordered_set<string> nonterminalSet;
    for (auto &rule : grammarRules)
        nonterminalSet.insert(rule.lhs);

    //scanning rules in input order and building order of nonterminals
    vector<string> overallOrder;
    unordered_set<string> seen;
    for (auto &rule : grammarRules) {
        if (seen.find(rule.lhs) == seen.end()) {
            overallOrder.push_back(rule.lhs);
            seen.insert(rule.lhs);
        }
        for (auto &sym : rule.rhs) {
            if (nonterminalSet.count(sym) && seen.find(sym) == seen.end()) {
                overallOrder.push_back(sym);
                seen.insert(sym);
            }
        }
    }

    unordered_set<string> nullable;
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto &rule : grammarRules) {
            if (nullable.count(rule.lhs))
                continue;
            // If production is epsilon, mark LHS as nullable.
            if (rule.rhs.empty()) {
                nullable.insert(rule.lhs);
                changed = true;
            } else {
                bool allNullable = true;
                for (auto &sym : rule.rhs) {
                    // If symbol is terminal or nonterminal not nullable, production isn't nullable.
                    if (nonterminalSet.find(sym) == nonterminalSet.end() || nullable.find(sym) == nullable.end()) {
                        allNullable = false;
                        break;
                    }
                }
                if (allNullable) {
                    nullable.insert(rule.lhs);
                    changed = true;
                }
            }
        }
    }

    // Print Nullable set in overall order.
    cout << "Nullable = { ";
    bool first = true;
    for (auto &nt : overallOrder) {
        if (nullable.count(nt)) {
            if (!first)
                cout << " , ";
            cout << nt;
            first = false;
        }
    }
    cout << " }" << endl;
}

void Task3() {
    auto ntSet = getNonterminals(grammarRules);
    auto overallNT = getOverallNTOrder(grammarRules, ntSet);
    auto nullable = computeNullableSet(grammarRules, ntSet);
    auto termOrder = getTerminalOrder(grammarRules, ntSet);
    auto first = computeFirstSets(grammarRules, ntSet, nullable, overallNT, termOrder);
    for (auto &nt : overallNT) {
        cout << "FIRST(" << nt << ") = { ";
        bool printed = false;
        for (auto &x : first[nt]) {
            if (printed)
                cout << ", ";
            cout << x;
            printed = true;
        }
        cout << " }" << endl;
    }
}

void Task4() {
    auto ntSet = getNonterminals(grammarRules);
    auto overallNT = getOverallNTOrder(grammarRules, ntSet);
    auto termOrder = getTerminalOrder(grammarRules, ntSet);
    auto nullable = computeNullableSet(grammarRules, ntSet);
    auto first = computeFirstSets(grammarRules, ntSet, nullable, overallNT, termOrder);
    auto follow = computeFollowSets(grammarRules, ntSet, first, nullable, overallNT);
    unordered_map<string,int> orderMap;
    orderMap["$"] = -1;
    for (int i = 0; i < termOrder.size(); i++)
        orderMap[termOrder[i]] = i;
    for (auto &nt : overallNT) {
        sort(follow[nt].begin(), follow[nt].end(), [&](const string &a, const string &b){
            return orderMap[a] < orderMap[b];
        });
    }
    for (auto &nt : overallNT) {
        cout << "FOLLOW(" << nt << ") = { ";
        bool printed = false;
        for (auto &t : follow[nt]) {
            if (printed)
                cout << ", ";
            cout << t;
            printed = true;
        }
        cout << " }" << endl;
    }
}


void Task5()
{
    // Build a map from nonterminal to list of productions.
    unordered_map<string, vector<vector<string>>> grammarMap;
    for (auto &rule : grammarRules)
        grammarMap[rule.lhs].push_back(rule.rhs);
    
    // Map for tracking new nonterminal names (e.g. X1, X2, etc.)
    unordered_map<string,int> newNameCount;
    for (auto &entry : grammarMap)
        newNameCount[entry.first] = 0;
    
    bool changed = true;
    while (changed) {
        changed = false;
        // Iterate over grammarMap; if a nonterminal has at least 2 productions, try to factor.
        for (auto it = grammarMap.begin(); it != grammarMap.end(); ++it) {
            string X = it->first;
            vector<vector<string>> prods = it->second;
            if (prods.size() < 2)
                continue;
            int bestLen = 0;
            vector<string> bestPrefix;
            // Compare every pair to find the longest common prefix.
            for (int i = 0; i < prods.size(); i++) {
                for (int j = i+1; j < prods.size(); j++) {
                    int cp = commonPrefixLength(prods[i], prods[j]);
                    if (cp > bestLen) {
                        bestLen = cp;
                        bestPrefix.assign(prods[i].begin(), prods[i].begin() + cp);
                    } else if (cp == bestLen && cp > 0) {
                        vector<string> currentPrefix(prods[i].begin(), prods[i].begin() + cp);
                        if (currentPrefix < bestPrefix)
                            bestPrefix = currentPrefix;
                    }
                }
            }
            if (bestLen > 0) {
                // Partition productions: those with the bestPrefix and those without.
                vector<vector<string>> factored, remaining;
                for (auto &prod : prods) {
                    if (prod.size() >= bestLen &&
                        equal(prod.begin(), prod.begin() + bestLen, bestPrefix.begin()))
                        factored.push_back(prod);
                    else
                        remaining.push_back(prod);
                }
                if (factored.size() < 2)
                    continue;
                // Create new nonterminal for left factoring.
                newNameCount[X]++;
                string newNonterm = X + to_string(newNameCount[X]);
                vector<vector<string>> newProds;
                // For each factored production, remove the common prefix.
                for (auto &prod : factored) {
                    vector<string> remainder(prod.begin() + bestLen, prod.end());
                    newProds.push_back(remainder);
                }
                // Replace factored productions in X with one production: bestPrefix newNonterm.
                vector<string> newXProd = bestPrefix;
                newXProd.push_back(newNonterm);
                remaining.push_back(newXProd);
                // Update grammarMap.
                grammarMap[X] = remaining;
                grammarMap[newNonterm] = newProds;
                changed = true;
                // Restart the loop after any change.
                break;
            }
        }
    }
    
    // Collect all rules from the grammarMap.
    vector<Parsing_Rules> newGrammar;
    for (auto &entry : grammarMap) {
        for (auto &rhs : entry.second) {
            Parsing_Rules pr;
            pr.lhs = entry.first;
            pr.rhs = rhs;
            newGrammar.push_back(pr);
        }
    }
    
    // Sort lexicographically: compare LHS first, then RHS (symbol by symbol; shorter comes first if prefix).
    sort(newGrammar.begin(), newGrammar.end(), [](const Parsing_Rules &a, const Parsing_Rules &b) {
        if (a.lhs != b.lhs)
            return a.lhs < b.lhs;
        int n = min(a.rhs.size(), b.rhs.size());
        for (int i = 0; i < n; i++) {
            if (a.rhs[i] != b.rhs[i])
                return a.rhs[i] < b.rhs[i];
        }
        return a.rhs.size() < b.rhs.size();
    });
    
    // Print the rules in the required format.
    for (auto &rule : newGrammar) {
        cout << rule.lhs << " ->";
        if (!rule.rhs.empty()) {
            for (auto &sym : rule.rhs)
                cout << " " << sym;
        }
        cout << " #" << endl;
    }
}

void eliminateImmediateLeftRec(
    const string &A,
    unordered_map<string, vector<vector<string>>> &rules,
    unordered_map<string,int> &newNameCount // how many times we've introduced A1, A2, ...
) {
    vector<vector<string>> &prods = rules[A];

    // Partition into alpha-rules (start with A) and beta-rules (start != A)
    vector<vector<string>> alphaSet;
    vector<vector<string>> betaSet;
    for (auto &rhs : prods) {
        if (!rhs.empty() && rhs[0] == A) {
            // A -> A alpha
            vector<string> alpha(rhs.begin() + 1, rhs.end());
            alphaSet.push_back(alpha);
        } else {
            // A -> β
            betaSet.push_back(rhs);
        }
    }

    // If no alpha-rules, nothing to do
    if (alphaSet.empty()) {
        return;
    }

    // We do have direct left recursion, so produce a new name A1 (or A2, etc.)
    newNameCount[A]++;
    string Aprime = A + to_string(newNameCount[A]);

    // Rewrite rules[A] to contain only the "beta Aprime" ones
    vector<vector<string>> newA;
    for (auto &beta : betaSet) {
        // A -> β Aprime
        vector<string> rhs = beta;    // copy β
        rhs.push_back(Aprime);        // append Aprime
        newA.push_back(rhs);
    }
    rules[A] = newA; // overwrite

    // Build rules for A'
    vector<vector<string>> newAprime;
    // For each alpha rule, A1 -> alpha A1
    for (auto &alpha : alphaSet) {
        vector<string> rhs = alpha;   // copy alpha
        rhs.push_back(Aprime);        // append Aprime
        newAprime.push_back(rhs);
    }
    // Also add A1 -> ε
    // We represent ε as an empty vector
    newAprime.push_back({});

    // Insert Aprime into the grammar
    rules[Aprime] = newAprime;
}

// For a rule Ai -> Aj γ, we replace it with Ai -> δ γ for each rule (Aj -> δ) in rules[Aj].
void rewriteIndirect(
    const string &Ai,
    const string &Aj,
    unordered_map<string, vector<vector<string>>> &rules
) {
    vector<vector<string>> &AiProds = rules[Ai];
    vector<vector<string>> newProds;  // will hold the updated list for Ai

    for (auto &rhs : AiProds) {
        // Check if Ai -> Aj γ
        if (!rhs.empty() && rhs[0] == Aj) {
            // For each production of Aj -> δ, produce Ai -> δ γ
            vector<vector<string>> &AjProds = rules[Aj];
            vector<string> gamma(rhs.begin() + 1, rhs.end()); // everything after Aj
            for (auto &delta : AjProds) {
                // build δ + γ
                vector<string> newRHS = delta;
                // then append gamma
                newRHS.insert(newRHS.end(), gamma.begin(), gamma.end());
                newProds.push_back(newRHS);
            }
        } else {
            // Keep Ai -> rhs as is
            newProds.push_back(rhs);
        }
    }
    // Replace Ai's rules
    AiProds = newProds;
}


void Task6()
{
    // 1) Collect all non-terminals and sort them lexicographically
    unordered_set<string> nonterminalSet;
    for (auto &rule : grammarRules) {
        nonterminalSet.insert(rule.lhs);
    }
    // We'll store the original non-terminals in a vector and sort them
    vector<string> sortedNT(nonterminalSet.begin(), nonterminalSet.end());
    sort(sortedNT.begin(), sortedNT.end());

    // 2) Build a map from NonTerminal -> list of productions (RHS vectors)
    //    We also want to collect any other "nonterminals" that appear as we go
    //    but the spec says we only do rewriting for these original ones in sorted order.
    unordered_map<string, vector<vector<string>>> rulesMap;
    for (auto &nt : sortedNT) {
        rulesMap[nt] = {}; // Initialize empty
    }

    // Also track any symbols that appear on the LHS that might not have been in sortedNT
    // (rare edge cases, but for cleanliness, let's store them too)
    for (auto &rule : grammarRules) {
        if (rulesMap.find(rule.lhs) == rulesMap.end()) {
            // It's a new NT not in sortedNT (?), we add it
            rulesMap[rule.lhs] = {};
        }
    }

    // Now actually populate
    for (auto &rule : grammarRules) {
        rulesMap[rule.lhs].push_back(rule.rhs);
    }

    // We'll need to track new nonterminals introduced for immediate left recursion, like A1, A2, ...
    // For each original A, how many times have we introduced an A1, A2...?
    unordered_map<string,int> newNameCount;

    // 3) The standard algorithm:
    //    for i in [1..n], for j in [1..i-1], rewrite Ai’s rules that begin with Aj
    //    then eliminate immediate left recursion from Ai
    // Remember that sortedNT is zero-based in C++ but that’s just an index shift
    for (int i = 0; i < (int)sortedNT.size(); i++) {
        string Ai = sortedNT[i];
        // For j in [0..i-1]
        for (int j = 0; j < i; j++) {
            string Aj = sortedNT[j];
            // rewrite Ai -> Aj gamma
            rewriteIndirect(Ai, Aj, rulesMap);
        }
        // Now eliminate immediate left recursion from Ai
        eliminateImmediateLeftRec(Ai, rulesMap, newNameCount);
    }

    // 4) At this point, we have removed all left recursion from the original non-terminals in sortedNT.
    //    The rewriting steps can produce new non-terminals (e.g., A1). We do NOT re-run the
    //    rewriting steps on those new ones, per the spec's standard method.

    // 5) Collect all rules (including newly introduced A1, etc.) into a big vector
    //    so we can sort them lexicographically and print
    vector<Parsing_Rules> finalGrammar;

    // We want to gather every nonterminal that ended up in rulesMap,
    // which includes the newly introduced ones. Let's do a second pass:
    vector<string> allNT;
    for (auto &kv : rulesMap) {
        allNT.push_back(kv.first);
    }
    sort(allNT.begin(), allNT.end()); // so that when we gather final rules, we gather in alpha order
    // Then gather
    for (auto &A : allNT) {
        for (auto &rhs : rulesMap[A]) {
            Parsing_Rules pr;
            pr.lhs = A;
            pr.rhs = rhs;
            finalGrammar.push_back(pr);
        }
    }

    sort(finalGrammar.begin(), finalGrammar.end(), [](const Parsing_Rules &a, const Parsing_Rules &b) {
        // Compare LHS first
        if (a.lhs != b.lhs) {
            return a.lhs < b.lhs;
        }
        // comparing RHS
        int n = min(a.rhs.size(), b.rhs.size());
        for (int i = 0; i < n; i++) {
            if (a.rhs[i] != b.rhs[i]) {
                return a.rhs[i] < b.rhs[i];
            }
        }
        return a.rhs.size() < b.rhs.size();
    });

    // printing in the required format
    for (auto &rule : finalGrammar) {
        cout << rule.lhs << " ->";
        if (!rule.rhs.empty()) {
            for (auto &sym : rule.rhs) {
                cout << " " << sym;
            }
        }
        cout << " #" << endl;
    }
}
    
int main (int argc, char* argv[])
{
    int task;
    if (argc < 2) {
        cout << "Error: missing argument\n";
        return 1;
    }
    task = atoi(argv[1]);
    ReadGrammar();
    switch (task) {
        case 1: Task1(); break;
        case 2: Task2(); break;
        case 3: Task3(); break;
        case 4: Task4(); break;
        case 5: Task5(); break;
        case 6: Task6(); break;
        default:
            cout << "Error: unrecognized task number " << task << "\n";
            break;
    }
    return 0;
}
