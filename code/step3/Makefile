# you should not need to modify this.
CC =gcc
CFLAGS = -Og -g -Wall -Wno-unused-variable

all: trojan-cc2 generate-attack-array attack-array.h trojan-cc2.check login-attacked

trojan-cc2: trojan-cc2.c attack.c attack-array.h
	$(CC) $(CFLAGS) $< -o $@

generate-attack-array: generate-attack-array.c
	$(CC) $(CFLAGS) $< -o $@

attack-array.h: attack.c generate-attack-array
	./generate-attack-array < $< > $@

trojan-cc2.check: trojan-cc2
	./trojan-cc2 ../step2/identity-cc.c -o cc-attacked
	./cc-attacked ../step2/identity-cc.c -o cc-attacked2
	diff cc-attacked cc-attacked2

login-attacked: trojan-cc2
	./cc-attacked2 ../step2/login.c -o $@

clean:
	rm -f cc-attacked cc-attacked2 login-attacked login-attacked2 out login identity-cc trojan-cc1 trojan-cc2 *.o a.out  *~ cc-attacked.c login-attacked.c out.c temp-out.c temp-out.c generate-attack-array
