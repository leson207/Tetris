#include <bits/stdc++.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
using namespace std;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef float f32;
typedef double f64;
#include "colors.h"
#define HEIGHT 22
#define WIDTH 10
#define GRID_SIZE 30
#define VISIBLE_HEIGHT 20
#define ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))
const u8 FRAMES_PER_DROP[]=
{
    48,
    43,
    38,
    33,
    28,
    23,
    18,
    13,
    8,
    6,
    5,
    5,
    5,
    4,
    4,
    4,
    3,
    3,
    3,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    1
};
const f32 TARGET_SECOND_PER_FRAME=1.f/60.f;
struct BLOCK
{
    const u8 *data;
    const s32 side;
};
inline BLOCK B_BUILD(const u8 *data,const s32 side)
{
    return {data,side};
}
inline u8 matrix_get(const u8 *matrix,s32 width,s32 row,s32 col)
{
    s32 index=row*width+col;
    return matrix[index];
}
inline void matrix_set(u8 *matrix,s32 width,s32 row,s32 col,u8 value)
{
    s32 index=row*width+col;
    matrix[index]=value;
}
inline u8 rotate_get(const BLOCK *block,s32 row,s32 col,s32 rotation)
{
    s32 side=block->side;
    switch(rotation)
    {
    case 0:
        return block->data[row*side+col];
    case 1:
        return block->data[(side-col-1)*side+row];
    case 2:
        return block->data[(side-row-1)*side+(side-col-1)];
    case 3:
        return block->data[col*side+(side-row-1)];
    }
    return 0;
}
const u8 BLOCK_1[]=
{
    0,0,0,0,
    1,1,1,1,
    0,0,0,0,
    0,0,0,0
};
const u8 BLOCK_2[]=
{
    2,2,
    2,2
};
const u8 BLOCK_3[]=
{
    3,3,3,
    0,3,0,
    0,0,0,
};
const u8 BLOCK_4[]=
{
    0,4,4,
    4,4,0,
    0,0,0,
};
const u8 BLOCK_5[]=
{
    5,5,0,
    0,5,5,
    0,0,0
};
const u8 BLOCK_6[]=
{
    6,0,0,
    6,6,6,
    0,0,0
};
const u8 BLOCK_7[]=
{
    0,0,7,
    7,7,7,
    0,0,0
};
BLOCK B_STORAGE[]=
{
    B_BUILD(BLOCK_1,4),
    B_BUILD(BLOCK_2,2),
    B_BUILD(BLOCK_3,3),
    B_BUILD(BLOCK_4,3),
    B_BUILD(BLOCK_5,3),
    B_BUILD(BLOCK_6,3),
    B_BUILD(BLOCK_7,3),
};
enum GAME_PHASE
{
    GAME_PHASE_START,
    GAME_PHASE_PLAY,
    GAME_PHASE_HANDLE,
    GAME_PHASE_OVER
};
struct Piece_State
{
    u8 index;
    s8 offset_row;
    s8 offset_col;
    u8 rotation;
};
struct Game_State
{
    u8 Board[HEIGHT*WIDTH];
    u8 filled_line[HEIGHT];
    Piece_State piece;
    Piece_State next_piece;

    s8 level;
    s8 start_level;
    s8 line_count;
    s8 pending_line_count;
    s32 points;

    GAME_PHASE Phase;

    f32 time;
    f32 next_drop_time;
    f32 highlight_end_time;
};
struct Input_State
{
    u8 left;
    u8 right;
    u8 up;
    u8 down;
    u8 a;

    s8 dleft;
    s8 dright;
    s8 dup;
    s8 ddown;
    s8 da;
};
enum Text_Align
{
    TEXT_ALIGNMENT_LEFT,
    TEXT_ALIGNMENT_CENTER,
    TEXT_ALIGNMENT_RIGHT,
};
inline bool check_fill_row(Game_State *game,s32 height,s32 width)
{
    game->pending_line_count=0;
    for(s32 row=0;row<height;row++)
    {
        game->filled_line[row]=0;
        for(s32 col=0;col<width;col++)
        {
            s32 value=matrix_get(game->Board,width,row,col);
            if(value) game->filled_line[row]++;
            else break;
        }
        if(game->filled_line[row]==width) game->pending_line_count++;
    }
    if(game->pending_line_count) return true;
    return false;
}
inline u8 check_row_empty(const u8 *values,s32 width,s32 row)
{
    for(u32 col=0;col<width;col++)
        if(matrix_get(values,width,row,col)) return 0;
    return 1;
}
void clear_line(u8 *board,s8 height,s8 width,const u8 *line_out,Mix_Chunk *handle_sound)
{
    s32 new_line=height-1;
    for(s32 row=height-1;row>=0;row--)
    {
        while(new_line>=0 && line_out[new_line]==width) new_line--;
        if(new_line<0) memset(board+row*width,0,width);
        else
        {
            if(new_line!=row) memcpy(board+row*width,board+new_line*width,width);
            new_line--;
        }
    }
    Mix_PlayChannel(-1,handle_sound,0);
    return;
}
bool check_piece_valid(const Piece_State *Piece,const u8 *board,s32 height,s32 width)
{
    const BLOCK *block=B_STORAGE+Piece->index;
    assert(block);
    for(u32 row=0;row<block->side;row++)
    {
        for(u32 col=0;col<block->side;col++)
        {
            u8 value=rotate_get(block,row,col,Piece->rotation);
            if(value)
            {
                s8 board_row=Piece->offset_row+row;
                s8 board_col=Piece->offset_col+col;
                if(board_row<0 || board_row>=height || board_col<0 || board_col>=width || matrix_get(board,width,board_row,board_col)) return false;
            }
        }
    }
    return true;
}
inline f32 get_time_to_next_drop(s32 level)
{
    return FRAMES_PER_DROP[level]*TARGET_SECOND_PER_FRAME;
}
inline s32 random_int(s32 minn,s32 maxx)
{
    s32 range=maxx-minn;
    return minn+rand()%range;
}
void spawn_piece(Game_State *game)
{
    game->piece=game->next_piece;
    game->next_piece={};
    game->next_piece.index=(u8)random_int(0,ARRAY_COUNT(B_STORAGE));
    game->next_piece.offset_col=WIDTH/2;
    game->next_drop_time=game->time+get_time_to_next_drop(game->level);
    return;
}
void merge_piece(Game_State *game)
{
    const BLOCK *block=B_STORAGE+game->piece.index;
    for(s32 row=0;row<block->side;row++)
    {
        for(s32 col=0;col<block->side;col++)
        {
            u8 value=rotate_get(block,row,col,game->piece.rotation);
            if(value)
            {
                s32 offset_row=game->piece.offset_row+row;
                s32 offset_col=game->piece.offset_col+col;
                matrix_set(game->Board,WIDTH,offset_row,offset_col,value);
            }
        }
    }
    return;
}
inline bool soft_drop(Game_State *game)
{
    game->piece.offset_row++;
    if(!check_piece_valid(&game->piece,game->Board,HEIGHT,WIDTH))
    {
        game->piece.offset_row--;
        merge_piece(game);
        spawn_piece(game);
        return false;
    }
    game->next_drop_time=game->time+get_time_to_next_drop(game->level);
    return true;
}
inline s32 Compute_point(s32 level,s32 line_count)
{
    switch(line_count)
    {
        case 1:
            return 40*(level+1);
        case 2:
            return 100*(level+1);
        case 3:
            return 300*(level+1);
        case 4:
            return 1200*(level+1);
    }
    return 0;
}
inline s32 get_lines_for_next_level(s32 start_level,s32 level)
{
    s32 first_level_up_limit=min(start_level*10+10,max(100,start_level*10-50));
    if(level==start_level) return first_level_up_limit;
    s32 diff=level-start_level;
    return first_level_up_limit+diff*10;
}
void Update_Game_Handle(Game_State *game,Mix_Chunk *handle_sound)
{
    if(game->time>=game->highlight_end_time)
    {
        clear_line(game->Board,HEIGHT,WIDTH,game->filled_line,handle_sound);
        Compute_point(game->level,game->pending_line_count);
        game->line_count+=game->pending_line_count;
        game->points+=Compute_point(game->level,game->pending_line_count);
        if(game->line_count>=get_lines_for_next_level(game->start_level,game->level)) game->level++;
        game->Phase=GAME_PHASE_PLAY;
    }
    return;
}
void Update_game_Play(Game_State *game,const Input_State *input)
{
    Piece_State Piece=game->piece;
    if(input->dleft>0) Piece.offset_col--;
    if(input->dright>0) Piece.offset_col++;
    if(input->dup>0) Piece.rotation=(Piece.rotation+1)%4;
    if(check_piece_valid(&Piece,game->Board,HEIGHT,WIDTH)) game->piece=Piece;
    if(input->ddown>0) soft_drop(game);
    if(input->da>0) while(soft_drop(game));
    while(game->time>=game->next_drop_time)
    {
        soft_drop(game);
    }
    if(check_fill_row(game,HEIGHT,WIDTH))
    {
        game->Phase=GAME_PHASE_HANDLE;
        game->highlight_end_time=game->time+0.5f;
    }
    if(!check_row_empty(game->Board,WIDTH,0)) game->Phase=GAME_PHASE_OVER;
    return;
}
void Update_game_Start(Game_State *game,const Input_State *input)
{
    if(input->ddown>0 && game->start_level>0) game->start_level--;
    if(input->dup>0 && game->start_level<30) game->start_level++;
    if(input->da>0)
    {
        memset(game->Board,0,WIDTH*HEIGHT);
        game->points=0;
        game->level=game->start_level;
        game->next_piece.index=random_int(0,ARRAY_COUNT(B_STORAGE));
        game->next_piece.offset_col=WIDTH/2;
        spawn_piece(game);
        game->Phase=GAME_PHASE_PLAY;
    }
    return;
}
void Update_game_Over(Game_State *game,const Input_State *input,Mix_Chunk *over_sound)
{
    Mix_PlayChannel(-1,over_sound,0);
    if(input->da>0) game->Phase=GAME_PHASE_START;
}
void Update_Game(Game_State *game,const Input_State *input,Mix_Chunk *handle_sound,Mix_Chunk *over_sound)
{
    switch(game->Phase)
    {
    case GAME_PHASE_START:
        Update_game_Start(game,input);break;
    case GAME_PHASE_PLAY:
        Update_game_Play(game,input);break;
    case GAME_PHASE_HANDLE:
        Update_Game_Handle(game,handle_sound);break;
    case GAME_PHASE_OVER:
        Update_game_Over(game,input,over_sound);break;
    }
};
void draw_string(SDL_Renderer *renderer,TTF_Font *font,const char *text,s32 x,s32 y,Text_Align alignment,Color color)
{
    SDL_Color sdlcolor=SDL_Color{color.r,color.b,color.g,color.a};
    SDL_Surface *surface=TTF_RenderText_Solid(font,text,sdlcolor);
    SDL_Texture *texture=SDL_CreateTextureFromSurface(renderer,surface);

    SDL_Rect rect;
    rect.y=y;
    rect.w=surface->w;
    rect.h=surface->h;
    switch(alignment)
    {
    case TEXT_ALIGNMENT_LEFT:
        rect.x=x;
        break;
    case TEXT_ALIGNMENT_CENTER:
        rect.x=x-surface->w/2;
        break;
    case TEXT_ALIGNMENT_RIGHT:
        rect.x=x-surface->w;
        break;
    }
    SDL_RenderCopy(renderer, texture,0,&rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    return;
}
void fill_rect(SDL_Renderer *renderer,s32 x,s32 y,s32 w,s32 h,Color color)
{
    SDL_Rect rect;
    rect.x=x;
    rect.y=y;
    rect.w=w;
    rect.h=h;
    SDL_SetRenderDrawColor(renderer,color.r,color.b,color.g,color.a);
    SDL_RenderFillRect(renderer,&rect);
};
void draw_rect(SDL_Renderer *renderer,s32 x,s32 y,s32 w,s32 h,Color color)
{
    SDL_Rect rect;
    rect.x=x;
    rect.y=y;
    rect.w=w;
    rect.h=h;
    SDL_SetRenderDrawColor(renderer,color.r,color.b,color.g,color.a);
    SDL_RenderDrawRect(renderer,&rect);
};
void draw_cell(SDL_Renderer *renderer,s32 row,s32 col,s32 margin_x,s32 margin_y,s32 index,bool ghost)
{
    Color base_color=BASE_COLORS[index];
    Color dark_color=DARK_COLORS[index];
    Color light_color=LIGHT_COLORS[index];

    s32 side=GRID_SIZE/8;
    s32 x=col*GRID_SIZE+margin_x;
    s32 y=row*GRID_SIZE+margin_y;
    if(ghost)
    {
        draw_rect(renderer,x,y,GRID_SIZE,GRID_SIZE,base_color);
        return;
    }

    fill_rect(renderer,x,y,GRID_SIZE,GRID_SIZE,dark_color);
    fill_rect(renderer,x+side,y,GRID_SIZE-side,GRID_SIZE-side,light_color);
    fill_rect(renderer,x+side,y+side,GRID_SIZE-2*side,GRID_SIZE-2*side,base_color);

};
void draw_piece(SDL_Renderer *renderer,const Piece_State *Piece,s32 margin_x,s32 margin_y,bool ghost)
{
    const BLOCK *block=B_STORAGE+Piece->index;
    for(s32 row=0;row<block->side;row++)
    {
        for(s32 col=0;col<block->side;col++)
        {
            u8 value=rotate_get(block,row,col,Piece->rotation);
            if(value) draw_cell(renderer,row+Piece->offset_row,col+Piece->offset_col,margin_x,margin_y,value,ghost);
        }
    }
};
void draw_board(SDL_Renderer *renderer,u8 *board,s32 height,s32 width,s32 margin_x,s32 margin_y)
{
    for(s32 row=0;row<height;row++)
    {
        for(s32 col=0;col<width;col++)
        {
            u8 value=matrix_get(board,WIDTH,row,col);
            draw_cell(renderer,row,col,margin_x,margin_y,value,false);
        }
    }
    return;
}
void render_game(SDL_Renderer *renderer,Game_State *game,TTF_Font *font)
{
    char buffer[5000];
    s32 margin_y=60;
    Color highlight_color=color(255,255,255,255);
    draw_board(renderer,game->Board,HEIGHT,WIDTH,0,margin_y);
    if(game->Phase==GAME_PHASE_PLAY)
    {
        draw_piece(renderer,&game->piece,0,margin_y,false);
        Piece_State Piece=game->piece;
        while(check_piece_valid(&Piece,game->Board,HEIGHT,WIDTH)) Piece.offset_row++;
        Piece.offset_row--;
        draw_piece(renderer,&Piece,0,margin_y,true);
    }
    else if(game->Phase==GAME_PHASE_HANDLE)
    {
        for(s32 row=0;row<HEIGHT;row++)
        {
            if(game->filled_line[row]==WIDTH)
            {
                s32 x=0;
                s32 y=row*GRID_SIZE+margin_y;
                fill_rect(renderer,x,y,WIDTH*GRID_SIZE,GRID_SIZE,highlight_color);
            }
        }
    }
    else if(game->Phase==GAME_PHASE_OVER)
    {
        s32 x=WIDTH*GRID_SIZE/2;
        s32 y=(HEIGHT*GRID_SIZE+margin_y)/2;
        draw_string(renderer,font,"GAME OVER",x,y,TEXT_ALIGNMENT_CENTER,highlight_color);
        draw_string(renderer,font,"TETRIS",0,0,TEXT_ALIGNMENT_LEFT,highlight_color);
    }
    else if(game->Phase==GAME_PHASE_START)
    {
        s32 x=WIDTH*GRID_SIZE/2;
        s32 y=(HEIGHT*GRID_SIZE+margin_y)/2;
        draw_string(renderer,font,"PRESS START",x,y,TEXT_ALIGNMENT_CENTER,highlight_color);

        sprintf_s(buffer,sizeof buffer,"STARTING LEVEL: %d",game->start_level);
        draw_string(renderer,font,buffer,x,y+30,TEXT_ALIGNMENT_CENTER,highlight_color);
    }
    fill_rect(renderer,0,margin_y,WIDTH*GRID_SIZE,(HEIGHT-VISIBLE_HEIGHT)*GRID_SIZE,color(0,0,0,0));
    draw_string(renderer,font,"NEXT",200,0,TEXT_ALIGNMENT_CENTER,highlight_color);
    if(game->Phase!=GAME_PHASE_START) draw_piece(renderer,&game->next_piece,20,50,0);

    sprintf_s(buffer,sizeof buffer,"LEVEL: %d",game->level);
    draw_string(renderer,font,buffer,5,5,TEXT_ALIGNMENT_LEFT,highlight_color);

    sprintf_s(buffer,sizeof buffer,"LINES: %d",game->line_count);
    draw_string(renderer,font,buffer,5,35,TEXT_ALIGNMENT_LEFT,highlight_color);

    sprintf_s(buffer,sizeof buffer,"POINTS: %d",game->points);
    draw_string(renderer,font,buffer,5,65,TEXT_ALIGNMENT_LEFT,highlight_color);
    return;
};
int main(int argv,char *argc[])
{
    if(SDL_Init(SDL_INIT_EVERYTHING)) return 1;
    if(TTF_Init()<0) return 2;

    SDL_Window *window=SDL_CreateWindow("TETRIS",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,300,720,SDL_WINDOW_OPENGL);
    SDL_Renderer *renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_TARGETTEXTURE);

    if(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY,MIX_DEFAULT_FORMAT,2,4096)==-1) return 3;
    TTF_Font *font=TTF_OpenFont("novem___.ttf",20);


    Game_State game={};
    Input_State input={};

    Mix_Chunk *handle=Mix_LoadWAV("handle.wav");
    Mix_Chunk *game_over=Mix_LoadWAV("game_over.wav");


    bool quit=false;

    while(!quit)
    {
        game.time=SDL_GetTicks()/1000.0f;
        SDL_Event e;
        while(SDL_PollEvent(&e)!=0)
        {
            if(e.type==SDL_QUIT) quit=true;
        }
        const u8 *key_state=SDL_GetKeyboardState(NULL);

        if(key_state[SDL_SCANCODE_ESCAPE]) quit=true;
        Input_State pre=input;

        input.left=key_state[SDL_SCANCODE_LEFT];
        input.right=key_state[SDL_SCANCODE_RIGHT];
        input.up=key_state[SDL_SCANCODE_UP];
        input.down=key_state[SDL_SCANCODE_DOWN];
        input.a=key_state[SDL_SCANCODE_SPACE];

        input.dleft=input.left-pre.left;
        input.dright=input.right-pre.right;
        input.dup=input.up-pre.up;
        input.ddown=input.down-pre.down;
        input.da=input.a-pre.a;

        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderClear(renderer);

        Update_Game(&game,&input,handle,game_over);
        render_game(renderer,&game,font);
        SDL_RenderPresent(renderer);
    }

    Mix_FreeChunk(handle);
    Mix_FreeChunk(game_over);
    Mix_CloseAudio();

    TTF_CloseFont(font);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    return 0;
}
