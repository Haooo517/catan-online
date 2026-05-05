#pragma once

#include <stdio.h>
#include <stdlib.h>

int start_menu(void);
void rule_menu(void);
int main_menu(void);
int build_menu(void);
int trade_menu(void);
int pause_menu(void);
char getch(void);
int read_int(const char *prompt, int min, int max);
