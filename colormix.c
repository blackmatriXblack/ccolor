#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_COLORS 32

typedef struct {
    const char *name;
    int r, g, b;
} Color;

static Color palette[MAX_COLORS] = {
    {"red",        255,   0,   0},
    {"green",        0, 255,   0},
    {"blue",         0,   0, 255},
    {"yellow",     255, 255,   0},
    {"cyan",         0, 255, 255},
    {"magenta",    255,   0, 255},
    {"white",      255, 255, 255},
    {"black",        0,   0,   0},
    {"orange",     255, 165,   0},
    {"purple",     128,   0, 128},
    {"pink",       255, 192, 203},
    {"brown",      139,  69,  19},
    {"gray",       128, 128, 128},
    {"lime",        50, 205,  50},
    {"teal",         0, 128, 128},
    {"navy",         0,   0, 128},
    {"maroon",     128,   0,   0},
    {"olive",      128, 128,   0},
    {"silver",     192, 192, 192},
    {"coral",      255, 127,  80},
    {"gold",       255, 215,   0},
    {"violet",     238, 130, 238},
    {"indigo",      75,   0, 130},
    {"turquoise",   64, 224, 208},
    {"salmon",     250, 128, 114},
    {"plum",       221, 160, 221},
    {"tan",        210, 180, 140},
    {"khaki",      240, 230, 140},
    {"lavender",   230, 230, 250},
    {"mint",       189, 252, 201},
    {"peach",      255, 229, 180},
    {"skyblue",    135, 206, 235},
};

static void str_tolower(char *s) {
    for (; *s; s++) *s = (char)tolower((unsigned char)*s);
}

static Color *find_color(const char *name) {
    for (int i = 0; i < MAX_COLORS; i++) {
        if (strcmp(palette[i].name, name) == 0) return &palette[i];
    }
    return NULL;
}

static const char *nearest_color(int r, int g, int b) {
    double best = INFINITY;
    const char *best_name = NULL;
    for (int i = 0; i < MAX_COLORS; i++) {
        double dr = palette[i].r - r;
        double dg = palette[i].g - g;
        double db = palette[i].b - b;
        double dist = dr * dr + dg * dg + db * db;
        if (dist < best) { best = dist; best_name = palette[i].name; }
    }
    return best_name;
}

static void show_color_block(int r, int g, int b, const char *label) {
    printf("\033[48;2;%d;%d;%dm      \033[0m", r, g, b);
    printf("  %-14s  ", label);
    printf("\033[48;2;%d;%d;%dm      \033[0m", r, g, b);
}

static void show_bar(int r, int g, int b) {
    printf("  ");
    for (int i = 0; i < 40; i++) printf("\033[48;2;%d;%d;%dm \033[0m", r, g, b);
}

static void print_usage(void) {
    printf("ColorMix - Mix two colors and display the result\n\n");
    printf("Usage: colormix <color1> + <color2>\n");
    printf("       colormix <color1> <color2>\n");
    printf("       colormix --list\n\n");
    printf("Examples:\n");
    printf("  colormix red + yellow\n");
    printf("  colormix blue green\n");
}

static void list_colors(void) {
    printf("Available colors:\n\n");
    for (int i = 0; i < MAX_COLORS; i++) {
        show_color_block(palette[i].r, palette[i].g, palette[i].b, palette[i].name);
        printf("\n");
    }
}

int main(int argc, char **argv) {
    if (argc < 2) { print_usage(); return 1; }

    if (strcmp(argv[1], "--list") == 0 || strcmp(argv[1], "-l") == 0) {
        list_colors();
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage();
        return 0;
    }

    char c1[32], c2[32];

    if (argc == 4 && strcmp(argv[2], "+") == 0) {
        strncpy(c1, argv[1], sizeof(c1) - 1); c1[sizeof(c1) - 1] = '\0';
        strncpy(c2, argv[3], sizeof(c2) - 1); c2[sizeof(c2) - 1] = '\0';
    } else if (argc == 3) {
        strncpy(c1, argv[1], sizeof(c1) - 1); c1[sizeof(c1) - 1] = '\0';
        strncpy(c2, argv[2], sizeof(c2) - 1); c2[sizeof(c2) - 1] = '\0';
    } else {
        print_usage();
        return 1;
    }

    str_tolower(c1);
    str_tolower(c2);

    Color *col1 = find_color(c1);
    Color *col2 = find_color(c2);

    if (!col1) { printf("Unknown color: %s\n", c1); return 1; }
    if (!col2) { printf("Unknown color: %s\n", c2); return 1; }

    int mr = (col1->r + col2->r) / 2;
    int mg = (col1->g + col2->g) / 2;
    int mb = (col1->b + col2->b) / 2;

    const char *result_name = nearest_color(mr, mg, mb);

    printf("\n");
    show_color_block(col1->r, col1->g, col1->b, col1->name);
    printf("  +  ");
    show_color_block(col2->r, col2->g, col2->b, col2->name);
    printf("  =  ");
    show_color_block(mr, mg, mb, result_name);
    printf("\n\n");

    printf("  RGB(%d, %d, %d)", col1->r, col1->g, col1->b);
    printf(" + RGB(%d, %d, %d)", col2->r, col2->g, col2->b);
    printf(" = RGB(%d, %d, %d)\n\n", mr, mg, mb);

    printf("  Result: ");
    show_bar(mr, mg, mb);
    printf("  %s\n\n", result_name);

    return 0;
}
