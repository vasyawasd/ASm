#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <locale.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_LINE 256

typedef enum { OP_REG, OP_MEM } OpType;

typedef struct {
    OpType type;
    int reg;
    int k_mask;
    bool z_flag;

    int base_reg;
    int index_reg;
    int scale;
    int32_t disp;
} Operand;

static inline bool is_space(char c) {
    return c == ' ' || c == '\t';
}

static inline void skip_space(const char **p) {
    while (is_space(**p)) (*p)++;
}

static inline int parse_reg_fast(const char **p) {
    const char *s = *p;
    char c0 = *s | 32;
    if (c0 == 'z') {
        if ((s[1] | 32) == 'm' && (s[2] | 32) == 'm') {
            s += 3;
            int v = 0;
            if (*s < '0' || *s > '9') return -1;
            while (*s >= '0' && *s <= '9') {
                v = v * 10 + (*s - '0');
                s++;
            }
            if (v > 31) return -1;
            *p = s;
            return v;
        }
        return -1;
    }
    if (c0 == 'r') {
        char c1 = s[1]; // Do not OR with 32 for digits!
        if (c1 >= '0' && c1 <= '9') {
            int v = 0;
            s++;
            while (*s >= '0' && *s <= '9') {
                v = v * 10 + (*s - '0');
                s++;
            }
            if (v >= 8 && v <= 15) {
                *p = s;
                return v;
            }
            return -1;
        }
        c1 |= 32;
        char c2 = s[2] | 32;
        s += 3;
        uint32_t val = (c0 << 16) | (c1 << 8) | c2;
        int reg = -1;
        switch (val) {
            case 0x726178: reg = 0; break; // rax
            case 0x726378: reg = 1; break; // rcx
            case 0x726478: reg = 2; break; // rdx
            case 0x726278: reg = 3; break; // rbx
            case 0x727370: reg = 4; break; // rsp
            case 0x726270: reg = 5; break; // rbp
            case 0x727369: reg = 6; break; // rsi
            case 0x726469: reg = 7; break; // rdi
        }
        if (reg != -1) {
            *p = s;
            return reg;
        }
    }
    return -1;
}

static inline bool parse_operand_fast(const char **p, Operand *op) {
    op->type = OP_REG;
    op->k_mask = 0;
    op->z_flag = false;
    op->base_reg = -1;
    op->index_reg = -1;
    op->scale = 1;
    op->disp = 0;

    skip_space(p);
    if (**p == '[') {
        op->type = OP_MEM;
        (*p)++;
        int sign = 1;
        while (**p && **p != ']') {
            skip_space(p);
            if (**p == '+') { sign = 1; (*p)++; continue; }
            if (**p == '-') { sign = -1; (*p)++; continue; }

            if ((**p >= '0' && **p <= '9')) {
                int base = 10;
                if ((*p)[0] == '0' && ((*p)[1] | 32) == 'x') {
                    base = 16;
                    (*p) += 2;
                }
                uint32_t val = 0;
                while (1) {
                    char c = **p;
                    if (c >= '0' && c <= '9') val = val * base + (c - '0');
                    else if (base == 16 && (c | 32) >= 'a' && (c | 32) <= 'f') val = val * base + ((c | 32) - 'a' + 10);
                    else break;
                    (*p)++;
                }
                op->disp += sign * (int32_t)val;
            } else if (**p != ']') {
                int reg = parse_reg_fast(p);
                if (reg == -1) return false;
                skip_space(p);
                if (**p == '*') {
                    (*p)++;
                    skip_space(p);
                    int scale = **p - '0';
                    if (scale != 1 && scale != 2 && scale != 4 && scale != 8) return false;
                    (*p)++;
                    op->index_reg = reg;
                    op->scale = scale;
                } else {
                    if (op->base_reg == -1) op->base_reg = reg;
                    else {
                        op->index_reg = reg;
                        op->scale = 1;
                    }
                }
            }
        }
        if (**p == ']') (*p)++;
        skip_space(p);
        while (**p == '{') {
            (*p)++;
            skip_space(p);
            char c = **p | 32;
            if (c == 'k') {
                (*p)++;
                op->k_mask = **p - '0';
                (*p)++;
            } else if (c == 'z') {
                (*p)++;
                op->z_flag = true;
            }
            skip_space(p);
            if (**p == '}') (*p)++;
            skip_space(p);
        }
        return true;
    } else {
        op->type = OP_REG;
        int reg = parse_reg_fast(p);
        if (reg == -1) return false;
        op->reg = reg;

        skip_space(p);
        while (**p == '{') {
            (*p)++;
            skip_space(p);
            char c = **p | 32;
            if (c == 'k') {
                (*p)++;
                op->k_mask = **p - '0';
                (*p)++;
            } else if (c == 'z') {
                (*p)++;
                op->z_flag = true;
            }
            skip_space(p);
            if (**p == '}') (*p)++;
            skip_space(p);
        }
        return true;
    }
}

static inline void print_canon_reg(int reg) {
    if (reg < 10) {
        putchar('0' + reg);
    } else {
        putchar('0' + (reg / 10));
        putchar('0' + (reg % 10));
    }
}

static inline void print_canon_mem_reg(int reg) {
    static const char *regs[] = {"RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI",
                                 "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15"};
    fputs(regs[reg], stdout);
}

static inline void print_hex(uint32_t val) {
    if (val == 0) {
        putchar('0');
        return;
    }
    char buf[16];
    int i = 0;
    while (val > 0) {
        int rem = val & 15;
        buf[i++] = rem < 10 ? '0' + rem : 'A' + rem - 10;
        val >>= 4;
    }
    while (i > 0) {
        putchar(buf[--i]);
    }
}

static inline void print_operand(const Operand *op) {
    if (op->type == OP_REG) {
        fputs("ZMM", stdout);
        print_canon_reg(op->reg);
        if (op->k_mask != 0) {
            fputs(" {K", stdout);
            putchar('0' + op->k_mask);
            putchar('}');
            if (op->z_flag) fputs("{Z}", stdout);
        }
    } else {
        putchar('[');
        bool first = true;
        if (op->base_reg != -1) {
            print_canon_mem_reg(op->base_reg);
            first = false;
        }
        if (op->index_reg != -1) {
            if (!first) fputs(" + ", stdout);
            print_canon_mem_reg(op->index_reg);
            if (op->scale > 1) {
                putchar('*');
                putchar('0' + op->scale);
            }
            first = false;
        }
        if (op->disp != 0 || first) {
            if (!first) {
                if (op->disp > 0) {
                    fputs(" + 0x", stdout);
                    print_hex(op->disp);
                } else {
                    fputs(" - 0x", stdout);
                    print_hex(-op->disp);
                }
            } else {
                fputs("0x", stdout);
                print_hex((uint32_t)op->disp);
            }
        }
        putchar(']');
        if (op->k_mask != 0) {
            fputs(" {K", stdout);
            putchar('0' + op->k_mask);
            putchar('}');
            if (op->z_flag) fputs("{Z}", stdout);
        }
    }
}

static inline void print_byte_hex(uint8_t b) {
    int h = b >> 4;
    int l = b & 15;
    putchar(h < 10 ? '0' + h : 'A' + h - 10);
    putchar(l < 10 ? '0' + l : 'A' + l - 10);
    putchar(' ');
}

static void process_instruction(const char *input) {
    const char *p = input;
    skip_space(&p);

    const char *exp = "vmovdqa64";
    for (int i = 0; i < 9; i++) {
        if ((p[i] | 32) != exp[i]) {
            fputs("Ошибка: Команда не совпадает с заданной (VMOVDQA64). Программа завершает работу.\n", stdout);
            exit(0);
        }
    }
    p += 9;
    if (*p && !is_space(*p) && *p != ',') {
        fputs("Ошибка: Команда не совпадает с заданной (VMOVDQA64). Программа завершает работу.\n", stdout);
        exit(0);
    }

    Operand dst_op, src_op;
    if (!parse_operand_fast(&p, &dst_op)) {
        fputs("Ошибка: Неверный операнд назначения. Завершение работы.\n", stdout);
        exit(0);
    }

    skip_space(&p);
    if (*p != ',') {
        fputs("Ошибка: Пропущена запятая. Завершение работы.\n", stdout);
        exit(0);
    }
    p++;

    if (!parse_operand_fast(&p, &src_op)) {
        fputs("Ошибка: Неверный операнд источника. Завершение работы.\n", stdout);
        exit(0);
    }

    fputs("Исходная команда: ", stdout);
    fputs(input, stdout);
    putchar('\n');

    fputs("Каноническая форма: VMOVDQA64 ", stdout);
    print_operand(&dst_op);
    fputs(", ", stdout);
    print_operand(&src_op);
    putchar('\n');

    fputs("Машинный код: ", stdout);

    uint8_t bytes[15];
    int len = 0;

    uint8_t opcode;
    Operand *reg_op;
    Operand *rm_op;

    if (dst_op.type == OP_REG && src_op.type == OP_REG) {
        opcode = 0x6F;
        reg_op = &dst_op;
        rm_op = &src_op;
    } else if (dst_op.type == OP_REG && src_op.type == OP_MEM) {
        opcode = 0x6F;
        reg_op = &dst_op;
        rm_op = &src_op;
    } else if (dst_op.type == OP_MEM && src_op.type == OP_REG) {
        opcode = 0x7F;
        reg_op = &src_op;
        rm_op = &dst_op;
    } else {
        fputs("Ошибка: Перемещение из памяти в память не поддерживается. Завершение работы.\n", stdout);
        exit(0);
    }

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

    uint8_t p1 = (r << 7) | (x << 6) | (b << 5) | (r_prime << 4) | 0x01;
    uint8_t p2 = (1 << 7) | (0xF << 3) | (1 << 2) | 1;

    int z = dst_op.z_flag ? 1 : 0;
    uint8_t p3 = (z << 7) | (2 << 5) | (0 << 4) | (1 << 3) | dst_op.k_mask;

    if (rm_op->type == OP_REG) {
        x = (rm_op->reg & 0x10) ? 0 : 1;
        p1 = (p1 & ~(1 << 6)) | (x << 6);
    }

    bytes[len++] = p0;
    bytes[len++] = p1;
    bytes[len++] = p2;
    bytes[len++] = p3;
    bytes[len++] = opcode;

    uint8_t mod = 0;
    uint8_t rm = 0;

    if (rm_op->type == OP_REG) {
        mod = 3;
        rm = rm_op->reg & 7;
        bytes[len++] = (mod << 6) | ((reg_op->reg & 7) << 3) | rm;
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

        bytes[len++] = (mod << 6) | ((reg_op->reg & 7) << 3) | rm;

        if (needs_sib) {
            uint8_t ss = 0;
            if (rm_op->scale == 2) ss = 1;
            else if (rm_op->scale == 4) ss = 2;
            else if (rm_op->scale == 8) ss = 3;

            uint8_t index = (rm_op->index_reg != -1) ? (rm_op->index_reg & 7) : 4;
            uint8_t base = rip_rel ? 5 : ((rm_op->base_reg == -1) ? 5 : (rm_op->base_reg & 7));

            bytes[len++] = (ss << 6) | (index << 3) | base;
        }

        if (disp_size == 1) {
            bytes[len++] = (uint8_t)(rm_op->disp / 64);
        } else if (disp_size == 4) {
            bytes[len++] = (uint8_t)(rm_op->disp & 0xFF);
            bytes[len++] = (uint8_t)((rm_op->disp >> 8) & 0xFF);
            bytes[len++] = (uint8_t)((rm_op->disp >> 16) & 0xFF);
            bytes[len++] = (uint8_t)((rm_op->disp >> 24) & 0xFF);
        }
    }

    for (int i = 0; i < len; i++) {
        print_byte_hex(bytes[i]);
    }
    putchar('\n');
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
#endif
    char line[MAX_LINE];
    fputs("Введите команду AVX-512 VMOVDQA64 (или пустую строку для выхода):\n", stdout);
    while (fgets(line, sizeof(line), stdin)) {
        size_t len = 0;
        while (line[len]) len++;
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[len-1] = '\0';
            len--;
        }
        if (len == 0) {
            break;
        }
        process_instruction(line);
        fputs("Введите следующую команду:\n", stdout);
    }
    return 0;
}
