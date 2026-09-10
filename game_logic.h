#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <stdbool.h>

#define MOCHILA_TAM 8
#define QTD_BOSSES 3
#define FLOAT_TEXT_MAX 8
#define MSG_LEN 160
#define HAND_MAX 12
#define CARD_BUDGET_MAX 10
#define FORCE_HIT_COST 4
#define SUPREMA_MAX 100
#define FE_MAX 100

typedef enum { CLASSE_GUERREIRO = 1, CLASSE_LADINO, CLASSE_MAGO, CLASSE_PALADINO } Classe;
typedef enum { ITEM_VAZIO = 0, POCAO_VIDA, PEDACO_ARMADURA, POCAO_ENERGIA, BOMBA } ItemId;
typedef enum { TIER_FRACO = 0, TIER_NORMAL, TIER_ELITE } Tier;

typedef enum {
    CARD_STRIKE = 0,
    CARD_SKILL_1,
    CARD_SKILL_2,
    CARD_SKILL_3,
    CARD_SUPREME,
    CARD_ITEM,
    CARD_FLEE
} CardKind;

struct arma {
    char nome[40];
    int dano;
    int crit_min;
    int hits;
    int bonus_def;
    int desconto_energia;
    int bonus_cura_pct;
    int bonus_fuga;
    int custo_carta; /* extra no golpe (armas pesadas) */
};

struct item_slot {
    ItemId id;
    int qtd;
};

struct jogador {
    Classe classe;
    char nome_classe[24];
    int vida, vida_max;
    int def, def_max;
    int energia, energia_max; /* MP ou Mana, conforme a classe */
    int nivel, xp, xp_para_proximo;
    int bonus_acerto;
    int iniciativa;           /* Paladino: agilidade geral */
    int suprema;              /* 0–100 */
    int fe;                   /* Paladino: 0–100 */
    float moeda;
    struct arma arma;
    struct item_slot mochila[MOCHILA_TAM];
};

struct inimigo {
    char nome[40];
    int tipo;
    int vida, vida_max;
    int dano;
    int defesa;
    int xp;
    int eh_boss;
    int eh_vazio_final;
};

typedef struct {
    CardKind kind;
    int item_slot;
    int cost;
    int mp_cost;
    int enabled;
    char label[40];
    char hint[72];
} CombatCard;

typedef struct {
    int active;
    CardKind kind;
    int item_slot;
} ForcePending;

/* Resultado de uma ação de combate (para feedback visual) */
typedef struct {
    int miss;
    int crit;         /* multiplicador >= x2 */
    int forced;
    int pending_force;
    int mult_x10;     /* 10, 15, 20, 30 */
    int damage;
    int heal;
    int skip_enemy;   /* 1 = inimigo não age (fumaça / cancelado) */
    int spent_turn;   /* 1 = ação válida (não cancela a carta) */
    int end_player_turn; /* 1 = encerra a mão (fuga falhou, etc.) */
    int special;      /* códigos de efeito especial */
    int suprema_ready;
    char msg[MSG_LEN];
    char msg2[MSG_LEN];
} ActionResult;

typedef struct {
    char text[48];
    float x, y;
    float life;       /* segundos restantes */
    float vy;
    int is_crit;
    int is_heal;
    unsigned char r, g, b;
} FloatingText;

extern struct jogador jogador;
extern int niveis_desde_boss;
extern int fugas_totais;
extern int bosses_derrotados[QTD_BOSSES];
extern int boss_derrotado;
extern ForcePending force_pending;

int d20(void);
int entre(int min, int max);
int custo_energia(int base);
int reduzir_dano(int dano, int defesa);

int usa_mana(void);
const char *nome_recurso(void);
int custo_carta_golpe(void);
void regenerar_recurso(int qtd);
void regenerar_mana_turno(void);
void suprema_ganhar(int n);
int suprema_pronta(void);

const char *nome_item(ItemId id);
const char *desc_item(ItemId id);
int mochila_adicionar(ItemId id, int qtd);

void game_reset_run(void);
void aplicar_classe(Classe c);
void preencher_armas_classe(struct arma out[3]);
void equipar_arma(const struct arma *a);

struct inimigo criar_inimigo(int tipo, Tier tier);
struct inimigo criar_boss(int tipo, int vazio_final);
int sortear_tipo_inimigo(void);
Tier sortear_tier(void);

/* Usar item: 1 = sucesso (turno gasto), 0 = falhou/cancelado.
   message preenchida se não-NULL. */
int usar_item_slot(int indice, struct inimigo *alvo, int em_combate, char *message, int msg_len);

ActionResult player_basic_attack(struct inimigo *e);
ActionResult player_ability(struct inimigo *e, int escolha, int *escudo, int *postura);
ActionResult player_play_card(struct inimigo *e, const CombatCard *card, int *escudo, int *postura, int *budget);
ActionResult player_force_pending(struct inimigo *e, int *escudo, int *postura, int *budget);
void player_cancel_force(void);
int montar_mao(CombatCard *out, int max, int budget, int is_boss);

ActionResult enemy_attack(struct inimigo *e, int *escudo, int *postura, int turno);
int tentar_fugir(char *message, int msg_len);

void aplicar_recompensa_nivel(int escolha); /* 1 ATK, 2 DEF, 3 sorte */
void aplicar_stats_nivel(void);            /* incrementa nível e stats base */
int tentar_level_up(void);                 /* 1 se subiu de nível (xp já descontado) */
void conceder_vitoria(struct inimigo *e, int *moedas_ganhas, int *xp_ganho);

/* Evento de exploração: 0 inimigo, 1 vazio, 2 baú, 3 mercador, 4 emboscada elite */
int sortear_resultado_caminho(void);

/* Mercador: preços e compra. Retorna 0 ok, -1 sem dinheiro, -2 mochila cheia, -3 inválido */
int preco_mercador(int escolha);
ItemId item_mercador(int escolha);
int comprar_mercador(int escolha, char *message, int msg_len);

const char *texto_carta(int carta); /* carta 1..5 */
void reset_status_porta_magica(void);
int escolher_proximo_boss(int *vazio_final);

void floating_clear(FloatingText *arr);
void floating_spawn(FloatingText *arr, float x, float y, const char *text, int crit, int heal);
void floating_update(FloatingText *arr, float dt);

#endif
