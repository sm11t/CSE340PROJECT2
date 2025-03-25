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
    unordered_set<string> ntSet;
    for (auto &rule : grammarRules)
        ntSet.insert(rule.lhs);

    //scanning rules in input order and building order of nonterminals
    vector<string> overallOrder;
    unordered_set<string> seen;
    for (auto &rule : grammarRules) {
        if (seen.find(rule.lhs) == seen.end()) {
            overallOrder.push_back(rule.lhs);
            seen.insert(rule.lhs);
        }
        for (auto &sym : rule.rhs) {
            if (ntSet.count(sym) && seen.find(sym) == seen.end()) {
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
                    if (ntSet.find(sym) == ntSet.end() || nullable.find(sym) == nullable.end()) {
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

void Task3()
{
    // Build set of nonterminals (those that appear as LHS)
    unordered_set<string> ntSet;
    for (auto &rule : grammarRules)
        ntSet.insert(rule.lhs);

    // Compute overall nonterminal order by scanning all tokens in grammar
    vector<string> overallNT;
    unordered_set<string> seen;
    for (auto &rule : grammarRules) {
        if (ntSet.find(rule.lhs) != ntSet.end() && seen.find(rule.lhs) == seen.end()){
            overallNT.push_back(rule.lhs);
            seen.insert(rule.lhs);
        }
        for (auto &sym : rule.rhs) {
            if (ntSet.find(sym) != ntSet.end() && seen.find(sym) == seen.end()){
                overallNT.push_back(sym);
                seen.insert(sym);
            }
        }
    }
    
    // Compute Nullable set.
    unordered_set<string> nullable;
    bool change = true;
    while (change) {
        change = false;
        for (auto &rule : grammarRules) {
            if (nullable.find(rule.lhs) != nullable.end())
                continue;
            if (rule.rhs.empty()) {
                nullable.insert(rule.lhs);
                change = true;
            } else {
                bool allNull = true;
                for (auto &sym : rule.rhs) {
                    if (ntSet.find(sym) == ntSet.end() || nullable.find(sym) == nullable.end()){
                        allNull = false;
                        break;
                    }
                }
                if (allNull) {
                    nullable.insert(rule.lhs);
                    change = true;
                }
            }
        }
    }
    
    // Group productions for each nonterminal.
    unordered_map<string, vector<vector<string>>> prodGroup;
    for (auto &nt : overallNT)
        prodGroup[nt] = vector<vector<string>>();
    for (auto &rule : grammarRules) {
        if (ntSet.find(rule.lhs) != ntSet.end())
            prodGroup[rule.lhs].push_back(rule.rhs);
    }
    
    // Build FIRST sets.
    // For terminals: FIRST(t) = { t }
    unordered_map<string, vector<string>> first;
    // Build termOrder by scanning rhs in order.
    vector<string> termOrder;
    for (auto &rule : grammarRules) {
        for (auto &sym : rule.rhs) {
            if (ntSet.find(sym) == ntSet.end()) {
                if(find(termOrder.begin(), termOrder.end(), sym) == termOrder.end())
                    termOrder.push_back(sym);
            }
        }
    }
    // Initialize FIRST for terminals and nonterminals.
    for (auto &t : termOrder)
        first[t] = { t };
    for (auto &nt : overallNT)
        first[nt] = vector<string>();
    
    // Fixpoint computation of FIRST sets.
    bool updated = true;
    while (updated) {
        updated = false;
        for (auto &rule : grammarRules) {
            vector<string> prodFirst;
            for (auto &sym : rule.rhs) {
                // Add FIRST(sym) (preserve order, no duplicates).
                for (auto &x : first[sym]) {
                    if (find(prodFirst.begin(), prodFirst.end(), x) == prodFirst.end())
                        prodFirst.push_back(x);
                }
                // Stop if sym is not nullable.
                if (ntSet.find(sym) != ntSet.end()) {
                    if (nullable.find(sym) == nullable.end())
                        break;
                } else {
                    break;
                }
            }
            // Merge prodFirst into FIRST(rule.lhs)
            for (auto &x : prodFirst) {
                if (find(first[rule.lhs].begin(), first[rule.lhs].end(), x) == first[rule.lhs].end()) {
                    first[rule.lhs].push_back(x);
                    updated = true;
                }
            }
        }
    }
    
    // Create a mapping from terminal to its index in termOrder.
    unordered_map<string,int> termIndex;
    for (int i = 0; i < termOrder.size(); i++)
        termIndex[termOrder[i]] = i;
    
    // Sort each nonterminal's FIRST set according to termOrder.
    for (auto &nt : overallNT) {
        sort(first[nt].begin(), first[nt].end(), [&](const string &a, const string &b) {
            return termIndex[a] < termIndex[b];
        });
    }
    
    // Print FIRST sets for nonterminals in overall order.
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


void Task4()
{
    // Build set of nonterminals (those that appear as LHS)
    unordered_set<string> ntSet;
    for (auto &rule : grammarRules)
        ntSet.insert(rule.lhs);

    // Compute overall nonterminal order by scanning all tokens in grammar.
    vector<string> overallNT;
    unordered_set<string> seen;
    for (auto &rule : grammarRules) {
        if (ntSet.find(rule.lhs) != ntSet.end() && seen.find(rule.lhs) == seen.end()){
            overallNT.push_back(rule.lhs);
            seen.insert(rule.lhs);
        }
        for (auto &sym : rule.rhs) {
            if (ntSet.find(sym) != ntSet.end() && seen.find(sym) == seen.end()){
                overallNT.push_back(sym);
                seen.insert(sym);
            }
        }
    }
    
    // Build termOrder by scanning RHS tokens (terminals are those not in ntSet)
    vector<string> termOrder;
    for (auto &rule : grammarRules) {
        for (auto &sym : rule.rhs) {
            if (ntSet.find(sym) == ntSet.end() && 
                find(termOrder.begin(), termOrder.end(), sym) == termOrder.end())
                termOrder.push_back(sym);
        }
    }
    
    // Compute Nullable set.
    unordered_set<string> nullable;
    bool change = true;
    while (change) {
        change = false;
        for (auto &rule : grammarRules) {
            if (nullable.find(rule.lhs) != nullable.end())
                continue;
            if (rule.rhs.empty()) {
                nullable.insert(rule.lhs);
                change = true;
            } else {
                bool allNull = true;
                for (auto &sym : rule.rhs) {
                    if (ntSet.find(sym) == ntSet.end() || nullable.find(sym) == nullable.end()){
                        allNull = false;
                        break;
                    }
                }
                if (allNull) {
                    nullable.insert(rule.lhs);
                    change = true;
                }
            }
        }
    }
    
    // Compute FIRST sets (like in Task3).
    unordered_map<string, vector<string>> first;
    // For terminals: FIRST(t) = { t }.
    for (auto &t : termOrder)
        first[t] = { t };
    for (auto &nt : overallNT)
        first[nt] = vector<string>();
    
    bool updated = true;
    while (updated) {
        updated = false;
        for (auto &rule : grammarRules) {
            vector<string> prodFirst;
            for (auto &sym : rule.rhs) {
                for (auto &x : first[sym])
                    if (find(prodFirst.begin(), prodFirst.end(), x) == prodFirst.end())
                        prodFirst.push_back(x);
                if (ntSet.find(sym) != ntSet.end()) {
                    if (nullable.find(sym) == nullable.end())
                        break;
                } else {
                    break;
                }
            }
            for (auto &x : prodFirst) {
                if (find(first[rule.lhs].begin(), first[rule.lhs].end(), x) == first[rule.lhs].end()) {
                    first[rule.lhs].push_back(x);
                    updated = true;
                }
            }
        }
    }
    
    // Initialize FOLLOW sets for each nonterminal.
    unordered_map<string, vector<string>> follow;
    for (auto &nt : overallNT)
        follow[nt] = vector<string>();
    // Start symbol gets "$".
    if (!overallNT.empty())
        follow[overallNT[0]].push_back("$");
    
    updated = true;
    while (updated) {
        updated = false;
        // For each production X -> Y1 Y2 ... Yn.
        for (auto &rule : grammarRules) {
            string X = rule.lhs;
            for (int i = 0; i < rule.rhs.size(); i++) {
                string Y = rule.rhs[i];
                if (ntSet.find(Y) == ntSet.end())
                    continue; // Only process nonterminals.
                // Compute FIRST(beta) for beta = Y(i+1)... end.
                vector<string> firstBeta;
                bool betaNullable = true;
                for (int j = i+1; j < rule.rhs.size(); j++) {
                    string sym = rule.rhs[j];
                    // Add FIRST(sym) (without duplicates).
                    for (auto &t : first[sym])
                        if (find(firstBeta.begin(), firstBeta.end(), t) == firstBeta.end())
                            firstBeta.push_back(t);
                    if (ntSet.find(sym) != ntSet.end()) {
                        if (nullable.find(sym) == nullable.end()){
                            betaNullable = false;
                            break;
                        }
                    } else {
                        betaNullable = false;
                        break;
                    }
                }
                // Add FIRST(beta) to FOLLOW(Y) (excluding epsilon, which isn't explicit here).
                for (auto &t : firstBeta) {
                    if (find(follow[Y].begin(), follow[Y].end(), t) == follow[Y].end()){
                        follow[Y].push_back(t);
                        updated = true;
                    }
                }
                // If beta is empty or beta is nullable, add FOLLOW(X) to FOLLOW(Y).
                if (i == rule.rhs.size()-1 || betaNullable) {
                    for (auto &t : follow[X]) {
                        if (find(follow[Y].begin(), follow[Y].end(), t) == follow[Y].end()){
                            follow[Y].push_back(t);
                            updated = true;
                        }
                    }
                }
            }
        }
    }
    
    // Build a mapping for ordering: "$" comes first, then terminals in termOrder.
    unordered_map<string, int> orderMap;
    orderMap["$"] = -1;
    for (int i = 0; i < termOrder.size(); i++)
        orderMap[termOrder[i]] = i;
    
    // Sort each FOLLOW set using orderMap.
    for (auto &nt : overallNT) {
        sort(follow[nt].begin(), follow[nt].end(), [&](const string &a, const string &b){
            return orderMap[a] < orderMap[b];
        });
    }
    
    // Print FOLLOW sets for nonterminals in overall order.
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


void Task6() {
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
