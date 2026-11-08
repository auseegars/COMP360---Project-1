/*****************************************************************************
 * Course:      COMP 360-01  Programming Languages   Fall 2026
 * Project:     Project 1 - Lexical Analyzer and Recursive-Descent Parser
 * Team:        Ayana Seegars, Jevaun Keith
 * Due Date:    October 3, 2026
*****************************************************************************/

#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

/*  Token types  */

enum TokenType {
    KEYWORD,        // float
    IDENT,          // lowercase letters only
    LEFT_PAREN,     // (
    RIGHT_PAREN,    // )
    LEFT_BRACE,     // {
    RIGHT_BRACE,    // }
    SEMICOLON,      // ;
    ASSIGN_OP,      // =
    MULT_OP,        // *
    DIV_OP,         // /
    INVALID,        // anything the grammar does not allow (e.g. "test2")
    END_OF_FILE     // end of input
};

/* Returns a printable name for a token type. */
string tokenName(TokenType type) {
    switch (type) {
        case KEYWORD:     return "KEYWORD";
        case IDENT:       return "IDENT";
        case LEFT_PAREN:  return "LEFT_PAREN";
        case RIGHT_PAREN: return "RIGHT_PAREN";
        case LEFT_BRACE:  return "LEFT_BRACE";
        case RIGHT_BRACE: return "RIGHT_BRACE";
        case SEMICOLON:   return "SEMICOLON";
        case ASSIGN_OP:   return "ASSIGN_OP";
        case MULT_OP:     return "MULT_OP";
        case DIV_OP:      return "DIV_OP";
        case INVALID:     return "INVALID";
        case END_OF_FILE: return "END_OF_FILE";
    }
    return "UNKNOWN";
}

/* One lexeme together with its token type and the line it appeared on. */
struct Token {
    TokenType type;
    string    lexeme;
    int       line;
};

/*  Lexical Analyzer */
class Lexer {
public:
    explicit Lexer(const string& source) : src(source), pos(0), line(1) {}

    /* Breaks the whole input into a list of tokens, ending with END_OF_FILE. */
    vector<Token> tokenize() {
        vector<Token> tokens;
        Token tok;
        do {
            tok = nextToken();
            tokens.push_back(tok);
        } while (tok.type != END_OF_FILE);
        return tokens;
    }

private:
    string src;   // entire contents of the input file
    size_t pos;   // index of the next character to read
    int    line;  // current line number (for error messages)

    /* Skips spaces, tabs and newlines, counting lines as it goes. */
    void skipWhitespace() {
        while (pos < src.size() && isspace(static_cast<unsigned char>(src[pos]))) {
            if (src[pos] == '\n') line++;
            pos++;
        }
    }

    /* Returns true if every character is a lowercase letter a-z. */
    static bool allLowercase(const string& s) {
        for (char c : s)
            if (c < 'a' || c > 'z') return false;
        return true;
    }

    /* Reads and returns the next token from the input. */
    Token nextToken() {
        skipWhitespace();

        if (pos >= src.size())
            return {END_OF_FILE, "EOF", line};

        char c = src[pos];

        /* Words: read the full run of letters and digits, then decide.   */
        /* Keep names such as "test2" together so the error points to the whole name. */
        if (isalnum(static_cast<unsigned char>(c)) || c == '_') {
            string word;
            while (pos < src.size() &&
                   (isalnum(static_cast<unsigned char>(src[pos])) || src[pos] == '_')) {
                word += src[pos++];
            }
            if (word == "float")      return {KEYWORD, word, line};
            if (allLowercase(word))   return {IDENT,   word, line};
            return {INVALID, word, line};   // digits, uppercase, or '_'
        }

        /* Single-character symbols. */
        pos++;
        switch (c) {
            case '(': return {LEFT_PAREN,  "(", line};
            case ')': return {RIGHT_PAREN, ")", line};
            case '{': return {LEFT_BRACE,  "{", line};
            case '}': return {RIGHT_BRACE, "}", line};
            case ';': return {SEMICOLON,   ";", line};
            case '=': return {ASSIGN_OP,   "=", line};
            case '*': return {MULT_OP,     "*", line};
            case '/': return {DIV_OP,      "/", line};
            default:  return {INVALID, string(1, c), line};
        }
    }
};

/* Each grammar rule has its own parsing function. Errors are kept in order. */
class Parser {
public:
    explicit Parser(const vector<Token>& input) : tokens(input), current(0) {}

    bool parse() {
        program();
        while (peek().type != END_OF_FILE) {
            report("end of program");
            advance();
        }
        return errors.empty();
    }

    const vector<string>& diagnostics() const { return errors; }

private:
    const vector<Token>& tokens;
    size_t current;
    vector<string> errors;

    const Token& peek() const { return tokens[current]; }

    void advance() {
        if (peek().type != END_OF_FILE) ++current;
    }

    void report(const string& expected) {
        const Token& token = peek();
        ostringstream message;
        message << "Syntax error on line " << token.line << ": expected "
                << expected << ", but found '" << token.lexeme << "' ("
                << tokenName(token.type) << ")";
        if (token.type == INVALID)
            message << "\n  Names must contain lowercase letters a-z only. "
                       "Supported symbols: ( ) { } ; = * /.";
        errors.push_back(message.str());
    }

    // Keep a likely next token when punctuation is missing. Consume invalid
    // lexemes so a bad name does not hide the rest of the program.
    void expect(TokenType type, const string& description) {
        if (peek().type == type) {
            advance();
        } else {
            report(description);
            if (peek().type == INVALID) advance();
        }
    }

    void program() {
        expect(KEYWORD, "keyword 'float'");
        expect(IDENT, "an identifier (program name)");
        expect(LEFT_PAREN, "'('");
        expect(KEYWORD, "keyword 'float'");
        expect(IDENT, "an identifier (parameter name)");
        expect(RIGHT_PAREN, "')'");
        expect(LEFT_BRACE, "'{'");
        declares();
        assign();
        // The supplied valid sample includes this optional semicolon.
        if (peek().type == SEMICOLON) advance();
        expect(RIGHT_BRACE, "'}'");
    }

    // Repetition implements the recursive declaration rule without growing
    // the call stack for each declaration.
    void declares() {
        if (peek().type != KEYWORD)
            report("at least one declaration beginning with 'float'");
        while (peek().type == KEYWORD) {
            advance();
            expect(IDENT, "an identifier in the declaration");
            if (peek().type == SEMICOLON) {
                advance();
            } else {
                report("';' at the end of the declaration");
                // A keyword starts the next declaration; a name may start
                // the assignment. Do not discard either during recovery.
                while (peek().type != KEYWORD && peek().type != IDENT &&
                       peek().type != RIGHT_BRACE &&
                       peek().type != END_OF_FILE && peek().type != SEMICOLON)
                    advance();
                if (peek().type == SEMICOLON) advance();
            }
        }
    }

    void assign() {
        expect(IDENT, "an identifier at the start of the assignment");
        expect(ASSIGN_OP, "'='");
        expr();
    }

    void expr() {
        expect(IDENT, "an identifier in the expression");
        while (peek().type != SEMICOLON && peek().type != RIGHT_BRACE &&
               peek().type != END_OF_FILE) {
            if (peek().type == MULT_OP || peek().type == DIV_OP) {
                advance();
                expect(IDENT, "an identifier after '*' or '/'");
            } else {
                report("'*', '/', or the end of the expression");
                advance();
            }
        }
    }
};

/*  Main */

/* Reads the entire file into a string. Returns false if it cannot be opened. */
bool readFile(const string& fileName, string& contents) {
    ifstream in(fileName);
    if (!in) return false;
    stringstream buffer;
    buffer << in.rdbuf();
    contents = buffer.str();
    return !in.bad();
}

int main(int argc, char* argv[]) {
    /* Input the Sample Program (one per run). */
    string fileName;
    if (argc > 1) {
        fileName = argv[1];
    } else {
        cout << "Enter the Sample Program file name: ";
        if (!getline(cin, fileName) || fileName.empty()) {
            cerr << "Error: enter an input file name.\n";
            return 1;
        }
    }

    string source;
    if (!readFile(fileName, source)) {
        cerr << "Error: cannot open file '" << fileName << "'\n";
        return 1;
    }

    cout << "Sample Program (" << fileName << "):\n"
         << "----------------------------------------\n"
         << source
         << "\n----------------------------------------\n\n";

    /* Lexical analysis: generate and display lexemes and tokens. */
    Lexer lexer(source);
    vector<Token> tokens = lexer.tokenize();

    int lexemeWidth = 12;
    for (const Token& token : tokens)
        if (token.lexeme.size() + 2 > static_cast<size_t>(lexemeWidth))
            lexemeWidth = static_cast<int>(token.lexeme.size() + 2);

    cout << "Lexical Analysis\n";
    cout << left << setw(6) << "Line" << setw(lexemeWidth) << "Lexeme" << "Token\n";
    cout << "----------------------------------------\n";
    for (const Token& t : tokens)
        cout << left << setw(6) << t.line << setw(lexemeWidth) << t.lexeme
             << tokenName(t.type) << "\n";
    cout << "\n";

    /* Syntax analysis with the recursive-descent parser. */
    cout << "Syntax Analysis\n";
    cout << "----------------------------------------\n";
    Parser parser(tokens);
    if (parser.parse()) {
        cout << "The Sample Program is generated by the BNF grammar\n";
    } else {
        cout << "The Sample Program cannot be generated by the "
                "LearnCompiler BNF Grammar\n";
        const vector<string>& errors = parser.diagnostics();
        cout << "First syntax error:\n" << errors.front() << "\n";
        for (size_t i = 1; i < errors.size(); ++i)
            cout << "\nAdditional error " << i << ":\n" << errors[i] << "\n";
        return 1;
    }
    return 0;
}


