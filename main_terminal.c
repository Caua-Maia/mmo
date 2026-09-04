#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#define LIMPAR() system("cls")
#define ESPERAR(s) Sleep((DWORD)((s) * 1000))
#else
#include <unistd.h>
#define LIMPAR() system("clear")
#define ESPERAR(s) sleep(s)
#endif

#define MOCHILA_TAM 8
#define QTD_BOSSES 3

/*------ ANOTAÇÕES ------*/
/* Mochila usa ID + quantidade (sem contador global).
   DEF agora reduz dano, em vez de ser uma segunda barra de vida.
   Classes definem arma, habilidades e atributos.
   Bosses não se repetem até o ciclo acabar; fugas altas atraem o Vazio. */


typedef enum { CLASSE_GUERREIRO = 1, CLASSE_LADINO, CLASSE_MAGO } Classe;
typedef enum { ITEM_VAZIO = 0, POCAO_VIDA, PEDACO_ARMADURA, POCAO_ENERGIA, BOMBA } ItemId;
typedef enum { TIER_FRACO = 0, TIER_NORMAL, TIER_ELITE } Tier;

struct arma {
    char nome[40];
    int dano;
    int crit_min;
    int hits;
    int bonus_def;
    int desconto_energia;
    int bonus_cura_pct;
    int bonus_fuga;
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
    int energia, energia_max;
    int nivel, xp, xp_para_proximo;
    int bonus_acerto;
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

struct jogador jogador;
int niveis_desde_boss = 0;
int fugas_totais = 0;
int bosses_derrotados[QTD_BOSSES];
int boss_derrotado = 0;

/*------ PROTÓTIPOS ------*/
int d20(void);
int entre(int min, int max);
int ler_opcao(void);
int custo_energia(int base);
int reduzir_dano(int dano, int defesa);
const char *nome_item(ItemId id);
int mochila_adicionar(ItemId id, int qtd);
int usar_item_slot(int indice, struct inimigo *alvo, int em_combate);
void STATUS(void);
void mochila_menu(void);
void aplicar_classe(Classe c);
void escolher_classe(void);
void arma_inicial(void);
void desenhar_inimigo(const struct inimigo *e);
void desenhar_boss(const struct inimigo *e);
void painel_ataque(int eh_boss);
void mostrar_habilidades(void);
void aplicar_dano_jogador(int dano, int *escudo, int *postura);
void ataque_inimigo(struct inimigo *e, int *escudo, int *postura, int turno);
void seu_ataque(struct inimigo *e, int forcar_crit, float mult, int ignora_def);
int menu_habilidades(struct inimigo *e, int *escudo, int *postura);
int menu_itens_combate(struct inimigo *e);
int tentar_fugir(void);
void ganhar_xp(int qtd);
void subir_nivel(void);
struct inimigo criar_inimigo(int tipo, Tier tier);
struct inimigo criar_boss(int tipo, int vazio_final);
void Sistema_de_Combate(struct inimigo e);
void escolher_inimigo(int forcar_elite);
void Combate_Boss(void);
void continue_caminhando(void);
void mercador(void);
void item(void);
void resultado(void);
void mensagem_apos_boss(void);
void menu_escolher_caminho(void);
void ENTRAR(void);
void Introdução(int op);
void tela(void);

/*------ UTIL ------*/
int d20(void) {
    return rand() % 20 + 1;
}

int entre(int min, int max) {
    if (max <= min) return min;
    return min + rand() % (max - min + 1);
}

int ler_opcao(void) {
    int op;
    fflush(stdout);
    if (scanf("%d", &op) != 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {}
        return -1;
    }
    return op;
}

int custo_energia(int base) {
    int c = base - jogador.arma.desconto_energia;
    return c < 1 ? 1 : c;
}

int reduzir_dano(int dano, int defesa) {
    int r = dano - defesa / 10;
    return r < 1 ? 1 : r;
}

/*------ ITENS / MOCHILA ------*/
const char *nome_item(ItemId id) {
    switch (id) {
        case POCAO_VIDA: return "Pocao de Vida";
        case PEDACO_ARMADURA: return "Pedaco de Armadura";
        case POCAO_ENERGIA: return "Pocao de Energia";
        case BOMBA: return "Bomba";
        default: return "";
    }
}

int mochila_adicionar(ItemId id, int qtd) {
    int s;
    for (s = 0; s < MOCHILA_TAM; s++) {
        if (jogador.mochila[s].id == id) {
            jogador.mochila[s].qtd += qtd;
            return 1;
        }
    }
    for (s = 0; s < MOCHILA_TAM; s++) {
        if (jogador.mochila[s].id == ITEM_VAZIO) {
            jogador.mochila[s].id = id;
            jogador.mochila[s].qtd = qtd;
            return 1;
        }
    }
    return 0;
}

static void consumir_slot(int indice) {
    jogador.mochila[indice].qtd--;
    if (jogador.mochila[indice].qtd <= 0) {
        jogador.mochila[indice].id = ITEM_VAZIO;
        jogador.mochila[indice].qtd = 0;
    }
}

/* 1 = turno gasto (item usado); 0 = cancelado / falhou */
int usar_item_slot(int indice, struct inimigo *alvo, int em_combate) {
    int usar;
    ItemId id;
    int cura;

    if (indice < 0 || indice >= MOCHILA_TAM) return 0;
    if (jogador.mochila[indice].id == ITEM_VAZIO) {
        printf("\n Voce nao possui item aqui!\n");
        ESPERAR(1);
        return 0;
    }

    id = jogador.mochila[indice].id;

    if (id == BOMBA && !em_combate) {
        printf("\n A bomba so pode ser usada em combate.\n");
        ESPERAR(2);
        return 0;
    }

    LIMPAR();
    printf("\n Usar %s?\n", nome_item(id));
    printf("\n 1: Sim  2: Nao\n");
    printf("\n Escolha: ");
    usar = ler_opcao();
    if (usar != 1) return 0;

    switch (id) {
        case POCAO_VIDA:
            if (jogador.vida >= jogador.vida_max) {
                printf("\n Sua vida esta cheia!\n");
                ESPERAR(1);
                return 0;
            }
            cura = jogador.vida_max * 30 / 100;
            if (cura < 15) cura = 15;
            jogador.vida += cura;
            if (jogador.vida > jogador.vida_max) jogador.vida = jogador.vida_max;
            printf("\n Voce recuperou %d de vida.\n", cura);
            consumir_slot(indice);
            ESPERAR(1);
            return 1;

        case PEDACO_ARMADURA:
            if (jogador.def >= jogador.def_max) {
                jogador.def_max += 2;
                jogador.def = jogador.def_max;
                printf("\n Sua armadura foi reforcada! DEF max +2.\n");
            } else {
                jogador.def += 10;
                if (jogador.def > jogador.def_max) jogador.def = jogador.def_max;
                printf("\n Voce reparou a armadura. DEF +10.\n");
            }
            consumir_slot(indice);
            ESPERAR(1);
            return 1;

        case POCAO_ENERGIA:
            if (jogador.energia >= jogador.energia_max) {
                printf("\n Sua energia esta cheia!\n");
                ESPERAR(1);
                return 0;
            }
            jogador.energia += 3;
            if (jogador.energia > jogador.energia_max) jogador.energia = jogador.energia_max;
            printf("\n Energia restaurada.\n");
            consumir_slot(indice);
            ESPERAR(1);
            return 1;

        case BOMBA:
            if (!alvo) return 0;
            {
                int dmg = 20 + jogador.nivel * 2;
                alvo->vida -= dmg;
                if (alvo->vida < 0) alvo->vida = 0;
                printf("  __________________________  ___________________________\n");
                printf(" |                          ||                           |\n");
                printf(" |  Voce explodiu uma bomba      Ele perdeu %d de vida   \n", dmg);
                printf(" |__________________________||___________________________|\n");
                consumir_slot(indice);
                ESPERAR(1);
            }
            return 1;

        default:
            return 0;
    }
}

void STATUS(void) {
    int sair;
    do {
        LIMPAR();
        printf("\033[1;38;2;75;227;93m");
        printf("   _____________________________________________ \n");
        printf("  |                                             |\n");
        printf("  |   STATUS:                       LEVEL: %d    \n", jogador.nivel);
        printf("  |   Classe: %s\n", jogador.nome_classe);
        printf("  |                                              \n");
        printf("  |   Vida: %d/%d                  R$: %.2f\n", jogador.vida, jogador.vida_max, jogador.moeda);
        printf("  |   Energia: %d/%d                             \n", jogador.energia, jogador.energia_max);
        printf("  |   EXP: %d/%d                                 \n", jogador.xp, jogador.xp_para_proximo);
        printf("  |                                              \n");
        printf("  |   DEF: %d/%d                                 \n", jogador.def, jogador.def_max);
        printf("  |   ATK: %d                                    \n", jogador.arma.dano);
        printf("  |   Acerto extra: +%d                          \n", jogador.bonus_acerto);
        printf("  |   ARMA: %s\n", jogador.arma.nome);
        printf("  |                                              \n");
        printf("  |_______                                       \n");
        printf("\033[0m");
        printf("\n 0: SAIR DOS STATUS\n");
        printf(" Escolha: ");
        sair = ler_opcao();
        if (sair == 0) {
            LIMPAR();
            break;
        }
    } while (sair != 0);
}

void mochila_menu(void) {
    int op, s;
    do {
        LIMPAR();
        printf("\033[1;38;2;111;220;247m");
        printf("   _____________________________________________ \n");
        printf("  |                                             |\n");
        printf("  |   ITENS:                                     \n");
        printf("  |                                              \n");
        for (s = 0; s < MOCHILA_TAM; s++) {
            if (jogador.mochila[s].id == ITEM_VAZIO) {
                printf("  |   %d: ---\n", s + 1);
            } else {
                printf("  |   %d: %s x%d\n", s + 1, nome_item(jogador.mochila[s].id), jogador.mochila[s].qtd);
            }
        }
        printf("  |                                              \n");
        printf("  |_______                                       \n");
        printf("\033[0m");
        printf("\n 0: SAIR DA MOCHILA\n");
        printf(" 9: VER STATUS\n");
        printf("\n Escolha: ");
        op = ler_opcao();
        if (op == 0) {
            LIMPAR();
            break;
        }
        if (op == 9) {
            STATUS();
            continue;
        }
        if (op >= 1 && op <= MOCHILA_TAM) {
            usar_item_slot(op - 1, NULL, 0);
        }
    } while (op != 0);
}

/*------ CLASSES / ARMAS ------*/
void aplicar_classe(Classe c) {
    int s;
    memset(&jogador, 0, sizeof(jogador));
    jogador.classe = c;
    jogador.moeda = 50;
    jogador.nivel = 0;
    jogador.xp = 0;
    jogador.xp_para_proximo = 40;
    jogador.bonus_acerto = 0;

    switch (c) {
        case CLASSE_GUERREIRO:
            strcpy(jogador.nome_classe, "Guerreiro");
            jogador.vida_max = 120;
            jogador.def_max = 60;
            jogador.energia_max = 5;
            break;
        case CLASSE_LADINO:
            strcpy(jogador.nome_classe, "Ladino");
            jogador.vida_max = 85;
            jogador.def_max = 35;
            jogador.energia_max = 7;
            jogador.bonus_acerto = 1;
            break;
        case CLASSE_MAGO:
            strcpy(jogador.nome_classe, "Mago");
            jogador.vida_max = 75;
            jogador.def_max = 25;
            jogador.energia_max = 10;
            break;
        default:
            break;
    }

    jogador.vida = jogador.vida_max;
    jogador.def = jogador.def_max;
    jogador.energia = jogador.energia_max;
    for (s = 0; s < MOCHILA_TAM; s++) {
        jogador.mochila[s].id = ITEM_VAZIO;
        jogador.mochila[s].qtd = 0;
    }
}

void escolher_classe(void) {
    int escolha;
    do {
        LIMPAR();
        printf("\n Escolha sua classe:\n");
        printf("\033[1;38;2;231;245;95m");
        printf("  ______________________________________________________\n");
        printf(" |                                                      |\n");
        printf(" |  1: Guerreiro  -- vida e defesa altas, golpe pesado  |\n");
        printf(" |  2: Ladino     -- acerto, critico e fuga             |\n");
        printf(" |  3: Mago       -- magias fortes, corpo fragil        |\n");
        printf(" |______________________________________________________|\n");
        printf("\033[0m");
        printf("\n Escolha: ");
        escolha = ler_opcao();
        if (escolha >= 1 && escolha <= 3) {
            aplicar_classe((Classe)escolha);
            break;
        }
        printf("\n Opcao Invalida!\n");
        ESPERAR(1);
    } while (1);
}

void arma_inicial(void) {
    int escolha;
    struct arma opcoes[3];
    int i;

    memset(opcoes, 0, sizeof(opcoes));
    for (i = 0; i < 3; i++) {
        opcoes[i].hits = 1;
        opcoes[i].crit_min = 20;
    }

    if (jogador.classe == CLASSE_GUERREIRO) {
        strcpy(opcoes[0].nome, "Espada");
        opcoes[0].dano = 16;
        opcoes[0].crit_min = 19;
        strcpy(opcoes[1].nome, "Machado");
        opcoes[1].dano = 20;
        strcpy(opcoes[2].nome, "Maca e Escudo");
        opcoes[2].dano = 14;
        opcoes[2].bonus_def = 10;
    } else if (jogador.classe == CLASSE_LADINO) {
        strcpy(opcoes[0].nome, "Adaga");
        opcoes[0].dano = 13;
        opcoes[0].crit_min = 18;
        strcpy(opcoes[1].nome, "Adagas Duplas");
        opcoes[1].dano = 10;
        opcoes[1].hits = 2;
        opcoes[1].crit_min = 18;
        strcpy(opcoes[2].nome, "Estoque");
        opcoes[2].dano = 15;
        opcoes[2].bonus_fuga = 4;
        opcoes[2].crit_min = 19;
    } else {
        strcpy(opcoes[0].nome, "Cajado");
        opcoes[0].dano = 9;
        strcpy(opcoes[1].nome, "Grimorio");
        opcoes[1].dano = 7;
        opcoes[1].desconto_energia = 1;
        strcpy(opcoes[2].nome, "Orbe");
        opcoes[2].dano = 8;
        opcoes[2].bonus_cura_pct = 20;
    }

    do {
        LIMPAR();
        printf("\n Escolha sua arma inicial (%s):\n", jogador.nome_classe);
        printf("\033[1;38;2;231;245;95m");
        printf("  __________________________________________\n");
        printf(" |                                          |\n");
        printf(" | 1: %-16s ------ (ATK: %2d)   |\n", opcoes[0].nome, opcoes[0].dano);
        printf(" | 2: %-16s ------ (ATK: %2d)   |\n", opcoes[1].nome, opcoes[1].dano);
        printf(" | 3: %-16s ------ (ATK: %2d)   |\n", opcoes[2].nome, opcoes[2].dano);
        printf(" |__________________________________________|\n");
        printf("\033[0m");
        if (jogador.classe == CLASSE_GUERREIRO) {
            printf("  Espada: mais critico. Machado: mais dano.\n  Maca e Escudo: +10 DEF.\n");
        } else if (jogador.classe == CLASSE_LADINO) {
            printf("  Adaga: criticos. Duplas: dois golpes.\n  Estoque: mais facil fugir.\n");
        } else {
            printf("  Cajado: ataque simples. Grimorio: magias mais baratas.\n  Orbe: curas mais fortes.\n");
        }
        printf("\n Escolha: ");
        escolha = ler_opcao();
        if (escolha >= 1 && escolha <= 3) {
            jogador.arma = opcoes[escolha - 1];
            jogador.def_max += jogador.arma.bonus_def;
            jogador.def = jogador.def_max;
            LIMPAR();
            menu_escolher_caminho();
            break;
        }
        printf("\n Opcao Invalida!\n");
        ESPERAR(1);
    } while (1);
}

/*------ DESENHO ------*/
void desenhar_inimigo(const struct inimigo *e) {
    switch (e->tipo) {
        case 1:
            printf("\033[1;38;2;173;216;230m");
            printf("  _______________________  _________________\n");
            printf(" |        _______        ||%-16s |\n", e->nome);
            printf(" |       |       |       |     HP: %d\n", e->vida);
            printf(" |      | >  -  < |      ||_________________|\n");
            printf(" |     |           |     |\n");
            printf(" |    |_____________|    |\n");
            printf(" |_______________________|\n");
            printf("\033[0m");
            break;
        case 2:
            printf("\033[1;38;2;181;89;240m");
            printf("  _______________________  _________________\n");
            printf(" |       ________        ||%-16s |\n", e->nome);
            printf(" |      |        |       |     HP: %d\n", e->vida);
            printf(" |      | O    O |       ||_________________|\n");
            printf(" |  [[]] |_||||_| [[]]   |\n");
            printf(" |   ||   __||__   ||    |\n");
            printf(" |___||__|______|__||____|\n");
            printf("\033[0m");
            break;
        default:
            printf("\033[1;38;2;62;222;71m");
            printf("  _______________________  _________________\n");
            printf(" |                       ||%-16s |\n", e->nome);
            printf(" |        (- - -) ^      |     HP: %d\n", e->vida);
            printf(" |       <       >|      ||_________________|\n");
            printf(" |        | ___ | |      |\n");
            printf(" |        ||   ||        |\n");
            printf(" |_______________________|\n");
            printf("\033[0m");
            break;
    }
}

void desenhar_boss(const struct inimigo *e) {
    switch (e->tipo) {
        case 1:
            printf("\033[1;38;2;117;110;245m");
            printf("  _______________________  _________________\n");
            printf(" |                       ||%-16s |\n", e->nome);
            printf(" |      <+>     <+>      |    HP: %d         \n", e->vida);
            printf(" |     _           _     ||_________________|\n");
            printf(" |    |_|_________|_|    |\n");
            printf(" |    |_|_|_|_|_|_|_|    |\n");
            printf(" |_______________________|\n");
            printf("\033[0m");
            break;
        case 2:
            printf("\033[1;38;2;222;207;0m");
            printf("  _______________________  _________________\n");
            printf(" |                       ||%-16s |\n", e->nome);
            printf(" |       ||_||_||        |    HP: %d         \n", e->vida);
            printf(" |       |._.._.|        ||_________________|\n");
            printf(" |      | O    O |       |\n");
            printf(" |     __|_||||_|__      |\n");
            printf(" |____|____________|_____|\n");
            printf("\033[0m");
            break;
        default:
            printf("\033[1;38;2;232;67;78m");
            printf("  _______________________  _________________\n");
            printf(" |                       ||%-16s |\n", e->nome);
            printf(" |         ____          |    HP: %d         \n", e->vida);
            printf(" |    _ __|.  .|__ _     ||_________________|\n");
            printf(" |   | |          | |    |\n");
            printf(" |   |  |        |  |    |\n");
            printf(" |____|__|______|__|_____|\n");
            printf("\033[0m");
            break;
    }
}

void painel_ataque(int eh_boss) {
    printf("\033[1;38;2;13;200;217m");
    printf("  _______________________  _________________\n");
    printf(" |                       ||                 |\n");
    printf(" |  1: ATACAR            |   HP: %d/%d\n", jogador.vida, jogador.vida_max);
    printf(" |  2: HABILIDADE        |   DEF: %d         \n", jogador.def);
    printf(" |  3: ITEM              |   Energia: %d/%d\n", jogador.energia, jogador.energia_max);
    if (eh_boss) {
        printf(" |  4: (nao pode fugir)  ||_________________|\n");
    } else {
        printf(" |  4: FUGIR             ||_________________|\n");
    }
    printf(" |_______________________|\n");
    printf("\033[0m");
}

void mostrar_habilidades(void) {
    printf("\n Energia: %d/%d  |  0s = custo de energia\n", jogador.energia, jogador.energia_max);
    printf("\033[1;38;2;231;245;95m");
    printf("   ______________________________ \n");
    printf("  |                              |\n");
    if (jogador.classe == CLASSE_GUERREIRO) {
        printf("  |   1: GOLPE PESADO     (custo %d)\n", custo_energia(3));
        printf("  |   2: POSTURA          (custo %d)\n", custo_energia(2));
        printf("  |   3: DESCANSAR              |\n");
        printf("  |  Golpe: x2.2 dano  Postura: metade do proximo hit\n");
    } else if (jogador.classe == CLASSE_LADINO) {
        printf("  |   1: PUNHALADA        (custo %d)\n", custo_energia(2));
        printf("  |   2: FUMACA           (custo %d)\n", custo_energia(2));
        printf("  |   3: DESCANSAR              |\n");
        printf("  |  Punhalada: crit se rolar alto. Fumaca: pula o turno inimigo\n");
    } else {
        printf("  |   1: BOLA DE FOGO     (custo %d)\n", custo_energia(3));
        printf("  |   2: CURA             (custo %d)\n", custo_energia(2));
        printf("  |   3: ESCUDO ARCANO    (custo %d)\n", custo_energia(2));
        printf("  |  Fogo ignora DEF. Escudo absorve o proximo golpe.\n");
    }
    printf("  |______________________________| \n");
    printf("\033[0m");
}

/*------ COMBATE ------*/
void aplicar_dano_jogador(int dano, int *escudo, int *postura) {
    int final;
    if (*escudo) {
        printf("  ______________________________\n");
        printf(" |                              |\n");
        printf(" |  O escudo arcano absorveu!   |\n");
        printf(" |______________________________|\n");
        *escudo = 0;
        ESPERAR(1);
        return;
    }
    final = reduzir_dano(dano, jogador.def);
    if (*postura) {
        final = final / 2;
        if (final < 1) final = 1;
        *postura = 0;
    }
    jogador.vida -= final;
    if (jogador.vida < 0) jogador.vida = 0;
    printf("  ____________________________  ___________________________\n");
    printf(" |                            ||                           |\n");
    printf(" |    Ele acertou o ataque        Ele te deu %d de dano   \n", final);
    printf(" |____________________________||___________________________|\n");
}

void ataque_inimigo(struct inimigo *e, int *escudo, int *postura, int turno) {
    int roll = d20();
    int dmg = e->dano;
    int especial = 0;

    if (e->eh_boss && e->tipo == 1 && fugas_totais >= 3) {
        dmg = dmg + dmg / 2;
    }
    if (e->eh_boss && e->tipo == 2 && turno % 3 == 0) {
        especial = 1;
        dmg = dmg + dmg / 2;
    }
    if (e->eh_boss && e->tipo == 3 && roll >= 15) {
        especial = 2;
        dmg += 8;
    }

    /* 1-3 erra; 4-14 normal; 15-17 especial do tipo; 18-20 critico */
    if (roll <= 3) {
        printf("  ___________________________\n");
        printf(" |                           | \n");
        printf(" |    Ele errou o ataque     |\n");
        printf(" |___________________________|\n");
        ESPERAR(2);
        LIMPAR();
        return;
    }

    if (roll >= 18) {
        dmg *= 2;
        printf("  __________________\n");
        printf(" |                  |\n");
        printf(" |   Dano Critico   |\n");
        printf(" |__________________|\n");
        ESPERAR(1);
    } else if (roll >= 15 && !e->eh_boss) {
        if (e->tipo == 1) {
            jogador.def -= 5;
            if (jogador.def < 0) jogador.def = 0;
            printf("  _________________________________\n");
            printf(" |                                 |\n");
            printf(" |  O slime corroeu sua armadura!  |\n");
            printf(" |_________________________________|\n");
            ESPERAR(1);
        } else if (e->tipo == 2) {
            printf("  ________________________________\n");
            printf(" |                                |\n");
            printf(" |  O esqueleto ataca duas vezes! |\n");
            printf(" |________________________________|\n");
            ESPERAR(1);
            aplicar_dano_jogador(e->dano / 2 + 1, escudo, postura);
            ESPERAR(1);
            if (jogador.vida <= 0) return;
            aplicar_dano_jogador(e->dano / 2 + 1, escudo, postura);
            ESPERAR(1);
            LIMPAR();
            return;
        } else {
            int roubo = 5;
            if (jogador.moeda < roubo) roubo = (int)jogador.moeda;
            jogador.moeda -= (float)roubo;
            printf("  ____________________________________\n");
            printf(" |                                    |\n");
            printf(" |  O goblin roubou %d moedas!        |\n", roubo);
            printf(" |____________________________________|\n");
            ESPERAR(1);
        }
    }

    if (especial == 1) {
        printf("  ___________________________________\n");
        printf(" |                                   |\n");
        printf(" |  O Rei invoca ossos afiados!      |\n");
        printf(" |___________________________________|\n");
        ESPERAR(1);
    } else if (especial == 2) {
        printf("  __________________________\n");
        printf(" |                          |\n");
        printf(" |  O Golem pisoteia voce!  |\n");
        printf(" |__________________________|\n");
        ESPERAR(1);
    } else if (e->eh_boss && e->tipo == 1 && fugas_totais >= 3) {
        printf("  ____________________________________________\n");
        printf(" |                                            |\n");
        printf(" |  O Vazio sorri para as suas fugas...       |\n");
        printf(" |____________________________________________|\n");
        ESPERAR(1);
    }

    aplicar_dano_jogador(dmg, escudo, postura);
    ESPERAR(2);
    LIMPAR();
}

void seu_ataque(struct inimigo *e, int forcar_crit, float mult, int ignora_def) {
    int h;
    int miss_min = 4;
    if (jogador.classe == CLASSE_LADINO) miss_min = 3;
    if (jogador.classe == CLASSE_MAGO && !ignora_def) miss_min = 5;
    if (ignora_def) miss_min = 3;
    miss_min -= jogador.bonus_acerto;
    if (miss_min < 2) miss_min = 2;

    for (h = 0; h < jogador.arma.hits; h++) {
        int roll = d20();
        int crit = forcar_crit || roll == 20 || roll >= jogador.arma.crit_min;
        int dmg;

        if (jogador.arma.hits > 1) {
            printf("  Golpe %d/%d\n", h + 1, jogador.arma.hits);
        }

        if (!forcar_crit && roll < miss_min) {
            printf("  __________________________\n");
            printf(" |                          | \n");
            printf(" |   Voce errou o ataque    |\n");
            printf(" |__________________________|\n");
            ESPERAR(1);
            continue;
        }

        dmg = jogador.arma.dano;
        if (crit) dmg *= 2;
        dmg = (int)(dmg * mult);
        if (dmg < 1) dmg = 1;
        if (!ignora_def) dmg = reduzir_dano(dmg, e->defesa);

        if (crit) {
            printf("  __________________  ___________________________\n");
            printf(" |                  ||                           |\n");
            printf(" |   Dano Critico       Ele perdeu %d de vida   \n", dmg);
            printf(" |__________________||___________________________|\n");
        } else {
            printf("  ____________________________  ___________________________\n");
            printf(" |                            ||                           |\n");
            printf(" |   Voce acertou o ataque        Ele perdeu %d de vida   \n", dmg);
            printf(" |____________________________||___________________________|\n");
        }
        e->vida -= dmg;
        if (e->vida < 0) e->vida = 0;
        ESPERAR(1);
        if (e->vida <= 0) break;
    }
}

int menu_habilidades(struct inimigo *e, int *escudo, int *postura) {
    int usar;
    int c_pesado = custo_energia(3);
    int c_medio = custo_energia(2);

    do {
        LIMPAR();
        mostrar_habilidades();
        printf("\n Usar Habilidade ou 0 para sair: ");
        usar = ler_opcao();
        if (usar == 0) return 0;

        if (jogador.classe == CLASSE_GUERREIRO) {
            if (usar == 1) {
                if (jogador.energia < c_pesado) {
                    printf("\n Sem energia suficiente!\n");
                    ESPERAR(1);
                    continue;
                }
                jogador.energia -= c_pesado;
                seu_ataque(e, 0, 2.2f, 0);
                return 1;
            }
            if (usar == 2) {
                if (jogador.energia < c_medio) {
                    printf("\n Sem energia suficiente!\n");
                    ESPERAR(1);
                    continue;
                }
                jogador.energia -= c_medio;
                *postura = 1;
                printf("  ________________________________\n");
                printf(" |                                |\n");
                printf(" |  Voce assume uma postura firme |\n");
                printf(" |________________________________|\n");
                ESPERAR(1);
                return 1;
            }
            if (usar == 3) {
                if (jogador.energia >= jogador.energia_max) {
                    printf("\n Sua energia esta cheia!\n");
                    ESPERAR(1);
                    continue;
                }
                jogador.energia += 2;
                if (jogador.energia > jogador.energia_max) jogador.energia = jogador.energia_max;
                printf("  ___________________________\n");
                printf(" |                           |\n");
                printf(" |  Voce resolveu descansar  |\n");
                printf(" |___________________________|\n");
                ESPERAR(1);
                return 1;
            }
        } else if (jogador.classe == CLASSE_LADINO) {
            if (usar == 1) {
                if (jogador.energia < c_medio) {
                    printf("\n Sem energia suficiente!\n");
                    ESPERAR(1);
                    continue;
                }
                jogador.energia -= c_medio;
                seu_ataque(e, d20() >= 12, 1.0f, 0);
                return 1;
            }
            if (usar == 2) {
                if (jogador.energia < c_medio) {
                    printf("\n Sem energia suficiente!\n");
                    ESPERAR(1);
                    continue;
                }
                jogador.energia -= c_medio;
                printf("  ______________________________________\n");
                printf(" |                                      |\n");
                printf(" |  Voce desaparece numa nuvem de fumaca |\n");
                printf(" |______________________________________|\n");
                ESPERAR(1);
                return 2; /* pula turno do inimigo */
            }
            if (usar == 3) {
                if (jogador.energia >= jogador.energia_max) {
                    printf("\n Sua energia esta cheia!\n");
                    ESPERAR(1);
                    continue;
                }
                jogador.energia += 2;
                if (jogador.energia > jogador.energia_max) jogador.energia = jogador.energia_max;
                printf("  ___________________________\n");
                printf(" |                           |\n");
                printf(" |  Voce resolveu descansar  |\n");
                printf(" |___________________________|\n");
                ESPERAR(1);
                return 1;
            }
        } else {
            if (usar == 1) {
                int dmg;
                if (jogador.energia < c_pesado) {
                    printf("\n Sem energia suficiente!\n");
                    ESPERAR(1);
                    continue;
                }
                jogador.energia -= c_pesado;
                dmg = 18 + jogador.nivel * 2 + jogador.arma.dano / 2;
                if (d20() < 3) {
                    printf("  __________________________\n");
                    printf(" |                          |\n");
                    printf(" |   A bola de fogo errou   |\n");
                    printf(" |__________________________|\n");
                    ESPERAR(1);
                    return 1;
                }
                e->vida -= dmg;
                if (e->vida < 0) e->vida = 0;
                printf("  ________________________________  ___________________________\n");
                printf(" |                                ||                           |\n");
                printf(" |  Bola de fogo (ignora defesa)      Ele perdeu %d de vida   \n", dmg);
                printf(" |________________________________||___________________________|\n");
                ESPERAR(1);
                return 1;
            }
            if (usar == 2) {
                int cura;
                if (jogador.energia < c_medio) {
                    printf("\n Sem energia suficiente!\n");
                    ESPERAR(1);
                    continue;
                }
                if (jogador.vida >= jogador.vida_max) {
                    printf("\n Sua vida esta cheia!\n");
                    ESPERAR(1);
                    continue;
                }
                jogador.energia -= c_medio;
                cura = jogador.vida_max * 25 / 100;
                cura += cura * jogador.arma.bonus_cura_pct / 100;
                if (cura < 18) cura = 18;
                jogador.vida += cura;
                if (jogador.vida > jogador.vida_max) jogador.vida = jogador.vida_max;
                printf("  ________________________\n");
                printf(" |                        |\n");
                printf(" |  Voce utilizou a cura  |\n");
                printf(" |________________________|\n");
                ESPERAR(1);
                return 1;
            }
            if (usar == 3) {
                if (jogador.energia < c_medio) {
                    printf("\n Sem energia suficiente!\n");
                    ESPERAR(1);
                    continue;
                }
                jogador.energia -= c_medio;
                *escudo = 1;
                printf("  ________________________________\n");
                printf(" |                                |\n");
                printf(" |  Um escudo arcano te envolve   |\n");
                printf(" |________________________________|\n");
                ESPERAR(1);
                return 1;
            }
        }

        printf("\n Opcao Invalida!\n");
        ESPERAR(1);
    } while (1);
}

int menu_itens_combate(struct inimigo *e) {
    int op, s;
    LIMPAR();
    printf("   _____________________________________________ \n");
    printf("  |   ITENS (combate)                            |\n");
    for (s = 0; s < MOCHILA_TAM; s++) {
        if (jogador.mochila[s].id == ITEM_VAZIO) printf("  |   %d: ---\n", s + 1);
        else printf("  |   %d: %s x%d\n", s + 1, nome_item(jogador.mochila[s].id), jogador.mochila[s].qtd);
    }
    printf("  |_______                                       \n");
    printf("\n 0: Voltar\n");
    printf(" Escolha: ");
    op = ler_opcao();
    if (op >= 1 && op <= MOCHILA_TAM) {
        return usar_item_slot(op - 1, e, 1);
    }
    return 0;
}

int tentar_fugir(void) {
    int alvo = 12;
    int roll = d20() + jogador.arma.bonus_fuga + jogador.bonus_acerto;
    if (jogador.classe == CLASSE_LADINO) roll += 2;
    if (jogador.classe == CLASSE_GUERREIRO) roll -= 2;

    if (roll >= alvo) {
        fugas_totais++;
        printf("  _________________________________________________________\n");
        printf(" |                                                         |\n");
        printf(" |  Voce esta cada vez mais longe de fugir dessa masmorra  |\n");
        printf(" |_________________________________________________________|\n");
        if (fugas_totais >= 5) {
            printf("\n Algo no abismo percebeu as suas fugas...\n");
        }
        ESPERAR(2);
        LIMPAR();
        return 1;
    }
    printf("  ______________________________\n");
    printf(" |                              |\n");
    printf(" |  Voce tentou fugir e falhou  |\n");
    printf(" |______________________________|\n");
    ESPERAR(2);
    return 0;
}

void subir_nivel(void) {
    int escolha, ok;
    int extra_vida;

    jogador.nivel += 1;
    niveis_desde_boss += 1;
    jogador.xp_para_proximo = 40 + jogador.nivel * 15;

    extra_vida = (jogador.classe == CLASSE_GUERREIRO) ? 12 : (jogador.classe == CLASSE_LADINO) ? 8 : 5;
    jogador.vida_max += extra_vida;
    if (jogador.classe == CLASSE_MAGO || jogador.nivel % 2 == 0) {
        jogador.energia_max += 1;
    }
    jogador.vida += jogador.vida_max / 2;
    if (jogador.vida > jogador.vida_max) jogador.vida = jogador.vida_max;
    jogador.def += 4;
    if (jogador.def > jogador.def_max) jogador.def = jogador.def_max;

    LIMPAR();
    printf("  _________________________\n");
    printf(" |                         | \n");
    printf(" |   Voce subiu de nivel   | \n");
    printf(" |_________________________|\n");
    ESPERAR(2);

    do {
        LIMPAR();
        ok = 0;
        printf("\n Escolha uma recompensa:\n");
        printf("  __________________________  __STATUS ATUAL__________________\n");
        printf(" |                          ||                                \n");
        printf(" |   1: +2 de ATK           || Vida: %d/%d\n", jogador.vida, jogador.vida_max);
        printf(" |   2: +4 DEF maxima       || ATK: %d\n", jogador.arma.dano);
        printf(" |   3: +1 Sorte (acerto)   || DEF: %d/%d\n", jogador.def, jogador.def_max);
        printf(" |__________________________||                                \n");
        printf("\n Escolha: ");
        escolha = ler_opcao();
        if (escolha == 1) {
            jogador.arma.dano += 2;
            ok = 1;
        } else if (escolha == 2) {
            jogador.def_max += 4;
            jogador.def += 4;
            ok = 1;
        } else if (escolha == 3) {
            jogador.bonus_acerto += 1;
            ok = 1;
        } else {
            printf("\n Opcao Indisponivel\n");
            ESPERAR(1);
        }
    } while (!ok);
}

void ganhar_xp(int qtd) {
    printf("\n +%d EXP\n", qtd);
    ESPERAR(1);
    jogador.xp += qtd;
    while (jogador.xp >= jogador.xp_para_proximo) {
        jogador.xp -= jogador.xp_para_proximo;
        subir_nivel();
    }
}

struct inimigo criar_inimigo(int tipo, Tier tier) {
    struct inimigo e;
    float mult_hp = 1.0f, mult_dano = 1.0f, mult_xp = 1.0f;
    const char *base = "SLIME";
    memset(&e, 0, sizeof(e));
    e.tipo = tipo;
    e.eh_boss = 0;

    if (tipo == 1) {
        base = "SLIME";
        e.vida_max = entre(30, 45) + jogador.nivel * 8;
        e.dano = entre(4, 7) + (int)(jogador.nivel * 1.2f);
        e.xp = 8 + jogador.nivel * 3;
        e.defesa = 2;
    } else if (tipo == 2) {
        base = "ESQUELETO";
        e.vida_max = entre(45, 60) + jogador.nivel * 8;
        e.dano = entre(8, 12) + (int)(jogador.nivel * 1.5f);
        e.xp = 16 + jogador.nivel * 3;
        e.defesa = 8;
    } else {
        tipo = 3;
        e.tipo = 3;
        base = "GOBLIN";
        e.vida_max = entre(40, 55) + jogador.nivel * 8;
        e.dano = entre(10, 14) + (int)(jogador.nivel * 1.6f);
        e.xp = 22 + jogador.nivel * 3;
        e.defesa = 5;
    }

    if (tier == TIER_FRACO) {
        mult_hp = 0.80f;
        mult_dano = 0.80f;
        mult_xp = 0.70f;
        snprintf(e.nome, sizeof(e.nome), "%s FRACO", base);
    } else if (tier == TIER_ELITE) {
        mult_hp = 1.35f;
        mult_dano = 1.25f;
        mult_xp = 1.50f;
        snprintf(e.nome, sizeof(e.nome), "%s ELITE", base);
    } else {
        snprintf(e.nome, sizeof(e.nome), "%s", base);
    }

    e.vida_max = (int)(e.vida_max * mult_hp);
    e.dano = (int)(e.dano * mult_dano);
    e.xp = (int)(e.xp * mult_xp);
    if (e.vida_max < 15) e.vida_max = 15;
    if (e.dano < 3) e.dano = 3;
    e.vida = e.vida_max;
    return e;
}

struct inimigo criar_boss(int tipo, int vazio_final) {
    struct inimigo e;
    memset(&e, 0, sizeof(e));
    e.eh_boss = 1;
    e.tipo = tipo;
    e.vida_max = 120 + jogador.nivel * 25;
    e.dano = 14 + jogador.nivel * 2;
    e.xp = 60 + jogador.nivel * 10;
    e.eh_vazio_final = vazio_final;

    if (tipo == 1) {
        strcpy(e.nome, vazio_final ? "VAZIO ETERNO" : "VAZIO SORRIDENTE");
        e.dano = 16 + jogador.nivel * 2;
        e.defesa = 6;
    } else if (tipo == 2) {
        strcpy(e.nome, "REI ESQUELETO");
        e.dano = 15 + jogador.nivel * 2;
        e.defesa = 10;
    } else {
        e.tipo = 3;
        strcpy(e.nome, "GOLEM");
        e.dano = 18 + jogador.nivel * 2;
        e.defesa = 22;
        e.vida_max += 30;
    }

    if (vazio_final) {
        e.vida_max += 80;
        e.dano += 8;
        e.xp += 40;
        e.defesa += 4;
    }
    e.vida = e.vida_max;
    return e;
}

void Sistema_de_Combate(struct inimigo e) {
    int op;
    int escudo = 0;
    int postura = 0;
    int turno = 0;
    int inimigo_age;
    int resultado_h;

    while (e.vida > 0 && jogador.vida > 0) {
        turno++;
        LIMPAR();
        if (e.eh_boss) desenhar_boss(&e);
        else desenhar_inimigo(&e);
        printf("\n");
        painel_ataque(e.eh_boss);
        printf("\n Escolha: ");
        op = ler_opcao();
        inimigo_age = 1;

        if (op == 1) {
            seu_ataque(&e, 0, 1.0f, 0);
        } else if (op == 2) {
            resultado_h = menu_habilidades(&e, &escudo, &postura);
            if (resultado_h == 0) inimigo_age = 0;
            else if (resultado_h == 2) inimigo_age = 0;
        } else if (op == 3) {
            if (!menu_itens_combate(&e)) inimigo_age = 0;
        } else if (op == 4) {
            if (e.eh_boss) {
                printf("\n");
                printf("   __???_______________________________\n");
                printf("  |                                    |\n");
                printf("  |  Voce nao pode fugir dessa vez :)   \n");
                printf("  |_______                              \n");
                ESPERAR(2);
            } else if (tentar_fugir()) {
                return;
            }
        } else {
            printf("\n Opcao Invalida!\n");
            ESPERAR(1);
            inimigo_age = 0;
        }

        if (e.vida <= 0 || jogador.vida <= 0) break;
        if (inimigo_age) {
            ataque_inimigo(&e, &escudo, &postura, turno);
        }
    }

    if (jogador.vida <= 0) return;

    if (e.vida <= 0) {
        LIMPAR();
        if (e.eh_boss) {
            printf("  _______________________\n");
            printf(" |                       | \n");
            printf(" |   Voce matou o Boss   | \n");
            printf(" |_______________________|\n");
        } else {
            printf("  __________________________\n");
            printf(" |                          | \n");
            printf(" |   Voce matou o inimigo   | \n");
            printf(" |__________________________|\n");
        }
        ESPERAR(2);
        jogador.moeda += (float)(e.eh_boss ? entre(18, 35) : entre(4, 12));
        ganhar_xp(e.xp);
        if (e.eh_boss) {
            if (!e.eh_vazio_final && e.tipo >= 1 && e.tipo <= 3) {
                bosses_derrotados[e.tipo - 1] = 1;
            }
            niveis_desde_boss = 0;
            boss_derrotado = 1;
        }
        LIMPAR();
    }
}

static int sortear_tipo_inimigo(void) {
    int r = rand() % 100;
    if (jogador.nivel < 2) {
        if (r < 70) return 1;
        if (r < 95) return 2;
        return 3;
    }
    if (jogador.nivel < 4) {
        if (r < 25) return 1;
        if (r < 65) return 2;
        return 3;
    }
    if (r < 15) return 1;
    if (r < 45) return 2;
    return 3;
}

static Tier sortear_tier(void) {
    int r = rand() % 100;
    if (r < 20) return TIER_FRACO;
    if (r < 85) return TIER_NORMAL;
    return TIER_ELITE;
}

void escolher_inimigo(int forcar_elite) {
    int tipo;
    Tier tier;
    printf("\033[1;38;2;209;73;93m");
    if (forcar_elite) {
        printf("  _____________________________________\n");
        printf(" |                                     |\n");
        printf(" |  Uma emboscada! Algo forte surgiu   |\n");
        printf(" |_____________________________________|\n");
        tier = TIER_ELITE;
    } else {
        printf("  _____________________________________\n");
        printf(" |                                     |\n");
        printf(" |  Um inimigo apareceu na sua frente  |\n");
        printf(" |_____________________________________|\n");
        tier = sortear_tier();
    }
    printf("\033[0m");
    ESPERAR(1);
    LIMPAR();
    tipo = sortear_tipo_inimigo();
    Sistema_de_Combate(criar_inimigo(tipo, tier));
}

void Combate_Boss(void) {
    int candidatos[QTD_BOSSES];
    int n = 0, i, escolhido, vazio_final = 0;
    struct inimigo boss;

    for (i = 0; i < QTD_BOSSES; i++) {
        if (!bosses_derrotados[i]) candidatos[n++] = i + 1;
    }

    if (n == 0) {
        escolhido = 1;
        vazio_final = 1;
    } else {
        escolhido = candidatos[rand() % n];
    }

    boss = criar_boss(escolhido, vazio_final);
    Sistema_de_Combate(boss);
}

void continue_caminhando(void) {
    printf("\033[1;38;2;73;209;198m");
    printf("  _______________________\n");
    printf(" |                       |\n");
    printf(" |  Continue Caminhando  |\n");
    printf(" |_______________________|\n");
    printf("\033[0m");
    ESPERAR(1);
    LIMPAR();
}

void mercador(void) {
    int escolher, pegar;
    printf("\033[1;38;2;231;245;95m");
    printf("  _________________________ \n");
    printf(" |       __________        |\n");
    printf(" |      | __    __ |       |\n");
    printf(" |     | | O|__|O | |      |\n");
    printf(" |     | |   -    | |      |\n");
    printf(" |     _|_|______|_|_      |\n");
    printf(" |____|______________|_____|\n");
    printf("  __________________________ \n");
    printf(" |                          |\n");
    printf(" |  Voce achou um mercador  |\n");
    printf(" |__________________________|\n");
    printf("\033[0m");
    ESPERAR(2);

    do {
        LIMPAR();
        printf("\n Escolha um item:\n");
        printf("\033[1;38;2;231;245;95m");
        printf("  _______________________________________\n");
        printf(" |                                       | __Saldo Atual_________\n");
        printf(" |   1: Pocao de Vida ----------- R$ 10  ||                      \n");
        printf(" |   2: Pedaco de Armadura ------ R$ 15  || R$ %.2f\n", jogador.moeda);
        printf(" |   3: Pocao de Energia -------- R$ 12  ||                      \n");
        printf(" |   4: Bomba ------------------- R$ 20  ||                      \n");
        printf(" |_______________________________________||                      \n");
        printf("\n 0: Sair\n");
        printf("\033[0m");
        printf("\n Escolha: ");
        escolher = ler_opcao();
        if (escolher == 0) break;
        if (escolher < 1 || escolher > 4) {
            printf("\n Opcao Invalida\n");
            ESPERAR(1);
            continue;
        }

        printf("\n Pegar item?\n 1: Sim     2: Nao\n\n Escolha: ");
        pegar = ler_opcao();
        if (pegar != 1) continue;

        {
            ItemId id = ITEM_VAZIO;
            int preco = 0;
            if (escolher == 1) { id = POCAO_VIDA; preco = 10; }
            else if (escolher == 2) { id = PEDACO_ARMADURA; preco = 15; }
            else if (escolher == 3) { id = POCAO_ENERGIA; preco = 12; }
            else { id = BOMBA; preco = 20; }

            if (jogador.moeda < preco) {
                printf("\n Sem moeda suficiente!\n");
                ESPERAR(2);
                continue;
            }
            if (!mochila_adicionar(id, 1)) {
                printf("\033[1;38;2;224;242;56m");
                printf("  ________________________ \n");
                printf(" |                        |\n");
                printf(" | Sua mochila esta cheia |\n");
                printf(" |________________________|\n");
                printf("\033[0m");
                ESPERAR(2);
                continue;
            }
            jogador.moeda -= (float)preco;
            printf("\n Item Comprado!\n");
            ESPERAR(2);
        }
    } while (1);
}

static void mostrar_carta(int carta) {
    printf("\n");
    printf("\033[1;38;2;138;250;112m");
    switch (carta) {
        case 1:
            printf("   ____________________________________\n");
            printf("  |                                    |\n");
            printf("  |  Finalmente entrei numa masmorra,  \n");
            printf("  |  estou tao empolgado, quero achar  \n");
            printf("  |  incriveis tesouros aqui e ficar   \n");
            printf("  |  muito forte!                      \n");
            printf("  |                                    \n");
            printf("  |  Derrotei alguns monstros, foi     \n");
            printf("  |  dificil, mas eu consegui, algo    \n");
            printf("  |  nessa masmorra me deixa intrigado,\n");
            printf("  |  parece que essa masmorra nao tem  \n");
            printf("  |  fim, e salao atras de salao e     \n");
            printf("  |  nunca chego no final dela...      \n");
            printf("  |                                    \n");
            printf("  |_______                             \n");
            break;
        case 2:
            printf("   ____________________________________\n");
            printf("  |                                    | \n");
            printf("  |  Me sinto observado, nao sei o que \n");
            printf("  |  esta acontecendo aqui dentro, mas \n");
            printf("  |  estou ficando assustado...        \n");
            printf("  |                                    \n");
            printf("  |  Continuo andando pelos saloes     \n");
            printf("  |  desse lugar, nao sei quanto tempo \n");
            printf("  |  se passou, estou desesperado...   \n");
            printf("  |                                    \n");
            printf("  |  Estou preso nessa masmorra, sinto \n");
            printf("  |  falta da luz do sol, nao sei se   \n");
            printf("  |  vou conseguir sair daqui...       \n");
            printf("  |                                    \n");
            printf("  |_______                             \n");
            break;
        case 3:
            printf("   ____________________________________\n");
            printf("  |                                    |\n");
            printf("  |  Eu deveria ter escutado aquele    \n");
            printf("  |  moco, foi como ele disse,         \n");
            printf("  |  'Cuidado com o abismo, as vezes   \n");
            printf("  |  quando voce sorri para ele, ele   \n");
            printf("  |  sorri de volta pra ti...'         \n");
            printf("  |  Finalmente entendi o que ele      \n");
            printf("  |  quis dizer...                     \n");
            printf("  |                                    \n");
            printf("  |  Ele esta aqui, nao acredito, isso \n");
            printf("  |  nao pode ser real...              \n");
            printf("  |                                    \n");
            printf("  |  Estou desesperado por ajuda...    \n");
            printf("  |                                    \n");
            printf("  |  Socorro...                        \n");
            printf("  |_______                             \n");
            break;
        case 4:
            printf("   ____________________________________\n");
            printf("  |                                    |\n");
            printf("  |  Uma dica do seu amigo das cartas, \n");
            printf("  |  evite fugir de suas batalhas,     \n");
            printf("  |  enfrente seus desafios, quanto    \n");
            printf("  |  mais fugirmos, mais perto estamos \n");
            printf("  |  do abismo...                      \n");
            printf("  |                                    \n");
            printf("  |  Se lutar, chegaremos ao sol mais  \n");
            printf("  |  cedo ou mais tarde...             \n");
            printf("  |                                    \n");
            printf("  |  Se fugirmos, ele chegara mais     \n");
            printf("  |  perto de nos...                   \n");
            printf("  |                                    \n");
            printf("  |  Lute...                           \n");
            printf("  |                                    \n");
            printf("  |  Cuidado com os sorrisos dele...   \n");
            printf("  |_______                             \n");
            break;
        default:
            printf("   ____________________________________\n");
            printf("  |                                    |\n");
            printf("  |  Qual a necessidade de lutar...    \n");
            printf("  |                                    \n");
            printf("  |  Continue fugindo...               \n");
            printf("  |                                    \n");
            printf("  |  Quanto mais voce fugir, mais cedo \n");
            printf("  |  chegara em casa...                \n");
            printf("  |                                    \n");
            printf("  |  Chegara vivo...                   \n");
            printf("  |                                    \n");
            printf("  |  Voce podera ver a luz do sol      \n");
            printf("  |  novamente...                      \n");
            printf("  |                                    \n");
            printf("  |  Fuja...                           \n");
            printf("  |                                    \n");
            printf("  |  :)                                \n");
            printf("  |_______                             \n");
            break;
    }
    printf("\033[0m");
}

void item(void) {
    int escolher, confirmar, carta, parar;
    printf("\033[1;38;2;231;245;95m");
    printf("  _________________________ \n");
    printf(" |                         |\n");
    printf(" |        ________         |\n");
    printf(" |       |        |        |\n");
    printf(" |_______|---[]---|________|\n");
    printf(" |*+*+*+*|________|*+*+*+*+|\n");
    printf(" |_________________________|\n");
    printf("    _____________________   \n");
    printf("   |                     |  \n");
    printf("   |  Voce achou um bau  |  \n");
    printf("   |_____________________|  \n");
    printf("\033[0m");
    ESPERAR(2);

    do {
        LIMPAR();
        printf("\n Escolha um item:\n");
        printf("\033[1;38;2;231;245;95m");
        printf("  __________________________ \n");
        printf(" |                          |\n");
        printf(" |   1: Pocao de Vida       |\n");
        printf(" |   2: Pedaco de Armadura  |\n");
        printf(" |   3: Pocao de Energia    |\n");
        printf(" |   4: Sacola de Moedas    |\n");
        printf(" |   5: Carta               |\n");
        printf(" |   0: Deixar o bau        |\n");
        printf(" |__________________________|\n");
        printf("\033[0m");
        printf("\n Escolha: ");
        escolher = ler_opcao();
        if (escolher == 0) break;

        if (escolher >= 1 && escolher <= 3) {
            ItemId id = (escolher == 1) ? POCAO_VIDA : (escolher == 2) ? PEDACO_ARMADURA : POCAO_ENERGIA;
            printf("\n Descricao do item:\n");
            printf("\033[1;38;2;138;250;112m");
            if (id == POCAO_VIDA) {
                printf("  _________________________________________ \n");
                printf(" |                                         |\n");
                printf(" | Uma pequena pocao, cura 30%% da vida max |\n");
                printf(" |_________________________________________|\n");
            } else if (id == PEDACO_ARMADURA) {
                printf("  ______________________________________ \n");
                printf(" |                                      |\n");
                printf(" | Um pedaco de armadura desconhecida,  |\n");
                printf(" | aumenta +10 de sua defesa.           |\n");
                printf(" |______________________________________|\n");
            } else {
                printf("  _____________________________________ \n");
                printf(" |                                     |\n");
                printf(" | Uma pocao de energia, recupera +3   |\n");
                printf(" | dos seus pontos de energia.         |\n");
                printf(" |_____________________________________|\n");
            }
            printf("\033[0m");
            printf("\n Pegar item?\n 1: Sim     2: Nao\n\n Escolha: ");
            confirmar = ler_opcao();
            if (confirmar == 1) {
                if (!mochila_adicionar(id, 1)) {
                    printf("\033[1;38;2;224;242;56m");
                    printf("  ____________________________ \n");
                    printf(" |                            |\n");
                    printf(" | Mas sua mochila esta cheia |\n");
                    printf(" |____________________________|\n");
                    printf("\033[0m");
                    ESPERAR(2);
                    continue;
                }
                break;
            }
            continue;
        }

        if (escolher == 4) {
            printf("\033[1;38;2;138;250;112m");
            printf("  __________________________ \n");
            printf(" |                          |\n");
            printf(" | Uma sacola com 10 moedas |\n");
            printf(" |__________________________|\n");
            printf("\033[0m");
            printf("\n Pegar item?\n 1: Sim     2: Nao\n\n Escolha: ");
            confirmar = ler_opcao();
            if (confirmar == 1) {
                jogador.moeda += 10;
                break;
            }
            continue;
        }

        if (escolher == 5) {
            printf("\033[1;38;2;138;250;112m");
            printf("  _____________________ \n");
            printf(" |                     |\n");
            printf(" |  Uma simples carta  |\n");
            printf(" |_____________________|\n");
            printf("\033[0m");
            printf("\n Ler carta?\n 1: Sim     2: Nao\n\n Escolha: ");
            confirmar = ler_opcao();
            carta = entre(1, 5);
            if (confirmar == 1) {
                do {
                    LIMPAR();
                    mostrar_carta(carta);
                    printf("\n 0: Parar de ler?\n\n Escolha: ");
                    parar = ler_opcao();
                } while (parar != 0);
                break;
            }
            continue;
        }

        printf("\n Opcao Invalida\n");
        ESPERAR(1);
    } while (1);
}

void resultado(void) {
    int r = rand() % 100;
    int p_ini, p_vazio, p_bau;

    if (fugas_totais >= 5 && rand() % 100 < 15) {
        escolher_inimigo(1);
        return;
    }

    if (jogador.nivel < 3) {
        p_ini = 45;
        p_vazio = 20;
        p_bau = 20;
    } else {
        p_ini = 60;
        p_vazio = 10;
        p_bau = 15;
    }

    if (r < p_ini) escolher_inimigo(0);
    else if (r < p_ini + p_vazio) continue_caminhando();
    else if (r < p_ini + p_vazio + p_bau) item();
    else mercador();
}

void mensagem_apos_boss(void) {
    printf("\n");
    printf("   __???_______________________________\n");
    printf("  |                                    |\n");
    printf("  |  Parabens, voce derrotou os meus    \n");
    printf("  |  servos, mas o tesouro nao esta     \n");
    printf("  |  aqui, ele esta em algum lugar mais \n");
    printf("  |  em baixo do abismo...              \n");
    printf("  |_______                              \n");
}

void menu_escolher_caminho(void) {
    int escolher_caminho, continuar_jogo;

    while (jogador.vida > 0) {
        if (niveis_desde_boss >= 3) {
            LIMPAR();
            printf("\033[1;38;2;209;73;93m");
            printf("  __________________________________\n");
            printf(" |                                  |\n");
            printf(" |  Um Boss apareceu na sua frente  |\n");
            printf(" |__________________________________|\n");
            printf("\033[0m");
            ESPERAR(1);
            LIMPAR();
            Combate_Boss();
        }

        if (boss_derrotado) {
            boss_derrotado = 0;
            do {
                LIMPAR();
                mensagem_apos_boss();
                printf("\n Gostaria de continuar: \n");
                printf("\n 1: Sim   2: Nao");
                printf("\n Escolha: ");
                continuar_jogo = ler_opcao();
            } while (continuar_jogo != 1 && continuar_jogo != 2);

            if (continuar_jogo == 2) {
                LIMPAR();
                printf("\n");
                printf("  __???_______________________________\n");
                printf(" |                                    |\n");
                printf(" |  As vezes, fugir e a melhor opcao!  \n");
                printf(" |_______                              \n");
                ESPERAR(2);
                LIMPAR();
                printf("\033[1;38;2;62;222;71m");
                printf("  _________________________ \n");
                printf(" |     .      +     +     +|\n");
                printf(" | . +   + _______  . + .  |\n");
                printf(" |  .  .  |   |   |   +    |\n");
                printf(" |  +  .  |  .|.  |    .   |\n");
                printf(" |  .   + |   |   | +   .  |\n");
                printf(" |________|___|___|________|\n");
                printf("  ________________________________________\n");
                printf(" |                                        |\n");
                printf(" |  Uma porta aparece magicamente em sua   \n");
                printf(" |  frente.                                \n");
                printf(" |_______                                  \n");
                ESPERAR(3);
                printf("  ______________________________________ \n");
                printf(" |                                      |\n");
                printf(" |  Voce abre a porta e se encontra do   \n");
                printf(" |  lado de fora da masmorra.            \n");
                printf(" |_______                                \n");
                ESPERAR(3);
                LIMPAR();
                printf("  ________________________________________ \n");
                printf(" |                                        |\n");
                printf(" |  Voce reve a luz do sol mais uma vez!  \n");
                printf(" |_______                                \n");
                ESPERAR(3);
                LIMPAR();
                printf("\033[0m");
                break;
            }

            LIMPAR();
            printf("\033[1;38;2;232;67;78m");
            printf("  _________________________ \n");
            printf(" |     .      +     +     +|\n");
            printf(" | . +   + _______  . + .  |\n");
            printf(" |  .  .  |   |   |   +    |\n");
            printf(" |  +  .  |  .|.  |    .   |\n");
            printf(" |  .   + |   |   | +   .  |\n");
            printf(" |________|___|___|________|\n");
            printf("  ________________________________________\n");
            printf(" |                                        |\n");
            printf(" |  Uma porta aparece magicamente em sua   \n");
            printf(" |  frente.                                \n");
            printf(" |_______                                  \n");
            ESPERAR(3);
            printf("  ______________________________________ \n");
            printf(" |                                      |\n");
            printf(" |  Voce abre a porta e se encontra em   \n");
            printf(" |  mais um salao daquela masmorra.      \n");
            printf(" |_______                                \n");
            ESPERAR(3);
            printf("  ______________________________________ \n");
            printf(" |                                      |\n");
            printf(" |  Uma voz misteriosa ecoa pela sua    \n");
            printf(" |  mente...                             \n");
            printf(" |_______                                \n");
            ESPERAR(3);
            LIMPAR();
            printf("\n");
            printf("  __???_______________________________\n");
            printf(" |                                    |\n");
            printf(" |  Lute mais uma vez...               \n");
            printf(" |_______                              \n");
            printf("\033[0m");
            jogador.vida = jogador.vida_max;
            jogador.energia = jogador.energia_max;
            jogador.def = jogador.def_max;
            ESPERAR(3);
        }

        if (jogador.vida <= 0) break;

        LIMPAR();
        printf("\n");
        printf(" Qual caminho voce quer seguir?\n");
        printf("\033[1;38;2;168;255;185m");
        printf("  _____________________________________________  ____________________\n");
        printf(" |                                             ||                    |\n");
        printf(" |                  1: FRENTE                  ||  4: OLHAR MOCHILA  |\n");
        printf(" |  2: ESQUERDA                    3: DIREITA  ||____________________|\n");
        printf(" |                                             |\n");
        printf(" |_____________________________________________| EXP: %d/%d  NV:%d\n",
               jogador.xp, jogador.xp_para_proximo, jogador.nivel);
        printf("\033[0m");
        printf("\n Escolha: ");
        escolher_caminho = ler_opcao();

        switch (escolher_caminho) {
            case 1:
            case 2:
            case 3:
                resultado();
                break;
            case 4:
                mochila_menu();
                break;
            default:
                printf("\n Opcao Invalida!\n");
                ESPERAR(2);
                break;
        }
    }

    if (jogador.vida <= 0) {
        printf("\033[1;38;2;219;9;19m");
        printf("  _________________\n");
        printf(" |                 | \n");
        printf(" |   Voce morreu   |\n");
        printf(" |_________________|\n");
        printf("\033[0m");
    }
}

void ENTRAR(void) {
    LIMPAR();
    printf("\033[1;38;2;238;130;238m");
    printf("  ______________________________\n");
    printf(" |                              |\n");
    printf(" |  BEM VINDO AO DUNGEON QUEST  |\n");
    printf(" |______________________________|\n");
    ESPERAR(2);
    LIMPAR();
    printf("  ____________________________________________ \n");
    printf(" |                                            |\n");
    printf(" |  AQUI SE INICIA SEU CAMINHO PELA DUNGEON!  |\n");
    printf(" |____________________________________________|\n");
    ESPERAR(2);
    LIMPAR();
    printf("  ___________________________________________ \n");
    printf(" |                                           |\n");
    printf(" |  DERROTE OS INIMIGOS E FIQUE MAIS FORTE!  |\n");
    printf(" |___________________________________________|\n");
    ESPERAR(2);
    LIMPAR();
    printf("\033[0m");
    escolher_classe();
    arma_inicial();
}

void Introdução(int op) {
    int resposta, continuar_dialogo;

    if (op != 1) {
        LIMPAR();
        return;
    }

    LIMPAR();
    printf("  ______________________________________\n");
    printf(" |                                      |\n");
    printf(" |  Enquanto isso, no lado de fora da    \n");
    printf(" |  masmorra.                            \n");
    printf(" |_______                                \n");
    ESPERAR(2);

    do {
        LIMPAR();
        printf("\n");
        printf("\033[1;38;2;222;165;65m");
        printf("   __AMIGO_____________________________\n");
        printf("  |                                    |\n");
        printf("  |  Faz tempo que a gente nao entra    \n");
        printf("  |  numa masmorra, isso me faz lembrar \n");
        printf("  |  do nosso comeco, sabe, quando      \n");
        printf("  |  ainda eramos iniciantes nesse      \n");
        printf("  |  negocio de aventureiro...          \n");
        printf("  |_______                              \n");
        printf("\033[0m");
        printf("\n");
        printf("\033[1;38;2;126;186;115m");
        printf("       __JOGADOR_________________________________\n");
        printf("      |                                          |\n");
        printf("      |  Voce disse que nao entraria nessa        \n");
        printf("      |  masmorra nem que te pagassem...          \n");
        printf("      |  ._.                                      \n");
        printf("      |_______                                    \n");
        printf("\033[0m");
        printf("\033[1;38;2;222;165;65m");
        printf("\n");
        printf("   __AMIGO___________________________________\n");
        printf("  |                                          |\n");
        printf("  |  Nao e culpa minha se eu tenho medo de    \n");
        printf("  |  esqueleto, eles sao aterrorizantes...    \n");
        printf("  |                                           \n");
        printf("  |  Enfim...                                 \n");
        printf("  |_______                                    \n");
        printf("\n");
        printf("   __AMIGO_____________________________\n");
        printf("  |                                    |\n");
        printf("  |  Dizem que essa masmorra e bem      \n");
        printf("  |  dificil, mas eu sei que voce vai   \n");
        printf("  |  da conta...                        \n");
        printf("  |_______                              \n");
        printf("\033[0m");
        printf("\n 0: Continuar\n\n Escolha: ");
        continuar_dialogo = ler_opcao();
    } while (continuar_dialogo != 0);

    do {
        LIMPAR();
        printf("\n");
        printf("\033[1;38;2;222;165;65m");
        printf("   __AMIGO_____________________________\n");
        printf("  |                                    |\n");
        printf("  |  Voce ainda lembra como funciona,   \n");
        printf("  |  certo?                             \n");
        printf("  |_______                              \n");
        printf("\033[0m");
        printf("\n 1: Sim   2: Nao\n ");
        printf("\n Escolha: ");
        resposta = ler_opcao();
    } while (resposta != 1 && resposta != 2);

    do {
        LIMPAR();
        printf("\n");
        printf("\033[1;38;2;222;165;65m");
        if (resposta == 1) {
            printf("   __AMIGO_____________________________\n");
            printf("  |                                    |\n");
            printf("  |  Entao nao preciso me preocupar,    \n");
            printf("  |  boa sorte, sei que voce vai        \n");
            printf("  |  conseguir!                         \n");
            printf("  |_______                              \n");
        } else {
            printf("   __AMIGO_____________________________\n");
            printf("  |                                    |\n");
            printf("  |  Nao e dificil, voce so precisa     \n");
            printf("  |  matar alguns inimigos la dentro,   \n");
            printf("  |  ficar mais forte, para no final    \n");
            printf("  |  voce derrotar o chefao...          \n");
            printf("  |                                     \n");
            printf("  |  Esse e o objetivo principal...     \n");
            printf("  |_______                              \n");
            printf("\n");
            printf("   __AMIGO_____________________________\n");
            printf("  |                                    |\n");
            printf("  |  Simples!                           \n");
            printf("  |_______                              \n");
            printf("\n");
            printf("   __AMIGO_________________________________\n");
            printf("  |                                        |\n");
            printf("  |  Vou te esperar aqui no lado de fora.   \n");
            printf("  |_______                                  \n");
            printf("\n");
            printf("   __AMIGO______________________________\n");
            printf("  |                                     |\n");
            printf("  |  Boa sorte, eu sei que voce vai      \n");
            printf("  |  conseguir!                          \n");
            printf("  |_______                               \n");
        }
        printf("\033[0m");
        printf("\n 0: Entrar na Masmorra\n\n Escolha: ");
        continuar_dialogo = ler_opcao();
        if (continuar_dialogo == 0) ENTRAR();
    } while (continuar_dialogo != 0);
}

void tela(void) {
    printf("  ___________________________\n");
    printf(" |                           |\n");
    printf(" |                           |\n");
    printf(" |       DUNGEON QUEST       |\n");
    printf(" |                           |\n");
    printf(" |___________________________|\n");
    printf("  ____________    ___________ \n");
    printf(" |            |  |           |\n");
    printf(" |  1: JOGAR  |  |  2: SAIR  |\n");
    printf(" |____________|  |___________|\n");
}

int main(void) {
    int op;
#ifdef _WIN32
    {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        SetConsoleOutputCP(65001);
        if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode)) {
            SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
#endif
    srand((unsigned)time(NULL));
    memset(bosses_derrotados, 0, sizeof(bosses_derrotados));

    do {
        LIMPAR();
        printf("\033[1;38;2;238;130;238m");
        tela();
        printf("\033[0m");
        printf("\n Escolha: ");
        op = ler_opcao();
    } while (op != 1 && op != 2);

    Introdução(op);
    return 0;
}
