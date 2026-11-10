# Gabinete do RASTRO (impressão 3D)

Caixa de parede ou de mesa (130 × 150 × 68 mm) com um olho na frente: a cúpula do sensor PIR é a pupila, a lente da ESP32-CAM fica acima com um ponto de "REC", e o LCD fica embaixo.

## Arquivos

| Arquivo | O que é |
|---|---|
| `rastro_gabinete.scad` | Modelo paramétrico no OpenSCAD (escolha `PECA = "caixa"` ou `"tampa"`, F6 e Export STL) |
| `build_case.py` | Gera `rastro_caixa.stl` e `rastro_tampa.stl` com Python (`pip install numpy pillow`, depois `python build_case.py`) |
| `render.py` | Gera as imagens de prévia a partir do modelo |

## Impressão

- As peças já saem na orientação certa: caixa com a frente na mesa, tampa com a gravação para cima. Não precisa de suporte.
- PLA, camada 0,2 mm, 3 paredes, 15% de preenchimento (cerca de 150 g a caixa e 50 g a tampa).
- Para gravações coloridas: imprimir em preto até 0,8 mm, trocar para vermelho até 1,2 mm e voltar ao preto.
- Se o fatiador pedir para reparar a malha, aceite (a malha é gerada a partir de voxels de 0,4 mm).

## Montagem

- LCD 16x2 I2C: parafusos M2,5 nas quatro colunas (furos a 75 × 31 mm).
- Cúpula do PIR no furo do olho, módulo da câmera no encaixe quadrado e buzzer atrás da grade: cola quente.
- Protoboard (82 × 55 mm) e ESP32-CAM: fita dupla-face no fundo.
- Tampa: 4 parafusos M3 × 8 mm auto-atarraxantes. Os furos de fechadura servem para pendurar na parede.
- O cabo USB sai pelo furo na parede de baixo.

As medidas são de módulos padrão: confira com régua ou paquímetro antes de imprimir e ajuste no início do `.scad` ou do `build_case.py`.
