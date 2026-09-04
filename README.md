# Dungeon Quest

RPG de texto em C com exploração de masmorra, classes, combate por turnos, bosses e lore em cartas.

## Como compilar (Windows + Visual Studio)

```powershell
cd "caminho\Dungeon Quest"
cmd /c 'call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" && cl /nologo /W3 /utf-8 /Fe:dungeon_quest.exe main.c'
```

## Como rodar

```powershell
.\dungeon_quest.exe
```

## Controles

- **1–3**: escolher caminho (Frente, Esquerda, Direita)
- **4**: abrir mochila
- No combate: **Atacar**, **Habilidade**, **Item**, **Fugir** (bosses não permitem fuga)

## Classes

- **Guerreiro** — vida e defesa altas
- **Ladino** — crítico e fuga
- **Mago** — magias e energia
