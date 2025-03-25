#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include "lexer.h"
#include <unordered_set>

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
    // Build set of nonterminals from LHS of rules.
    unordered_set<string> ntSet;
    for (auto &rule : grammarRules)
        ntSet.insert(rule.lhs);

    // Build overall order of nonterminals by scanning rules in input order.
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

    // Compute Nullable set iteratively.
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

void Task3() {
}

void Task4() {
}

void Task5() {
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
