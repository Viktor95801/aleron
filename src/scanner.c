#include "aleron.h"

#include <ctype.h>
#include <string.h>

#include "crc.h"
#include "util.h"
#include "vendor/stb_ds.h"

#define _isalpha(c) (isalpha(c) || (c) == '_')
#define _isalnum(c) (isalnum(c) || (c) == '_')

const char *token_to_str(Token *token)
{
        switch (token->kind) {
        case TK_INVALID:
                return format("invalid:%.*s", Mtokstr_fmt(*token));
        case TK_EOF:
                return "eof";
        case TK_ID:
                return format("ident:%.*s", Mtokstr_fmt(*token));
        case TK_INT:
                return format("int:%.*s", Mtokstr_fmt(*token));

        case TK_ADD:
                return "+";
        case TK_SUB:
                return "-";
        case TK_MUL:
                return "*";
        case TK_DIV:
                return "/";
        case TK_ASS:
                return "=";
        case TK_AMP:
                return "&";

        case TK_OPAREN:
                return "(";
        case TK_CPAREN:
                return ")";
        case TK_OCURLY:
                return "{";
        case TK_CCURLY:
                return "}";

        case TK_SEMI:
                return ";";

        case KW_RETURN:
                return "kw:return";
        case KW_IF:
                return "kw:if";
        case KW_ELSE:
                return "kw:else";
        case KW_FOR:
                return "kw:for";
        }
}

// here for tracing
char *scanner_next_char(char **src)
{
        // printf("*src = %s\n", *src);
        return (*src)++;
}

KeywordHT keyword_ht = NULL;
static bool init_scanner_mod = false;

ScanResult next_token(char **src)
{
        if (!init_scanner_mod) {
                TokenKind t = 0;
                // here to get compiler warnings whenever i forget to update the
                // keyword hashtable
                switch (t) {
                case TK_ADD:
                case TK_EOF:
                case TK_SEMI:
                case TK_SUB:
                case TK_ASS:
                case TK_CPAREN:
                case TK_DIV:
                case TK_ID:
                case TK_INT:
                case TK_INVALID:
                case TK_MUL:
                case TK_OPAREN:
                case TK_CCURLY:
                case TK_AMP:
                case TK_OCURLY:

                case KW_RETURN:
                case KW_IF:
                case KW_ELSE:
                case KW_FOR:
                }

                shdefault(keyword_ht, TK_EOF);

                shput(keyword_ht, "return", KW_RETURN);
                shput(keyword_ht, "if", KW_IF);
                shput(keyword_ht, "else", KW_ELSE);
                shput(keyword_ht, "for", KW_FOR);
                init_scanner_mod = true;
        }
        while (isspace(**src)) {
                scanner_next_char(src);
        }

        if (isdigit(**src)) {
                char *start = *src;
                bool invalid_number = false;
                while (_isalnum(**src)) {
                        if (_isalpha(**src)) {
                                invalid_number = true;
                        }
                        scanner_next_char(src);
                }
                auto result = (ScanResult){
                        .token = { TK_INT, { *src - start, start } }
                };
                if (invalid_number) {
                        result.token.kind = TK_INVALID;
                        result.error = strdup(
                                format("number contains letters: " SV_Fmt "\n",
                                       Mtokstr_fmt(result.token)));
                }
                return result;
        }

        if (_isalpha(**src)) {
                char *start = *src;
                while (_isalnum(**src)) {
                        scanner_next_char(src);
                }

                auto result = (ScanResult){
                        .token = { TK_ID, { *src - start, start } }
                };
                const char *null_terminated =
                        format(SV_Fmt, Mtokstr_fmt(result.token));
                TokenKind kind = shget(keyword_ht, null_terminated);
                // if its a keyword
                if (kind != TK_EOF) {
                        result.token.kind = kind;
                }

                return result;
        }

#define singleCharCase(kind)                                     \
                                                                 \
        return (ScanResult)                                      \
        {                                                        \
                .token = { kind, { 1, scanner_next_char(src) } } \
        }

        switch (**src) {
        case '+':
                singleCharCase(TK_ADD);
        case '-':
                singleCharCase(TK_SUB);
        case '*':
                singleCharCase(TK_MUL);
        case '/':
                singleCharCase(TK_DIV);
        case '=':
                singleCharCase(TK_ASS);
        case '&':
                singleCharCase(TK_AMP);

        case '(':
                singleCharCase(TK_OPAREN);
        case ')':
                singleCharCase(TK_CPAREN);
        case '{':
                singleCharCase(TK_OCURLY);
        case '}':
                singleCharCase(TK_CCURLY);

        case ';':
                singleCharCase(TK_SEMI);

        // we want it to stay here forever
        case '\0': {
                return (ScanResult){ .token = { TK_EOF, { 1, *src } } };
        }
        default: {
                auto result = (ScanResult){
                        .token = { TK_INVALID, { 1, *src } },
                        .error = strdup(format("stray '%c' in source\n", **src))
                };
                scanner_next_char(src);
                return result;
        }
        }
#undef singleCharCase
}
