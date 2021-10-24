#include <iostream>
#include <time.h>
#include "Game.cpp"

int main(){
    srand(time(NULL));
    initscr();
    curs_set(0);
    if(has_colors() == FALSE){
        endwin();
        printf("Your terminal does not support color\n");
        getch();
        return 0;
    }
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    keypad(stdscr,true);
    noecho();
    cbreak();
    Game game;
    game.Start();
    
    endwin();   
   
    return 0;
}