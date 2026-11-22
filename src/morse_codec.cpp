#include "morse_codec.h"
#include <ctype.h>

namespace {
const char TREE[] = "  " "ET" "IANM" "SURWDKGO" "HVF L PJBXCYZQ  " "54 3   2       16       7   8 90";
bool put(char *out, size_t cap, size_t &len, char c) { if (len + 1 >= cap) return false; out[len++] = c; out[len] = '\0'; return true; }
int lookup(char c) { for (int i = 2; i <= 63; ++i) if (TREE[i] == c) return i; return 0; }
}

bool morseTextToBits(const char *text, char *bits, size_t cap) {
  if (!text || !bits || !cap) return false;
  size_t len = 0; bits[0] = '\0'; bool haveLetter = false, wordGap = false;
  for (const char *p = text; *p; ++p) {
    unsigned char c = static_cast<unsigned char>(*p);
    if (isspace(c)) { if (haveLetter) wordGap = true; continue; }
    int index = lookup(static_cast<char>(toupper(c)));
    if (!index) continue;
    if (haveLetter) for (int i = 0; i < (wordGap ? 7 : 3); ++i) if (!put(bits, cap, len, '0')) return false;
    int path[6], count = 0;
    for (int n = index; n > 1; n >>= 1) path[count++] = n & 1;
    for (int i = count - 1; i >= 0; --i) {
      for (int j = 0; j < (path[i] ? 3 : 1); ++j) if (!put(bits, cap, len, '1')) return false;
      if (i && !put(bits, cap, len, '0')) return false;
    }
    haveLetter = true; wordGap = false;
  }
  return len != 0;
}

bool morseBitsToText(const char *bits, char *text, size_t cap) {
  if (!bits || !text || !cap) return false;
  size_t len = 0, pos = 0; int node = 1; text[0] = '\0';
  while (bits[pos]) {
    char value = bits[pos]; if (value != '0' && value != '1') return false;
    size_t end = pos; while (bits[end] == value) ++end;
    size_t run = end - pos;
    if (value == '1') {
      if (run == 1 || run == 3) { if (node) { node = 2 * node + (run == 3); if (node > 63) node = 0; } }
      else node = 0;
    } else {
      // A single zero separates Morse marks inside one character. Three or
      // seven zeros terminate the character; seven also separates words.
      if (run == 1) {
        // Keep walking the current Morse tree path.
      } else if (run == 3 || run == 7) {
        if (node != 1) { if (!put(text, cap, len, node && TREE[node] != ' ' ? TREE[node] : '?')) return false; node = 1; }
        if (run == 7 && !put(text, cap, len, ' ')) return false;
      } else {
        if (node != 1 && !put(text, cap, len, '?')) return false;
        node = 1;
        if (!put(text, cap, len, '?')) return false;
      }
    }
    pos = end;
  }
  if (node != 1 && !put(text, cap, len, node && TREE[node] != ' ' ? TREE[node] : '?')) return false;
  if (len && text[len - 1] == ' ') text[--len] = '\0';
  return len != 0;
}
