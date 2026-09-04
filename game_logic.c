#include "game_logic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct jogador jogador;
int niveis_desde_boss = 0;
int fugas_totais = 0;
int bosses_derrotados[QTD_BOSSES];
int boss_derrotado = 0;

int d20(void) {
    return rand() % 20 + 1;
}

int entre(int min, int max) {
    if (max <= min) return min;
    return min + rand() % (max - min + 1);
}

int custo_energia(int base) {
    int c = base - jogador.arma.desconto_energia;
    return c < 1 ? 1 : c;
}

int reduzir_dano(int dano, int defesa) {
    int r = dano - defesa / 10;
    return r < 1 ? 1 : r;
}

const char *nome_item(ItemId id) {
    switch (id) {
        case POCAO_VIDA: return "Pocao de Vida";
        case PEDACO_ARMADURA: return "Pedaco de Armadura";
        case POCAO_ENERGIA: return "Pocao de Energia";
        case BOMBA: return "Bomba";
        default: return "";
    }
}

const char *desc_item(ItemId id) {
    switch (id) {
        case POCAO_VIDA: return "Cura 30% da vida maxima (min. 15).";
        case PEDACO_ARMADURA: return "Repara +10 DEF ou reforca DEF max +2.";
        case POCAO_ENERGIA: return "Recupera +3 pontos de energia.";
        case BOMBA: return "Causa 20 + 2*nivel de dano (so em combate).";
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

void game_reset_run(void) {
    memset(&jogador, 0, sizeof(jogador));
    niveis_desde_boss = 0;
    fugas_totais = 0;
    boss_derrotado = 0;
    memset(bosses_derrotados, 0, sizeof(bosses_derrotados));
}

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

void preencher_armas_classe(struct arma out[3]) {
    int i;
    memset(out, 0, sizeof(struct arma) * 3);
    for (i = 0; i < 3; i++) {
        out[i].hits = 1;
        out[i].crit_min = 20;
    }

    if (jogador.classe == CLASSE_GUERREIRO) {
        strcpy(out[0].nome, "Espada");
        out[0].dano = 16;
        out[0].crit_min = 19;
        strcpy(out[1].nome, "Machado");
        out[1].dano = 20;
        strcpy(out[2].nome, "Maca e Escudo");
        out[2].dano = 14;
        out[2].bonus_def = 10;
    } else if (jogador.classe == CLASSE_LADINO) {
        strcpy(out[0].nome, "Adaga");
        out[0].dano = 13;
        out[0].crit_min = 18;
        strcpy(out[1].nome, "Adagas Duplas");
        out[1].dano = 10;
        out[1].hits = 2;
        out[1].crit_min = 18;
        strcpy(out[2].nome, "Estoque");
        out[2].dano = 15;
        out[2].bonus_fuga = 4;
        out[2].crit_min = 19;
    } else {
        strcpy(out[0].nome, "Cajado");
        out[0].dano = 9;
        strcpy(out[1].nome, "Grimorio");
        out[1].dano = 7;
        out[1].desconto_energia = 1;
        strcpy(out[2].nome, "Orbe");
        out[2].dano = 8;
        out[2].bonus_cura_pct = 20;
    }
}

void equipar_arma(const struct arma *a) {
    jogador.arma = *a;
    jogador.def_max += jogador.arma.bonus_def;
    jogador.def = jogador.def_max;
}

int usar_item_slot(int indice, struct inimigo *alvo, int em_combate, char *message, int msg_len) {
    ItemId id;
    int cura;

    if (message && msg_len > 0) message[0] = '\0';
    if (indice < 0 || indice >= MOCHILA_TAM) return 0;
    if (jogador.mochila[indice].id == ITEM_VAZIO) {
        if (message) snprintf(message, msg_len, "Voce nao possui item aqui!");
        return 0;
    }

    id = jogador.mochila[indice].id;
    if (id == BOMBA && !em_combate) {
        if (message) snprintf(message, msg_len, "A bomba so pode ser usada em combate.");
        return 0;
    }

    switch (id) {
        case POCAO_VIDA:
            if (jogador.vida >= jogador.vida_max) {
                if (message) snprintf(message, msg_len, "Sua vida esta cheia!");
                return 0;
            }
            cura = jogador.vida_max * 30 / 100;
            if (cura < 15) cura = 15;
            jogador.vida += cura;
            if (jogador.vida > jogador.vida_max) jogador.vida = jogador.vida_max;
            if (message) snprintf(message, msg_len, "Voce recuperou %d de vida.", cura);
            consumir_slot(indice);
            return 1;

        case PEDACO_ARMADURA:
            if (jogador.def >= jogador.def_max) {
                jogador.def_max += 2;
                jogador.def = jogador.def_max;
                if (message) snprintf(message, msg_len, "Armadura reforcada! DEF max +2.");
            } else {
                jogador.def += 10;
                if (jogador.def > jogador.def_max) jogador.def = jogador.def_max;
                if (message) snprintf(message, msg_len, "Voce reparou a armadura. DEF +10.");
            }
            consumir_slot(indice);
            return 1;

        case POCAO_ENERGIA:
            if (jogador.energia >= jogador.energia_max) {
                if (message) snprintf(message, msg_len, "Sua energia esta cheia!");
                return 0;
            }
            jogador.energia += 3;
            if (jogador.energia > jogador.energia_max) jogador.energia = jogador.energia_max;
            if (message) snprintf(message, msg_len, "Energia restaurada.");
            consumir_slot(indice);
            return 1;

        case BOMBA:
            if (!alvo) return 0;
            {
                int dmg = 20 + jogador.nivel * 2;
                alvo->vida -= dmg;
                if (alvo->vida < 0) alvo->vida = 0;
                if (message) snprintf(message, msg_len, "Bomba! Inimigo perdeu %d de vida.", dmg);
                consumir_slot(indice);
            }
            return 1;

        default:
            return 0;
    }
}

static ActionResult empty_result(void) {
    ActionResult r;
    memset(&r, 0, sizeof(r));
    return r;
}

static void aplicar_dano_jogador_logic(int dano, int *escudo, int *postura, ActionResult *out) {
    int final;
    if (*escudo) {
        snprintf(out->msg, MSG_LEN, "O escudo arcano absorveu!");
        *escudo = 0;
        out->damage = 0;
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
    out->damage = final;
    snprintf(out->msg2, MSG_LEN, "Voce sofreu %d de dano", final);
}

ActionResult player_basic_attack(struct inimigo *e) {
    ActionResult r = empty_result();
    int h;
    int miss_min = 4;
    int total_dmg = 0;
    int any_hit = 0;
    int any_crit = 0;
    int any_miss = 0;

    if (jogador.classe == CLASSE_LADINO) miss_min = 3;
    if (jogador.classe == CLASSE_MAGO) miss_min = 5;
    miss_min -= jogador.bonus_acerto;
    if (miss_min < 2) miss_min = 2;

    r.spent_turn = 1;

    for (h = 0; h < jogador.arma.hits; h++) {
        int roll = d20();
        int crit = (roll == 20 || roll >= jogador.arma.crit_min);
        int dmg;

        if (roll < miss_min) {
            any_miss = 1;
            continue;
        }

        dmg = jogador.arma.dano;
        if (crit) {
            dmg *= 2;
            any_crit = 1;
        }
        if (dmg < 1) dmg = 1;
        dmg = reduzir_dano(dmg, e->defesa);
        e->vida -= dmg;
        if (e->vida < 0) e->vida = 0;
        total_dmg += dmg;
        any_hit = 1;
        if (e->vida <= 0) break;
    }

    r.damage = total_dmg;
    r.crit = any_crit;
    r.miss = any_miss && !any_hit;
    if (r.miss) {
        snprintf(r.msg, MSG_LEN, "Voce errou o ataque");
    } else if (any_crit) {
        snprintf(r.msg, MSG_LEN, "Dano Critico! -%d", total_dmg);
    } else {
        snprintf(r.msg, MSG_LEN, "Voce acertou! -%d", total_dmg);
    }
    return r;
}

ActionResult player_ability(struct inimigo *e, int escolha, int *escudo, int *postura) {
    ActionResult r = empty_result();
    int c_pesado = custo_energia(3);
    int c_medio = custo_energia(2);

    if (escolha < 1 || escolha > 3) {
        snprintf(r.msg, MSG_LEN, "Opcao invalida");
        return r;
    }

    if (jogador.classe == CLASSE_GUERREIRO) {
        if (escolha == 1) {
            int h, miss_min = 4, total = 0, hit = 0, crit_any = 0;
            if (jogador.energia < c_pesado) {
                snprintf(r.msg, MSG_LEN, "Sem energia suficiente!");
                return r;
            }
            jogador.energia -= c_pesado;
            miss_min -= jogador.bonus_acerto;
            if (miss_min < 2) miss_min = 2;
            r.spent_turn = 1;
            for (h = 0; h < jogador.arma.hits; h++) {
                int roll = d20();
                int crit = (roll == 20 || roll >= jogador.arma.crit_min);
                int dmg;
                if (roll < miss_min) continue;
                dmg = jogador.arma.dano;
                if (crit) { dmg *= 2; crit_any = 1; }
                dmg = (int)(dmg * 2.2f);
                if (dmg < 1) dmg = 1;
                dmg = reduzir_dano(dmg, e->defesa);
                e->vida -= dmg;
                if (e->vida < 0) e->vida = 0;
                total += dmg;
                hit = 1;
                if (e->vida <= 0) break;
            }
            r.damage = total;
            r.crit = crit_any;
            r.miss = !hit;
            if (!hit) snprintf(r.msg, MSG_LEN, "Golpe Pesado errou");
            else if (crit_any) snprintf(r.msg, MSG_LEN, "Golpe Pesado CRITICO! -%d", total);
            else snprintf(r.msg, MSG_LEN, "Golpe Pesado! -%d", total);
            return r;
        }
        if (escolha == 2) {
            if (jogador.energia < c_medio) {
                snprintf(r.msg, MSG_LEN, "Sem energia suficiente!");
                return r;
            }
            jogador.energia -= c_medio;
            *postura = 1;
            r.spent_turn = 1;
            snprintf(r.msg, MSG_LEN, "Voce assume uma postura firme");
            return r;
        }
        if (escolha == 3) {
            if (jogador.energia >= jogador.energia_max) {
                snprintf(r.msg, MSG_LEN, "Sua energia esta cheia!");
                return r;
            }
            jogador.energia += 2;
            if (jogador.energia > jogador.energia_max) jogador.energia = jogador.energia_max;
            r.spent_turn = 1;
            snprintf(r.msg, MSG_LEN, "Voce resolveu descansar");
            return r;
        }
    } else if (jogador.classe == CLASSE_LADINO) {
        if (escolha == 1) {
            int forcar = d20() >= 12;
            int h, miss_min = 3, total = 0, hit = 0, crit_any = 0;
            if (jogador.energia < c_medio) {
                snprintf(r.msg, MSG_LEN, "Sem energia suficiente!");
                return r;
            }
            jogador.energia -= c_medio;
            miss_min -= jogador.bonus_acerto;
            if (miss_min < 2) miss_min = 2;
            r.spent_turn = 1;
            for (h = 0; h < jogador.arma.hits; h++) {
                int roll = d20();
                int crit = forcar || roll == 20 || roll >= jogador.arma.crit_min;
                int dmg;
                if (!forcar && roll < miss_min) continue;
                dmg = jogador.arma.dano;
                if (crit) { dmg *= 2; crit_any = 1; }
                if (dmg < 1) dmg = 1;
                dmg = reduzir_dano(dmg, e->defesa);
                e->vida -= dmg;
                if (e->vida < 0) e->vida = 0;
                total += dmg;
                hit = 1;
                if (e->vida <= 0) break;
            }
            r.damage = total;
            r.crit = crit_any;
            r.miss = !hit;
            if (!hit) snprintf(r.msg, MSG_LEN, "Punhalada errou");
            else if (crit_any) snprintf(r.msg, MSG_LEN, "Punhalada CRITICA! -%d", total);
            else snprintf(r.msg, MSG_LEN, "Punhalada! -%d", total);
            return r;
        }
        if (escolha == 2) {
            if (jogador.energia < c_medio) {
                snprintf(r.msg, MSG_LEN, "Sem energia suficiente!");
                return r;
            }
            jogador.energia -= c_medio;
            r.spent_turn = 1;
            r.skip_enemy = 1;
            snprintf(r.msg, MSG_LEN, "Voce desaparece numa nuvem de fumaca");
            return r;
        }
        if (escolha == 3) {
            if (jogador.energia >= jogador.energia_max) {
                snprintf(r.msg, MSG_LEN, "Sua energia esta cheia!");
                return r;
            }
            jogador.energia += 2;
            if (jogador.energia > jogador.energia_max) jogador.energia = jogador.energia_max;
            r.spent_turn = 1;
            snprintf(r.msg, MSG_LEN, "Voce resolveu descansar");
            return r;
        }
    } else {
        if (escolha == 1) {
            int dmg;
            if (jogador.energia < c_pesado) {
                snprintf(r.msg, MSG_LEN, "Sem energia suficiente!");
                return r;
            }
            jogador.energia -= c_pesado;
            r.spent_turn = 1;
            dmg = 18 + jogador.nivel * 2 + jogador.arma.dano / 2;
            if (d20() < 3) {
                r.miss = 1;
                snprintf(r.msg, MSG_LEN, "A bola de fogo errou");
                return r;
            }
            e->vida -= dmg;
            if (e->vida < 0) e->vida = 0;
            r.damage = dmg;
            snprintf(r.msg, MSG_LEN, "Bola de fogo! -%d (ignora DEF)", dmg);
            return r;
        }
        if (escolha == 2) {
            int cura;
            if (jogador.energia < c_medio) {
                snprintf(r.msg, MSG_LEN, "Sem energia suficiente!");
                return r;
            }
            if (jogador.vida >= jogador.vida_max) {
                snprintf(r.msg, MSG_LEN, "Sua vida esta cheia!");
                return r;
            }
            jogador.energia -= c_medio;
            cura = jogador.vida_max * 25 / 100;
            cura += cura * jogador.arma.bonus_cura_pct / 100;
            if (cura < 18) cura = 18;
            jogador.vida += cura;
            if (jogador.vida > jogador.vida_max) jogador.vida = jogador.vida_max;
            r.spent_turn = 1;
            r.heal = cura;
            snprintf(r.msg, MSG_LEN, "Voce utilizou a cura (+%d)", cura);
            return r;
        }
        if (escolha == 3) {
            if (jogador.energia < c_medio) {
                snprintf(r.msg, MSG_LEN, "Sem energia suficiente!");
                return r;
            }
            jogador.energia -= c_medio;
            *escudo = 1;
            r.spent_turn = 1;
            snprintf(r.msg, MSG_LEN, "Um escudo arcano te envolve");
            return r;
        }
    }

    snprintf(r.msg, MSG_LEN, "Opcao invalida");
    return r;
}

ActionResult enemy_attack(struct inimigo *e, int *escudo, int *postura, int turno) {
    ActionResult r = empty_result();
    int roll = d20();
    int dmg = e->dano;
    int especial = 0;

    r.spent_turn = 1;

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

    if (roll <= 3) {
        r.miss = 1;
        snprintf(r.msg, MSG_LEN, "Ele errou o ataque");
        return r;
    }

    if (roll >= 18) {
        dmg *= 2;
        r.crit = 1;
        snprintf(r.msg, MSG_LEN, "Dano Critico!");
    } else if (roll >= 15 && !e->eh_boss) {
        if (e->tipo == 1) {
            jogador.def -= 5;
            if (jogador.def < 0) jogador.def = 0;
            snprintf(r.msg, MSG_LEN, "O slime corroeu sua armadura!");
            r.special = 10;
        } else if (e->tipo == 2) {
            ActionResult a = empty_result(), b = empty_result();
            snprintf(r.msg, MSG_LEN, "O esqueleto ataca duas vezes!");
            aplicar_dano_jogador_logic(e->dano / 2 + 1, escudo, postura, &a);
            if (jogador.vida > 0) {
                aplicar_dano_jogador_logic(e->dano / 2 + 1, escudo, postura, &b);
            }
            r.damage = a.damage + b.damage;
            r.special = 11;
            snprintf(r.msg2, MSG_LEN, "Total: %d de dano", r.damage);
            return r;
        } else {
            int roubo = 5;
            if (jogador.moeda < roubo) roubo = (int)jogador.moeda;
            jogador.moeda -= (float)roubo;
            snprintf(r.msg, MSG_LEN, "O goblin roubou %d moedas!", roubo);
            r.special = 12;
        }
    }

    if (especial == 1) {
        snprintf(r.msg, MSG_LEN, "O Rei invoca ossos afiados!");
        r.special = 1;
    } else if (especial == 2) {
        snprintf(r.msg, MSG_LEN, "O Golem pisoteia voce!");
        r.special = 2;
    } else if (e->eh_boss && e->tipo == 1 && fugas_totais >= 3) {
        snprintf(r.msg, MSG_LEN, "O Vazio sorri para as suas fugas...");
        r.special = 3;
    }

    {
        ActionResult hit = empty_result();
        aplicar_dano_jogador_logic(dmg, escudo, postura, &hit);
        r.damage = hit.damage;
        if (hit.msg[0]) {
            if (r.msg[0] == '\0') snprintf(r.msg, MSG_LEN, "%s", hit.msg);
            else snprintf(r.msg2, MSG_LEN, "%s", hit.msg2[0] ? hit.msg2 : hit.msg);
        } else if (hit.msg2[0]) {
            snprintf(r.msg2, MSG_LEN, "%s", hit.msg2);
        }
        if (r.msg2[0] == '\0' && r.damage > 0) {
            snprintf(r.msg2, MSG_LEN, "Voce sofreu %d de dano", r.damage);
        }
    }
    return r;
}

int tentar_fugir(char *message, int msg_len) {
    int alvo = 12;
    int roll = d20() + jogador.arma.bonus_fuga + jogador.bonus_acerto;
    if (jogador.classe == CLASSE_LADINO) roll += 2;
    if (jogador.classe == CLASSE_GUERREIRO) roll -= 2;

    if (roll >= alvo) {
        fugas_totais++;
        if (message) {
            if (fugas_totais >= 5)
                snprintf(message, msg_len, "Fugiu... algo no abismo percebeu.");
            else
                snprintf(message, msg_len, "Voce escapou da batalha.");
        }
        return 1;
    }
    if (message) snprintf(message, msg_len, "Voce tentou fugir e falhou");
    return 0;
}

void aplicar_stats_nivel(void) {
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
}

void aplicar_recompensa_nivel(int escolha) {
    if (escolha == 1) {
        jogador.arma.dano += 2;
    } else if (escolha == 2) {
        jogador.def_max += 4;
        jogador.def += 4;
    } else if (escolha == 3) {
        jogador.bonus_acerto += 1;
    }
}

int tentar_level_up(void) {
    if (jogador.xp >= jogador.xp_para_proximo) {
        jogador.xp -= jogador.xp_para_proximo;
        aplicar_stats_nivel();
        return 1;
    }
    return 0;
}

void conceder_vitoria(struct inimigo *e, int *moedas_ganhas, int *xp_ganho) {
    int moedas = e->eh_boss ? entre(18, 35) : entre(4, 12);
    jogador.moeda += (float)moedas;
    jogador.xp += e->xp;
    if (moedas_ganhas) *moedas_ganhas = moedas;
    if (xp_ganho) *xp_ganho = e->xp;

    if (e->eh_boss) {
        if (!e->eh_vazio_final && e->tipo >= 1 && e->tipo <= 3) {
            bosses_derrotados[e->tipo - 1] = 1;
        }
        niveis_desde_boss = 0;
        boss_derrotado = 1;
    }
}

int sortear_tipo_inimigo(void) {
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

Tier sortear_tier(void) {
    int r = rand() % 100;
    if (r < 20) return TIER_FRACO;
    if (r < 85) return TIER_NORMAL;
    return TIER_ELITE;
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

int sortear_resultado_caminho(void) {
    int r = rand() % 100;
    int p_ini, p_vazio, p_bau;

    if (fugas_totais >= 5 && rand() % 100 < 15) return 4;

    if (jogador.nivel < 3) {
        p_ini = 45;
        p_vazio = 20;
        p_bau = 20;
    } else {
        p_ini = 60;
        p_vazio = 10;
        p_bau = 15;
    }

    if (r < p_ini) return 0;
    if (r < p_ini + p_vazio) return 1;
    if (r < p_ini + p_vazio + p_bau) return 2;
    return 3;
}

int preco_mercador(int escolha) {
    if (escolha == 1) return 10;
    if (escolha == 2) return 15;
    if (escolha == 3) return 12;
    if (escolha == 4) return 20;
    return 0;
}

ItemId item_mercador(int escolha) {
    if (escolha == 1) return POCAO_VIDA;
    if (escolha == 2) return PEDACO_ARMADURA;
    if (escolha == 3) return POCAO_ENERGIA;
    if (escolha == 4) return BOMBA;
    return ITEM_VAZIO;
}

int comprar_mercador(int escolha, char *message, int msg_len) {
    ItemId id = item_mercador(escolha);
    int preco = preco_mercador(escolha);
    if (id == ITEM_VAZIO) {
        if (message) snprintf(message, msg_len, "Opcao invalida");
        return -3;
    }
    if (jogador.moeda < preco) {
        if (message) snprintf(message, msg_len, "Sem moeda suficiente!");
        return -1;
    }
    if (!mochila_adicionar(id, 1)) {
        if (message) snprintf(message, msg_len, "Sua mochila esta cheia");
        return -2;
    }
    jogador.moeda -= (float)preco;
    if (message) snprintf(message, msg_len, "Item comprado: %s", nome_item(id));
    return 0;
}

const char *texto_carta(int carta) {
    switch (carta) {
        case 1:
            return "Finalmente entrei numa masmorra, estou tao empolgado, quero achar "
                   "incriveis tesouros aqui e ficar muito forte!\n\n"
                   "Derrotei alguns monstros, foi dificil, mas eu consegui, algo "
                   "nessa masmorra me deixa intrigado, parece que essa masmorra nao tem "
                   "fim, e salao atras de salao e nunca chego no final dela...";
        case 2:
            return "Me sinto observado, nao sei o que esta acontecendo aqui dentro, "
                   "mas estou ficando assustado...\n\n"
                   "Continuo andando pelos saloes desse lugar, nao sei quanto tempo "
                   "se passou, estou desesperado...\n\n"
                   "Estou preso nessa masmorra, sinto falta da luz do sol, nao sei se "
                   "vou conseguir sair daqui...";
        case 3:
            return "Eu deveria ter escutado aquele moco, foi como ele disse, "
                   "'Cuidado com o abismo, as vezes quando voce sorri para ele, ele "
                   "sorri de volta pra ti...'\n"
                   "Finalmente entendi o que ele quis dizer...\n\n"
                   "Ele esta aqui, nao acredito, isso nao pode ser real...\n\n"
                   "Estou desesperado por ajuda...\n\nSocorro...";
        case 4:
            return "Uma dica do seu amigo das cartas, evite fugir de suas batalhas, "
                   "enfrente seus desafios, quanto mais fugirmos, mais perto estamos "
                   "do abismo...\n\n"
                   "Se lutar, chegaremos ao sol mais cedo ou mais tarde...\n\n"
                   "Se fugirmos, ele chegara mais perto de nos...\n\n"
                   "Lute...\n\nCuidado com os sorrisos dele...";
        default:
            return "Qual a necessidade de lutar...\n\n"
                   "Continue fugindo...\n\n"
                   "Quanto mais voce fugir, mais cedo chegara em casa...\n\n"
                   "Chegara vivo...\n\n"
                   "Voce podera ver a luz do sol novamente...\n\n"
                   "Fuja...\n\n:)";
    }
}

void reset_status_porta_magica(void) {
    jogador.vida = jogador.vida_max;
    jogador.energia = jogador.energia_max;
    jogador.def = jogador.def_max;
}

int escolher_proximo_boss(int *vazio_final) {
    int candidatos[QTD_BOSSES];
    int n = 0, i;
    for (i = 0; i < QTD_BOSSES; i++) {
        if (!bosses_derrotados[i]) candidatos[n++] = i + 1;
    }
    if (n == 0) {
        if (vazio_final) *vazio_final = 1;
        return 1;
    }
    if (vazio_final) *vazio_final = 0;
    return candidatos[rand() % n];
}

void floating_clear(FloatingText *arr) {
    int i;
    for (i = 0; i < FLOAT_TEXT_MAX; i++) arr[i].life = 0;
}

void floating_spawn(FloatingText *arr, float x, float y, const char *text, int crit, int heal) {
    int i;
    for (i = 0; i < FLOAT_TEXT_MAX; i++) {
        if (arr[i].life <= 0) {
            strncpy(arr[i].text, text, sizeof(arr[i].text) - 1);
            arr[i].text[sizeof(arr[i].text) - 1] = '\0';
            arr[i].x = x;
            arr[i].y = y;
            arr[i].life = 1.4f;
            arr[i].vy = -40.0f;
            arr[i].is_crit = crit;
            arr[i].is_heal = heal;
            if (heal) { arr[i].r = 120; arr[i].g = 220; arr[i].b = 120; }
            else if (crit) { arr[i].r = 230; arr[i].g = 190; arr[i].b = 60; }
            else { arr[i].r = 220; arr[i].g = 70; arr[i].b = 70; }
            return;
        }
    }
}

void floating_update(FloatingText *arr, float dt) {
    int i;
    for (i = 0; i < FLOAT_TEXT_MAX; i++) {
        if (arr[i].life > 0) {
            arr[i].life -= dt;
            arr[i].y += arr[i].vy * dt;
        }
    }
}
