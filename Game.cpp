#include <vector>
#include <deque>
#include <array>
#include <time.h>
#include <string>
#include <iostream>
#include <ncurses.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

class Game{
    using duration = std::chrono::duration<double>;
    using clock = std::chrono::system_clock;

    enum class CollisionType{
        Enemy,Player,EnemyBullet,PlayerBullet,Nothing
    };

public:
    struct update{
        duration updateDuration;
        clock::time_point lastUpdate;
        update(duration updateDuration,clock::time_point lastUpdate = clock::now()):
        updateDuration(updateDuration),lastUpdate(lastUpdate){}
    };
    
    //map
    static constexpr int WIDTH = 25;
    static constexpr int HEIGHT = 30;
    std::array<std::string,HEIGHT> space;
    update u_space = update((duration)0.1f);
    update u_draw = update((duration)1.0f / 15);
   
    //player
    int playerposX = 0;
    int playerposY = 0;
    int playerHealth = 5;
    std::array<std::array<char,3>,2> player {{
        {' ', '^', ' '},
        {'|', 'o', '|'}
    }};
    std::vector<std::pair<int, int>> pbullets;
    update u_shoot = (update((duration)0.4f));
    std::vector<update> u_pbullets;
    const float bulletSpeed = 0.07f;

    //enemies
    std::array<std::array<char,3>,2> enemy {{
        {'(', 'o', ')'},
        {' ', '\'', ' '}
    }};
    std::vector<std::pair<int,int>> enemies;
    std::vector<std::pair<int, int>> ebullets;
    std::vector<update> u_ebullets;
    std::vector<update> u_shootebullets;
    std::vector<update> u_enemies;
    const float ebulletSpeed = 0.1f;
    update spawnEnemy = (update((duration)0.25f));
    const float enemySpeed = 0.2f;

    //states & stuff 
    bool gameOver = false;
    std::mutex Bulletmtx;

    void Start(){
        //init stuff...
        for(int j = 0; j < HEIGHT-1; j++){
            for(int i = 0; i < WIDTH; i++){
                space[j][i] = rand() % 10 == 1 ? '.' : ' ';
            }
        }
        
        std::thread t_player(&Game::Player,this);
        u_space.lastUpdate = clock::now(); 
        u_draw.lastUpdate = clock::now();
        playerposY = WIDTH / 2 - player[0].size();
        playerposX = (HEIGHT - 1) - player.size()-1;
        
        //gameLoop
        do{
            Update();
            Draw();
        } while(!isGameOver());
        
        
        std::string str= "Your Health: "; //+ std::to_string(playerHealth);
        mvaddstr(HEIGHT+1, WIDTH / 2 - (str.size()+1) / 2, str.c_str());

        attron(COLOR_PAIR(playerHealth == 0 ? 2 : 1));
        mvaddstr(HEIGHT+1, WIDTH / 2 + (str.size()) / 2, std::to_string(playerHealth).c_str());
        attroff(COLOR_PAIR(playerHealth == 0 ? 2 : 1));

        attron(COLOR_PAIR(5));
        mvaddstr(HEIGHT / 2, WIDTH / 2 - 9, "Thanks for playing!");
        attroff(COLOR_PAIR(5));
    
        move(HEIGHT+2,0);
        char c = getch();
    }
    
    void Update(){       
        MoveSpace();
        Enemies();
        Bullets();
    }

     void MoveSpace(){
        if(!updateTime(u_space)) return;
        
        {
            std::array<std::string,HEIGHT> tmp = space;
            for(int j = 0; j < HEIGHT-1; j++){
                if(j+1 != HEIGHT-1){
                    tmp[j+1] = space[j];
                }
            }
        
            space = tmp;
        }

        for(int i = 0; i < WIDTH; i++)
          space[0][i] = rand() % 10 == 1 ? '.' : ' ';
        u_space.lastUpdate = clock::now();
    }
    
    void Enemies(){
        if(updateTime(spawnEnemy)) {
            if(rand() % 8 == 1){
                enemies.push_back({-enemy.size(), rand() % (WIDTH-enemy[0].size())});
                u_enemies.push_back(update((duration)enemySpeed));
                u_shootebullets.push_back(update((duration)1.0f));
            }
            spawnEnemy.lastUpdate = clock::now();
        }

        int i = 0;
         while(1){
            if(i >= enemies.size()) break;
            
            if(!updateTime(u_enemies[i])) {
                i++;
                continue;
            }

            if(enemies[i].first + 1 + enemy[0].size() < 0 || enemies[i].first + 1 > HEIGHT-1){
                enemies.erase(enemies.begin() + i);
                u_enemies.erase(u_enemies.begin() + i);
                u_shootebullets.erase(u_enemies.begin() + i);
            }
            else{
                enemies[i].first++;

                if(HandleCollision(enemies[i].first, enemies[i].second, CollisionType::Enemy, i))
                    continue;

                if(updateTime(u_shootebullets[i])){
                    if(enemies[i].first < HEIGHT-1-enemies.size() && rand() % 4 == 1){
                        ebullets.push_back({enemies[i].first + enemy.size(), enemies[i].second + (enemy[0].size() / 2)});
                        u_ebullets.push_back(update((duration)ebulletSpeed));
                    }
                    u_shootebullets[i].lastUpdate = clock::now();
                }

                u_enemies[i].lastUpdate = clock::now();
                i++;
            }
        }   
    }

    void Bullets(){
        auto BulletsManager = [&](std::vector<std::pair<int,int>>& bullets, std::vector<update>& u_bullets, const int dir, CollisionType collisiontype){
            int i = 0;
            while(1){
                if(i >= bullets.size()) break;
                
                if(!updateTime(u_bullets[i])) {
                    i++;
                    continue;
                }

                if(!CanMove(bullets[i].first + dir, bullets[i].second)){
                    bullets.erase(bullets.begin() + i);
                    u_bullets.erase(u_bullets.begin() + i);
                }
                else{
                    bullets[i].first += dir;
                    if(!HandleCollision(bullets[i].first,bullets[i].second,collisiontype,i)){
                        u_bullets[i].lastUpdate = clock::now();
                        i++;
                    }
                }
            }
        };

        Bulletmtx.lock();
        BulletsManager(pbullets, u_pbullets, -1, CollisionType::PlayerBullet);
        Bulletmtx.unlock();

        BulletsManager(ebullets, u_ebullets, 1, CollisionType::EnemyBullet);
    }

    inline bool updateTime(update u){
        return clock::now() - u.lastUpdate >= u.updateDuration;
    }
    
    void Player(){
        u_shoot.lastUpdate = clock::now();
        while(!isGameOver()){
            switch(getch()){
                case KEY_LEFT:
                    if(CanMove(playerposX, playerposY-1)) {
                        playerposY--;
                        HandleCollision(playerposX, playerposY, CollisionType::Player);
                    }
                    break;
                case KEY_RIGHT:
                    if(CanMove(playerposX,playerposY + 1 + player[0].size())) {
                        playerposY++;
                        HandleCollision(playerposX, playerposY, CollisionType::Player);
                    }
                    break;
                case ' ':
                    if(!updateTime(u_shoot)) break;

                    Bulletmtx.lock();
                    pbullets.push_back( { playerposX - 1, playerposY + (player[0].size() / 2) } );
                    u_pbullets.push_back(update((duration)bulletSpeed, clock::now()));
                    u_shoot.lastUpdate = clock::now();
                    Bulletmtx.unlock();
                    
                    break;
                case 'q': 
                    gameOver = true;
                    break;
                default: break;
            }
        }
    }
    bool isGameOver(){
        return playerHealth == 0 || gameOver;
    }
    
    bool CanMove(int x, int y){
        if(y < 0 || x < 0 || y > WIDTH-1 || x > HEIGHT-2) return false;
        return true;
    }
 
    bool HandleCollision(int x, int y, CollisionType type, int index = 0){
        switch(type){
            case CollisionType::Enemy:  //small bug fix please with the collision enemy bumping into player!!
                for(int i = 0; i < player.size(); i++)
                    for(int j = 0; j < player[0].size(); j++)
                        if(player[i][j] != ' ' && x == playerposX + i && y == playerposY + j){
                            enemies.erase(enemies.begin() + index);
                            u_enemies.erase(u_enemies.begin() + index);
                            u_shootebullets.erase(u_shootebullets.begin() + index);
                            playerHealth--;
                            return true;
                        }
                break;
            case CollisionType::EnemyBullet:
                for(int i = 0; i < player.size(); i++)
                    for(int j = 0; j < player[0].size(); j++){
                        if(player[i][j] != ' ' && x == playerposX + i && y == playerposY + j){
                            ebullets.erase(ebullets.begin() + index);
                            u_ebullets.erase(u_ebullets.begin() + index);
                            playerHealth--;
                            return true;
                        }
                    }
               
                for(int e = 0; e < enemies.size(); e++)
                    for(int j = 0; j < enemy[0].size(); j++){
                        if(enemy[0][j] != ' ' && x == enemies[e].first && y == enemies[e].second + j){
                            ebullets.erase(ebullets.begin() + index);
                            u_ebullets.erase(u_ebullets.begin() + index);
                            return true;
                        }
                    }
                break;
            case CollisionType::PlayerBullet:
                for(int e = 0; e < enemies.size(); e++)
                    for(int i = 0; i < enemy.size(); i++)
                        for(int j = 0; j < enemy[0].size(); j++){
                            if(enemy[i][j] != ' ' && x == enemies[e].first + i && y == enemies[e].second + j){
                                enemies.erase(enemies.begin() + e);
                                u_enemies.erase(u_enemies.begin() + e);
                                u_shootebullets.erase(u_shootebullets.begin() + e);
                                pbullets.erase(pbullets.begin() + index);
                                return true;
                            }
                        }
                break;
            case CollisionType::Player:
                for(int e = 0; e < enemies.size(); e++)
                    for(int i = 0; i < enemy.size(); i++)
                        for(int j = 0; j < enemy[0].size(); j++){
                            if(enemy[i][j] != ' ' && x == enemies[e].first + i && y == enemies[e].second + j){
                                enemies.erase(enemies.begin() + e);
                                u_enemies.erase(u_enemies.begin() + e);
                                u_shootebullets.erase(u_shootebullets.begin() + e);
                                playerHealth--;
                                return true;
                            }
                        }
                for(int i = 0; i < ebullets.size(); i++)
                    for(int j = 0; j < player.size(); j++)
                        for(int p = 0; p < player[j].size();p++)
                            if(player[i][j] != ' ' && x + j == ebullets[i].first && y + p == ebullets[i].second){
                                ebullets.erase(ebullets.begin() + i);
                                u_ebullets.erase(u_ebullets.begin() + i);
                                playerHealth--;
                                return true;
                            }
                break;
            default: break;
        }
      
        return false;
    }

    void Draw(){
        if(!updateTime(u_draw)) return;
        
        //drawing the space
        bool colored;
        for(int i = 0; i < HEIGHT-1; i++){
            for(int j = 0; j < WIDTH-1; j++){
                colored = rand() % 3 == 1;
                attron(COLOR_PAIR(/*colored ? 1 : */3));
                mvaddch(i,j, space[i][j]);
                attroff(COLOR_PAIR(/*colored ? 1 : */3));
            }
        }
        
        //drawing the player
        for(int i = 0; i < player.size(); i++){
            for(int j = 0; j < player[i].size(); j++){
                if(player[i][j] != ' ')
                    mvaddch(playerposX + i,playerposY + j, player[i][j]);
            }
        }

        //drawing the bullets
        for(const auto b : pbullets){
            attron(COLOR_PAIR(4));
            mvaddch(b.first,b.second,'*');
            attroff(COLOR_PAIR(4));
        }

        for(const auto b : ebullets){
            attron(COLOR_PAIR(2));
            mvaddch(b.first,b.second,'*');
            attroff(COLOR_PAIR(2));
        }

        //drawing the enemies
        for(auto& e : enemies)
            for(int i = 0; i < enemy.size(); i++){
                for(int j = 0; j < enemy[i].size(); j++){
                    if(enemy[i][j] != ' ' && i + e.first > -1 && e.first+i < HEIGHT-1)
                        mvaddch(e.first + i, e.second + j, enemy[i][j]);
                }
            }
        std::string str= "Your Health: " + std::to_string(playerHealth);
        mvaddstr(HEIGHT+1, WIDTH / 2 - 7, str.c_str());
        
        refresh();
        u_draw.lastUpdate = clock::now();
    }   
};