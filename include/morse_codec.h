#pragma once
#include <stddef.h>
bool morseTextToBits(const char *text, char *bits, size_t capacity);
bool morseBitsToText(const char *bits, char *text, size_t capacity);
