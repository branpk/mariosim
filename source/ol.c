#include "ol.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>


static void error(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  fprintf(stderr, "\x1b[91mError:\x1b[0m ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  
  va_end(args);
  exit(1);
}

static void *reallocate(void *p, size_t size) {
  p = realloc(p, size);
  if (size > 0 && p == NULL) {
    error("Out of memory");
  }
  return p;
}

static void *allocate(size_t size) {
  return reallocate(NULL, size);
}


typedef struct {
  char *cstr;
  size_t cap;
  size_t len;
} string;

static string *str_new(void) {
  string *s = (string *) allocate(sizeof(string));
  s->cstr = (char *) allocate(1);
  s->cstr[0] = '\0';
  s->cap = 1;
  s->len = 0;
  return s;
}

static void str_delete(string *s) {
  free(s->cstr);
  free(s);
}

static char *str_release(string *s) {
  char *cstr = s->cstr;
  free(s);
  return cstr;
}

static void str_expand(string *s, size_t cap) {
  if (s->cap >= cap)
    return;
  
  s->cap = 3 * s->cap / 2;
  if (s->cap < cap)
    s->cap = cap;
  
  s->cstr = (char *) reallocate(s->cstr, s->cap);
}

static void str_append(string *s, char c) {
  str_expand(s, s->len + 2);
  s->len += 1;
  s->cstr[s->len - 1] = c;
  s->cstr[s->len] = '\0';
}


typedef struct {
  char *filename;
  size_t lineNum;
  size_t colNum;
} Location;


typedef enum {
  tk_val,
  tk_sym,
  tk_eof,
} TokenType;

typedef struct {
  Location loc;
  TokenType type;
  union {
    OlValue *val;
    string *sym;
  };
} Token;


typedef struct {
  FILE *file;
  Location loc;
  char **funcNames;

  int curChar;
  int nextChar;

  Token *curTok;
} ParseCxt;


static void parseError(Location *loc, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  fprintf(stderr, "\x1b[91mSyntax Error:\x1b[0m ");
  fprintf(stderr, "\x1b[1m%s(%d, %d):\x1b[0m ",
    loc->filename, (int) loc->lineNum, (int) loc->colNum);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  
  va_end(args);
  exit(1);
}


static int nextChar(ParseCxt *p) {
  if (p->curChar == '\n') {
    p->loc.lineNum += 1;
    p->loc.colNum = 1;
  }
  else {
    p->loc.colNum += 1;
  }

  p->curChar = p->nextChar;
  p->nextChar = fgetc(p->file);

  if ((p->curChar == '\r' && p->nextChar == '\n') ||
    (p->curChar == '\n' && p->nextChar == '\r'))
  {
    p->curChar = '\n';
    p->nextChar = fgetc(p->file);
  }
  else if (p->curChar == '\r') {
    p->curChar = '\n';
  }

  return p->curChar;
}


static char *showChar(int c) {
  static char buffer[2];

  switch (c) {
  case '\n': return "new line";
  case '\t': return "tab";
  case ' ': return "' '";
  case EOF:  return "end of file";
  }

  buffer[0] = c;
  buffer[1] = '\0';
  return &buffer[0];
}


static char *escapeChar(int c) {
  static char buffer[2];

  switch (c) {
  case '\n': return "\\n";
  case '\t': return "\\t";
  case '\'': return "\\'";
  }

  buffer[0] = c;
  buffer[1] = '\0';
  return &buffer[0];
}


static char *tokenStr(Token *t) {
  switch (t->type) {
  case tk_val: return ol_valueStr(t->val);
  case tk_sym: return showChar(t->sym->cstr[0]);
  case tk_eof: return "end of file";
  }
}


static Token *parseIdent(ParseCxt *p) {
  if (p->curTok != NULL)
    error("Internal error: free token before calling parseIdent");

  Token *t = (Token *) allocate(sizeof(Token));
  t->loc = p->loc;
  t->type = tk_val;

  t->val = (OlValue *) allocate(sizeof(OlValue));
  t->val->type = ol_ident;

  string *ident = str_new();
  int c = p->curChar;
  while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
    (c >= '0' && c <= '9') || c == '_' || c == '-')
  {
    if (c >= 'A' && c <= 'Z')
      c += 'a' - 'A';
    if (c != '_' && c != '-')
      str_append(ident, c);
    c = nextChar(p);
  }

  if (ident->len == 0)
    parseError(&t->loc, "Expected identifier, found %s", showChar(p->curChar));

  if (ident->cstr[0] >= '0' && ident->cstr[0] <= '9')
    parseError(&t->loc, "Identifier cannot start with digit");

  t->val->ident = str_release(ident);
  p->curTok = t;
  return t;
}


static uint64_t parseInt(char *s, int base) {
  uint64_t result = 0;

  while (*s != '\0') {
    result *= base;

    if (*s >= 'a' && *s <= 'f')
      result += *s - 'a' + 10;
    else if (*s >= 'A' && *s <= 'F')
      result += *s - 'A' + 10;
    else
      result += *s - '0';

    s++;
  }

  return result;
}


static Token *parseNumber(ParseCxt *p) {
  if (p->curTok != NULL)
    error("Internal error: free token before calling parseIdent");

  Token *t = (Token *) allocate(sizeof(Token));
  t->loc = p->loc;
  t->type = tk_val;

  int sign = 1;
  while (p->curChar == '+' || p->curChar == '-') {
    if (p->curChar == '-') sign *= -1;
    nextChar(p);
  }

  int base = 10;
  if (p->curChar == '0' && p->nextChar == 'x') {
    nextChar(p);
    nextChar(p);
    base = 16;
  }

  t->val = (OlValue *) allocate(sizeof(OlValue));
  t->val->type = base == 10 ? ol_dec : ol_hex;

  string *digits = str_new();
  char c = p->curChar;
  while ((c >= '0' && c <= '9') ||
    (t->val->type == ol_hex && c >= 'a' && c <= 'f') ||
    (t->val->type == ol_hex && c >= 'A' && c <= 'F') ||
    (t->val->type == ol_dec && c == '.') ||
    c == '_')
  {
    if (c == '.')
      t->val->type = ol_fp;
    if (c != '_')
      str_append(digits, c);

    c = nextChar(p);
  }

  if (digits->len == 0 &&
    !((c >= '0' && c <= '9') ||
    (t->val->type == ol_hex && c >= 'a' && c <= 'f') ||
    (t->val->type == ol_hex && c >= 'A' && c <= 'F')))
  {
    parseError(&t->loc, "Expected number, found '%s'", digits->cstr);
  }

  switch (t->val->type) {
  case ol_dec:
    t->val->dec = sign * parseInt(digits->cstr, 10);
    break;

  case ol_hex:
    t->val->hex = sign * parseInt(digits->cstr, 16);
    break;

  case ol_fp:
    t->val->fp = sign * atof(digits->cstr);
    break;

  default: break;
  }

  str_delete(digits);
  p->curTok = t;
  return t;
}


static Token *parseString(ParseCxt *p) {
  if (p->curTok != NULL)
    error("Internal error: free token before calling parseIdent");

  Token *t = (Token *) allocate(sizeof(Token));
  t->loc = p->loc;
  t->type = tk_val;

  t->val = (OlValue *) allocate(sizeof(OlValue));
  t->val->type = ol_str;

  char quote = p->curChar;
  if (quote != '\'' && quote != '"')
    parseError(&p->loc, "Expected string, found %c", quote);

  string *str = str_new();

  char c = nextChar(p);
  while (c != quote) {
    if (c == EOF)
      parseError(&t->loc, "Missing closing quotation mark");

    if (c == '\\')
      c = nextChar(p);

    str_append(str, c);
    c = nextChar(p);
  }

  nextChar(p);

  t->val->str = str_release(str);
  p->curTok = t;
  return t;
}


static Token *nextToken(ParseCxt *p) {
  if (p->curTok != NULL)
    error("Internal error: free token before calling nextToken");

  while (p->curChar == ' ' || p->curChar == '\t' || p->curChar == '\n')
    nextChar(p);

  int c = p->curChar;

  if (c == EOF) {
    Token *t = (Token *) allocate(sizeof(Token));
    t->loc = p->loc;
    t->type = tk_eof;
    p->curTok = t;
    return t;
  }

  if ((c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-')
    return parseNumber(p);

  if (c == '"' || c == '\'')
    return parseString(p);

  if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
    return parseIdent(p);

  Token *t = (Token *) allocate(sizeof(Token));
  t->loc = p->loc;
  t->type = tk_sym;

  t->sym = str_new();
  str_append(t->sym, c);
  nextChar(p);

  p->curTok = t;
  return t;
}


static int isSymbol(ParseCxt *p, char *sym) {
  return p->curTok->type == tk_sym && strcmp(p->curTok->sym->cstr, sym) == 0;
}


static void eatSymbol(ParseCxt *p) {
  str_delete(p->curTok->sym);
  free(p->curTok);
  p->curTok = NULL;
  nextToken(p);
}


static OlBlock *parseBlock(ParseCxt *p);


static OlValue *parseValue(ParseCxt *p) {
  if (isSymbol(p, "{")) {
    eatSymbol(p);

    OlValue *v = (OlValue *) allocate(sizeof(OlValue));
    v->type = ol_block;
    v->block = parseBlock(p);

    if (!isSymbol(p, "}"))
      parseError(&p->curTok->loc, "Expected }, found %s", tokenStr(p->curTok));
    eatSymbol(p);

    return v;
  }

  if (p->curTok->type != tk_val)
    parseError(&p->curTok->loc, "Expected value, found %s", tokenStr(p->curTok));

  OlValue *v = p->curTok->val;
  free(p->curTok);
  p->curTok = NULL;
  nextToken(p);

  if (v->type == ol_ident) {
    int isFunc = 0;
    for (char **funcName = p->funcNames; *funcName != NULL; funcName++) {
      if (strcmp(v->ident, *funcName) == 0)
        isFunc = 1;
    }

    if (isFunc) {
      if (!isSymbol(p, "("))
        parseError(&p->curTok->loc, "Expected (, found %s", tokenStr(p->curTok));
      eatSymbol(p);

      OlValue *vc = (OlValue *) allocate(sizeof(OlValue));
      vc->type = ol_call;
      vc->call.func = v;
      vc->call.args = parseBlock(p);

      if (!isSymbol(p, ")"))
        parseError(&p->curTok->loc, "Expected ), found %s", tokenStr(p->curTok));
      eatSymbol(p);

      return vc;
    }
  }

  return v;
}


static OlField *parseField(ParseCxt *p) {
  OlValue *v1 = parseValue(p);

  OlValue *v2 = NULL;
  if (isSymbol(p, "=")) {
    eatSymbol(p);
    v2 = parseValue(p);
  }

  OlField *f = (OlField *) allocate(sizeof(OlField));
  f->next = NULL;
  if (v2 == NULL) {
    f->key = NULL;
    f->value = v1;
  }
  else {
    f->key = v1;
    f->value = v2;
  }

  return f;
}


static OlBlock *parseBlock(ParseCxt *p) {
  OlBlock *b = (OlBlock *) allocate(sizeof(OlBlock));
  b->head = NULL;
  OlField **end = &b->head;

  while (!isSymbol(p, "}") && !isSymbol(p, ")") && p->curTok->type != tk_eof) {
    OlField *f = parseField(p);
    *end = f;
    end = &f->next;

    if (isSymbol(p, ",") || isSymbol(p, ";"))
      eatSymbol(p);
  }

  return b;
}


OlBlock *ol_parseFile(char *filename, char **funcNames) {
  char *nullFuncName = NULL;

  ParseCxt *p = (ParseCxt *) allocate(sizeof(ParseCxt));
  p->file = fopen(filename, "rb");
  if (p->file == NULL)
    error("Failed to open file: %s", filename);

  p->loc.filename = filename;
  p->loc.lineNum = 1;
  p->loc.colNum = 1;

  p->funcNames = funcNames == NULL ? &nullFuncName : funcNames;

  p->curChar = fgetc(p->file);
  p->nextChar = fgetc(p->file);

  p->curTok = NULL;
  nextToken(p);

  OlBlock *b = parseBlock(p);

  if (p->curTok->type != tk_eof)
    error("Unexpected token %s", tokenStr(p->curTok));
  free(p->curTok);

  fclose(p->file);
  free(p);
  return b;
}


static void freeValue(OlValue *v) {
  if (v == NULL) return;
  switch (v->type) {
  case ol_dec:   break;
  case ol_hex:   break;
  case ol_fp:    break;
  case ol_ident: free(v->ident); break;
  case ol_str:   free(v->str); break;
  case ol_call:  freeValue(v->call.func); ol_free(v->call.args); break;
  case ol_block: ol_free(v->block); break;
  }
  free(v);
}


static void freeField(OlField *f) {
  if (f == NULL) return;
  freeField(f->next);
  freeValue(f->key);
  freeValue(f->value);
  free(f);
}


void ol_free(OlBlock *b) {
  freeField(b->head);
  free(b);
}


char *ol_valueStr(OlValue *v) {
  static char buffer[32];

  if (v == NULL) return "null";

  switch (v->type) {

  case ol_dec:
    sprintf(&buffer[0], "%lld", v->dec);
    return &buffer[0];
  
  case ol_hex:
    sprintf(&buffer[0], "0x%llX", v->hex);
    return &buffer[0];

  case ol_fp:
    sprintf(&buffer[0], "%f", v->fp);
    return &buffer[0];
  
  case ol_ident:
    return v->ident;
  
  case ol_str: {
      size_t i = 0;
      buffer[i++] = '\'';
      size_t k = 0;
      while (i < sizeof(buffer) - 2 && v->str[k] != '\0') {
        char *ec = escapeChar(v->str[k++]);
        while (i < sizeof(buffer) - 2 && *ec != '\0')
          buffer[i++] = *ec++;
      }
      buffer[i++] = '\'';
      buffer[i] = '\0';

      if (v->str[k] != '\0') {
        buffer[sizeof(buffer) - 5] = '.';
        buffer[sizeof(buffer) - 4] = '.';
        buffer[sizeof(buffer) - 3] = '.';
      }
      return &buffer[0];
    }

  case ol_call:
    return "<call>";

  case ol_block:
    return "<block>";
  }
}


void ol_checkNoInvalidFields(OlBlock *b, char **idents) {
  for (OlField *f = b->head; f != NULL; f = f->next) {
    if (f->key == NULL || f->key->type != ol_ident)
      error("Invalid field: '%s'", ol_valueStr(f->key));

    int valid = 0;

    for (char **ident = idents; *ident != NULL; ident++) {
      if (strcmp(f->key->ident, *ident) == 0)
        valid = 1;
    }

    if (!valid)
      error("Invalid field: '%s'", ol_valueStr(f->key));
  }
}


OlValue *ol_getField(OlBlock *b, char *ident) {
  for (OlField *f = b->head; f != NULL; f = f->next) {
    if (f->key == NULL || f->key->type != ol_ident) continue;
    if (strcmp(f->key->ident, ident) == 0)
      return f->value;
  }

  return NULL;
}


OlValue *ol_checkField(OlBlock *b, char *ident, OlValueType types) {
  OlValue *result = NULL;

  for (OlField *f = b->head; f != NULL; f = f->next) {
    if (f->key == NULL || f->key->type != ol_ident) continue;
    if (strcmp(f->key->ident, ident) == 0) {

      if (result != NULL)
        error("Duplicate field: '%s'", ident);
      
      result = f->value;
      if (!(types & result->type))
        error("Invalid value for field '%s': %s", ident, ol_valueStr(result));
    }
  }

  if (result == NULL)
    error("Missing field: '%s'", ident);
  return result;
}


int ol_checkFieldInt(OlBlock *b, char *ident) {
  OlValue *v = ol_checkField(b, ident, ol_dec | ol_hex);

  switch (v->type) {
  case ol_dec:
    return (int) v->dec;

  case ol_hex:
    return (int) v->hex;

  default:
    return 0;
  }
}


float ol_checkFieldFloat(OlBlock *b, char *ident) {
  union {
    uint32_t i;
    float f;
  } u;

  OlValue *v = ol_checkField(b, ident, ol_dec | ol_hex | ol_fp);

  switch (v->type) {
  case ol_dec:
    return (float) v->dec;

  case ol_hex:
    u.i = (uint32_t) v->hex;
    return u.f;

  case ol_fp:
    return (float) v->fp;

  default:
    return 0;
  }
}


OlBlock *ol_checkFieldArray(OlBlock *b, char *ident, OlValueType types) {
  OlBlock *a = ol_checkField(b, ident, ol_block)->block;

  for (OlField *f = a->head; f != NULL; f = f->next) {
    if (f->key != NULL)
      error("Invalid assignment in field '%s': %s", ident, ol_valueStr(f->key));
    if (!(types & f->value->type))
      error("Invalid array element in field '%s': %s",
        ident, ol_valueStr(f->value));
  }

  return a;
}


OlBlock *ol_createFieldNamedArray(OlBlock *b, char *ident, char **names) {
  OlBlock *array = ol_checkField(b, ident, ol_block)->block;

  int expSize = 0;
  for (char **p = names; *p != NULL; p++)
    expSize += 1;

  int actualSize = 0;
  for (OlField *f = array->head; f != NULL; f = f->next)
    actualSize += 1;

  if (expSize != actualSize)
    error("Incorrectly sized array in field '%s' (expected %d)", ident, expSize);

  OlBlock *result = (OlBlock *) allocate(sizeof(OlBlock));
  result->head = NULL;
  OlField **end = &result->head;

  if (expSize == 0)
    return result;

  if (array->head->key == NULL) {
    for (OlField *f = array->head; f != NULL; f = f->next) {
      if (f->key != NULL)
        error("Invalid assignment in field '%s': %s", ident, ol_valueStr(f->key));

      OlValue *key = (OlValue *) malloc(sizeof(OlValue));
      key->type = ol_ident;
      key->ident = (char *) malloc(sizeof(*names) + 1);
      strcpy(key->str, *names);

      names++;

      *end = (OlField *) malloc(sizeof(OlField));
      (*end)->key = key;
      (*end)->value = f->value;
      (*end)->next = NULL;
      end = &(*end)->next;
    }
  }
  else {
    for (char **p = names; *p != NULL; p++) {
      OlValue *value = ol_getField(array, *p);
      if (value == NULL)
        error("Missing element in '%s' in field '%s'", *p, ident);

      OlValue *key = (OlValue *) malloc(sizeof(OlValue));
      key->type = ol_ident;
      key->ident = (char *) malloc(sizeof(*p) + 1);
      strcpy(key->str, *p);

      *end = (OlField *) malloc(sizeof(OlField));
      (*end)->key = key;
      (*end)->value = value;
      (*end)->next = NULL;
      end = &(*end)->next;
    }
  }

  return result;
}


void ol_freeNamedArray(OlBlock *b) {
  for (OlField *f = b->head; f != NULL; f = f->next) {
    f->value = NULL;
  }
  ol_free(b);
}
