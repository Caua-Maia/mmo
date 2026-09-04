#include "raylib.h"
#include "game_logic.h"
#include "ui_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum {
    SCREEN_MAIN_MENU,
    SCREEN_DIALOGUE,
    SCREEN_CLASS_SELECT,
    SCREEN_WEAPON_SELECT,
    SCREEN_ENTER,
    SCREEN_PATH_CHOICE,
    SCREEN_CONTINUE,
    SCREEN_ENEMY_ALERT,
    SCREEN_COMBAT,
    SCREEN_ABILITY,
    SCREEN_COMBAT_ITEMS,
    SCREEN_COMBAT_FEEDBACK,
    SCREEN_VICTORY,
    SCREEN_BOSS_ALERT,
    SCREEN_POST_BOSS,
    SCREEN_LEVEL_UP_BANNER,
    SCREEN_LEVEL_REWARD,
    SCREEN_MERCHANT,
    SCREEN_CHEST,
    SCREEN_CHEST_CONFIRM,
    SCREEN_CARD,
    SCREEN_STATUS,
    SCREEN_INVENTORY,
    SCREEN_INVENTORY_CONFIRM,
    SCREEN_MAGIC_DOOR,
    SCREEN_ESCAPE_END,
    SCREEN_DEATH
} GameScreen;

typedef struct {
    GameScreen screen;
    GameScreen return_to;

    /* diálogo */
    int dialogue_step;
    int dialogue_reply; /* -1 pendente, 1 sim, 2 nao */

    /* intro narrative */
    int enter_step;
    float fade;

    /* combate */
    struct inimigo enemy;
    int combat_escudo;
    int combat_postura;
    int combat_turno;
    int combat_is_boss;
    int awaiting_enemy;
    float feedback_timer;
    char feedback_msg[MSG_LEN];
    char feedback_msg2[MSG_LEN];
    int flash_player;
    float damage_flash;
    FloatingText floats[FLOAT_TEXT_MAX];

    /* vitória / xp */
    int last_xp;
    int last_gold;
    int pending_level_ups;

    /* path / events */
    float transition_timer;
    int elite_force;

    /* post boss / door */
    int door_step;
    int continue_after_boss; /* 1 sim, 2 nao */

    /* merchant / chest */
    int shop_sel;
    int chest_sel;
    int chest_carta;
    char toast[MSG_LEN];
    float toast_timer;

    /* inventory */
    int inv_sel;

    /* armas cache */
    struct arma armas[3];

    float enemy_turn_delay;
    int running;
} AppState;

static AppState app;

static void set_screen(GameScreen s) {
    app.screen = s;
    app.fade = 0;
}

static void toast(const char *msg) {
    snprintf(app.toast, sizeof(app.toast), "%s", msg);
    app.toast_timer = 2.0f;
}

static void start_combat(struct inimigo e) {
    app.enemy = e;
    app.combat_escudo = 0;
    app.combat_postura = 0;
    app.combat_turno = 0;
    app.combat_is_boss = e.eh_boss;
    app.awaiting_enemy = 0;
    floating_clear(app.floats);
    set_screen(SCREEN_COMBAT);
}

static void after_path_event(void) {
    int ev = sortear_resultado_caminho();
    if (ev == 0) {
        app.elite_force = 0;
        set_screen(SCREEN_ENEMY_ALERT);
    } else if (ev == 1) {
        set_screen(SCREEN_CONTINUE);
        app.transition_timer = 1.2f;
    } else if (ev == 2) {
        app.chest_sel = -1;
        set_screen(SCREEN_CHEST);
    } else if (ev == 3) {
        app.shop_sel = -1;
        set_screen(SCREEN_MERCHANT);
    } else {
        app.elite_force = 1;
        set_screen(SCREEN_ENEMY_ALERT);
    }
}

static void begin_enemy_from_alert(void) {
    Tier tier = app.elite_force ? TIER_ELITE : sortear_tier();
    int tipo = sortear_tipo_inimigo();
    start_combat(criar_inimigo(tipo, tier));
}

static void resolve_victory_flow(void) {
    conceder_vitoria(&app.enemy, &app.last_gold, &app.last_xp);
    set_screen(SCREEN_VICTORY);
}

static void handle_victory_ok(void) {
    if (jogador.xp >= jogador.xp_para_proximo) {
        jogador.xp -= jogador.xp_para_proximo;
        aplicar_stats_nivel();
        set_screen(SCREEN_LEVEL_UP_BANNER);
        app.transition_timer = 1.6f;
        return;
    }
    if (app.combat_is_boss) {
        boss_derrotado = 0; /* mensagem já será mostrada agora */
        set_screen(SCREEN_POST_BOSS);
    } else {
        set_screen(SCREEN_PATH_CHOICE);
    }
}

static void after_level_reward(void) {
    if (jogador.xp >= jogador.xp_para_proximo) {
        jogador.xp -= jogador.xp_para_proximo;
        aplicar_stats_nivel();
        set_screen(SCREEN_LEVEL_UP_BANNER);
        app.transition_timer = 1.6f;
        return;
    }
    if (app.combat_is_boss) {
        boss_derrotado = 0;
        set_screen(SCREEN_POST_BOSS);
    } else {
        set_screen(SCREEN_PATH_CHOICE);
    }
}

static void draw_toast(void) {
    if (app.toast_timer <= 0) return;
    Rectangle r = { SCREEN_W / 2.0f - 220, 40, 440, 50 };
    DrawRectangleRec(r, (Color){ 20, 12, 10, 220 });
    DrawRectangleLinesEx(r, 2, COLOR_GOLD);
    Vector2 ts = MeasureTextEx(ui_font(), app.toast, 18, 1);
    DrawTextEx(ui_font(), app.toast, (Vector2){ r.x + (r.width - ts.x) / 2, r.y + 14 }, 18, 1, COLOR_TEXT);
}

/* ---------- SCREENS ---------- */

static void update_main_menu(void) {
    UiButton play = { { SCREEN_W / 2.0f - 200, 400, 180, 56 }, "JOGAR", 1 };
    UiButton quit = { { SCREEN_W / 2.0f + 20, 400, 180, 56 }, "SAIR", 1 };

    BeginDrawing();
    ui_draw_background(GetTime());
    {
        const char *title = "DUNGEON QUEST";
        Vector2 ts = MeasureTextEx(ui_font_title(), title, 56, 1);
        DrawTextEx(ui_font_title(), title, (Vector2){ (SCREEN_W - ts.x) / 2, 180 }, 56, 1, COLOR_GOLD);
        DrawTextEx(ui_font(), "Um eco no abismo...", (Vector2){ SCREEN_W / 2.0f - 90, 260 }, 20, 1, COLOR_TEXT_DIM);
    }
    if (ui_button_key(play, KEY_ONE) || IsKeyPressed(KEY_ENTER)) {
        app.dialogue_step = 0;
        app.dialogue_reply = -1;
        game_reset_run();
        set_screen(SCREEN_DIALOGUE);
    }
    if (ui_button_key(quit, KEY_TWO) || IsKeyPressed(KEY_ESCAPE)) {
        app.running = 0;
    }
    ui_draw_vignette();
    EndDrawing();
}

static void update_dialogue(void) {
    UiButton cont = { { SCREEN_W / 2.0f - 110, 620, 220, 48 }, "Continuar", 1 };
    UiButton yes = { { SCREEN_W / 2.0f - 200, 620, 180, 48 }, "1: Sim", 1 };
    UiButton no = { { SCREEN_W / 2.0f + 20, 620, 180, 48 }, "2: Nao", 1 };
    UiButton enter = { { SCREEN_W / 2.0f - 140, 620, 280, 48 }, "Entrar na Masmorra", 1 };

    BeginDrawing();
    ui_draw_background(GetTime());

    Rectangle box = { 160, 120, SCREEN_W - 320, 460 };
    ui_draw_panel(box, "FORA DA MASMORRA");

    if (app.dialogue_step == 0) {
        DrawTextEx(ui_font(), "AMIGO", (Vector2){ 200, 160 }, 22, 1, COLOR_FRIEND);
        ui_draw_wrapped_text(ui_font(),
            "Faz tempo que a gente nao entra numa masmorra, isso me faz lembrar "
            "do nosso comeco, sabe, quando ainda eramos iniciantes nesse negocio de aventureiro...",
            (Rectangle){ 200, 190, box.width - 80, 80 }, 18, COLOR_TEXT);

        DrawTextEx(ui_font(), "JOGADOR", (Vector2){ 280, 290 }, 22, 1, COLOR_PLAYER_NAME);
        ui_draw_wrapped_text(ui_font(),
            "Voce disse que nao entraria nessa masmorra nem que te pagassem...  ._.",
            (Rectangle){ 280, 320, box.width - 160, 60 }, 18, COLOR_TEXT);

        DrawTextEx(ui_font(), "AMIGO", (Vector2){ 200, 400 }, 22, 1, COLOR_FRIEND);
        ui_draw_wrapped_text(ui_font(),
            "Nao e culpa minha se eu tenho medo de esqueleto... Dizem que essa masmorra e bem dificil, "
            "mas eu sei que voce vai dar conta...",
            (Rectangle){ 200, 430, box.width - 80, 80 }, 18, COLOR_TEXT);

        if (ui_button_key(cont, KEY_ZERO) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            app.dialogue_step = 1;
        }
    } else if (app.dialogue_step == 1) {
        DrawTextEx(ui_font(), "AMIGO", (Vector2){ 200, 200 }, 22, 1, COLOR_FRIEND);
        ui_draw_wrapped_text(ui_font(), "Voce ainda lembra como funciona, certo?",
            (Rectangle){ 200, 240, box.width - 80, 60 }, 20, COLOR_TEXT);
        if (ui_button_key(yes, KEY_ONE)) { app.dialogue_reply = 1; app.dialogue_step = 2; }
        if (ui_button_key(no, KEY_TWO)) { app.dialogue_reply = 2; app.dialogue_step = 2; }
    } else {
        DrawTextEx(ui_font(), "AMIGO", (Vector2){ 200, 180 }, 22, 1, COLOR_FRIEND);
        if (app.dialogue_reply == 1) {
            ui_draw_wrapped_text(ui_font(),
                "Entao nao preciso me preocupar, boa sorte, sei que voce vai conseguir!",
                (Rectangle){ 200, 220, box.width - 80, 120 }, 20, COLOR_TEXT);
        } else {
            ui_draw_wrapped_text(ui_font(),
                "Nao e dificil: mate inimigos, fique mais forte e derrote o chefao. "
                "Esse e o objetivo principal... Simples! Vou te esperar do lado de fora. Boa sorte!",
                (Rectangle){ 200, 220, box.width - 80, 160 }, 20, COLOR_TEXT);
        }
        if (ui_button_key(enter, KEY_ZERO) || IsKeyPressed(KEY_ENTER)) {
            app.enter_step = 0;
            app.transition_timer = 2.0f;
            set_screen(SCREEN_ENTER);
        }
    }

    ui_draw_vignette();
    EndDrawing();
}

static void update_enter(void) {
    const char *msgs[] = {
        "BEM VINDO AO DUNGEON QUEST",
        "AQUI SE INICIA SEU CAMINHO PELA DUNGEON!",
        "DERROTE OS INIMIGOS E FIQUE MAIS FORTE!"
    };
    app.transition_timer -= GetFrameTime();
    BeginDrawing();
    ui_draw_background(GetTime());
    Rectangle r = { 240, 260, SCREEN_W - 480, 140 };
    ui_draw_panel(r, NULL);
    if (app.enter_step < 3) {
        Vector2 ts = MeasureTextEx(ui_font_title(), msgs[app.enter_step], 26, 1);
        DrawTextEx(ui_font_title(), msgs[app.enter_step],
            (Vector2){ r.x + (r.width - ts.x) / 2, r.y + 50 }, 26, 1, COLOR_GOLD);
    }
    float a = app.transition_timer < 0.4f ? (0.4f - app.transition_timer) / 0.4f : 0;
    if (a > 0) DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){ 0, 0, 0, (unsigned char)(a * 255) });
    ui_draw_vignette();
    EndDrawing();

    if (app.transition_timer <= 0) {
        app.enter_step++;
        if (app.enter_step >= 3) {
            set_screen(SCREEN_CLASS_SELECT);
        } else {
            app.transition_timer = 2.0f;
        }
    }
}

static void update_class_select(void) {
    UiButton g = { { 140, 280, 300, 160 }, "1: Guerreiro", 1 };
    UiButton l = { { 490, 280, 300, 160 }, "2: Ladino", 1 };
    UiButton m = { { 840, 280, 300, 160 }, "3: Mago", 1 };

    BeginDrawing();
    ui_draw_background(GetTime());
    DrawTextEx(ui_font_title(), "Escolha sua classe", (Vector2){ 420, 100 }, 36, 1, COLOR_GOLD);

    if (ui_button_key(g, KEY_ONE)) { aplicar_classe(CLASSE_GUERREIRO); preencher_armas_classe(app.armas); set_screen(SCREEN_WEAPON_SELECT); }
    DrawTextEx(ui_font(), "Vida e defesa altas,\ngolpe pesado", (Vector2){ 170, 360 }, 16, 1, COLOR_TEXT_DIM);

    if (ui_button_key(l, KEY_TWO)) { aplicar_classe(CLASSE_LADINO); preencher_armas_classe(app.armas); set_screen(SCREEN_WEAPON_SELECT); }
    DrawTextEx(ui_font(), "Acerto, critico e fuga", (Vector2){ 530, 360 }, 16, 1, COLOR_TEXT_DIM);

    if (ui_button_key(m, KEY_THREE)) { aplicar_classe(CLASSE_MAGO); preencher_armas_classe(app.armas); set_screen(SCREEN_WEAPON_SELECT); }
    DrawTextEx(ui_font(), "Magias fortes,\ncorpo fragil", (Vector2){ 890, 360 }, 16, 1, COLOR_TEXT_DIM);

    ui_draw_vignette();
    EndDrawing();
}

static void update_weapon_select(void) {
    char label[64];
    BeginDrawing();
    ui_draw_background(GetTime());
    {
        char t[80];
        snprintf(t, sizeof(t), "Arma inicial (%s)", jogador.nome_classe);
        DrawTextEx(ui_font_title(), t, (Vector2){ 360, 80 }, 32, 1, COLOR_GOLD);
    }

    for (int i = 0; i < 3; i++) {
        snprintf(label, sizeof(label), "%d: %s  (ATK %d)", i + 1, app.armas[i].nome, app.armas[i].dano);
        UiButton b = { { 340, 180.0f + i * 110, 600, 80 }, label, 1 };
        int key = KEY_ONE + i;
        if (ui_button_key(b, key)) {
            equipar_arma(&app.armas[i]);
            set_screen(SCREEN_PATH_CHOICE);
        }
    }

    if (jogador.classe == CLASSE_GUERREIRO)
        DrawTextEx(ui_font(), "Espada: critico. Machado: dano. Maca: +10 DEF.", (Vector2){ 340, 540 }, 18, 1, COLOR_TEXT_DIM);
    else if (jogador.classe == CLASSE_LADINO)
        DrawTextEx(ui_font(), "Adaga: crit. Duplas: 2 golpes. Estoque: fuga.", (Vector2){ 340, 540 }, 18, 1, COLOR_TEXT_DIM);
    else
        DrawTextEx(ui_font(), "Cajado: simples. Grimorio: magias baratas. Orbe: curas.", (Vector2){ 340, 540 }, 18, 1, COLOR_TEXT_DIM);

    ui_draw_vignette();
    EndDrawing();
}

static void update_path_choice(void) {
    if (boss_derrotado) {
        boss_derrotado = 0;
        app.continue_after_boss = 0;
        set_screen(SCREEN_POST_BOSS);
        return;
    }
    if (niveis_desde_boss >= 3) {
        set_screen(SCREEN_BOSS_ALERT);
        app.transition_timer = 1.4f;
        return;
    }
    if (jogador.vida <= 0) {
        set_screen(SCREEN_DEATH);
        return;
    }

    UiButton frente = { { SCREEN_W / 2.0f - 100, 220, 200, 70 }, "1: FRENTE", 1 };
    UiButton esq = { { 280, 360, 200, 70 }, "2: ESQUERDA", 1 };
    UiButton dir = { { SCREEN_W - 480.0f, 360, 200, 70 }, "3: DIREITA", 1 };
    UiButton pack = { { SCREEN_W - 280.0f, 80, 220, 56 }, "4: MOCHILA", 1 };

    BeginDrawing();
    ui_draw_background(GetTime());
    ui_draw_panel((Rectangle){ 200, 140, SCREEN_W - 400, 400 }, "QUAL CAMINHO?");

    DrawTextEx(ui_font(), "O corredor se divide nas trevas...", (Vector2){ 420, 180 }, 18, 1, COLOR_TEXT_DIM);

    char xp[64];
    snprintf(xp, sizeof(xp), "EXP %d/%d   NV %d", jogador.xp, jogador.xp_para_proximo, jogador.nivel);
    DrawTextEx(ui_font(), xp, (Vector2){ 220, 80 }, 20, 1, COLOR_GOLD);

    if (ui_button_key(frente, KEY_ONE) || ui_button_key(esq, KEY_TWO) || ui_button_key(dir, KEY_THREE)) {
        after_path_event();
    }
    if (ui_button_key(pack, KEY_FOUR)) {
        app.return_to = SCREEN_PATH_CHOICE;
        app.inv_sel = -1;
        set_screen(SCREEN_INVENTORY);
    }

    ui_draw_player_hud((Rectangle){ 40, SCREEN_H - 200.0f, 420, 180 });
    ui_draw_vignette();
    draw_toast();
    EndDrawing();
}

static void update_continue_walk(void) {
    app.transition_timer -= GetFrameTime();
    BeginDrawing();
    ui_draw_background(GetTime());
    Rectangle r = { 390, 280, 500, 120 };
    ui_draw_panel(r, NULL);
    Vector2 ts = MeasureTextEx(ui_font_title(), "Continue Caminhando", 28, 1);
    DrawTextEx(ui_font_title(), "Continue Caminhando",
        (Vector2){ r.x + (r.width - ts.x) / 2, r.y + 40 }, 28, 1, (Color){ 73, 209, 198, 255 });
    ui_draw_vignette();
    EndDrawing();
    if (app.transition_timer <= 0) set_screen(SCREEN_PATH_CHOICE);
}

static void update_enemy_alert(void) {
    UiButton go = { { SCREEN_W / 2.0f - 110, 480, 220, 50 }, "Enfrentar", 1 };
    BeginDrawing();
    ui_draw_background(GetTime());
    Rectangle r = { 340, 240, 600, 160 };
    ui_draw_panel(r, "ALERTA");
    const char *msg = app.elite_force
        ? "Uma emboscada! Algo forte surgiu"
        : "Um inimigo apareceu na sua frente";
    Vector2 ts = MeasureTextEx(ui_font(), msg, 22, 1);
    DrawTextEx(ui_font(), msg, (Vector2){ r.x + (r.width - ts.x) / 2, r.y + 60 }, 22, 1, COLOR_BLOOD);
    if (ui_button_key(go, KEY_ENTER) || IsKeyPressed(KEY_SPACE)) begin_enemy_from_alert();
    ui_draw_vignette();
    EndDrawing();
}

static void update_boss_alert(void) {
    app.transition_timer -= GetFrameTime();
    BeginDrawing();
    ui_draw_background(GetTime());
    Rectangle r = { 340, 250, 600, 140 };
    ui_draw_panel_boss(r, "BOSS");
    Vector2 ts = MeasureTextEx(ui_font(), "Um Boss apareceu na sua frente", 22, 1);
    DrawTextEx(ui_font(), "Um Boss apareceu na sua frente",
        (Vector2){ r.x + (r.width - ts.x) / 2, r.y + 55 }, 22, 1, COLOR_TEXT);
    ui_draw_vignette();
    EndDrawing();
    if (app.transition_timer <= 0) {
        int vazio = 0;
        int tipo = escolher_proximo_boss(&vazio);
        start_combat(criar_boss(tipo, vazio));
    }
}

static void show_feedback(const ActionResult *r, int against_player) {
    snprintf(app.feedback_msg, sizeof(app.feedback_msg), "%s", r->msg);
    snprintf(app.feedback_msg2, sizeof(app.feedback_msg2), "%s", r->msg2);
    app.feedback_timer = 1.3f;
    set_screen(SCREEN_COMBAT_FEEDBACK);

    char dmg[32];
    if (r->miss) {
        floating_spawn(app.floats, SCREEN_W / 2.0f, 260, "ERROU", 0, 0);
    } else if (r->heal > 0) {
        snprintf(dmg, sizeof(dmg), "+%d", r->heal);
        floating_spawn(app.floats, 280, 400, dmg, 0, 1);
    } else if (r->damage > 0) {
        snprintf(dmg, sizeof(dmg), "-%d", r->damage);
        if (against_player) {
            floating_spawn(app.floats, 280, 380, dmg, r->crit, 0);
            app.damage_flash = 0.45f;
        } else {
            floating_spawn(app.floats, SCREEN_W - 360.0f, 280, dmg, r->crit, 0);
            if (r->crit) floating_spawn(app.floats, SCREEN_W - 380.0f, 240, "CRITICO!", 1, 0);
        }
    } else if (r->msg[0]) {
        floating_spawn(app.floats, SCREEN_W / 2.0f - 40, 300, r->msg, 0, 0);
    }
}

static void combat_end_check_after_player(int skip_enemy) {
    if (app.enemy.vida <= 0) {
        resolve_victory_flow();
        return;
    }
    if (jogador.vida <= 0) {
        set_screen(SCREEN_DEATH);
        return;
    }
    if (skip_enemy) {
        set_screen(SCREEN_COMBAT);
        return;
    }
    app.awaiting_enemy = 1;
}

static void do_enemy_turn(void) {
    app.combat_turno++;
    ActionResult r = enemy_attack(&app.enemy, &app.combat_escudo, &app.combat_postura, app.combat_turno);
    app.awaiting_enemy = 0;
    show_feedback(&r, 1);
    app.flash_player = 1;
}

static void update_combat(void) {
    if (app.awaiting_enemy) {
        app.enemy_turn_delay += GetFrameTime();
        if (app.enemy_turn_delay > 0.35f) {
            app.enemy_turn_delay = 0;
            do_enemy_turn();
            return;
        }
    } else {
        app.enemy_turn_delay = 0;
    }

    UiButton atk = { { 80, 520, 200, 56 }, "1: ATACAR", 1 };
    UiButton hab = { { 300, 520, 200, 56 }, "2: HABILIDADE", 1 };
    UiButton item = { { 520, 520, 200, 56 }, "3: ITEM", 1 };
    UiButton fugir = { { 740, 520, 200, 56 },
        app.combat_is_boss ? "4: (sem fuga)" : "4: FUGIR",
        app.combat_is_boss ? 0 : 1 };

    BeginDrawing();
    ui_draw_background(GetTime());

    if (app.combat_is_boss)
        ui_draw_panel_boss((Rectangle){ 60, 40, SCREEN_W - 120.0f, 450 }, "COMBATE — BOSS");
    else
        ui_draw_panel((Rectangle){ 60, 40, SCREEN_W - 120.0f, 450 }, "COMBATE");

    ui_draw_player_hud((Rectangle){ 90, 80, 420, 200 });
    ui_draw_enemy_portrait((Rectangle){ SCREEN_W - 520.0f, 80, 400, 320 }, &app.enemy);

    if (!app.awaiting_enemy) {
        if (ui_button_key(atk, KEY_ONE)) {
            ActionResult r = player_basic_attack(&app.enemy);
            show_feedback(&r, 0);
            combat_end_check_after_player(0);
            /* show_feedback already changed screen; mark enemy turn after feedback */
            if (app.enemy.vida > 0 && jogador.vida > 0) app.awaiting_enemy = 1;
        }
        if (ui_button_key(hab, KEY_TWO)) set_screen(SCREEN_ABILITY);
        if (ui_button_key(item, KEY_THREE)) set_screen(SCREEN_COMBAT_ITEMS);
        if (!app.combat_is_boss && ui_button_key(fugir, KEY_FOUR)) {
            char msg[MSG_LEN];
            if (tentar_fugir(msg, sizeof(msg))) {
                toast(msg);
                set_screen(SCREEN_PATH_CHOICE);
            } else {
                ActionResult r = {0};
                snprintf(r.msg, MSG_LEN, "%s", msg);
                show_feedback(&r, 1);
                app.awaiting_enemy = 1;
            }
        } else if (app.combat_is_boss && IsKeyPressed(KEY_FOUR)) {
            ActionResult r = {0};
            snprintf(r.msg, MSG_LEN, "Voce nao pode fugir dessa vez :)");
            show_feedback(&r, 0);
            /* não gasta turno inimigo no original? Na verdade só mostra e continua — inimigo não age no original quando tenta fugir de boss */
            app.awaiting_enemy = 0;
            set_screen(SCREEN_COMBAT);
            toast("Voce nao pode fugir dessa vez :)");
        }
    }

    floating_update(app.floats, GetFrameTime());
    ui_draw_floating(app.floats);
    if (app.damage_flash > 0) {
        ui_draw_damage_flash(app.damage_flash);
        app.damage_flash -= GetFrameTime();
    }
    ui_draw_vignette();
    EndDrawing();
}

static void update_ability(void) {
    int c3 = custo_energia(3), c2 = custo_energia(2);
    char l1[64], l2[64], l3[64];

    if (jogador.classe == CLASSE_GUERREIRO) {
        snprintf(l1, sizeof(l1), "1: Golpe Pesado (%d)", c3);
        snprintf(l2, sizeof(l2), "2: Postura (%d)", c2);
        snprintf(l3, sizeof(l3), "3: Descansar");
    } else if (jogador.classe == CLASSE_LADINO) {
        snprintf(l1, sizeof(l1), "1: Punhalada (%d)", c2);
        snprintf(l2, sizeof(l2), "2: Fumaca (%d)", c2);
        snprintf(l3, sizeof(l3), "3: Descansar");
    } else {
        snprintf(l1, sizeof(l1), "1: Bola de Fogo (%d)", c3);
        snprintf(l2, sizeof(l2), "2: Cura (%d)", c2);
        snprintf(l3, sizeof(l3), "3: Escudo Arcano (%d)", c2);
    }

    UiButton b1 = { { 400, 200, 480, 60 }, l1, 1 };
    UiButton b2 = { { 400, 280, 480, 60 }, l2, 1 };
    UiButton b3 = { { 400, 360, 480, 60 }, l3, 1 };
    UiButton back = { { 400, 460, 480, 56 }, "0: Voltar", 1 };

    BeginDrawing();
    ui_draw_background(GetTime());
    ui_draw_panel((Rectangle){ 300, 100, 680, 480 }, "HABILIDADES");
    {
        char e[40];
        snprintf(e, sizeof(e), "Energia: %d/%d", jogador.energia, jogador.energia_max);
        DrawTextEx(ui_font(), e, (Vector2){ 420, 150 }, 20, 1, COLOR_ENERGY);
    }

    int choice = 0;
    if (ui_button_key(b1, KEY_ONE)) choice = 1;
    if (ui_button_key(b2, KEY_TWO)) choice = 2;
    if (ui_button_key(b3, KEY_THREE)) choice = 3;
    if (ui_button_key(back, KEY_ZERO)) { set_screen(SCREEN_COMBAT); choice = -1; }

    if (choice > 0) {
        ActionResult r = player_ability(&app.enemy, choice, &app.combat_escudo, &app.combat_postura);
        if (!r.spent_turn) {
            toast(r.msg);
        } else {
            show_feedback(&r, 0);
            if (app.enemy.vida <= 0) resolve_victory_flow();
            else if (jogador.vida <= 0) set_screen(SCREEN_DEATH);
            else if (r.skip_enemy) app.awaiting_enemy = 0;
            else app.awaiting_enemy = 1;
        }
    }

    ui_draw_vignette();
    EndDrawing();
}

static void update_combat_items(void) {
    BeginDrawing();
    ui_draw_background(GetTime());
    ui_draw_panel((Rectangle){ 280, 80, 720, 560 }, "ITENS (COMBATE)");

    for (int s = 0; s < MOCHILA_TAM; s++) {
        char lab[80];
        if (jogador.mochila[s].id == ITEM_VAZIO) snprintf(lab, sizeof(lab), "%d: ---", s + 1);
        else snprintf(lab, sizeof(lab), "%d: %s x%d", s + 1, nome_item(jogador.mochila[s].id), jogador.mochila[s].qtd);
        UiButton b = { { 360, 130.0f + s * 48, 560, 42 }, lab, jogador.mochila[s].id != ITEM_VAZIO };
        if (ui_button(b) || (IsKeyPressed(KEY_ONE + s) && s < 8 && jogador.mochila[s].id != ITEM_VAZIO)) {
            char msg[MSG_LEN];
            if (usar_item_slot(s, &app.enemy, 1, msg, sizeof(msg))) {
                ActionResult r = {0};
                snprintf(r.msg, MSG_LEN, "%s", msg);
                r.spent_turn = 1;
                show_feedback(&r, 0);
                if (app.enemy.vida <= 0) resolve_victory_flow();
                else app.awaiting_enemy = 1;
            } else {
                toast(msg);
            }
        }
    }
    UiButton back = { { 360, 560, 560, 48 }, "0: Voltar", 1 };
    if (ui_button_key(back, KEY_ZERO)) set_screen(SCREEN_COMBAT);

    ui_draw_vignette();
    draw_toast();
    EndDrawing();
}

static void update_combat_feedback(void) {
    app.feedback_timer -= GetFrameTime();
    floating_update(app.floats, GetFrameTime());
    if (app.damage_flash > 0) app.damage_flash -= GetFrameTime();

    BeginDrawing();
    ui_draw_background(GetTime());
    if (app.combat_is_boss)
        ui_draw_panel_boss((Rectangle){ 60, 40, SCREEN_W - 120.0f, 450 }, "COMBATE — BOSS");
    else
        ui_draw_panel((Rectangle){ 60, 40, SCREEN_W - 120.0f, 450 }, "COMBATE");
    ui_draw_player_hud((Rectangle){ 90, 80, 420, 200 });
    ui_draw_enemy_portrait((Rectangle){ SCREEN_W - 520.0f, 80, 400, 320 }, &app.enemy);

    Rectangle banner = { 340, 480, 600, 100 };
    ui_draw_panel(banner, NULL);
    DrawTextEx(ui_font(), app.feedback_msg, (Vector2){ 370, 505 }, 22, 1, COLOR_GOLD);
    if (app.feedback_msg2[0])
        DrawTextEx(ui_font(), app.feedback_msg2, (Vector2){ 370, 540 }, 18, 1, COLOR_TEXT);

    ui_draw_floating(app.floats);
    ui_draw_damage_flash(app.damage_flash);
    ui_draw_vignette();
    EndDrawing();

    if (app.feedback_timer <= 0) {
        if (app.enemy.vida <= 0) resolve_victory_flow();
        else if (jogador.vida <= 0) set_screen(SCREEN_DEATH);
        else set_screen(SCREEN_COMBAT);
    }
}

static void update_victory(void) {
    UiButton ok = { { SCREEN_W / 2.0f - 110, 480, 220, 50 }, "Continuar", 1 };
    BeginDrawing();
    ui_draw_background(GetTime());
    Rectangle r = { 340, 200, 600, 220 };
    ui_draw_panel(r, app.combat_is_boss ? "VITORIA — BOSS" : "VITORIA");
    const char *msg = app.combat_is_boss ? "Voce matou o Boss" : "Voce matou o inimigo";
    Vector2 ts = MeasureTextEx(ui_font_title(), msg, 28, 1);
    DrawTextEx(ui_font_title(), msg, (Vector2){ r.x + (r.width - ts.x) / 2, r.y + 60 }, 28, 1, COLOR_GOLD);
    char info[80];
    snprintf(info, sizeof(info), "+%d EXP    +%d moedas", app.last_xp, app.last_gold);
    Vector2 is = MeasureTextEx(ui_font(), info, 22, 1);
    DrawTextEx(ui_font(), info, (Vector2){ r.x + (r.width - is.x) / 2, r.y + 120 }, 22, 1, COLOR_TEXT);
    if (ui_button_key(ok, KEY_ENTER) || IsKeyPressed(KEY_SPACE)) handle_victory_ok();
    ui_draw_vignette();
    EndDrawing();
}

static void update_level_banner(void) {
    app.transition_timer -= GetFrameTime();
    BeginDrawing();
    ui_draw_background(GetTime());
    Rectangle r = { 390, 260, 500, 140 };
    ui_draw_panel(r, NULL);
    Vector2 ts = MeasureTextEx(ui_font_title(), "Voce subiu de nivel", 30, 1);
    DrawTextEx(ui_font_title(), "Voce subiu de nivel",
        (Vector2){ r.x + (r.width - ts.x) / 2, r.y + 50 }, 30, 1, COLOR_GOLD);
    ui_draw_vignette();
    EndDrawing();
    if (app.transition_timer <= 0) set_screen(SCREEN_LEVEL_REWARD);
}

static void update_level_reward(void) {
    UiButton a = { { 200, 240, 360, 70 }, "1: +2 de ATK", 1 };
    UiButton b = { { 200, 330, 360, 70 }, "2: +4 DEF maxima", 1 };
    UiButton c = { { 200, 420, 360, 70 }, "3: +1 Sorte (acerto)", 1 };

    BeginDrawing();
    ui_draw_background(GetTime());
    DrawTextEx(ui_font_title(), "Escolha uma recompensa", (Vector2){ 400, 100 }, 32, 1, COLOR_GOLD);
    ui_draw_panel((Rectangle){ 620, 220, 420, 280 }, "STATUS ATUAL");
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "Vida: %d/%d\nATK: %d\nDEF: %d/%d\nNivel: %d",
                 jogador.vida, jogador.vida_max, jogador.arma.dano,
                 jogador.def, jogador.def_max, jogador.nivel);
        ui_draw_wrapped_text(ui_font(), buf, (Rectangle){ 650, 270, 360, 200 }, 20, COLOR_TEXT);
    }
    if (ui_button_key(a, KEY_ONE)) { aplicar_recompensa_nivel(1); after_level_reward(); }
    if (ui_button_key(b, KEY_TWO)) { aplicar_recompensa_nivel(2); after_level_reward(); }
    if (ui_button_key(c, KEY_THREE)) { aplicar_recompensa_nivel(3); after_level_reward(); }
    ui_draw_vignette();
    EndDrawing();
}

static void update_post_boss(void) {
    BeginDrawing();
    ui_draw_background(GetTime());
    ui_draw_panel((Rectangle){ 200, 100, SCREEN_W - 400.0f, 360 }, "???");
    ui_draw_wrapped_text(ui_font(),
        "Parabens, voce derrotou os meus servos, mas o tesouro nao esta aqui, "
        "ele esta em algum lugar mais em baixo do abismo...",
        (Rectangle){ 240, 160, SCREEN_W - 480.0f, 160 }, 22, COLOR_MYSTERY);

    DrawTextEx(ui_font(), "Gostaria de continuar?", (Vector2){ 480, 360 }, 20, 1, COLOR_TEXT);
    UiButton yes = { { 400, 480, 200, 56 }, "1: Sim", 1 };
    UiButton no = { { 680, 480, 200, 56 }, "2: Nao", 1 };
    if (ui_button_key(yes, KEY_ONE)) {
        app.door_step = 0;
        app.continue_after_boss = 1;
        app.transition_timer = 2.5f;
        set_screen(SCREEN_MAGIC_DOOR);
    }
    if (ui_button_key(no, KEY_TWO)) {
        app.door_step = 0;
        app.continue_after_boss = 2;
        app.transition_timer = 2.5f;
        set_screen(SCREEN_MAGIC_DOOR);
    }
    ui_draw_vignette();
    EndDrawing();
}

static void update_magic_door(void) {
    app.transition_timer -= GetFrameTime();
    const char *lines_yes[] = {
        "Uma porta aparece magicamente em sua frente.",
        "Voce abre a porta e se encontra em mais um salao daquela masmorra.",
        "Uma voz misteriosa ecoa pela sua mente...",
        "Lute mais uma vez..."
    };
    const char *lines_no[] = {
        "As vezes, fugir e a melhor opcao!",
        "Uma porta aparece magicamente em sua frente.",
        "Voce abre a porta e se encontra do lado de fora da masmorra.",
        "Voce reve a luz do sol mais uma vez!"
    };
    const char **lines = app.continue_after_boss == 1 ? lines_yes : lines_no;

    BeginDrawing();
    ClearBackground(app.continue_after_boss == 1 ? COLOR_BG_DEEP : (Color){ 30, 50, 40, 255 });
    if (app.continue_after_boss != 1) {
        /* sol */
        DrawCircle(SCREEN_W / 2, 160, 80, (Color){ 240, 200, 80, 255 });
        DrawCircle(SCREEN_W / 2, 160, 110, (Color){ 240, 200, 80, 60 });
    } else {
        ui_draw_background(GetTime());
    }

    Rectangle r = { 220, 280, SCREEN_W - 440.0f, 160 };
    ui_draw_panel(r, app.door_step == 3 && app.continue_after_boss == 1 ? "???" : NULL);
    if (app.door_step < 4) {
        ui_draw_wrapped_text(ui_font(), lines[app.door_step],
            (Rectangle){ 260, 320, r.width - 80, 100 }, 22,
            app.continue_after_boss == 1 ? COLOR_MYSTERY : COLOR_PLAYER_NAME);
    }
    ui_draw_vignette();
    EndDrawing();

    if (app.transition_timer <= 0) {
        app.door_step++;
        if (app.door_step >= 4) {
            if (app.continue_after_boss == 1) {
                reset_status_porta_magica();
                set_screen(SCREEN_PATH_CHOICE);
            } else {
                set_screen(SCREEN_ESCAPE_END);
            }
        } else {
            app.transition_timer = 2.5f;
        }
    }
}

static void update_escape_end(void) {
    UiButton menu = { { SCREEN_W / 2.0f - 120, 480, 240, 56 }, "Menu Principal", 1 };
    BeginDrawing();
    ClearBackground((Color){ 25, 45, 35, 255 });
    DrawCircle(SCREEN_W / 2, 150, 70, COLOR_GOLD);
    DrawTextEx(ui_font_title(), "Voce escapou da masmorra", (Vector2){ 360, 280 }, 32, 1, COLOR_GOLD);
    DrawTextEx(ui_font(), "A luz do sol aquece seu rosto mais uma vez.", (Vector2){ 400, 340 }, 20, 1, COLOR_TEXT);
    if (ui_button(menu) || IsKeyPressed(KEY_ENTER)) set_screen(SCREEN_MAIN_MENU);
    EndDrawing();
}

static void update_merchant(void) {
    char lab[80];
    const char *names[] = { "Pocao de Vida", "Pedaco de Armadura", "Pocao de Energia", "Bomba" };
    BeginDrawing();
    ui_draw_background(GetTime());
    ui_draw_panel((Rectangle){ 180, 60, 920, 580 }, "MERCADOR");

    /* NPC face */
    DrawCircle(280, 180, 40, COLOR_GOLD_DIM);
    DrawCircle(268, 172, 5, COLOR_BG);
    DrawCircle(292, 172, 5, COLOR_BG);
    DrawTextEx(ui_font(), "O que deseja, aventureiro?", (Vector2){ 340, 160 }, 20, 1, COLOR_FRIEND);

    char saldo[40];
    snprintf(saldo, sizeof(saldo), "Saldo: R$ %.2f", jogador.moeda);
    DrawTextEx(ui_font_title(), saldo, (Vector2){ 800, 120 }, 24, 1, COLOR_GOLD);

    for (int i = 0; i < 4; i++) {
        snprintf(lab, sizeof(lab), "%d: %s --- R$ %d", i + 1, names[i], preco_mercador(i + 1));
        UiButton b = { { 300, 240.0f + i * 70, 560, 56 }, lab, 1 };
        if (ui_button_key(b, KEY_ONE + i)) {
            char msg[MSG_LEN];
            int rc = comprar_mercador(i + 1, msg, sizeof(msg));
            toast(msg);
            (void)rc;
        }
    }
    UiButton leave = { { 300, 540, 560, 50 }, "0: Sair", 1 };
    if (ui_button_key(leave, KEY_ZERO)) set_screen(SCREEN_PATH_CHOICE);

    draw_toast();
    ui_draw_vignette();
    EndDrawing();
}

static void update_chest(void) {
    const char *opts[] = {
        "1: Pocao de Vida", "2: Pedaco de Armadura", "3: Pocao de Energia",
        "4: Sacola de Moedas", "5: Carta", "0: Deixar o bau"
    };
    BeginDrawing();
    ui_draw_background(GetTime());
    ui_draw_panel((Rectangle){ 300, 60, 680, 580 }, "BAU ENCONTRADO");

    /* chest icon */
    DrawRectangle(560, 120, 160, 70, COLOR_GOLD_DIM);
    DrawRectangle(560, 160, 160, 40, (Color){ 80, 50, 20, 255 });
    DrawRectangle(630, 170, 20, 20, COLOR_GOLD);

    for (int i = 0; i < 6; i++) {
        UiButton b = { { 380, 230.0f + i * 55, 520, 48 }, opts[i], 1 };
        int key = (i < 5) ? KEY_ONE + i : KEY_ZERO;
        if (ui_button_key(b, key)) {
            if (i == 5) { set_screen(SCREEN_PATH_CHOICE); }
            else {
                app.chest_sel = i + 1;
                set_screen(SCREEN_CHEST_CONFIRM);
            }
        }
    }
    ui_draw_vignette();
    EndDrawing();
}

static void update_chest_confirm(void) {
    const char *desc = "";
    ItemId id = ITEM_VAZIO;
    if (app.chest_sel == 1) { id = POCAO_VIDA; desc = desc_item(POCAO_VIDA); }
    else if (app.chest_sel == 2) { id = PEDACO_ARMADURA; desc = desc_item(PEDACO_ARMADURA); }
    else if (app.chest_sel == 3) { id = POCAO_ENERGIA; desc = desc_item(POCAO_ENERGIA); }
    else if (app.chest_sel == 4) { desc = "Uma sacola com 10 moedas."; }
    else if (app.chest_sel == 5) { desc = "Uma simples carta..."; }

    BeginDrawing();
    ui_draw_background(GetTime());
    ui_draw_panel((Rectangle){ 300, 180, 680, 300 }, "DESCRICAO");
    ui_draw_wrapped_text(ui_font(), desc, (Rectangle){ 340, 240, 600, 100 }, 20, COLOR_TEXT);

    UiButton yes = { { 360, 380, 240, 50 }, "1: Pegar / Ler", 1 };
    UiButton no = { { 640, 380, 240, 50 }, "2: Nao", 1 };

    if (ui_button_key(yes, KEY_ONE)) {
        if (app.chest_sel >= 1 && app.chest_sel <= 3) {
            if (!mochila_adicionar(id, 1)) toast("Mochila cheia!");
            else { toast("Item obtido!"); set_screen(SCREEN_PATH_CHOICE); }
        } else if (app.chest_sel == 4) {
            jogador.moeda += 10;
            toast("+10 moedas");
            set_screen(SCREEN_PATH_CHOICE);
        } else if (app.chest_sel == 5) {
            app.chest_carta = entre(1, 5);
            set_screen(SCREEN_CARD);
        }
    }
    if (ui_button_key(no, KEY_TWO)) set_screen(SCREEN_CHEST);

    draw_toast();
    ui_draw_vignette();
    EndDrawing();
}

static void update_card(void) {
    UiButton done = { { SCREEN_W / 2.0f - 120, 620, 240, 48 }, "0: Fechar", 1 };
    BeginDrawing();
    ClearBackground((Color){ 40, 32, 22, 255 });
    Rectangle scroll = { 260, 60, 760, 540 };
    DrawRectangleRec(scroll, (Color){ 210, 190, 150, 255 });
    DrawRectangleLinesEx(scroll, 6, (Color){ 90, 60, 30, 255 });
    DrawTextEx(ui_font_title(), "CARTA", (Vector2){ 560, 80 }, 28, 1, (Color){ 60, 40, 20, 255 });
    ui_draw_wrapped_text(ui_font(), texto_carta(app.chest_carta),
        (Rectangle){ 300, 130, 680, 450 }, 18, (Color){ 40, 30, 20, 255 });
    if (ui_button_key(done, KEY_ZERO) || IsKeyPressed(KEY_ENTER)) set_screen(SCREEN_PATH_CHOICE);
    EndDrawing();
}

static void update_inventory(void) {
    BeginDrawing();
    ui_draw_background(GetTime());
    ui_draw_panel((Rectangle){ 200, 60, 880, 580 }, "MOCHILA");

    for (int s = 0; s < MOCHILA_TAM; s++) {
        int col = s % 4;
        int row = s / 4;
        Rectangle slot = { 260.0f + col * 180, 140.0f + row * 160, 160, 130 };
        int filled = jogador.mochila[s].id != ITEM_VAZIO;
        DrawRectangleRec(slot, filled ? (Color){ 50, 40, 28, 255 } : (Color){ 28, 24, 20, 255 });
        DrawRectangleLinesEx(slot, 3, filled ? COLOR_GOLD : COLOR_PANEL_BORDER);
        char lab[64];
        if (!filled) snprintf(lab, sizeof(lab), "%d\n---", s + 1);
        else snprintf(lab, sizeof(lab), "%d\n%s\nx%d", s + 1, nome_item(jogador.mochila[s].id), jogador.mochila[s].qtd);
        ui_draw_wrapped_text(ui_font(), lab, (Rectangle){ slot.x + 10, slot.y + 20, 140, 100 }, 16, COLOR_TEXT);

        if (CheckCollisionPointRec(GetMousePosition(), slot) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && filled) {
            app.inv_sel = s;
            set_screen(SCREEN_INVENTORY_CONFIRM);
        }
        if (IsKeyPressed(KEY_ONE + s) && filled && s < 8) {
            app.inv_sel = s;
            set_screen(SCREEN_INVENTORY_CONFIRM);
        }
    }

    UiButton st = { { 260, 520, 240, 50 }, "9: Status", 1 };
    UiButton back = { { 520, 520, 240, 50 }, "0: Fechar", 1 };
    if (ui_button_key(st, KEY_NINE)) { app.return_to = SCREEN_INVENTORY; set_screen(SCREEN_STATUS); }
    if (ui_button_key(back, KEY_ZERO)) set_screen(app.return_to ? app.return_to : SCREEN_PATH_CHOICE);

    draw_toast();
    ui_draw_vignette();
    EndDrawing();
}

static void update_inventory_confirm(void) {
    char msg[MSG_LEN];
    BeginDrawing();
    ui_draw_background(GetTime());
    ui_draw_panel((Rectangle){ 360, 200, 560, 280 }, "USAR ITEM?");
    {
        ItemId id = jogador.mochila[app.inv_sel].id;
        char line[80];
        snprintf(line, sizeof(line), "%s\n%s", nome_item(id), desc_item(id));
        ui_draw_wrapped_text(ui_font(), line, (Rectangle){ 400, 260, 480, 100 }, 18, COLOR_TEXT);
    }
    UiButton yes = { { 400, 380, 200, 50 }, "1: Sim", 1 };
    UiButton no = { { 680, 380, 200, 50 }, "2: Nao", 1 };
    if (ui_button_key(yes, KEY_ONE)) {
        if (usar_item_slot(app.inv_sel, NULL, 0, msg, sizeof(msg))) toast(msg);
        else toast(msg);
        set_screen(SCREEN_INVENTORY);
    }
    if (ui_button_key(no, KEY_TWO)) set_screen(SCREEN_INVENTORY);
    ui_draw_vignette();
    EndDrawing();
}

static void update_status(void) {
    UiButton back = { { SCREEN_W / 2.0f - 110, 580, 220, 50 }, "0: Voltar", 1 };
    BeginDrawing();
    ui_draw_background(GetTime());
    ui_draw_panel((Rectangle){ 300, 80, 680, 460 }, "STATUS");
    char buf[512];
    snprintf(buf, sizeof(buf),
        "Level: %d\nClasse: %s\nVida: %d/%d\nEnergia: %d/%d\nEXP: %d/%d\n"
        "DEF: %d/%d\nATK: %d\nAcerto extra: +%d\nArma: %s\nMoedas: R$ %.2f",
        jogador.nivel, jogador.nome_classe, jogador.vida, jogador.vida_max,
        jogador.energia, jogador.energia_max, jogador.xp, jogador.xp_para_proximo,
        jogador.def, jogador.def_max, jogador.arma.dano, jogador.bonus_acerto,
        jogador.arma.nome, jogador.moeda);
    ui_draw_wrapped_text(ui_font(), buf, (Rectangle){ 360, 140, 560, 360 }, 22, COLOR_TEXT);
    if (ui_button_key(back, KEY_ZERO)) set_screen(app.return_to ? app.return_to : SCREEN_PATH_CHOICE);
    ui_draw_vignette();
    EndDrawing();
}

static void update_death(void) {
    UiButton restart = { { SCREEN_W / 2.0f - 220, 480, 200, 56 }, "Reiniciar", 1 };
    UiButton quit = { { SCREEN_W / 2.0f + 20, 480, 200, 56 }, "Sair", 1 };
    BeginDrawing();
    ClearBackground(COLOR_BG_DEEP);
    ui_draw_background(GetTime());
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){ 40, 0, 0, 140 });
    Vector2 ts = MeasureTextEx(ui_font_title(), "Voce morreu", 48, 1);
    DrawTextEx(ui_font_title(), "Voce morreu", (Vector2){ (SCREEN_W - ts.x) / 2, 280 }, 48, 1, COLOR_BLOOD);
    DrawTextEx(ui_font(), "O abismo cobra seu preco.", (Vector2){ SCREEN_W / 2.0f - 120, 360 }, 20, 1, COLOR_TEXT_DIM);
    if (ui_button(restart)) set_screen(SCREEN_MAIN_MENU);
    if (ui_button(quit)) app.running = 0;
    ui_draw_vignette();
    EndDrawing();
}

int main(void) {
    srand((unsigned)time(NULL));
    memset(&app, 0, sizeof(app));
    app.running = 1;
    app.screen = SCREEN_MAIN_MENU;
    app.return_to = SCREEN_PATH_CHOICE;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_W, SCREEN_H, "Dungeon Quest");
    SetTargetFPS(60);
    ui_init();

    while (app.running && !WindowShouldClose()) {
        if (app.toast_timer > 0) app.toast_timer -= GetFrameTime();

        switch (app.screen) {
            case SCREEN_MAIN_MENU: update_main_menu(); break;
            case SCREEN_DIALOGUE: update_dialogue(); break;
            case SCREEN_ENTER: update_enter(); break;
            case SCREEN_CLASS_SELECT: update_class_select(); break;
            case SCREEN_WEAPON_SELECT: update_weapon_select(); break;
            case SCREEN_PATH_CHOICE: update_path_choice(); break;
            case SCREEN_CONTINUE: update_continue_walk(); break;
            case SCREEN_ENEMY_ALERT: update_enemy_alert(); break;
            case SCREEN_BOSS_ALERT: update_boss_alert(); break;
            case SCREEN_COMBAT: update_combat(); break;
            case SCREEN_ABILITY: update_ability(); break;
            case SCREEN_COMBAT_ITEMS: update_combat_items(); break;
            case SCREEN_COMBAT_FEEDBACK: update_combat_feedback(); break;
            case SCREEN_VICTORY: update_victory(); break;
            case SCREEN_LEVEL_UP_BANNER: update_level_banner(); break;
            case SCREEN_LEVEL_REWARD: update_level_reward(); break;
            case SCREEN_POST_BOSS: update_post_boss(); break;
            case SCREEN_MAGIC_DOOR: update_magic_door(); break;
            case SCREEN_ESCAPE_END: update_escape_end(); break;
            case SCREEN_MERCHANT: update_merchant(); break;
            case SCREEN_CHEST: update_chest(); break;
            case SCREEN_CHEST_CONFIRM: update_chest_confirm(); break;
            case SCREEN_CARD: update_card(); break;
            case SCREEN_INVENTORY: update_inventory(); break;
            case SCREEN_INVENTORY_CONFIRM: update_inventory_confirm(); break;
            case SCREEN_STATUS: update_status(); break;
            case SCREEN_DEATH: update_death(); break;
            default: set_screen(SCREEN_MAIN_MENU); break;
        }
    }

    ui_shutdown();
    CloseWindow();
    return 0;
}
