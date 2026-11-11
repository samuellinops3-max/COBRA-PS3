#include <stdio.h>
#include <stdlib.h>

int main() {
    int x = 10, y = 10;
    int comidaX = 15, comidaY = 12;
    int pontos = 0;
    int tamanho = 3;
    char tecla;
    
    printf("=== SLITHER.IO OFFLINE PS3 ===\n");
    printf("Com audio! \a\n");
    printf("Use W A S D pra andar, Q pra sair\n");
    printf("Coma as bolinhas pra crescer!\n\n");

    while(1) {
        printf("\n--- MAPA ---\n");
        printf("Voce: X=%d Y=%d | Tamanho: %d | Pontos: %d\n", x, y, tamanho, pontos);
        printf("Comida: X=%d Y=%d\n", comidaX, comidaY);
        printf("       ^\n");
        printf("       W\n");
        printf("A < POU > D  <- seu Slither\n");
        printf("       S\n");
        printf("       v\n");
        printf("Move (W/A/S/D/Q): ");
        scanf(" %c", &tecla);

        if(tecla=='q' || tecla=='Q') break;
        if(tecla=='w' || tecla=='W') y--;
        if(tecla=='s' || tecla=='S') y++;
        if(tecla=='a' || tecla=='A') x--;
        if(tecla=='d' || tecla=='D') x++;

        // Comeu?
        if(x==comidaX && y==comidaY) {
            pontos += 10;
            tamanho++;
            comidaX = rand() % 20 + 1;
            comidaY = rand() % 20 + 1;
            printf("\a"); // AUDIO - BIP quando come
            printf("\nNHAM! +10 PONTOS! \a\n");
            printf("Cresceu! Tamanho agora: %d\n", tamanho);
        } else {
            printf("... andando ...\n");
        }

        if(pontos >= 50) {
            printf("\nVOCE VIROU A MAIOR COBRA! \a\a\a\n");
            break;
        }
    }

    printf("\nFIM DE JOGO! Pontos: %d Tamanho: %d\n", pontos, tamanho);
    return 0;
}