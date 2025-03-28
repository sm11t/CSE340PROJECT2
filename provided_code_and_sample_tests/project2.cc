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

// Basic lexer functions
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

// Parsing functions
vector<string> id_list();
vector< vector<string> > rrhs();

vector< vector<string> > rrhs() {
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
    while (currentToken.token_type == ID) {
        string lhs = currentToken.lexeme;
        eat(ID);
        eat(ARROW);
        vector< vector<string> > alternatives = rrhs();
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

/* Helper Functions */
// Get set of nonterminals (LHS symbols)
unordered_set<string> getNonterminals(const vector<Parsing_Rules>& rules) {
    unordered_set<string> nts;
    for (auto &r : rules)
        nts.insert(r.lhs);
    return nts;
}

// Build overall order of nonterminals as they appear
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

// Compute nullable set
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

// Build terminal order from RHS
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

// Compute FIRST sets
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

// Compute FOLLOW sets
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

// Compute common prefix length
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

// Lexicographic comparison for RHS vectors
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

/* Task Functions */

// Task 1: Print terminals then nonterminals in order
void Task1() {
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
        }
        else {
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

// Task 2: Compute and print Nullable set
void Task2() {
    unordered_set<string> nonterminalSet;
    for (auto &rule : grammarRules)
        nonterminalSet.insert(rule.lhs);
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
            if (rule.rhs.empty()) {
                nullable.insert(rule.lhs);
                changed = true;
            } else {
                bool allNull = true;
                for (auto &sym : rule.rhs) {
                    if (nonterminalSet.find(sym) == nonterminalSet.end() || nullable.find(sym) == nullable.end()) {
                        allNull = false;
                        break;
                    }
                }
                if (allNull) {
                    nullable.insert(rule.lhs);
                    changed = true;
                }
            }
        }
    }
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

// Task 3: Compute and print FIRST sets
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

// Task 4: Compute and print FOLLOW sets
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

// Task 5: Left Factoring
void Task5() {
    unordered_map<string, vector<vector<string>>> grammarMap;
    for (auto &rule : grammarRules)
        grammarMap[rule.lhs].push_back(rule.rhs);
    unordered_map<string,int> count;
    for (auto &entry : grammarMap)
        count[entry.first] = 0;
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto it = grammarMap.begin(); it != grammarMap.end(); ++it) {
            string X = it->first;
            vector<vector<string>> prods = it->second;
            if (prods.size() < 2)
                continue;
            int bestLen = 0;
            vector<string> bestPrefix;
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
                count[X]++;
                string newNonterm = X + to_string(count[X]);
                vector<vector<string>> newprod;
                for (auto &prod : factored) {
                    vector<string> remainder(prod.begin() + bestLen, prod.end());
                    newprod.push_back(remainder);
                }
                vector<string> newXProd = bestPrefix;
                newXProd.push_back(newNonterm);
                remaining.push_back(newXProd);
                grammarMap[X] = remaining;
                grammarMap[newNonterm] = newprod;
                changed = true;
                break;
            }
        }
    }
    vector<Parsing_Rules> newGrammar;
    for (auto &entry : grammarMap) {
        for (auto &rhs : entry.second) {
            Parsing_Rules pr;
            pr.lhs = entry.first;
            pr.rhs = rhs;
            newGrammar.push_back(pr);
        }
    }
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
    for (auto &rule : newGrammar) {
        cout << rule.lhs << " ->";
        if (!rule.rhs.empty()) {
            for (auto &sym : rule.rhs)
                cout << " " << sym;
        }
        cout << " #" << endl;
    }
}

// Left recursion elimination helpers
void eliminateImmediateLeftRec(
    const string &A,
    unordered_map<string, vector<vector<string>>> &rules,
    unordered_map<string,int> &count
) {
    vector<vector<string>> &prods = rules[A];
    vector<vector<string>> alphaSet, betaSet;
    for (auto &rhs : prods) {
        if (!rhs.empty() && rhs[0] == A) {
            vector<string> alpha(rhs.begin() + 1, rhs.end());
            alphaSet.push_back(alpha);
        } else {
            betaSet.push_back(rhs);
        }
    }
    if (alphaSet.empty())
        return;
    count[A]++;
    string Aprime = A + to_string(count[A]);
    vector<vector<string>> newA;
    for (auto &beta : betaSet) {
        vector<string> rhs = beta;
        rhs.push_back(Aprime);
        newA.push_back(rhs);
    }
    rules[A] = newA;
    vector<vector<string>> newAprime;
    for (auto &alpha : alphaSet) {
        vector<string> rhs = alpha;
        rhs.push_back(Aprime);
        newAprime.push_back(rhs);
    }
    newAprime.push_back({});
    rules[Aprime] = newAprime;
}

void rewriteIndirect(
    const string &Ai,
    const string &Aj,
    unordered_map<string, vector<vector<string>>> &rules
) {
    vector<vector<string>> &iniprod = rules[Ai];
    vector<vector<string>> newprod;
    for (auto &rhs : iniprod) {
        if (!rhs.empty() && rhs[0] == Aj) {
            vector<vector<string>> &AjProds = rules[Aj];
            vector<string> gamma(rhs.begin() + 1, rhs.end());
            for (auto &delta : AjProds) {
                vector<string> newRHS = delta;
                newRHS.insert(newRHS.end(), gamma.begin(), gamma.end());
                newprod.push_back(newRHS);
            }
        } else {
            newprod.push_back(rhs);
        }
    }
    iniprod = newprod;
}

// Task 6: Eliminate left recursion and print final grammar
void Task6() {
    unordered_set<string> ntSet;
    for (auto &r : grammarRules)
        ntSet.insert(r.lhs);
    vector<string> sortedNT(ntSet.begin(), ntSet.end());
    sort(sortedNT.begin(), sortedNT.end());
    unordered_map<string, vector<vector<string>>> rulesMap;
    for (auto &nt : sortedNT)
        rulesMap[nt] = {};
    for (auto &r : grammarRules) {
        if (!rulesMap.count(r.lhs))
            rulesMap[r.lhs] = {};
    }
    for (auto &r : grammarRules)
        rulesMap[r.lhs].push_back(r.rhs);
    unordered_map<string,int> count;
    for (int i = 0; i < (int)sortedNT.size(); i++) {
        string Ai = sortedNT[i];
        for (int j = 0; j < i; j++) {
            string Aj = sortedNT[j];
            rewriteIndirect(Ai, Aj, rulesMap);
        }
        eliminateImmediateLeftRec(Ai, rulesMap, count);
    }
    vector<string> allNT;
    for (auto &kv : rulesMap)
        allNT.push_back(kv.first);
    sort(allNT.begin(), allNT.end());
    vector<Parsing_Rules> finalGrammar;
    for (auto &A : allNT) {
        for (auto &rhs : rulesMap[A]) {
            Parsing_Rules pr;
            pr.lhs = A;
            pr.rhs = rhs;
            finalGrammar.push_back(pr);
        }
    }
    sort(finalGrammar.begin(), finalGrammar.end(), [](const Parsing_Rules &a, const Parsing_Rules &b) {
        if (a.lhs != b.lhs)
            return a.lhs < b.lhs;
        int n = min(a.rhs.size(), b.rhs.size());
        for (int i = 0; i < n; i++) {
            if (a.rhs[i] != b.rhs[i])
                return a.rhs[i] < b.rhs[i];
        }
        return a.rhs.size() < b.rhs.size();
    });
    for (auto &r : finalGrammar) {
        cout << r.lhs << " ->";
        if (!r.rhs.empty()) {
            for (auto &sym : r.rhs)
                cout << " " << sym;
        }
        cout << " #" << endl;
    }
}
    
int main (int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Error: missing argument\n";
        return 1;
    }
    int task = atoi(argv[1]);
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
