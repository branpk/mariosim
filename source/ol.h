#ifndef OL_H
#define OL_H


#include <stdint.h>


typedef struct OlBlock OlBlock;
typedef struct OlField OlField;
typedef struct OlValue OlValue;


typedef enum {
  ol_dec = 1,
  ol_hex = 2,
  ol_fp = 4,
  ol_ident = 8,
  ol_str = 16,
  ol_call = 32,
  ol_block = 64,
} OlValueType;


struct OlValue {
  OlValueType type;
  union {
    uint64_t dec;
    uint64_t hex;
    double fp;
    char *ident;
    char *str;
    struct { OlValue *func; OlBlock *args; } call;
    OlBlock *block;
  };
};

struct OlField {
  OlValue *key;
  OlValue *value;
  OlField *next;
};

struct OlBlock {
  OlField *head;
};


OlBlock *ol_parseFile(char *filename);
void ol_free(OlBlock *b);
char *ol_valueStr(OlValue *v);
int ol_getNumFields(OlBlock *b);
int ol_isIdent(OlValue *v, char *ident);

void ol_checkNoInvalidFields(OlBlock *b, char **idents);
OlValue *ol_getField(OlBlock *b, char *ident);
OlValue *ol_checkField(OlBlock *b, char *ident, OlValueType types);
int ol_checkFieldInt(OlBlock *b, char *ident);
float ol_checkFieldFloat(OlBlock *b, char *ident);
OlBlock *ol_checkFieldArray(OlBlock *b, char *ident, OlValueType types);
void ol_checkNumArgs(OlValue *c, int expected, int allowNamed);
OlValue *ol_checkArg(OlValue *c, int arg, OlValueType types);
float ol_checkArgFloat(OlValue *c, int arg);

OlBlock *ol_createFieldNamedArray(OlBlock *b, char *ident, char **names);
void ol_freeNamedArray(OlBlock *b);


#endif