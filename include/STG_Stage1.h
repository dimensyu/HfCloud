#include "HfCloud.h"
#include <unordered_map>
using namespace HfCloud;

struct FPS_Counter{
    long long last;
    long long now;
    long long feq;
    const char *title;
    char buf[200];
    int count;
    void start(){
        feq = SDL_GetPerformanceFrequency();
        last = SDL_GetPerformanceCounter();
        count = 0;
    }
    void addcnt(){
        if(++count == 60){
            now = SDL_GetPerformanceCounter();
            double sec = 1.0*(now-last)/feq;
            printf("FPS : %.2lf\n", 60.0/sec);
            last = SDL_GetPerformanceCounter();
            count = 0;
        }
    }
};

class SceneStage1 : public Scene{
    bool disposed;
    Sprite *sp_back;

    FPS_Counter fps;
public:
    std::vector<std::function<void(void)> > tasks;  //render tasks
    std::map<std::pair<int, int>, std::function<void(void)> >key_handlers;
    std::function<void(void)> logic_updater;

    Module *battle_module;
    virtual void start_scene(){
        Scene::start_scene();

        //---
        sp_back = new Sprite("sources/stage/back.png");
        main_module->manage(sp_back);

        battle_module = new Module(25, 25, 400, 430);

        fps.start();

        disposed = false;
    }
    virtual void end_scene(){
        if(disposed)return;
        Scene::end_scene();

        //----
        key_handlers.clear();
        tasks.clear();

        //
        auto dispose = [&](Sprite *s){
            s->bitmap->dispose();
            delete s->bitmap;
            s->dispose();
            delete s;
        };
        dispose(sp_back);

        delete battle_module;

        disposed = true;
    }
    virtual void update(){
        Scene::update();
        if (disposed)return;
        for(auto &task : tasks)task();
        tasks.clear();
        for(auto &handler : key_handlers){
            int key = handler.first.first;
            int state = handler.first.second;
            if((state == SDL_KEYDOWN && Input::key_is_pressed(key)) || (state == SDL_KEYUP && !Input::key_is_pressed(key)))
                handler.second();
            if(disposed)break;
        }
        if (disposed)return;

        battle_module->update();

        logic_updater();

        fps.addcnt();
    }
};
struct Player{
    bool disposed;
    int dir;  //moving dir 0 stand 2 up 8 down 4 left 6 right. 10-dir
    Bitmap *player_map;
    Bitmap *player_bmp;
    Sprite *sprite;
    SceneStage1 *scene;
    int x, y, speed; double SQRT2;

    static const int SPEED_FAST = 5;
    static const int SPEED_SLOW = 2;

    void init(SceneStage1 *s){
        scene = s;
        player_map = new Bitmap("sources/th14/player/pl00/pl00.png");
        player_bmp = new Bitmap(32, 48);
        sprite = new Sprite(player_bmp);
        sprite->opacity = 0;
        x = 400/2-32/2, y = 430-48; SQRT2 = sqrt(2);
        speed = SPEED_FAST;
        scene->tasks.push_back([&](){
            scene->battle_module->manage(sprite);
            sprite->setpos(x, y);
            sprite->opacity = 255;
        });
        disposed = false;

        dir = 2;
        update();
    }
    void destroy(){
        if(disposed)return;
        player_bmp->dispose();
        delete player_bmp;
        player_map->dispose();
        delete player_map;
        sprite->dispose();
        delete sprite;
        disposed = true;
    }

    void update(){
        if(disposed)return;
        #define calc(x, y) {x*32, y*48}
        //speed
        static int pos[10][2] = {
        calc(2, 0),   //0
        {0,0},
        calc(1, 0),   //2
        {0,0},
        calc(4, 1),    //4
        {0,0},
        calc(4, 2),    //6
        {0,0},
        calc(0, 2),    //8
        {0,0}
        };
        player_bmp->clear();
        player_bmp->blt(HfRect(0, 0, 32, 48), player_map, HfRect(pos[dir][0], pos[dir][1], 32, 48));
        if(speed == SPEED_SLOW){
            player_bmp->fill_rect(32/2-8/2, 48/2-8/2, 8, 8, RGBA(0xff, 0xff, 0xff, 0xff));
            player_bmp->fill_rect(32/2-6/2, 48/2-6/2, 6, 6, RGBA(0xff, 0, 0, 0xff));
        }
        sprite->update();
        dir = 2;
        #undef calc

        //speed
    }
    inline void pos_correct(){
        if(x < 0)x = 0;
        if(y < 0)y = 0;
        if(x > 400-32)x = 400-32;
        if(y > 430-48)y = 430-48;
        sprite->setpos(x, y);
    }
    void on_keyup(){
        dir = 2;
        y -= (Input::key_is_pressed(SDLK_LEFT) || Input::key_is_pressed(SDLK_RIGHT))?(int)(1.0*speed/SQRT2):speed;
        pos_correct();
    }
    void on_keydown(){
        dir = 8;
        y += (Input::key_is_pressed(SDLK_LEFT) || Input::key_is_pressed(SDLK_RIGHT))?(int)(1.0*speed/SQRT2):speed;
        pos_correct();
    }
    void on_keyleft(){
        dir = 4;
        x -= (Input::key_is_pressed(SDLK_UP) || Input::key_is_pressed(SDLK_DOWN))?(int)(1.0*speed/SQRT2):speed;
        pos_correct();
    }
    void on_keyright(){
        dir = 6;
        x += (Input::key_is_pressed(SDLK_UP) || Input::key_is_pressed(SDLK_DOWN))?(int)(1.0*speed/SQRT2):speed;
        pos_correct();
    }
    void on_keyshift(bool keyup = false){
        if(keyup && speed == SPEED_SLOW)speed = SPEED_FAST;
        else if(!keyup)speed = SPEED_SLOW;
    }
};
struct PlayerShoot{
    bool disposed;
    SceneStage1 *scene;

    Bitmap *shoot_map;
    Bitmap *shoot_bmp;

    Bitmap *shoots_bmp;
    Sprite *shoots_sprite;
    void init(SceneStage1 *s){
        scene = s;

        shoot_map = new Bitmap("sources/th14/player/pl00/pl00.png");
        shoot_bmp = new Bitmap(20, 16);
        shoot_bmp->blt_ex(HfRect(4, 0, 16, 16), shoot_map, HfRect(240, 48*3, 16, 16), 255, HfPoint(8, 8), 90, false, false);
        shoot_bmp->blt_ex(HfRect(2, 2, 4, 16), shoot_map, HfRect(128, 48*3, 16, 4), 255, HfPoint(2, 8), 0, false, true);

        shoots_sprite = new Sprite(shoot_bmp);
        scene->tasks.push_back([&](){
            scene->battle_module->manage(shoots_sprite);
        });

        disposed = false;
    }
    void destroy(){
        if(disposed)return;
        shoot_map->dispose();
        shoot_bmp->dispose();
        delete shoot_map;
        delete shoot_bmp;

        disposed = true;
    }
};
class STAGE1{
    SceneStage1 *scene;
    Player player;
    PlayerShoot pshoot;

    void leave(){
        scene = nullptr;
        player.destroy();
        pshoot.destroy();
    }
public:
    void start(){
        SceneManager::call(scene = new SceneStage1);
        scene->logic_updater = [&,this](){update();};

        scene->key_handlers[std::make_pair(SDLK_ESCAPE, SDL_KEYDOWN)] = [&,this](){leave(); SceneManager::jumpback();};

        player.init(scene);
        scene->key_handlers[std::make_pair(SDLK_UP, SDL_KEYDOWN)] = [&,this](){player.on_keyup();};
        scene->key_handlers[std::make_pair(SDLK_DOWN, SDL_KEYDOWN)] = [&,this](){player.on_keydown();};
        scene->key_handlers[std::make_pair(SDLK_LEFT, SDL_KEYDOWN)] = [&,this](){player.on_keyleft();};
        scene->key_handlers[std::make_pair(SDLK_RIGHT, SDL_KEYDOWN)] = [&,this](){player.on_keyright();};
        scene->key_handlers[std::make_pair(SDLK_LSHIFT, SDL_KEYDOWN)] = [&,this](){player.on_keyshift(false);};
        scene->key_handlers[std::make_pair(SDLK_LSHIFT, SDL_KEYUP)] = [&,this](){player.on_keyshift(true);};

        pshoot.init(scene);
    }
    void update(){
        player.update();
    }
}stage1;
