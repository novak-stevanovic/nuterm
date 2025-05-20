#include "uconv.h"
#include <stdio.h>

void print_utf8_seq(uint8_t* seq, size_t len)
{
    if(len == 0) return;

    size_t i;
    printf("%.*s", (int)len, seq);
    printf(" (");
    for(i = 0; i < len - 1; i++) printf("%x ", seq[i]);
    printf("%x", seq[i]);
    printf(")");
}

void print_utf32_seq(uint32_t* seq, size_t width)
{
    size_t i;
    for(i = 0; i < width; i++) printf("%x ", seq[i]);
}

int main(int argc, char *argv[])
{
    unsigned char seq1[] = { 0x41 };               // “A” (U+0041)
    unsigned char seq2[] = { 0x7A };               // “z” (U+007A)

    unsigned char seq3[] = { 0xC3, 0xA9 };         // “é” (U+00E9)
    unsigned char seq4[] = { 0xC3, 0x9F };         // “ß” (U+00DF)

    unsigned char seq5[] = { 0xE2, 0x82, 0xAC };   // “€” (U+20AC)
    unsigned char seq6[] = { 0xE0, 0xA4, 0xB9 };   // “ह” (U+0939)
                                                   //
    unsigned char seq7[] = { 0xF0, 0x9F, 0x98, 0x80 }; // “😀” (U+1F600)

     unsigned char seq8[] = {
        0x48, 0x65, 0x6C, 0x6C, 0x6F
    }; // length = 5

    unsigned char seq9[] = {
        0x41,                   /* 'A' (U+0041)       */
        0xC3, 0xA9,             /* 'é' (U+00E9)       */
        0xE2, 0x82, 0xAC,       /* '€' (U+20AC)       */
        0xF0, 0x9F, 0x98, 0x80  /* '😀' (U+1F600)     */
    }; // length = 10

    unsigned char seq10[] = {
        0xD0,0x9F, /* П (U+041F) */
        0xD1,0x80, /* р (U+0440) */
        0xD0,0xB8, /* и (U+0438) */
        0xD0,0xB2, /* в (U+0432) */
        0xD0,0xB5, /* е (U+0435) */
        0xD1,0x82  /* т (U+0442) */
    }; // length = 12

    unsigned char seq11[] = {
        0xF0,0x9F,0x98,0x80,  /* 😀 U+1F600 */
        0xF0,0x9F,0x98,0x82,  /* 😂 U+1F602 */
        0xF0,0x9F,0x91,0x8D   /* 👍 U+1F44D */
    }; // length = 12

    unsigned char seq12[] = {
        0xE0,0xA4,0xA8,  /* न U+0928 */
        0xE0,0xA4,0xAE,  /* म U+092E */
        0xE0,0xA4,0xB8,  /* स U+0938 */
        0xE0,0xA5,0x8D,  /* ् U+094D */
        0xE0,0xA4,0xA4,  /* त U+0924 */
        0xE0,0xA5,0x87   /* े U+0947 */
    }; // length = 18
    
    // 0xD800 - surrogate start
    unsigned char seq13[] = {
        0xED, 0xA0, 0x80
    };

    // overlong + surrogate start
    unsigned char seq14[] = {
        0xF0, 0x80, 0x80, 0x81,
        0xED, 0xA0, 0x80
    };

    uint32_t cps[20];
    uint8_t seq[50];
    uc_status_t _status;

#define SEQ seq11

    printf("--------------------------------------------------------------\n");
    printf("[UTF-8] Starting sequence: ");
    print_utf8_seq(SEQ, sizeof(SEQ));
    printf(" - length: %ld", sizeof(SEQ));
    printf("\n");

    printf("--------------------------------------------------------------\n");
    printf("[UTF-8 -> UTF-32] Converting...\n");
    size_t width1;
    uc_utf8_to_utf32(SEQ, sizeof(SEQ), cps, 20, 0, &width1, &_status);
    printf("[UTF-8 -> UTF-32] Result: width - %ld | status - %d\n",
            width1, _status);
    printf("[UTF-8 -> UTF-32] Resulting codepoints: ");
    print_utf32_seq(cps, width1);
    printf("\n");

    if(_status == UC_SUCCESS)
    {
        printf("--------------------------------------------------------------\n");
        printf("[UTF-32 -> UTF-8] Converting back...\n");
        size_t width2, len2;
        uc_utf32_to_utf8(cps, width1, seq, 50, 0, &width2, &len2, &_status);
        printf("[UTF-32 -> UTF-8] Result: width - %ld | len - %ld | status - %d\n",
                width2, len2, _status);
        printf("[UTF-32 -> UTF-8] Resulting sequence: ");
        print_utf8_seq(seq, len2);
        printf("\n");
    }

    return 0;
}
