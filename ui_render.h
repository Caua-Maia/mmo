#ifndef UI_RENDER_H
#define UI_RENDER_H

#include "raylib.h"
#include "game_logic.h"

#define SCREEN_W 1280
#define SCREEN_H 720

/* Paleta Darkest Dungeon */
extern const Color COLOR_BG;
extern const Color COLOR_BG_DEEP;
extern const Color COLOR_PANEL;
extern const Color COLOR_PANEL_BORDER;
extern const Color COLOR_PANEL_INNER;
extern const Color COLOR_GOLD;
extern const Color COLOR_GOLD_DIM;
extern const Color COLOR_BLOOD;
extern const Color COLOR_HP;
extern const Color COLOR_HP_BG;
extern const Color COLOR_ENERGY;
extern const Color COLOR_MANA;
extern const Color COLOR_SUPREMA;
extern const Color COLOR_FAITH;
extern const Color COLOR_DEF;
extern const Color COLOR_TEXT;
extern const Color COLOR_TEXT_DIM;
extern const Color COLOR_FRIEND;
extern const Color COLOR_PLAYER_NAME;
extern const Color COLOR_MYSTERY;
extern const Color COLOR_BTN;
extern const Color COLOR_BTN_HOVER;
extern const Color COLOR_BTN_BORDER;

typedef struct {
    Rectangle rect;
    const char *label;
    int enabled;
} UiButton;

void ui_init(void);
void ui_shutdown(void);
Font ui_font(void);
Font ui_font_title(void);

void ui_draw_background(float time);
void ui_draw_vignette(void);
void ui_draw_panel(Rectangle r, const char *title);
void ui_draw_panel_boss(Rectangle r, const char *title);
void ui_draw_bar(Rectangle r, float ratio, Color fill, Color back, const char *label);
int ui_button(UiButton btn); /* 1 se clicado neste frame */
int ui_button_key(UiButton btn, int key);

void ui_draw_enemy_portrait(Rectangle area, const struct inimigo *e);
void ui_draw_player_hud(Rectangle area);
void ui_draw_card(Rectangle r, const CombatCard *card, int selected);
void ui_draw_budget(Rectangle r, int budget, int max_budget);
void ui_draw_floating(const FloatingText *arr);
void ui_draw_damage_flash(float intensity);
void ui_draw_wrapped_text(Font font, const char *text, Rectangle box, float size, Color color);

#endif
