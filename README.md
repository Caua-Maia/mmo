# Dungeon Quest

RPG em C com camada gráfica **raylib** (estilo Darkest Dungeon). A lógica de combate, classes, itens e bosses permanece a mesma do jogo de terminal original.

## Estrutura

| Arquivo | Função |
|---------|--------|
| `main.c` | Loop raylib + máquina de estados de tela |
| `game_logic.c/.h` | Structs, combate, itens, XP (sem I/O de terminal) |
| `ui_render.c/.h` | Paleta, painéis, HUD, retratos, botões |
| `main_terminal.c` | Backup do jogo 100% texto original |
| `assets/` | Fontes opcionais (Cinzel / IM Fell English) |

## Como compilar (Windows + Visual Studio)

```powershell
cd "caminho\mmo"
cmd /c 'call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" && cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release && cmake --build build'
```

Se Ninja não estiver disponível, use o gerador do Visual Studio:

```powershell
cmd /c 'call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" && cmake -B build -G "Visual Studio 18 2026" -A x64 && cmake --build build --config Release'
```

A primeira compilação baixa o **raylib** via CMake FetchContent (precisa de internet e git).

## Como rodar

```powershell
.\build\Release\dungeon_quest.exe
# ou, com Ninja:
.\build\dungeon_quest.exe
```

## Controles

- **Mouse** nos botões, ou teclas **1–4** / **0** / **Enter** / **Espaço** conforme a tela
- Combate: Atacar, Habilidade, Item, Fugir (bosses não permitem fuga)
- Caminho: Frente / Esquerda / Direita / Mochila

## Classes

- **Guerreiro** — vida e defesa altas
- **Ladino** — crítico e fuga
- **Mago** — magias e energia
