#include "ui_render.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

const Color COLOR_BG          = { 18, 14, 12, 255 };
const Color COLOR_BG_DEEP     = { 8, 6, 5, 255 };
const Color COLOR_PANEL       = { 32, 26, 22, 230 };
const Color COLOR_PANEL_BORDER= { 90, 70, 42, 255 };
const Color COLOR_PANEL_INNER = { 55, 42, 28, 255 };
const Color COLOR_GOLD        = { 196, 160, 72, 255 };
const Color COLOR_GOLD_DIM    = { 140, 110, 50, 255 };
const Color COLOR_BLOOD       = { 140, 24, 28, 255 };
const Color COLOR_HP          = { 160, 36, 36, 255 };
const Color COLOR_HP_BG       = { 40, 18, 18, 255 };
const Color COLOR_ENERGY      = { 200, 120, 40, 255 };
const Color COLOR_DEF         = { 90, 120, 150, 255 };
const Color COLOR_TEXT        = { 210, 198, 170, 255 };
const Color COLOR_TEXT_DIM    = { 140, 130, 110, 255 };
const Color COLOR_FRIEND      = { 200, 150, 55, 255 };
const Color COLOR_PLAYER_NAME = { 110, 170, 100, 255 };
const Color COLOR_MYSTERY     = { 180, 60, 70, 255 };
const Color COLOR_BTN         = { 40, 32, 26, 255 };
const Color COLOR_BTN_HOVER   = { 58, 46, 34, 255 };
const Color COLOR_BTN_BORDER  = { 120, 90, 48, 255 };

static Font g_font;
static Font g_font_title;
static int g_has_custom;

void ui_init(void) {
    g_has_custom = 0;
    if (FileExists("assets/Cinzel-Bold.ttf")) {
        g_font_title = LoadFontEx("assets/Cinzel-Bold.ttf", 64, 0, 250);
        g_has_custom = 1;
    } else {
        g_font_title = GetFontDefault();
    }
    if (FileExists("assets/IMFellEnglish-Regular.ttf")) {
        g_font = LoadFontEx("assets/IMFellEnglish-Regular.ttf", 32, 0, 250);
        g_has_custom = 1;
    } else if (FileExists("assets/Cinzel-Regular.ttf")) {
        g_font = LoadFontEx("assets/Cinzel-Regular.ttf", 32, 0, 250);
        g_has_custom = 1;
    } else {
        g_font = GetFontDefault();
    }
    SetTextureFilter(g_font.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(g_font_title.texture, TEXTURE_FILTER_BILINEAR);
}

void ui_shutdown(void) {
    if (g_has_custom) {
        if (g_font.glyphCount > 0) UnloadFont(g_font);
        if (g_font_title.glyphCount > 0) UnloadFont(g_font_title);
    }
}

Font ui_font(void) { return g_font; }
Font ui_font_title(void) { return g_font_title; }

void ui_draw_background(float time) {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, COLOR_BG_DEEP);

    /* Corredor de pedra estilizado */
    DrawRectangle(0, SCREEN_H - 120, SCREEN_W, 120, (Color){ 28, 22, 18, 255 });
    DrawRectangle(0, 0, 90, SCREEN_H, (Color){ 22, 18, 14, 255 });
    DrawRectangle(SCREEN_W - 90, 0, 90, SCREEN_H, (Color){ 22, 18, 14, 255 });

    /* Arcos / pedras */
    for (int i = 0; i < 8; i++) {
        int x = 100 + i * 140;
        DrawRectangle(x, 80, 18, SCREEN_H - 200, (Color){ 35, 28, 22, 255 });
        DrawRectangleLines(x, 80, 18, SCREEN_H - 200, (Color){ 50, 40, 30, 255 });
    }

    /* Tochas com flicker */
    float flicker = 0.7f + 0.3f * sinf(time * 7.0f);
    Color torch = (Color){ (unsigned char)(180 * flicker), (unsigned char)(90 * flicker), 20, 180 };
    DrawCircle(70, 220, 28, torch);
    DrawCircle(SCREEN_W - 70, 220, 28, torch);
    DrawCircle(70, 420, 22, torch);
    DrawCircle(SCREEN_W - 70, 420, 22, torch);

    /* Brilho quente central sutil */
    DrawRectangle(200, 100, SCREEN_W - 400, SCREEN_H - 220, (Color){ 40, 28, 18, 40 });
}

void ui_draw_vignette(void) {
    int thick = 90;
    for (int i = 0; i < thick; i++) {
        float a = (float)i / thick;
        unsigned char alpha = (unsigned char)(a * a * 200);
        Color c = { 0, 0, 0, alpha };
        DrawRectangle(0, i, SCREEN_W, 1, c);
        DrawRectangle(0, SCREEN_H - 1 - i, SCREEN_W, 1, c);
        DrawRectangle(i, 0, 1, SCREEN_H, c);
        DrawRectangle(SCREEN_W - 1 - i, 0, 1, SCREEN_H, c);
    }
}

static void draw_ornament_corners(Rectangle r, Color c) {
    float s = 14;
    /* cantos reforçados */
    DrawRectangle((int)r.x, (int)r.y, (int)s, 3, c);
    DrawRectangle((int)r.x, (int)r.y, 3, (int)s, c);
    DrawRectangle((int)(r.x + r.width - s), (int)r.y, (int)s, 3, c);
    DrawRectangle((int)(r.x + r.width - 3), (int)r.y, 3, (int)s, c);
    DrawRectangle((int)r.x, (int)(r.y + r.height - 3), (int)s, 3, c);
    DrawRectangle((int)r.x, (int)(r.y + r.height - s), 3, (int)s, c);
    DrawRectangle((int)(r.x + r.width - s), (int)(r.y + r.height - 3), (int)s, 3, c);
    DrawRectangle((int)(r.x + r.width - 3), (int)(r.y + r.height - s), 3, (int)s, c);
}

void ui_draw_panel(Rectangle r, const char *title) {
    DrawRectangleRec(r, COLOR_PANEL);
    DrawRectangleLinesEx(r, 4, COLOR_PANEL_BORDER);
    DrawRectangleLinesEx((Rectangle){ r.x + 6, r.y + 6, r.width - 12, r.height - 12 }, 1, COLOR_PANEL_INNER);
    draw_ornament_corners(r, COLOR_GOLD_DIM);

    if (title && title[0]) {
        float size = 22;
        Vector2 ts = MeasureTextEx(g_font_title, title, size, 1);
        float tx = r.x + (r.width - ts.x) / 2;
        DrawRectangle((int)(tx - 12), (int)(r.y - 2), (int)(ts.x + 24), 28, COLOR_BG);
        DrawTextEx(g_font_title, title, (Vector2){ tx, r.y + 2 }, size, 1, COLOR_GOLD);
    }
}

void ui_draw_panel_boss(Rectangle r, const char *title) {
    DrawRectangleRec(r, (Color){ 40, 18, 18, 235 });
    DrawRectangleLinesEx(r, 5, COLOR_BLOOD);
    DrawRectangleLinesEx((Rectangle){ r.x + 7, r.y + 7, r.width - 14, r.height - 14 }, 1, (Color){ 120, 50, 40, 255 });
    draw_ornament_corners(r, COLOR_GOLD);
    if (title && title[0]) {
        float size = 24;
        Vector2 ts = MeasureTextEx(g_font_title, title, size, 1);
        float tx = r.x + (r.width - ts.x) / 2;
        DrawRectangle((int)(tx - 12), (int)(r.y - 2), (int)(ts.x + 24), 30, COLOR_BG_DEEP);
        DrawTextEx(g_font_title, title, (Vector2){ tx, r.y + 2 }, size, 1, COLOR_GOLD);
    }
}

void ui_draw_bar(Rectangle r, float ratio, Color fill, Color back, const char *label) {
    if (ratio < 0) ratio = 0;
    if (ratio > 1) ratio = 1;
    DrawRectangleRec(r, back);
    DrawRectangle((int)r.x, (int)r.y, (int)(r.width * ratio), (int)r.height, fill);
    DrawRectangleLinesEx(r, 2, COLOR_PANEL_BORDER);
    if (label) {
        DrawTextEx(g_font, label, (Vector2){ r.x + 6, r.y + 2 }, 16, 1, COLOR_TEXT);
    }
}

int ui_button(UiButton btn) {
    Vector2 m = GetMousePosition();
    int hover = CheckCollisionPointRec(m, btn.rect) && btn.enabled;
    Color fill = hover ? COLOR_BTN_HOVER : COLOR_BTN;
    Color border = btn.enabled ? COLOR_BTN_BORDER : COLOR_TEXT_DIM;
    Color text = btn.enabled ? COLOR_TEXT : COLOR_TEXT_DIM;

    DrawRectangleRec(btn.rect, fill);
    DrawRectangleLinesEx(btn.rect, 3, border);
    draw_ornament_corners(btn.rect, border);

    if (btn.label) {
        float size = 20;
        Vector2 ts = MeasureTextEx(g_font, btn.label, size, 1);
        Vector2 pos = {
            btn.rect.x + (btn.rect.width - ts.x) / 2,
            btn.rect.y + (btn.rect.height - ts.y) / 2
        };
        DrawTextEx(g_font, btn.label, pos, size, 1, text);
    }

    if (!btn.enabled) return 0;
    return hover && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

int ui_button_key(UiButton btn, int key) {
    int clicked = ui_button(btn);
    if (btn.enabled && IsKeyPressed(key)) return 1;
    return clicked;
}

static Color enemy_tint(const struct inimigo *e) {
    if (e->eh_boss) {
        if (e->tipo == 1) return (Color){ 120, 110, 220, 255 };
        if (e->tipo == 2) return (Color){ 210, 190, 40, 255 };
        return (Color){ 210, 70, 80, 255 };
    }
    if (e->tipo == 1) return (Color){ 140, 190, 210, 255 };
    if (e->tipo == 2) return (Color){ 170, 100, 220, 255 };
    return (Color){ 80, 190, 90, 255 };
}

void ui_draw_enemy_portrait(Rectangle area, const struct inimigo *e) {
    Color tint = enemy_tint(e);
    float cx = area.x + area.width / 2;
    float cy = area.y + area.height / 2 + 10;
    float scale = e->eh_boss ? 1.25f : 1.0f;

    DrawRectangleRec(area, (Color){ 20, 14, 12, 200 });
    DrawRectangleLinesEx(area, 2, tint);

    if (e->eh_boss) {
        if (e->tipo == 1) {
            DrawCircle((int)(cx - 40 * scale), (int)(cy - 40), 18 * scale, tint);
            DrawCircle((int)(cx + 40 * scale), (int)(cy - 40), 18 * scale, tint);
            DrawRectangle((int)(cx - 50 * scale), (int)cy, (int)(100 * scale), (int)(70 * scale), tint);
            DrawTextEx(g_font_title, ":)", (Vector2){ cx - 18, cy + 10 }, 36 * scale, 1, COLOR_BG_DEEP);
        } else if (e->tipo == 2) {
            DrawRectangle((int)(cx - 35 * scale), (int)(cy - 70), (int)(70 * scale), (int)(20 * scale), COLOR_GOLD);
            DrawRectangle((int)(cx - 45 * scale), (int)(cy - 50), (int)(90 * scale), (int)(100 * scale), tint);
            DrawCircle((int)(cx - 18 * scale), (int)(cy - 20), 8, COLOR_BG_DEEP);
            DrawCircle((int)(cx + 18 * scale), (int)(cy - 20), 8, COLOR_BG_DEEP);
        } else {
            DrawRectangle((int)(cx - 55 * scale), (int)(cy - 30), (int)(110 * scale), (int)(90 * scale), tint);
            DrawRectangle((int)(cx - 30 * scale), (int)(cy - 70), (int)(60 * scale), (int)(40 * scale), tint);
            DrawCircle((int)(cx - 12 * scale), (int)(cy - 50), 6, COLOR_BG_DEEP);
            DrawCircle((int)(cx + 12 * scale), (int)(cy - 50), 6, COLOR_BG_DEEP);
        }
    } else if (e->tipo == 1) {
        DrawEllipse((int)cx, (int)cy, 55 * scale, 45 * scale, tint);
        DrawCircle((int)(cx - 18), (int)(cy - 8), 6, COLOR_BG_DEEP);
        DrawCircle((int)(cx + 18), (int)(cy - 8), 6, COLOR_BG_DEEP);
        DrawRectangle((int)(cx - 12), (int)(cy + 8), 24, 4, COLOR_BG_DEEP);
    } else if (e->tipo == 2) {
        DrawRectangle((int)(cx - 40), (int)(cy - 55), 80, 90, tint);
        DrawCircle((int)(cx - 15), (int)(cy - 25), 7, COLOR_BG_DEEP);
        DrawCircle((int)(cx + 15), (int)(cy - 25), 7, COLOR_BG_DEEP);
        DrawRectangle((int)(cx - 55), (int)(cy - 10), 20, 50, tint);
        DrawRectangle((int)(cx + 35), (int)(cy - 10), 20, 50, tint);
    } else {
        DrawTriangle((Vector2){ cx, cy - 50 }, (Vector2){ cx - 50, cy + 40 }, (Vector2){ cx + 50, cy + 40 }, tint);
        DrawCircle((int)(cx - 12), (int)(cy - 10), 5, COLOR_BG_DEEP);
        DrawCircle((int)(cx + 12), (int)(cy - 10), 5, COLOR_BG_DEEP);
        DrawRectangle((int)(cx - 8), (int)(cy + 5), 16, 18, COLOR_BG_DEEP);
    }

    char hp[64];
    snprintf(hp, sizeof(hp), "%s", e->nome);
    Vector2 ns = MeasureTextEx(g_font, hp, 20, 1);
    DrawTextEx(g_font, hp, (Vector2){ area.x + (area.width - ns.x) / 2, area.y + 8 }, 20, 1, COLOR_GOLD);

    snprintf(hp, sizeof(hp), "HP %d / %d", e->vida, e->vida_max);
    float ratio = e->vida_max > 0 ? (float)e->vida / e->vida_max : 0;
    ui_draw_bar((Rectangle){ area.x + 20, area.y + area.height - 36, area.width - 40, 22 },
                ratio, COLOR_HP, COLOR_HP_BG, hp);
}

void ui_draw_player_hud(Rectangle area) {
    char buf[80];
    ui_draw_panel(area, "AVENTUREIRO");

    snprintf(buf, sizeof(buf), "%s  Nv.%d", jogador.nome_classe, jogador.nivel);
    DrawTextEx(g_font, buf, (Vector2){ area.x + 20, area.y + 36 }, 18, 1, COLOR_TEXT);

    snprintf(buf, sizeof(buf), "HP %d/%d", jogador.vida, jogador.vida_max);
    ui_draw_bar((Rectangle){ area.x + 20, area.y + 70, area.width - 40, 22 },
                jogador.vida_max ? (float)jogador.vida / jogador.vida_max : 0,
                COLOR_HP, COLOR_HP_BG, buf);

    snprintf(buf, sizeof(buf), "ENERGIA %d/%d", jogador.energia, jogador.energia_max);
    ui_draw_bar((Rectangle){ area.x + 20, area.y + 100, area.width - 40, 22 },
                jogador.energia_max ? (float)jogador.energia / jogador.energia_max : 0,
                COLOR_ENERGY, (Color){ 40, 28, 14, 255 }, buf);

    snprintf(buf, sizeof(buf), "DEF %d/%d   ATK %d   R$ %.0f",
             jogador.def, jogador.def_max, jogador.arma.dano, jogador.moeda);
    DrawTextEx(g_font, buf, (Vector2){ area.x + 20, area.y + 136 }, 16, 1, COLOR_TEXT_DIM);

    snprintf(buf, sizeof(buf), "Arma: %s", jogador.arma.nome);
    DrawTextEx(g_font, buf, (Vector2){ area.x + 20, area.y + 158 }, 16, 1, COLOR_GOLD_DIM);
}

void ui_draw_floating(const FloatingText *arr) {
    int i;
    for (i = 0; i < FLOAT_TEXT_MAX; i++) {
        if (arr[i].life <= 0) continue;
        float a = arr[i].life / 1.4f;
        if (a > 1) a = 1;
        Color c = { arr[i].r, arr[i].g, arr[i].b, (unsigned char)(a * 255) };
        float size = arr[i].is_crit ? 28.0f : 22.0f;
        DrawTextEx(g_font_title, arr[i].text, (Vector2){ arr[i].x, arr[i].y }, size, 1, c);
    }
}

void ui_draw_damage_flash(float intensity) {
    if (intensity <= 0) return;
    if (intensity > 1) intensity = 1;
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){ 160, 20, 20, (unsigned char)(intensity * 120) });
}

void ui_draw_wrapped_text(Font font, const char *text, Rectangle box, float size, Color color) {
    if (!text) return;
    char line[256];
    int li = 0;
    float y = box.y;
    float spacing = size + 4;

    for (const char *p = text; ; p++) {
        char ch = *p;
        int end = (ch == '\0' || ch == '\n' || ch == ' ');
        if (!end) {
            if (li < (int)sizeof(line) - 1) line[li++] = ch;
            continue;
        }
        line[li] = '\0';
        if (li > 0) {
            Vector2 m = MeasureTextEx(font, line, size, 1);
            if (m.x > box.width && li > 1) {
                /* palavra grande demais — desenha e quebra */
            }
            /* Tentar encaixar palavra na linha atual acumulada: abordagem simples por palavra */
        }

        /* Reimplementação simples palavra a palavra */
        break;
    }

    /* Word wrap simples */
    {
        const char *p = text;
        char word[64];
        int wi;
        line[0] = '\0';
        li = 0;
        while (*p) {
            if (*p == '\n') {
                line[li] = '\0';
                DrawTextEx(font, line, (Vector2){ box.x, y }, size, 1, color);
                y += spacing;
                if (y > box.y + box.height - spacing) return;
                li = 0;
                line[0] = '\0';
                p++;
                continue;
            }
            wi = 0;
            while (*p && *p != ' ' && *p != '\n' && wi < 63) word[wi++] = *p++;
            word[wi] = '\0';
            while (*p == ' ') p++;

            char trial[256];
            if (li == 0) snprintf(trial, sizeof(trial), "%s", word);
            else snprintf(trial, sizeof(trial), "%s %s", line, word);

            Vector2 m = MeasureTextEx(font, trial, size, 1);
            if (m.x > box.width && li > 0) {
                line[li] = '\0';
                DrawTextEx(font, line, (Vector2){ box.x, y }, size, 1, color);
                y += spacing;
                if (y > box.y + box.height - spacing) return;
                snprintf(line, sizeof(line), "%s", word);
                li = (int)strlen(line);
            } else {
                snprintf(line, sizeof(line), "%s", trial);
                li = (int)strlen(line);
            }
        }
        if (li > 0) {
            DrawTextEx(font, line, (Vector2){ box.x, y }, size, 1, color);
        }
    }
}
