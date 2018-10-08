//===----------------------------------------------------------------------===//
// Golang specification can be found here: https://golang.org/ref/spec
//
// In development, I consider raise runtime_error  since it's helpful to locate 
// where error occurred and do further debugging. 
//
// Written by racaljk@github<1948638989@qq.com>
//===----------------------------------------------------------------------===//
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <exception>
#include <fstream>
#include <functional>
#include <string>
#include <vector>
#include <tuple>
#include <map>
using namespace std;

//===----------------------------------------------------------------------===//
// global data
//===----------------------------------------------------------------------===//

static int line = 1, column = 1, lastToken = -1;
static string package;
static map<string, string> imports;

static struct goruntime{
} grt;


//===----------------------------------------------------------------------===//
// various declarations 
//===----------------------------------------------------------------------===//


string keywords[] = { "break",    "default",     "func",   "interface", "select",
                     "case",     "defer",       "go",     "map",       "struct",
                     "chan",     "else",        "goto",   "package",   "switch",
                     "const",    "fallthrough", "if",     "range",     "type",
                     "continue", "for",         "import", "return",    "var" };

enum Token : signed int{
    KW_break, KW_default, KW_func, KW_interface, KW_select, KW_case, KW_defer, KW_go, KW_map, KW_struct, KW_chan, KW_else, KW_goto, KW_package, KW_switch,
    KW_const, KW_fallthrough, KW_if, KW_range, KW_type, KW_continue, KW_for, KW_import, KW_return, KW_var, OP_ADD, OP_BITAND, OP_ADDASSIGN, OP_BITANDASSIGN,
    OP_AND, OP_EQ, OP_NE, OP_LPAREN, OP_RPAREN, OP_SUB, OP_BITOR, OP_SUBASSIGN, OP_BITORASSIGN, OP_OR, OP_LT, OP_LE, OP_LBRACKET, OP_RBRACKET, OP_MUL, OP_XOR,
    OP_MULASSIGN, OP_BITXORASSIGN, OP_CHAN, OP_GT, OP_GE, OP_LBRACE, OP_RBRACE, OP_DIV, OP_LSHIFT, OP_DIVASSIGN, OP_LSFTASSIGN, OP_INC, OP_ASSIGN,
    OP_SHORTASSIGN, OP_COMMA, OP_SEMI, OP_MOD, OP_RSHIFT, OP_MODASSIGN, OP_RSFTASSIGN, OP_DEC, OP_NOT, OP_VARIADIC, OP_DOT, OP_COLON, OP_ANDXOR,
    OP_ANDXORASSIGN, TK_ID, LITERAL_INT, LITERAL_FLOAT, LITERAL_IMG, LITERAL_RUNE, LITERAL_STR, TK_EOF
};

//===----------------------------------------------------------------------===//
// Give me 5, I'll give you a minimal but complete golang impl back.
//===----------------------------------------------------------------------===//

tuple<Token, string> next(fstream& f) {
     auto consumePeek = [&](char& c) {
            f.get();
            column++;
            char oc = c;
            c = f.peek();
            return oc;
        };
     char c = f.peek();

skip_comment_and_find_next:

    for (; c == ' ' || c == '\r' || c == '\t' || c == '\n'; column++) {
        if (c == '\n') {
            line++;
            column = 1;
            if ((lastToken >= TK_ID && lastToken <= LITERAL_STR)
                || lastToken == KW_break || lastToken == KW_continue || lastToken == KW_fallthrough || lastToken == KW_return ||
                lastToken == OP_INC || lastToken == OP_DEC || lastToken == OP_RPAREN || lastToken == OP_RBRACKET || lastToken == OP_RBRACE) {
                consumePeek(c);
                lastToken = OP_SEMI;
                return make_tuple(OP_SEMI, ";");
            }
        }
        consumePeek(c);
    }
    if (f.eof()) {
        lastToken = TK_EOF;
        return make_tuple(TK_EOF, "");
    }

    string lexeme;
   

    // identifier = letter { letter | unicode_digit } .
    if (isalpha(c) || c == '_') {
        while (isalnum(c) || c == '_') {
            lexeme += consumePeek(c);
        }

        for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
            if (keywords[i] == lexeme) {
                lastToken = static_cast<Token>(i);
                return make_tuple(static_cast<Token>(i), lexeme);
            }
        lastToken = TK_ID;
        return make_tuple(TK_ID, lexeme);
    }

    // int_lit     = decimal_lit | octal_lit | hex_lit .
    // decimal_lit = ( "1" … "9" ) { decimal_digit } .
    // octal_lit   = "0" { octal_digit } .
    // hex_lit     = "0" ( "x" | "X" ) hex_digit { hex_digit } .

    // float_lit = decimals "." [ decimals ] [ exponent ] |
    //         decimals exponent |
    //         "." decimals [ exponent ] .
    // decimals  = decimal_digit { decimal_digit } .
    // exponent  = ( "e" | "E" ) [ "+" | "-" ] decimals .

    // imaginary_lit = (decimals | float_lit) "i" .
    if (isdigit(c) || c == '.') {
        if (c == '0') {
            lexeme += consumePeek(c);
            if (c == 'x' || c == 'X') {
                do {
                    lexeme += consumePeek(c);
                } while (isdigit(c) || c >= 'a' && c <= 'f' || c >= 'A' && c <= 'F');
                lastToken = LITERAL_INT;
                return make_tuple(LITERAL_INT, lexeme);
            }
            else if ((c >= '0' && c <= '9') ||
                    (c == '.' || c == 'e' || c == 'E' || c == 'i')) {
                while ((c >= '0' && c <= '9') ||
                    (c == '.' || c == 'e' || c == 'E' || c == 'i')) {
                    if (c >= '0' && c <= '7') {
                        lexeme += consumePeek(c);
                    }
                    else {
                        goto shall_float;
                    }
                }
                lastToken = LITERAL_INT;
                return make_tuple(LITERAL_INT, lexeme);
            }
            goto may_float;
        }
        else {  // 1-9 or . or just a single 0
        may_float:
            Token type = LITERAL_INT;
            if (c == '.') {
                lexeme += consumePeek(c);
                if (c == '.') {
                    lexeme += consumePeek(c);
                    if (c == '.') {
                        lexeme += consumePeek(c);
                        lastToken = OP_VARIADIC;
                        return make_tuple(OP_VARIADIC, lexeme);
                    }
                    else {
                        throw runtime_error(
                            "expect variadic notation(...) but got .." + c);
                    }
                }
                type = LITERAL_FLOAT;
                goto shall_float;
            }
            else if (c >= '1'&&c <= '9') {
                lexeme += consumePeek(c);
            shall_float:  // skip char consuming and appending since we did that before jumping here;
                bool hasDot = false, hasExponent = false;
                while ((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' ||
                    c == 'i') {
                    if (c >= '0' && c <= '9') {
                        lexeme += consumePeek(c);
                    }
                    else if (c == '.' && !hasDot) {
                        lexeme += consumePeek(c);
                        type = LITERAL_FLOAT;
                    }
                    else if ((c == 'e' && !hasExponent) ||
                        (c == 'E' && !hasExponent)) {
                        hasExponent = true;
                        type = LITERAL_FLOAT;
                        lexeme += consumePeek(c);
                        if (c == '+' || c == '-') {
                            lexeme += consumePeek(c);
                        }
                    }
                    else {
                        f.get();
                        column++;
                        lexeme += c;
                        lastToken = LITERAL_IMG;
                        return make_tuple(LITERAL_IMG, lexeme);
                    }
                }
                lastToken = type;
                return make_tuple(type, lexeme);
            }
            else {
                lastToken = type;
                return make_tuple(type, lexeme);
            }
        }
    }

    //! NOT FULLY SUPPORT UNICODE RELATED LITERALS

    // rune_lit         = "'" ( unicode_value | byte_value ) "'" .
    // unicode_value    = unicode_char | little_u_value | big_u_value |
    // escaped_char . byte_value       = octal_byte_value | hex_byte_value .
    // octal_byte_value = `\` octal_digit octal_digit octal_digit .
    // hex_byte_value   = `\` "x" hex_digit hex_digit .
    // little_u_value   = `\` "u" hex_digit hex_digit hex_digit hex_digit .
    // big_u_value      = `\` "U" hex_digit hex_digit hex_digit hex_digit
    //                            hex_digit hex_digit hex_digit hex_digit .
    // escaped_char     = `\` ( "a" | "b" | "f" | "n" | "r" | "t" | "v" | `\` |
    // "'" | `"` ) .
    if (c == '\'') {
        lexeme += consumePeek(c);
        if (c == '\\') {
            lexeme += consumePeek(c);

            if (c == 'U' || c == 'u' || c == 'x' || c == 'X') {
                do {
                    lexeme += consumePeek(c);
                } while (isdigit(c) || (c >= 'a' && c <= 'f') ||
                    (c >= 'A' && c <= 'F'));
            }
            else if (c >= '0' && c <= '7') {
                do {
                    lexeme += consumePeek(c);
                } while (c >= '0' && c <= '7');
            }
            else if (c == 'a' || c == 'b' || c == 'f' || c == 'n' || c == 'r' || c == 't' || c == 'v' || c == '\\' || c == '\'' || c == '"') {
                lexeme += consumePeek(c);
            }
            else {
                throw runtime_error("illegal rune");
            }

        }
        else {
            lexeme += consumePeek(c);
        }

        if (c != '\'') {
            throw runtime_error(
                "illegal rune at least in current implementation of g8");
        }
        lexeme += consumePeek(c);
        lastToken = LITERAL_RUNE;
        return make_tuple(LITERAL_RUNE, lexeme);
    }

    // string_lit             = raw_string_lit | interpreted_string_lit .
    // raw_string_lit         = "`" { unicode_char | newline } "`" .
    // interpreted_string_lit = `"` { unicode_value | byte_value } `"` .
    if (c == '`') {
        do {
            lexeme += consumePeek(c);
            if (c == '\n') line++;
        } while (f.good() && c != '`');
        if (c != '`') {
            throw runtime_error(
                "raw string literal does not have a closed symbol \"`\"");
        }
        lexeme += consumePeek(c);
        lastToken = LITERAL_STR;
        return make_tuple(LITERAL_STR, lexeme);
    }
    else if (c == '"') {
        do {
            lexeme += consumePeek(c);
            if (c == '\\') {
                lexeme += consumePeek(c);
                lexeme += consumePeek(c);
            }
        } while (f.good() && (c != '\n' && c != '\r' && c != '"'));
        if (c != '"') {
            throw runtime_error(
                "string literal does not have a closed symbol \"\"\"");
        }
        lexeme += consumePeek(c);
        lastToken = LITERAL_STR;
        return make_tuple(LITERAL_STR, lexeme);
    }

    // operators
    switch (c) {
    case '+':  //+  += ++
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_ADDASSIGN;
            return make_tuple(OP_ADDASSIGN, lexeme);
        }
        else if (c == '+') {
            lexeme += consumePeek(c);
            lastToken = OP_INC;
            return make_tuple(OP_INC, lexeme);
        }
        return make_tuple(OP_ADD, lexeme);
    case '&':  //&  &=  &&  &^  &^=
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_BITANDASSIGN;
            return make_tuple(OP_BITANDASSIGN, lexeme);
        }
        else if (c == '&') {
            lexeme += consumePeek(c);
            lastToken = OP_AND;
            return make_tuple(OP_AND, lexeme);
        }
        else if (c == '^') {
            lexeme += consumePeek(c);
            if (c == '=') {
                lexeme += consumePeek(c);
                lastToken = OP_ANDXORASSIGN;
                return make_tuple(OP_ANDXORASSIGN, lexeme);
            }
            lastToken = OP_ANDXOR;
            return make_tuple(OP_ANDXOR, lexeme);
        }
        lastToken = OP_BITAND;
        return make_tuple(OP_BITAND, lexeme);
    case '=':  //=  ==
        lexeme += consumePeek(c);
        if (c == '=') {
            lastToken = OP_EQ;
            return make_tuple(OP_EQ, lexeme);
        }
        lastToken = OP_ASSIGN;
        return make_tuple(OP_ASSIGN, lexeme);
    case '!':  //!  !=
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_NE;
            return make_tuple(OP_NE, lexeme);
        }
        lastToken = OP_NOT;
        return make_tuple(OP_NOT, lexeme);
    case '(':
        lexeme += consumePeek(c);
        lastToken = OP_LPAREN;
        return make_tuple(OP_LPAREN, lexeme);
    case ')':
        lexeme += consumePeek(c);
        lastToken = OP_RPAREN;
        return make_tuple(OP_RPAREN, lexeme);
    case '-':  //-  -= --
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_SUBASSIGN;
            return make_tuple(OP_SUBASSIGN, lexeme);
        }
        else if (c == '-') {
            lexeme += consumePeek(c);
            lastToken = OP_DEC;
            return make_tuple(OP_DEC, lexeme);
        }
        lastToken = OP_SUB;
        return make_tuple(OP_SUB, lexeme);
    case '|':  //|  |=  ||
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_BITORASSIGN;
            return make_tuple(OP_BITORASSIGN, lexeme);
        }
        else if (c == '|') {
            lexeme += consumePeek(c);
            lastToken = OP_OR;
            return make_tuple(OP_OR, lexeme);
        }
        lastToken = OP_BITOR;
        return make_tuple(OP_BITOR, lexeme);
    case '<':  //<  <=  <- <<  <<=
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_LE;
            return make_tuple(OP_LE, lexeme);
        }
        else if (c == '-') {
            lexeme += consumePeek(c);
            lastToken = OP_CHAN;
            return make_tuple(OP_CHAN, lexeme);
        }
        else if (c == '<') {
            lexeme += consumePeek(c);
            if (c == '=') {
                lastToken = OP_LSFTASSIGN;
                return make_tuple(OP_LSFTASSIGN, lexeme);
            }
            lastToken = OP_LSHIFT;
            return make_tuple(OP_LSHIFT, lexeme);
        }
        lastToken = OP_LT;
        return make_tuple(OP_LT, lexeme);
    case '[':
        lexeme += consumePeek(c);
        lastToken = OP_LBRACKET;
        return make_tuple(OP_LBRACKET, lexeme);
    case ']':
        lexeme += consumePeek(c);
        lastToken = OP_RBRACKET;
        return make_tuple(OP_RBRACKET, lexeme);
    case '*':  //*  *=
        lexeme += consumePeek(c);
        if (c == '=') {
            lastToken = OP_MULASSIGN;
            return make_tuple(OP_MULASSIGN, lexeme);
        }
        lastToken = OP_MUL;
        return make_tuple(OP_MUL, lexeme);
    case '^':  //^  ^=
        lexeme += consumePeek(c);
        if (c == '=') {
            lastToken = OP_BITXORASSIGN;
            return make_tuple(OP_BITXORASSIGN, lexeme);
        }
        lastToken = OP_XOR;
        return make_tuple(OP_XOR, lexeme);
    case '>':  //>  >=  >>  >>=
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_GE;
            return make_tuple(OP_GE, lexeme);
        }
        else if (c == '>') {
            lexeme += consumePeek(c);
            if (c == '=') {
                lastToken = OP_RSFTASSIGN;
                return make_tuple(OP_RSFTASSIGN, lexeme);
            }
            lastToken = OP_RSHIFT;
            return make_tuple(OP_RSHIFT, lexeme);
        }
        lastToken = OP_GT;
        return make_tuple(OP_GT, lexeme);
    case '{':
        lexeme += consumePeek(c);
        lastToken = OP_LBRACE;
        return make_tuple(OP_LBRACE, lexeme);
    case '}':
        lexeme += consumePeek(c);
        lastToken = OP_RBRACE;
        return make_tuple(OP_RBRACE, lexeme);
    case '/': {  // /  /= // /*...*/
        char pending = consumePeek(c);
        if (c == '=') {
            lexeme += pending;
            lexeme += consumePeek(c);
            lastToken = OP_DIVASSIGN;
            return make_tuple(OP_DIVASSIGN, lexeme);
        }
        else if (c == '/') {
            do {
                consumePeek(c);
            } while (f.good() && (c != '\n' && c != '\r'));
            goto skip_comment_and_find_next;
        }
        else if (c == '*') {
            do {
                consumePeek(c);
                if (c == '\n') line++;
                if (c == '*') {
                    consumePeek(c);
                    if (c == '/') {
                        consumePeek(c);
                        goto skip_comment_and_find_next;
                    }
                }
            } while (f.good());
        }
        lexeme += pending;
        lastToken = OP_DIV;
        return make_tuple(OP_DIV, lexeme);
    }
    case ':':  // :=
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_SHORTASSIGN;
            return make_tuple(OP_SHORTASSIGN, lexeme);
        }
        lastToken = OP_COLON;
        return make_tuple(OP_COLON, lexeme);
    case ',':
        lexeme += consumePeek(c);
        lastToken = OP_COMMA;
        return make_tuple(OP_COMMA, lexeme);
    case ';':
        lexeme += consumePeek(c);
        lastToken = OP_SEMI;
        return make_tuple(OP_SEMI, lexeme);
    case '%':  //%  %=
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_MODASSIGN;
            return make_tuple(OP_MODASSIGN, lexeme);
        }
        lastToken = OP_MOD;
        return make_tuple(OP_MOD, lexeme);
        // case '.' has already checked
    }

    throw runtime_error("illegal token in source file");
}

void parse(const string & filename) {
    fstream f(filename, ios::binary | ios::in); 

    auto expect = [&f](Token tk,const string& msg){
        auto t = next(f);
        if (get<0>(t) != tk) throw runtime_error(msg);

        struct _Anony{
            Token token;
            string lexeme;
            _Anony(tuple<Token, string> & t) :token(get<0>(t)), lexeme(get<1>(t)) {}
        };
        return _Anony(t);
    };
    auto decompose = [](tuple<Token,string> & t) {
        struct _Anony {
            Token token;
            string lexeme;
            _Anony(tuple<Token, string> & t) :token(get<0>(t)), lexeme(get<1>(t)) {}
        };
        return _Anony(t);
    };


    // SourceFile = PackageClause ";" { ImportDecl ";" } { TopLevelDecl ";" } .
    // PackageClause  = "package" PackageName .
    // PackageName = identifier .
    expect(KW_package,"a go source file should always start with \"package\" besides meanlessing chars");
    package = expect(TK_ID,"expect an identifier after package keyword").lexeme;
    expect(OP_SEMI,"expect a semicolon");

    // ImportDecl       = "import" ( ImportSpec | "(" { ImportSpec ";" } ")" ) .
    // ImportSpec       = [ "." | PackageName ] ImportPath .
    // ImportPath       = string_lit .
    auto[token, lexeme] = next(f);
    while (token == KW_import) {
        auto t = decompose(next(f));
        if (t.token == OP_LPAREN) {
            t = decompose(next(f));
            do {
                string importName,alias;
                if (t.token == OP_DOT || t.token == TK_ID) {
                    alias = t.lexeme;
                }
                importName = expect(LITERAL_STR, "import path should not empty").lexeme;
                expect(OP_COLON, "expect an explicit semicolon after import declaration").lexeme;
                imports[importName] = alias;
                t = decompose(next(f));
            } while (t.token != OP_RPAREN);
        }

        expect(OP_SEMI, "expect an explicit semicolon");
    }
    
}



void emitStub() {}
void runtimeStub() {}

//===----------------------------------------------------------------------===//
// debug auxiliary functions, they are not part of 5 functions
//===----------------------------------------------------------------------===//
void printLex(const string & filename) {
    fstream f(filename, ios::binary | ios::in);
    while (f.good()) {
        auto[token, lexeme] = next(f);
        fprintf(stdout, "<%d,%s,%d,%d>\n", token, lexeme.c_str(), line, column);
    }
}

int main() {
    const string filename = "b.go";
    //printLex(filename);
    parse(filename);
    getchar();
    return 0;
}