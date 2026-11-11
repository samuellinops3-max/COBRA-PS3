#include <psl1ght.h>
#include <tiny3d.h>
#include <io/pad.h>
#include <stdio.h>

int main(){
    tiny3d_Init(1024*1024);
    padInfo padinfo;
    padData paddata;
    ioPadInit(7);

    int snakeX = 640, snakeY = 360;
    int foodX = 300, foodY = 200;
    int dirX = 10, dirY = 0;
    int score = 0;

    while(1){
        ioPadGetInfo(&padinfo);
        if(padinfo.status[0]){
            ioPadGetData(0, &paddata);
            if(paddata.BTN_CROSS) break;
            if(paddata.BTN_UP) {dirX=0; dirY=-10;}
            if(paddata.BTN_DOWN) {dirX=0; dirY=10;}
            if(paddata.BTN_LEFT) {dirX=-10; dirY=0;}
            if(paddata.BTN_RIGHT) {dirX=10; dirY=0;}
        }
        snakeX += dirX; snakeY += dirY;
        if(snakeX<0) snakeX=1280; if(snakeX>1280) snakeX=0;
        if(snakeY<0) snakeY=720; if(snakeY>720) snakeY=0;
        if(abs(snakeX-foodX)<20 && abs(snakeY-foodY)<20){
            score++;
            foodX = rand()%1200+40; foodY = rand()%650+40;
        }

        tiny3d_Clear(0xff000000, TINY3D_CLEAR_ALL);
        tiny3d_Project2D();
        // comida vermelha
        tiny3d_SetPolygon(TINY3D_QUADS);
        tiny3d_VertexPos(foodX, foodY, 0);
        tiny3d_VertexColor(0xff0000ff);
        tiny3d_VertexPos(foodX+20, foodY, 0);
        tiny3d_VertexColor(0xff0000ff);
        tiny3d_VertexPos(foodX+20, foodY+20, 0);
        tiny3d_VertexColor(0xff0000ff);
        tiny3d_VertexPos(foodX, foodY+20, 0);
        tiny3d_VertexColor(0xff0000ff);
        tiny3d_End();
        // cobrinha verde
        tiny3d_SetPolygon(TINY3D_QUADS);
        tiny3d_VertexPos(snakeX, snakeY, 0);
        tiny3d_VertexColor(0xff00ff00);
        tiny3d_VertexPos(snakeX+30, snakeY, 0);
        tiny3d_VertexColor(0xff00ff00);
        tiny3d_VertexPos(snakeX+30, snakeY+30, 0);
        tiny3d_VertexColor(0xff00ff00);
        tiny3d_VertexPos(snakeX, snakeY+30, 0);
        tiny3d_VertexColor(0xff00ff00);
        tiny3d_End();
        tiny3d_Flip();
    }
    return 0;
}