#include <unistd.h>

void print_str(const char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    int unused = write(1, str, len);
    (void)unused;
}

int strings_equal(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return 0;
        s1++;
        s2++;
    }
    return (*s1 == '\0' && *s2 == '\0');
}

int main() {
    print_str("Добро пожаловать в Windows 12!\n");
    print_str("Введите 'помощь' для списка команд.\n");

    char buffer[256];

    while (1) {
        print_str("C:\\Windows12> ");

        int pos = 0;
        char c;
        while (1) {
            int bytes_read = read(0, &c, 1);
            if (bytes_read <= 0) {
                if (pos == 0) return 0;
                break;
            }
            if (c == '\n' || c == '\r') {
                if (pos > 0 || c == '\n') break; // Если мы прочитали \r, следущим может быть \n, но в простых терминалах \n достаточно
                continue;
            }
            if (pos < 255) {
                buffer[pos++] = c;
            }
        }
        buffer[pos] = '\0';

        if (strings_equal(buffer, "выход")) {
            print_str("Завершение работы Windows 12...\n");
            break;
        } else if (strings_equal(buffer, "помощь")) {
            print_str("Доступные команды:\n");
            print_str("  помощь - Показать это сообщение\n");
            print_str("  версия - Показать версию системы\n");
            print_str("  выход  - Выйти из системы\n");
        } else if (strings_equal(buffer, "версия")) {
            print_str("Windows 12 (Сборка 1.0.0 Экстремальная Производительность)\n");
        } else if (buffer[0] != '\0') {
            print_str("Неизвестная команда. Введите 'помощь' для списка команд.\n");
        }
    }
    return 0;
}
