/*************************************************************************************
 *  Copyright (C) 2011 by Martín Muñoz del Río <martinmunozdr@gmail.com>        *

 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *************************************************************************************/
#include <ncurses.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

#define PROFUNDIDAD 7//hasta 7 juega en un tiempo rápido y aceptable en corel duo 2 de ram
#define MAX_JUGADAS 300 //Este número probablemente sea excesivo, mas vale que sobre.

//MACROS
#define invertir(j) ( (j=='N') ? 'B' : 'N' )
#define es_ficha(f,j) (arr_fichas[(j-64)>>3][f-32])

int arr_fichas[2][85];
int evaluar_fichas[2][85];

//[FILA*8+COLUMNA]
char tablero[64];
char turno = 'B'; //'B': juegan blancas, 'N' juegan negras

//FUNCIONES INTERFACE USUARIO
void inicializar_tablero();
void dibujar_tablero();
void juega_usuario();

//FUNCIONES RESPETAR REGLAS

inline void convertir_peones_reinas(char *ptablero){
    register int i=0;
    for(i=0;i<8;i++){
        if(ptablero[i]=='P')
            ptablero[i]='Y';
        if(ptablero[56 + i]=='p')
            ptablero[56 + i]='y';
    }
}

inline void realizar_jugada(char *ptablero,int fi,int ci,int ff,int cf){
    ptablero[ff*8 + cf] = ptablero[fi*8 + ci];
    ptablero[fi*8 + ci] = ' ';
    convertir_peones_reinas(ptablero);
}

inline void copiar_tablero(char *ptablero, char *destino){
    register int i=0;
    for(;i<64;i++)
        destino[i] = ptablero[i];
}

inline void encontrar_posicion_rey(char *ptablero,char jugador,int *fila,int *colu){
    register int i=0;

    if(jugador=='B')
        for(;i<64;i++)
            if(ptablero[i]=='R'){
                *fila = i>>3;
                *colu = i - (i>>3)*8;
                return;
            }

    for(;i<64;i++)
        if(ptablero[i]=='r'){
            *fila = i>>3;
            *colu = i - (i>>3)*8;
            return;
        }
}

int vacio_en_medio(char* ptablero,int fi,int ci, int ff, int cf);
int cumple_reglas(char* ptablero, char jugador,int fi,int ci, int ff, int cf);
int jugador_en_jaque(char* ptablero, char jugador);
int es_mov_valido(char* ptablero, char jugador,int fi,int ci, int ff, int cf);
int es_mate(char *ptablero); //retorna el ganador (1:BLANCAS GANA, 2:NEGRAS GANA), o 0 si no hay ganador aun
int es_empate(char *ptablero,char jugador);
int evaluar_termino();

//FUNCIONES OPTIMIZAN
void quicksort(int* izq, int* der,int *init,int *jugadas); //ordena jugadas menor a mayor segun puntaje
void inicializar_adicionales();

inline int evaluar_tablero(char *ptablero,char jugador)
{
    int puntaje=0, jug = (jugador-64)>>3;
    register int i;

    for(i=0;i<64;i++)
            puntaje += evaluar_fichas[jug][ptablero[i]-32];

    return puntaje;
}

//FUNCIONES IA
int encontrar_jugada_posible(char* ptablero, char jugador,int *i,int *j,int nueva_llamada);
void alfa_beta(int profundidad,char *ptablero,char jugador,int *jugada_mejor, int *puntaje,int niv_rec,int a,int b);
void alfa_beta_opt(int profundidad,char *ptablero,char jugador,int *jugada_mejor, int *puntaje,int niv_rec,int a,int b);
void juega_maquina();

int main () {
        srand(time(NULL));
        inicializar_tablero();
        inicializar_adicionales();
        initscr();
        dibujar_tablero();
        refresh();

        while(1){
            juega_usuario();
            dibujar_tablero();
            refresh();
            turno = 'N';
            if(evaluar_termino())
                break;

            juega_maquina();
            dibujar_tablero();
            refresh();
            turno = 'B';
            if(evaluar_termino())
                break;
        }

        refresh();
        getch();
        endwin();
        return 0;
}

void quicksort(int* izq, int* der,int *init,int *jugadas)
{
        if(der<izq) return;
        int pivot=*izq;
        int* ult=der;
        int* pri=izq;
        int temp;
 
        while(izq<der)
        {
                while(*izq<=pivot && izq<der+1) izq++;
                while(*der>pivot && der>izq-1) der--;
                if(izq<der){
                   temp=*izq;
                   *izq=*der;
                   *der=temp;

                   temp = *(jugadas+(izq-init));
                   *(jugadas+(izq-init)) = *(jugadas+(der-init));
                   *(jugadas+(der-init)) = temp;
                }
        }

        temp=*pri;
        *pri=*der;
        *der=temp;

        temp = *(jugadas+(pri-init));
        *(jugadas+(pri-init)) = *(jugadas+(der-init));
        *(jugadas+(der-init)) = temp;

        quicksort(pri,der-1,init,jugadas);
        quicksort(der+1,ult,init,jugadas);
}

void inicializar_adicionales(){
    arr_fichas[0][0] = arr_fichas[1][0] = 0;
    arr_fichas[1]['t'-32] = arr_fichas[0]['T'-32] = 1;
    arr_fichas[1]['c'-32] = arr_fichas[0]['C'-32] = 1;
    arr_fichas[1]['a'-32] = arr_fichas[0]['A'-32] = 1;
    arr_fichas[1]['y'-32] = arr_fichas[0]['Y'-32] = 1;
    arr_fichas[1]['p'-32] = arr_fichas[0]['P'-32] = 1;
    arr_fichas[1]['r'-32] = arr_fichas[0]['R'-32] = 1;
    arr_fichas[0]['t'-32] = arr_fichas[1]['T'-32] = 0;
    arr_fichas[0]['c'-32] = arr_fichas[1]['C'-32] = 0;
    arr_fichas[0]['a'-32] = arr_fichas[1]['A'-32] = 0;
    arr_fichas[0]['y'-32] = arr_fichas[1]['Y'-32] = 0;
    arr_fichas[0]['p'-32] = arr_fichas[1]['P'-32] = 0;
    arr_fichas[0]['r'-32] = arr_fichas[1]['R'-32] = 0;

    evaluar_fichas[0][0] = evaluar_fichas[1][0] = 0;
    evaluar_fichas[1]['t'-32] = evaluar_fichas[0]['T'-32] = 500;
    evaluar_fichas[1]['c'-32] = evaluar_fichas[0]['C'-32] = 300;
    evaluar_fichas[1]['a'-32] = evaluar_fichas[0]['A'-32] = 300;
    evaluar_fichas[1]['y'-32] = evaluar_fichas[0]['Y'-32] = 900;
    evaluar_fichas[1]['p'-32] = evaluar_fichas[0]['P'-32] = 100;
    evaluar_fichas[0]['t'-32] = evaluar_fichas[1]['T'-32] = -evaluar_fichas[0]['T'-32];
    evaluar_fichas[0]['c'-32] = evaluar_fichas[1]['C'-32] = -evaluar_fichas[0]['C'-32];
    evaluar_fichas[0]['a'-32] = evaluar_fichas[1]['A'-32] = -evaluar_fichas[0]['A'-32];
    evaluar_fichas[0]['y'-32] = evaluar_fichas[1]['Y'-32] = -evaluar_fichas[0]['Y'-32];
    evaluar_fichas[0]['p'-32] = evaluar_fichas[1]['P'-32] = -evaluar_fichas[0]['P'-32];
}

void inicializar_tablero(){
    int i=0,j=0;
    for(i=0;i<8;i++){
        tablero[1*8+i]='p';
        tablero[6*8+i]='P';
    }

    for(i=2;i<6;i++)
        for(j=0;j<8;j++)
            tablero[i*8+j] = ' ';

    tablero[0*8+0]='t';
    tablero[0*8+1]='c';
    tablero[0*8+2]='a';
    tablero[0*8+3]='y';
    tablero[0*8+4]='r';
    tablero[0*8+5]='a';
    tablero[0*8+6]='c';
    tablero[0*8+7]='t';
    tablero[7*8+0]='T';
    tablero[7*8+1]='C';
    tablero[7*8+2]='A';
    tablero[7*8+3]='Y';
    tablero[7*8+4]='R';
    tablero[7*8+5]='A';
    tablero[7*8+6]='C';
    tablero[7*8+7]='T';
}

void dibujar_tablero(){
    int i=1,j=0;
    mvprintw(1,3,"a b c d e f g h");
    for(i=0;i<8;i++)
        mvprintw(2+i,1,"%d",8-i);

    for(i=0;i<8;i++)
        for(j=0;j<8;j++)
            mvprintw(i+2,j*2 +3,"%c",tablero[i*8+j]);
}

int cumple_reglas(char* ptablero, char jugador,int fi,int ci, int ff, int cf){
         if(!es_ficha(ptablero[fi*8 + ci],jugador))
             return 0;

         switch(ptablero[fi*8 + ci]){
             case 'T':
             case 't':
                 if((fi==ff || ci==cf) && !es_ficha(ptablero[ff*8 + cf],jugador) &&
                    vacio_en_medio(ptablero,fi,ci,ff,cf))
                      return 1;
                 break;
             case 'C':
             case 'c':
                 if( ((abs(fi-ff) == 2 && abs(ci-cf) == 1) ||
                    (abs(fi-ff) == 1 && abs(ci-cf) == 2) ) &&
                   !es_ficha(ptablero[ff*8 + cf],jugador))
                      return 1;
                 break;
             case 'A':
             case 'a':
                 if((abs(fi-ff) == abs(ci-cf)) &&
                   !es_ficha(ptablero[ff*8 + cf],jugador) &&
                    vacio_en_medio(ptablero,fi,ci,ff,cf))
                      return 1;
                 break;
             case 'Y':
             case 'y':
                 if((fi==ff || ci==cf) && !es_ficha(ptablero[ff*8 + cf],jugador) &&
                    vacio_en_medio(ptablero,fi,ci,ff,cf))
                      return 1;
                 if((abs(fi-ff) == abs(ci-cf)) &&
                   !es_ficha(ptablero[ff*8 + cf],jugador) &&
                    vacio_en_medio(ptablero,fi,ci,ff,cf))
                      return 1;
                 break;
             case 'R':
             case 'r':
                 if( ((abs(fi-ff) == 1 && abs(ci-cf) == 0) ||
                    (abs(fi-ff) == 0 && abs(ci-cf) == 1) ||
                    (abs(fi-ff) == 1 && abs(ci-cf) == 1)  ) &&
                   !es_ficha(ptablero[ff*8 + cf],jugador))
                      return 1;
                 break;
             case 'P':
                 if( fi-ff==1 && ptablero[ff*8 + cf]==' ' && cf==ci)
                      return 1;
                 if( fi-ff==2 && ptablero[ff*8 + cf]==' ' && cf==ci &&
                     fi==6 && vacio_en_medio(ptablero,fi,ci,ff,cf) )
                      return 1;
                 if( fi-ff==1 && abs(ci-cf)==1 &&
                     es_ficha(ptablero[ff*8 + cf],invertir(jugador)))
                      return 1;
                 break;
             case 'p':
                 if( ff-fi==1 && ptablero[ff*8 + cf]==' ' && cf==ci)
                      return 1;
                 if( ff-fi==2 && ptablero[ff*8 + cf]==' ' && cf==ci &&
                     fi==1 && vacio_en_medio(ptablero,fi,ci,ff,cf) )
                      return 1;
                 if( ff-fi==1 && abs(ci-cf)==1 &&
                     es_ficha(ptablero[ff*8 + cf],invertir(jugador)))
                      return 1;
                 break;
         }
    
         return 0;
}

int jugador_en_jaque(char* ptablero, char jugador){
    int fila=0,col=0;
    register int i=0;
    encontrar_posicion_rey(ptablero, jugador, &fila, &col);

    for(i=0;i<64;i++)
            if(es_ficha(ptablero[i],invertir(jugador)))
                if(cumple_reglas(ptablero,invertir(jugador),i>>3,i-(i>>3)*8,fila,col))
                    return 1;

    return 0;
}

int es_mov_valido(char* ptablero, char jugador,int fi,int ci, int ff, int cf){
    if(!cumple_reglas(ptablero,jugador,fi,ci,ff,cf))
        return 0;

    char copia_tablero[64];
    copiar_tablero(ptablero,copia_tablero);
    realizar_jugada(copia_tablero,fi,ci,ff,cf);

    if(jugador_en_jaque(copia_tablero,jugador))
        return 0;

    return 1;
}

int vacio_en_medio(char* ptablero,int fi,int ci, int ff, int cf){
    register int i=0;

    int inc = 0;

    if(fi==ff){
        inc = (abs(cf-ci)==(cf-ci))?1:-1;

        for(i=ci+inc;i<cf || i>cf; i+=inc  )
            if(ptablero[fi*8+i]!=' ')
                return 0;
    }

    if(ci==cf){
        inc = (abs(ff-fi)==(ff-fi))?1:-1;

        for(i=fi+inc; i<ff || i>ff; i+=inc  )
            if(ptablero[i*8+ci]!=' ')
                return 0;
    }

    //asumo la diferencia de abs es igual (cf-ci == ff-fi), es una precondicion de esta funcion
    if(fi!=ff && ci!=cf){
        int lim = abs(ff-fi), mul1 = (lim==(ff-fi))?1:-1, mul2 = (abs(cf-ci)==(cf-ci))?1:-1;
        for(i=1;i<lim; i++ )
            if(ptablero[(fi+i*mul1)*8 + ci+i*mul2]!=' ')
                return 0;
    }

    return 1;
}

int es_mate(char *ptablero){
    int a1=0,a2=0;
    if(jugador_en_jaque(ptablero, 'B')){
        if(!encontrar_jugada_posible(ptablero,'B',&a1,&a2,0))
            return 2;
    }

    if(jugador_en_jaque(ptablero, 'N')){
        if(!encontrar_jugada_posible(ptablero,'N',&a1,&a2,0))
            return 1;        
    }

    return 0;
}

int es_empate(char *ptablero,char jugador)
{
    int a1=0,a2=0;
    if(jugador=='B' && !encontrar_jugada_posible(ptablero,'B',&a1,&a2,0))
        return 1;

    if(jugador=='N' && !encontrar_jugada_posible(ptablero,'N',&a1,&a2,0))
        return 1;

    return 0;
}

int encontrar_jugada_posible(char* ptablero, char jugador,int *i2,int *j2,int nueva_llamada){
    register int j=*j2,i=*i2;

    for(;i<64;i++)
           if(es_ficha(ptablero[i],jugador))
                for(j = (nueva_llamada)?(j+1+(nueva_llamada=0)):0;j<64;j++)
                        if(es_mov_valido(ptablero,jugador,i>>3,i-((i>>3)*8),j>>3,j-((j>>3)*8))){
                            *i2=i; *j2=j;
                            return i*64 + j;
                        }

    return 0;
}

void alfa_beta(int profundidad,char *ptablero,char jugador,int *jugada_mejor, int *puntaje,int niv_rec,int a,int b)
{
    int jugada_elegida=0,puntaje_elegido=10000,jugada=0;
    char copia_tablero[64];
    int a1=0,a2=0;

        while(jugada = encontrar_jugada_posible(ptablero,jugador,&a1,&a2,jugada)){

            copiar_tablero(ptablero,copia_tablero);
            realizar_jugada(copia_tablero,jugada>>9,(jugada>>6)%8,(jugada>>3)%8,jugada%8);

            if(profundidad>1)
                alfa_beta(profundidad-1, copia_tablero, invertir(jugador),jugada_mejor,puntaje,niv_rec+1,a,b);
            else
                *puntaje = evaluar_tablero(copia_tablero,invertir(jugador));

            if((niv_rec + PROFUNDIDAD)%2){ //MIN
                if(*puntaje<puntaje_elegido || jugada_elegida==0){
                    puntaje_elegido = *puntaje;
                    jugada_elegida = jugada;
                }

                b = puntaje_elegido;

                if(b<=a){
                    *puntaje = a;
                    *jugada_mejor = jugada_elegida;
                    return;
                }
            }
            else{   //MAX
                if(*puntaje>puntaje_elegido || jugada_elegida==0){
                    puntaje_elegido = *puntaje;
                    jugada_elegida = jugada;
                }

                a = puntaje_elegido;

                if(a>=b){
                    *puntaje = b;
                    *jugada_mejor = jugada_elegida;
                    return;
                }
            }

        }

       *puntaje = puntaje_elegido;
       *jugada_mejor = jugada_elegida;
    
}



void alfa_beta_opt(int profundidad,char *ptablero,char jugador,int *jugada_mejor, int *puntaje,int niv_rec,int a,int b)
{
    int jugada_elegida=0,puntaje_elegido=10000,jugada=0,num_jugadas=0;
    char copia_tablero[64];
    register int i=0;
    int a1=0,a2=0,puntajes_ordenar[MAX_JUGADAS],jugadas_ordenadas[MAX_JUGADAS];

        //MAX_JUGADAS no se está comprobando, pero se está tomando un número de sobra grande (evaluar)
        while(jugadas_ordenadas[num_jugadas] = encontrar_jugada_posible(ptablero,jugador,&a1,&a2,num_jugadas)){

            copiar_tablero(ptablero,copia_tablero);
            realizar_jugada(copia_tablero,jugadas_ordenadas[num_jugadas]>>9,
                 (jugadas_ordenadas[num_jugadas]>>6)%8,(jugadas_ordenadas[num_jugadas]>>3)%8,
                 jugadas_ordenadas[num_jugadas]%8
            );


    if(es_mate(copia_tablero) ){
       *jugada_mejor = jugadas_ordenadas[num_jugadas];
if(profundidad%2)       *puntaje = -10000; else *puntaje=10000;//FALTA PROBAR PUEDEN SER AL REVES LOS SIGNOS
return;
}


            puntajes_ordenar[num_jugadas++] = -evaluar_tablero(copia_tablero, jugador);
        }

        quicksort(puntajes_ordenar, &puntajes_ordenar[num_jugadas-1],puntajes_ordenar,jugadas_ordenadas);

        for(;i<num_jugadas;i++){
            jugada = jugadas_ordenadas[i];

            copiar_tablero(ptablero,copia_tablero);
            realizar_jugada(copia_tablero,jugada>>9,(jugada>>6)%8,(jugada>>3)%8,jugada%8);

            if(profundidad<3)
                alfa_beta(profundidad-1, copia_tablero, invertir(jugador),jugada_mejor,puntaje,niv_rec+1,a,b);
            else
                alfa_beta_opt(profundidad-1, copia_tablero, invertir(jugador),jugada_mejor,puntaje,niv_rec+1,a,b);

            if((niv_rec + PROFUNDIDAD)%2){ //MIN
                if(*puntaje<puntaje_elegido || jugada_elegida==0){
                    puntaje_elegido = *puntaje;
                    jugada_elegida = jugada;
                }

                b = puntaje_elegido;

                if(b<=a){
                    *puntaje = a;
                    *jugada_mejor = jugada_elegida;
                    return;
                }
            }
            else{   //MAX
                if(*puntaje>puntaje_elegido || jugada_elegida==0){
                    puntaje_elegido = *puntaje;
                    jugada_elegida = jugada;
                }

                a = puntaje_elegido;

                if(a>=b){
                    *puntaje = b;
                    *jugada_mejor = jugada_elegida;
                    return;
                }
            }

        }

       *puntaje = puntaje_elegido;
       *jugada_mejor = jugada_elegida;
    
}

int evaluar_termino(){
    int final=0;
    if(final=es_mate(tablero)){
        if(final==1)
            mvprintw(14,3,"BLANCAS GANAN (MAYUSCULAS)");
        else
            mvprintw(14,3,"NEGRAS GANAN (MINUSCULAS)");

        return 1;
    }

    if(es_empate(tablero,turno)){
        mvprintw(14,3,"EMPATE");
        return 1;
    }

    return 0;
}

void juega_maquina(){
    int jugada, puntaje=0;

    alfa_beta_opt(PROFUNDIDAD, tablero, 'N',&jugada,&puntaje,0,-20000,20000);

    realizar_jugada(tablero,jugada>>9,(jugada>>6)%8,(jugada>>3)%8,jugada%8);
}

void juega_usuario(){
    int l=0,n=0,l2=0, n2=0;

    do{
        mvprintw(4,23,"Letra origen(a-h)");
        while(!((l=getch())>=97 && l<=104));

        mvprintw(5,23,"Numero origen(1-8)");
        while(!((n=getch())>=49 && n<=57));

        mvprintw(7,23,"Letra destino(a-h)");
        while(!((l2=getch())>=97 && l2<=104));

        mvprintw(8,23,"Numero destino(1-8)");
        while(!((n2=getch())>=49 && n2<=56));
    }while(!es_mov_valido(tablero,'B',8-(n-48),l-97,8-(n2-48),l2-97));

    realizar_jugada(tablero,8-(n-48),l-97,8-(n2-48),l2-97);
}
