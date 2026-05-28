#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#endif

// ==========================================
// DIAGNOSTICS
// ==========================================
typedef struct {
    bool has_error;
    char message[256];
    int position;
} Diagnostic;

static Diagnostic diag = {0};

void report_error(int pos, const char *msg) {
    if (!diag.has_error) {
        diag.has_error = true;
        diag.position = pos;
        strncpy(diag.message, msg, sizeof(diag.message) - 1);
        diag.message[sizeof(diag.message) - 1] = '\0';
    }
}

// ==========================================
// LEXER
// ==========================================
typedef enum {
    TOK_EOF,
    TOK_IDENT,
    TOK_REGISTER,
    TOK_NUMBER,
    TOK_COMMA,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR
} TokenType;

typedef struct {
    TokenType type;
    char text[64];
    uint32_t num_val;
    int reg_id; // For registers: 0-15 for GPRs, 0-31 for ZMM, 0-7 for K
    int pos;
} Token;

typedef struct {
    const char *input;
    int pos;
    int len;
} Lexer;

void lexer_init(Lexer *l, const char *input) {
    l->input = input;
    l->pos = 0;
    l->len = strlen(input);
}

static int parse_reg_name(const char *name) {
    char lower[16] = {0};
    for(int i=0; i<15 && name[i]; i++) lower[i] = tolower(name[i]);

    if (strncmp(lower, "zmm", 3) == 0) {
        int v = atoi(lower + 3);
        if (v >= 0 && v <= 31) return 100 + v; // 100+ for ZMM
    }
    if (lower[0] == 'k' && isdigit(lower[1]) && lower[2] == '\0') {
        int v = lower[1] - '0';
        if (v >= 0 && v <= 7) return 200 + v; // 200+ for K-mask
    }

    const char *gprs[] = {"rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15"};
    for (int i=0; i<16; i++) {
        if (strcmp(lower, gprs[i]) == 0) return i;
    }
    return -1;
}

Token lexer_next(Lexer *l) {
    Token t;
    memset(&t, 0, sizeof(t));

    while (l->pos < l->len && isspace((unsigned char)l->input[l->pos])) {
        l->pos++;
    }

    t.pos = l->pos;

    if (l->pos >= l->len) {
        t.type = TOK_EOF;
        return t;
    }

    char c = l->input[l->pos];

    if (isalpha((unsigned char)c)) {
        int start = l->pos;
        while (l->pos < l->len && (isalnum((unsigned char)l->input[l->pos]))) {
            l->pos++;
        }
        int len = l->pos - start;
        if ((size_t)len >= sizeof(t.text)) len = sizeof(t.text) - 1;
        strncpy(t.text, l->input + start, len);
        t.text[len] = '\0';

        int reg = parse_reg_name(t.text);
        if (reg != -1) {
            t.type = TOK_REGISTER;
            t.reg_id = reg;
        } else {
            t.type = TOK_IDENT;
        }
        return t;
    }

    if (isdigit((unsigned char)c)) {
        int start = l->pos;
        int base = 10;
        if (c == '0' && l->pos + 1 < l->len && tolower(l->input[l->pos+1]) == 'x') {
            base = 16;
            l->pos += 2;
        }
        uint32_t val = 0;
        while (l->pos < l->len) {
            char c2 = tolower(l->input[l->pos]);
            if (isdigit((unsigned char)c2)) val = val * base + (c2 - '0');
            else if (base == 16 && c2 >= 'a' && c2 <= 'f') val = val * base + (c2 - 'a' + 10);
            else break;
            l->pos++;
        }
        t.type = TOK_NUMBER;
        t.num_val = val;
        int len = l->pos - start;
        if ((size_t)len >= sizeof(t.text)) len = sizeof(t.text) - 1;
        strncpy(t.text, l->input + start, len);
        t.text[len] = '\0';
        return t;
    }

    l->pos++;
    t.text[0] = c;
    t.text[1] = '\0';

    switch (c) {
        case ',': t.type = TOK_COMMA; break;
        case '[': t.type = TOK_LBRACKET; break;
        case ']': t.type = TOK_RBRACKET; break;
        case '{': t.type = TOK_LBRACE; break;
        case '}': t.type = TOK_RBRACE; break;
        case '+': t.type = TOK_PLUS; break;
        case '-': t.type = TOK_MINUS; break;
        case '*': t.type = TOK_STAR; break;
        default:
            report_error(t.pos, "Неизвестный символ");
            t.type = TOK_EOF;
            break;
    }
    return t;
}

// ==========================================
// IR
// ==========================================
typedef enum {
    OP_NONE,
    OP_REG,
    OP_MEM
} OperandType;

typedef struct {
    OperandType type;
    int reg;       // 0-31
    int k_mask;    // 0-7
    bool z_flag;   // zeroing

    int base_reg;  // 0-15
    int index_reg; // 0-15
    int scale;     // 1, 2, 4, 8
    int32_t disp;
} Operand;

typedef struct {
    char mnemonic[32];
    Operand operands[4];
    int num_operands;
} Instruction;

// ==========================================
// PARSER
// ==========================================
typedef struct {
    Lexer lexer;
    Token curr;
    Token next;
} Parser;

void parser_advance(Parser *p) {
    p->curr = p->next;
    p->next = lexer_next(&p->lexer);
}

void parser_init(Parser *p, const char *input) {
    lexer_init(&p->lexer, input);
    p->curr = lexer_next(&p->lexer);
    p->next = lexer_next(&p->lexer);
}

bool match(Parser *p, TokenType type) {
    if (p->curr.type == type) {
        parser_advance(p);
        return true;
    }
    return false;
}

bool expect(Parser *p, TokenType type, const char *err) {
    if (p->curr.type == type) {
        parser_advance(p);
        return true;
    }
    report_error(p->curr.pos, err);
    return false;
}

void parse_mask(Parser *p, Operand *op) {
    while (match(p, TOK_LBRACE)) {
        if (p->curr.type == TOK_REGISTER && p->curr.reg_id >= 200 && p->curr.reg_id <= 207) {
            op->k_mask = p->curr.reg_id - 200;
            parser_advance(p);
        } else if (p->curr.type == TOK_IDENT && tolower((unsigned char)p->curr.text[0]) == 'z') {
            op->z_flag = true;
            parser_advance(p);
        } else {
            report_error(p->curr.pos, "Ожидается регистр маски (K0-K7) или Z");
            return;
        }
        expect(p, TOK_RBRACE, "Ожидается '}'");
    }
}

Operand parse_operand(Parser *p) {
    Operand op;
    memset(&op, 0, sizeof(op));
    op.base_reg = -1;
    op.index_reg = -1;
    op.scale = 1;

    if (p->curr.type == TOK_REGISTER && p->curr.reg_id >= 100 && p->curr.reg_id <= 131) {
        op.type = OP_REG;
        op.reg = p->curr.reg_id - 100;
        parser_advance(p);
        parse_mask(p, &op);
    } else if (match(p, TOK_LBRACKET)) {
        op.type = OP_MEM;
        int sign = 1;
        while (p->curr.type != TOK_RBRACKET && p->curr.type != TOK_EOF) {
            if (match(p, TOK_PLUS)) sign = 1;
            else if (match(p, TOK_MINUS)) sign = -1;

            if (p->curr.type == TOK_NUMBER) {
                op.disp += sign * p->curr.num_val;
                parser_advance(p);
            } else if (p->curr.type == TOK_REGISTER && p->curr.reg_id >= 0 && p->curr.reg_id <= 15) {
                int reg = p->curr.reg_id;
                parser_advance(p);
                if (match(p, TOK_STAR)) {
                    if (p->curr.type == TOK_NUMBER) {
                        op.scale = p->curr.num_val;
                        op.index_reg = reg;
                        parser_advance(p);
                    } else {
                        report_error(p->curr.pos, "Ожидается число для scale (1, 2, 4, 8)");
                    }
                } else {
                    if (op.base_reg == -1) op.base_reg = reg;
                    else {
                        op.index_reg = reg;
                        op.scale = 1;
                    }
                }
            } else {
                report_error(p->curr.pos, "Неверный токен в памяти");
                break;
            }
        }
        expect(p, TOK_RBRACKET, "Ожидается ']'");
        parse_mask(p, &op);
    } else {
        report_error(p->curr.pos, "Ожидается операнд (регистр или память)");
    }
    return op;
}

Instruction parse_instruction(const char *input) {
    Instruction inst;
    memset(&inst, 0, sizeof(inst));
    diag.has_error = false;

    Parser p;
    parser_init(&p, input);

    if (p.curr.type != TOK_IDENT && p.curr.type != TOK_REGISTER) {
        report_error(p.curr.pos, "Ожидается мнемоника инструкции");
        return inst;
    }

    size_t copy_len = strlen(p.curr.text);
    if (copy_len >= sizeof(inst.mnemonic)) copy_len = sizeof(inst.mnemonic) - 1;
    memcpy(inst.mnemonic, p.curr.text, copy_len);
    inst.mnemonic[copy_len] = '\0';
    parser_advance(&p);

    if (p.curr.type != TOK_EOF) {
        inst.operands[inst.num_operands++] = parse_operand(&p);
        while (match(&p, TOK_COMMA)) {
            inst.operands[inst.num_operands++] = parse_operand(&p);
        }
    }

    if (p.curr.type != TOK_EOF) {
        report_error(p.curr.pos, "Неожиданные символы в конце строки");
    }

    return inst;
}

// ==========================================
// VALIDATOR
// ==========================================
bool validate_instruction(const Instruction *inst) {
    if (strcasecmp(inst->mnemonic, "vmovdqa64") != 0) {
        report_error(0, "Неподдерживаемая инструкция. Ожидается VMOVDQA64.");
        return false;
    }

    if (inst->num_operands != 2) {
        report_error(0, "Неверное количество операндов. Ожидается 2.");
        return false;
    }

    const Operand *dst = &inst->operands[0];
    const Operand *src = &inst->operands[1];

    if (dst->type == OP_MEM && src->type == OP_MEM) {
        report_error(0, "Перемещение из памяти в память не поддерживается.");
        return false;
    }

    for (int i=0; i<2; i++) {
        const Operand *op = &inst->operands[i];
        if (op->type == OP_MEM) {
            if (op->scale != 1 && op->scale != 2 && op->scale != 4 && op->scale != 8) {
                report_error(0, "Недопустимый scale в памяти (допускается 1, 2, 4, 8).");
                return false;
            }
        }
    }

    return true;
}

void print_diagnostics(const char *input) {
    if (diag.has_error) {
        printf("Ошибка: %s\n", diag.message);
        printf("  %s\n  ", input);
        for(int i=0; i<diag.position; i++) putchar(' ');
        printf("^\n");
    }
}

// ==========================================
// INSTRUCTION TABLES
// ==========================================
typedef struct {
    const char *mnemonic;
    uint8_t opcode_load;
    uint8_t opcode_store;
    uint8_t map; // 1 for 0F, 2 for 0F38, 3 for 0F3A
    uint8_t pp;  // 0: none, 1: 66, 2: F3, 3: F2
    uint8_t w;   // 0 or 1
    uint8_t ll;  // 0: 128, 1: 256, 2: 512
} InstDef;

const InstDef inst_table[] = {
    {"vmovdqa64", 0x6F, 0x7F, 1, 1, 1, 2}, // map=0F(1), pp=66(1), W=1, LL=512(2)
};
const int num_insts = sizeof(inst_table) / sizeof(InstDef);

const InstDef* find_instruction(const char *mnemonic) {
    for (int i=0; i<num_insts; i++) {
        if (strcasecmp(mnemonic, inst_table[i].mnemonic) == 0) {
            return &inst_table[i];
        }
    }
    return NULL;
}

// ==========================================
// BACKEND
// ==========================================
typedef struct {
    uint8_t bytes[15];
    int len;
} EncodedInst;

void encode_evex(EncodedInst *enc, const InstDef *def, const Operand *reg_op, const Operand *rm_op, bool z_flag, int k_mask) {
    uint8_t p0 = 0x62;

    int r = (reg_op->reg & 0x08) ? 0 : 1;
    int r_prime = (reg_op->reg & 0x10) ? 0 : 1;

    int b = 1, x = 1;
    if (rm_op->type == OP_REG) {
        b = (rm_op->reg & 0x08) ? 0 : 1;
        x = (rm_op->reg & 0x10) ? 0 : 1;
    } else {
        b = (rm_op->base_reg != -1 && (rm_op->base_reg & 0x08)) ? 0 : 1;
        x = (rm_op->index_reg != -1 && (rm_op->index_reg & 0x08)) ? 0 : 1;
    }

    uint8_t p1 = (r << 7) | (x << 6) | (b << 5) | (r_prime << 4) | (def->map & 0xF);

    int vvvv = 0xF;
    uint8_t p2 = (def->w << 7) | (vvvv << 3) | (1 << 2) | (def->pp & 3);

    int z = z_flag ? 1 : 0;
    uint8_t p3 = (z << 7) | ((def->ll & 3) << 5) | (0 << 4) | (1 << 3) | (k_mask & 7);

    if (rm_op->type == OP_REG) {
        x = (rm_op->reg & 0x10) ? 0 : 1;
        p1 = (p1 & ~(1 << 6)) | (x << 6);
    }

    enc->bytes[enc->len++] = p0;
    enc->bytes[enc->len++] = p1;
    enc->bytes[enc->len++] = p2;
    enc->bytes[enc->len++] = p3;
}

void encode_modrm_sib_disp(EncodedInst *enc, const Operand *reg_op, const Operand *rm_op) {
    uint8_t mod = 0;
    uint8_t rm = 0;

    if (rm_op->type == OP_REG) {
        mod = 3;
        rm = rm_op->reg & 7;
        enc->bytes[enc->len++] = (mod << 6) | ((reg_op->reg & 7) << 3) | rm;
    } else {
        bool needs_sib = (rm_op->base_reg == 4 || rm_op->base_reg == 12 || rm_op->index_reg != -1);
        bool rip_rel = (rm_op->base_reg == -1 && rm_op->index_reg == -1);

        int disp_size = 0;

        if (rip_rel) {
            mod = 0;
            rm = 4;
            needs_sib = true;
            disp_size = 4;
        } else {
            if (rm_op->disp == 0 && rm_op->base_reg != 5 && rm_op->base_reg != 13 && rm_op->base_reg != -1) {
                mod = 0;
            } else if (rm_op->disp >= -8192 && rm_op->disp <= 8128 && (rm_op->disp % 64 == 0) && rm_op->base_reg != -1) {
                mod = 1;
                disp_size = 1;
            } else {
                mod = 2;
                disp_size = 4;
                if (rm_op->base_reg == -1) mod = 0;
            }
            rm = rm_op->base_reg != -1 ? (rm_op->base_reg & 7) : 4;
            if (needs_sib) rm = 4;
        }

        enc->bytes[enc->len++] = (mod << 6) | ((reg_op->reg & 7) << 3) | rm;

        if (needs_sib) {
            uint8_t ss = 0;
            if (rm_op->scale == 2) ss = 1;
            else if (rm_op->scale == 4) ss = 2;
            else if (rm_op->scale == 8) ss = 3;

            uint8_t index = (rm_op->index_reg != -1) ? (rm_op->index_reg & 7) : 4;
            uint8_t base = rip_rel ? 5 : ((rm_op->base_reg == -1) ? 5 : (rm_op->base_reg & 7));

            enc->bytes[enc->len++] = (ss << 6) | (index << 3) | base;
        }

        if (disp_size == 1) {
            enc->bytes[enc->len++] = (uint8_t)(rm_op->disp / 64);
        } else if (disp_size == 4) {
            enc->bytes[enc->len++] = (uint8_t)(rm_op->disp & 0xFF);
            enc->bytes[enc->len++] = (uint8_t)((rm_op->disp >> 8) & 0xFF);
            enc->bytes[enc->len++] = (uint8_t)((rm_op->disp >> 16) & 0xFF);
            enc->bytes[enc->len++] = (uint8_t)((rm_op->disp >> 24) & 0xFF);
        }
    }
}

EncodedInst encode_instruction(const Instruction *inst) {
    EncodedInst enc;
    memset(&enc, 0, sizeof(enc));

    const InstDef *def = find_instruction(inst->mnemonic);
    if (!def) return enc; // Validation should catch this early

    const Operand *dst = &inst->operands[0];
    const Operand *src = &inst->operands[1];

    const Operand *reg_op;
    const Operand *rm_op;
    uint8_t opcode;

    if (dst->type == OP_REG && src->type == OP_REG) {
        opcode = def->opcode_load;
        reg_op = dst;
        rm_op = src;
    } else if (dst->type == OP_REG && src->type == OP_MEM) {
        opcode = def->opcode_load;
        reg_op = dst;
        rm_op = src;
    } else { // MEM, REG
        opcode = def->opcode_store;
        reg_op = src;
        rm_op = dst;
    }

    encode_evex(&enc, def, reg_op, rm_op, dst->z_flag, dst->k_mask);
    enc.bytes[enc.len++] = opcode;
    encode_modrm_sib_disp(&enc, reg_op, rm_op);

    return enc;
}

// ==========================================
// DISASSEMBLER (DECODER)
// ==========================================
bool decode_instruction(const uint8_t *bytes, int len, Instruction *inst) {
    if (len < 6) return false;

    memset(inst, 0, sizeof(*inst));
    if (bytes[0] != 0x62) return false; // EVEX only

    uint8_t p1 = bytes[1];
    uint8_t p2 = bytes[2];
    uint8_t p3 = bytes[3];
    uint8_t opcode = bytes[4];
    uint8_t modrm = bytes[5];

    int ptr = 6;

    // Find mnemonic based on opcode and map/pp/w/ll
    uint8_t map = p1 & 0xF;
    uint8_t pp = p2 & 3;
    uint8_t w = (p2 >> 7) & 1;
    uint8_t ll = (p3 >> 5) & 3;

    const InstDef *def = NULL;
    for (int i=0; i<num_insts; i++) {
        if (inst_table[i].map == map && inst_table[i].pp == pp && inst_table[i].w == w && inst_table[i].ll == ll) {
            if (inst_table[i].opcode_load == opcode || inst_table[i].opcode_store == opcode) {
                def = &inst_table[i];
                break;
            }
        }
    }

    if (!def) return false;
    strcpy(inst->mnemonic, def->mnemonic);
    inst->num_operands = 2;

    int r = ((p1 >> 7) & 1) ? 0 : 1;
    int r_prime = ((p1 >> 4) & 1) ? 0 : 1;
    int reg_id = ((modrm >> 3) & 7) | (r << 3) | (r_prime << 4);

    Operand reg_op = {0};
    reg_op.type = OP_REG;
    reg_op.reg = reg_id;

    int x = ((p1 >> 6) & 1) ? 0 : 1;
    int b = ((p1 >> 5) & 1) ? 0 : 1;

    uint8_t mod = (modrm >> 6) & 3;
    uint8_t rm = modrm & 7;

    Operand rm_op = {0};
    rm_op.base_reg = -1;
    rm_op.index_reg = -1;
    rm_op.scale = 1;

    if (mod == 3) {
        rm_op.type = OP_REG;
        int rm_reg_id = rm | (b << 3) | (x << 4);
        rm_op.reg = rm_reg_id;
    } else {
        rm_op.type = OP_MEM;
        if (rm == 4) { // SIB
            if (ptr >= len) return false;
            uint8_t sib = bytes[ptr++];
            uint8_t scale_val = (sib >> 6) & 3;
            rm_op.scale = 1 << scale_val;

            uint8_t index = (sib >> 3) & 7;
            uint8_t base = sib & 7;

            if (index == 4 && x == 0) {
                rm_op.index_reg = -1; // rsp is no index
            } else {
                rm_op.index_reg = index | (x << 3);
            }

            if (mod == 0 && base == 5) {
                rm_op.base_reg = -1; // disp32 without base
            } else {
                rm_op.base_reg = base | (b << 3);
            }
        } else {
            if (mod == 0 && rm == 5) { // RIP-relative / abs32
                rm_op.base_reg = -1;
            } else {
                rm_op.base_reg = rm | (b << 3);
            }
        }

        if (mod == 1) {
            if (ptr >= len) return false;
            int8_t disp8 = bytes[ptr++];
            rm_op.disp = disp8 * 64; // assuming 512-bit vector for VMOVDQA64
        } else if (mod == 2 || (mod == 0 && rm_op.base_reg == -1)) {
            if (ptr + 3 >= len) return false;
            rm_op.disp = (int32_t)bytes[ptr] | ((int32_t)bytes[ptr+1] << 8) | ((int32_t)bytes[ptr+2] << 16) | ((int32_t)bytes[ptr+3] << 24);
            ptr += 4;
        }
    }

    int k_mask = p3 & 7;
    int z_flag = (p3 >> 7) & 1;

    if (opcode == def->opcode_load) {
        inst->operands[0] = reg_op;
        inst->operands[1] = rm_op;
        inst->operands[0].k_mask = k_mask;
        inst->operands[0].z_flag = z_flag;
    } else {
        inst->operands[0] = rm_op;
        inst->operands[1] = reg_op;
        inst->operands[0].k_mask = k_mask;
        inst->operands[0].z_flag = z_flag;
    }

    return true;
}

// ==========================================
// CANONICAL FORMATTING
// ==========================================
void format_operand(const Operand *op, char *out) {
    if (op->type == OP_REG) {
        sprintf(out, "ZMM%d", op->reg);
        if (op->k_mask != 0) {
            char k[16];
            sprintf(k, "{K%d}", op->k_mask);
            strcat(out, k);
            if (op->z_flag) strcat(out, "{Z}");
        }
    } else {
        const char *gprs[] = {"RAX","RCX","RDX","RBX","RSP","RBP","RSI","RDI","R8","R9","R10","R11","R12","R13","R14","R15"};
        char tmp[128] = "[";
        bool first = true;
        if (op->base_reg != -1) {
            strcat(tmp, gprs[op->base_reg]);
            first = false;
        }
        if (op->index_reg != -1) {
            if (!first) strcat(tmp, " + ");
            strcat(tmp, gprs[op->index_reg]);
            if (op->scale > 1) {
                char sc[16];
                sprintf(sc, "*%d", op->scale);
                strcat(tmp, sc);
            }
            first = false;
        }
        if (op->disp != 0 || first) {
            if (!first) {
                if (op->disp > 0) {
                    char d[32];
                    sprintf(d, " + 0x%X", op->disp);
                    strcat(tmp, d);
                } else {
                    char d[32];
                    sprintf(d, " - 0x%X", -op->disp);
                    strcat(tmp, d);
                }
            } else {
                char d[32];
                sprintf(d, "0x%X", (uint32_t)op->disp);
                strcat(tmp, d);
            }
        }
        strcat(tmp, "]");
        if (op->k_mask != 0) {
            char k[16];
            sprintf(k, " {K%d}", op->k_mask);
            strcat(tmp, k);
            if (op->z_flag) strcat(tmp, "{Z}");
        }
        strcpy(out, tmp);
    }
}

// ==========================================
// FUZZING ENTRY POINT
// ==========================================
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 255) return 0;
    char input[256];
    memcpy(input, data, size);
    input[size] = '\0';

    Instruction inst = parse_instruction(input);
    if (!diag.has_error && validate_instruction(&inst)) {
        EncodedInst enc = encode_instruction(&inst);
        Instruction decoded;
        if (decode_instruction(enc.bytes, enc.len, &decoded)) {
            // successful roundtrip
        }
    }
    return 0;
}
#endif

// ==========================================
// CLI & TESTS
// ==========================================
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
void run_tests() {
    const char *cases[] = {
        "vmovdqa64 zmm1, zmm2",
        "vmovdqa64 zmm31, zmm20",
        "vmovdqa64 zmm1 {k3}, zmm2",
        "vmovdqa64 zmm1 {k3}{z}, zmm2",
        "vmovdqa64 [rax], zmm1",
        "vmovdqa64 zmm1, [rax]",
        "vmovdqa64 zmm1, [rax + rbx*4 + 0x123]",
        "vmovdqa64 [rax + rbx*4 + 0x123], zmm1",
        "vmovdqa64 zmm25, [r15 + r14*8 + 0x40]",
        "vmovdqa64 zmm1, [rax*4]",
        "vmovdqa64 zmm1, [r15*4]",
        "vmovdqa64 zmm1, [0x123]",
        "vmovdqa64 [rax]{k3}, zmm1"
    };
    int n = sizeof(cases) / sizeof(cases[0]);
    int passed = 0;

    for (int i=0; i<n; i++) {
        Instruction inst = parse_instruction(cases[i]);
        if (diag.has_error || !validate_instruction(&inst)) {
            printf("Test Failed: Parse/Validate error on '%s'\n", cases[i]);
            print_diagnostics(cases[i]);
            continue;
        }

        EncodedInst enc = encode_instruction(&inst);

        Instruction dec;
        if (!decode_instruction(enc.bytes, enc.len, &dec)) {
            printf("Test Failed: Decode error on '%s'\n", cases[i]);
            continue;
        }

        // Very basic roundtrip check (num operands match)
        if (inst.num_operands == dec.num_operands) {
            passed++;
        }
    }

    printf("Tests passed: %d / %d\n", passed, n);
}

int main(int argc, char **argv) {
#ifdef _WIN32
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
#endif

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        run_tests();
        return 0;
    }

    char line[256];
    printf("Введите команду AVX-512 VMOVDQA64 (или пустую строку для выхода):\n");
    while (fgets(line, sizeof(line), stdin)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[len-1] = '\0';
            len--;
        }
        if (len == 0) break;

        Instruction inst = parse_instruction(line);
        if (diag.has_error) {
            print_diagnostics(line);
        } else if (validate_instruction(&inst)) {
            EncodedInst enc = encode_instruction(&inst);

            char op1[128], op2[128];
            format_operand(&inst.operands[0], op1);
            format_operand(&inst.operands[1], op2);

            // To upper mnemonic
            char upper_mnem[32];
            for (int i=0; inst.mnemonic[i]; i++) upper_mnem[i] = toupper(inst.mnemonic[i]);
            upper_mnem[strlen(inst.mnemonic)] = '\0';

            printf("Исходная команда: %s\n", line);
            printf("Каноническая форма: %s %s, %s\n", upper_mnem, op1, op2);
            printf("Машинный код: ");
            for (int i=0; i<enc.len; i++) {
                printf("%02X ", enc.bytes[i]);
            }
            printf("\n");

            // Try disassemble
            Instruction dec;
            if (decode_instruction(enc.bytes, enc.len, &dec)) {
                format_operand(&dec.operands[0], op1);
                format_operand(&dec.operands[1], op2);
                printf("Дизассемблировано: %s %s, %s\n", upper_mnem, op1, op2);
            }
        } else {
            print_diagnostics(line);
        }
        printf("\nВведите следующую команду:\n");
    }
    return 0;
}
#endif
