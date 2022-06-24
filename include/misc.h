#include <Arduino.h>

char *rtrim(char *s) {
	char *p = s + strlen(s) - 1;
	const char *end = p;
	while (p >= s && isspace((unsigned char) *p)) {
		p--;
	}
	if (p != end) {
        p++;
		*p = '\0';
    }
	return s;
}
