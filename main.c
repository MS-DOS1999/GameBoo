#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <vector>
#include <SDL/SDL.h>

int loopCounter = 0;
int cyclesTime = 0;
bool quit = false ;

#include "z80.c"

void initSDL();
void initScreen();
void quitSDL();
void initPixel();
void drawPixel(PIXEL);
void RenderScreen();
bool LoadGame(char *);
void HandleInput(SDL_Event&);
void Update();


SDL_Surface *screen = NULL;
SDL_Surface *square[4];




int main(int argc, char* argv[]){

	//freopen( "CON", "w", stdout );
	//freopen( "CON", "w", stderr );

    char filePath[200];
    printf("Indiquez le 'chemin absolu/absolute path' de votre rom :  ");
    scanf("%[^\n]", filePath);
    printf("Scanf: OK\n\n");



    initSDL();

    initScreen();
    initPixel();
    
    bool getReady = false;

    getReady = LoadGame(filePath);
	
	initZ80();
    printf("initZ80: OK\n\n");
	
    if(getReady){
        printf("RomLoaded: OK\n\n");
    }
    else{
        printf("RomLoaded: NOP, error :/\n\n");
    }

    //on definie le pcb de la rom après son chargement//
    DetectMapper();

    if(z80.m_MBC1){
        printf("ROM PCB: MBC1\n\n");
    }
    else if(z80.m_MBC2){
        printf("ROM PCB: MBC2\n\n");
    }
	else{
		printf("ROM PCB: NO ROM\n\n");
	}

	
	
	
    if(getReady){
		SDL_Event event;
		while(!quit){
			while(SDL_PollEvent(&event)){
				HandleInput(event);
			}
			Update();
		}
		SDL_Quit();
    }
	
	//freopen( "CON", "w", stdout );
	//freopen( "CON", "w", stderr );
	
    return EXIT_SUCCESS;
}

void Update(){
	
	loopCounter = 0;
	
    while(loopCounter < MAXLOOP){
        cyclesTime = 0;
        ExecuteNextOpcode();
        UpdateTimers(cyclesTime);
        UpdateGraphics(cyclesTime);
        DoInterupts();
    }
    RenderScreen();
}

bool LoadGame(char *gameName){
	
	memset(z80.m_CartridgeMemory,0,sizeof(z80.m_CartridgeMemory));
    memset(&z80.m_RAMBanks,0,sizeof(z80.m_RAMBanks));
	memset(z80.m_Rom,0,sizeof(z80.m_Rom)) ;
	
    FILE *game = NULL;
    game = fopen(gameName,"rb");

    if(game != NULL){
        fread(z80.m_CartridgeMemory, 1, 0x200000, game);
        fclose(game);
		memcpy(&z80.m_Rom[0x0], &z80.m_CartridgeMemory[0], 0x8000) ;
        return true;
    }
    else{
        printf("Error opening file, try again :/");
        return false;
    }
	
	z80.m_CurrentROMBank = 1;
}

void initSDL(){

    atexit(quitSDL);

    if(SDL_Init(SDL_INIT_VIDEO)==-1) {
        fprintf(stderr,"Erreur lors de l'initialisation de la SDL %s",SDL_GetError());
        exit(EXIT_FAILURE);
    }

    printf("initSDL: OK\n\n");

}

void quitSDL(){

    SDL_FreeSurface(square[0]);
    SDL_FreeSurface(square[1]);
    SDL_FreeSurface(square[2]);
    SDL_FreeSurface(square[3]);
    SDL_Quit();

    printf("quitSDL: OK\n\n");
}

void initScreen(){
    screen = NULL;
    square[0] = NULL;
    square[1] = NULL;
    square[2] = NULL;
    square[3] = NULL;
    //SET SCREEN
    screen = SDL_SetVideoMode(160, 144, 32, SDL_HWSURFACE); //initialiser ecran
    SDL_WM_SetCaption("GameBoo by Julien MAGNIN", NULL);

    if(screen==NULL){
        fprintf(stderr,"Erreur lors du chargement du mode vidéo %s",SDL_GetError());
        exit(EXIT_FAILURE);
    }
    //PIXEL WHITE
    square[0] = SDL_CreateRGBSurface(SDL_HWSURFACE, 1, 1, 32, 0, 0, 0, 0);
    if(square[0]==NULL){
        fprintf(stderr,"Erreur lors du chargement de la surface %s",SDL_GetError());
    }
    SDL_FillRect(square[0], NULL, SDL_MapRGB(square[0]->format, 255, 255, 255));
    //PIXEL LIGHT_GREY
    square[1] = SDL_CreateRGBSurface(SDL_HWSURFACE, 1, 1, 32, 0, 0, 0, 0);
    if(square[1]==NULL){
        fprintf(stderr,"Erreur lors du chargement de la surface %s",SDL_GetError());
    }
    SDL_FillRect(square[1], NULL, SDL_MapRGB(square[1]->format, 0xCC, 0xCC, 0xCC));

    //PIXEL DARK_GREY
    square[2] = SDL_CreateRGBSurface(SDL_HWSURFACE, 1, 1, 32, 0, 0, 0, 0);
    if(square[2]==NULL){
        fprintf(stderr,"Erreur lors du chargement de la surface %s",SDL_GetError());
    }
    SDL_FillRect(square[2], NULL, SDL_MapRGB(square[2]->format, 0x77, 0x77, 0x77));
    //PIXEL BLACK
    square[3] = SDL_CreateRGBSurface(SDL_HWSURFACE, 1, 1, 32, 0, 0, 0, 0);
    if(square[3]==NULL){
        fprintf(stderr,"Erreur lors du chargement de la surface %s",SDL_GetError());
    }
    SDL_FillRect(square[3], NULL, SDL_MapRGB(square[3]->format, 0x00, 0x00, 0x00));

    printf("initScreen: OK\n\n");

}

void initPixel(){
    Uint8 x = 0;
    Uint8 y = 0;

    for(x=0; x<160; x++){
        for(y=0; y<144; y++){
            z80.m_ScreenData[x][y].position.x = x;
            z80.m_ScreenData[x][y].position.y = y;
            z80.m_ScreenData[x][y].color = WHITE;
        }
    }

    printf("initPixel: OK\n\n");
}

void drawPixel(PIXEL pixel){
    SDL_BlitSurface(square[pixel.color], NULL, screen, &pixel.position);
}

void RenderScreen(){
    Uint8 x = 0;
    Uint8 y = 0;

    for(x=0; x<160; x++){
        for(y=0; y<144; y++){
            drawPixel(z80.m_ScreenData[x][y]);
        }
    }
    SDL_Flip(screen);
}


void HandleInput(SDL_Event& event){
	if( event.type == SDL_KEYDOWN ){
		int key = -1 ;
		switch( event.key.keysym.sym ){
			case SDLK_a : key = 4 ; break ;
			case SDLK_s : key = 5 ; break ;
			case SDLK_RETURN : key = 7 ; break ;
			case SDLK_SPACE : key = 6; break ;
			case SDLK_RIGHT : key = 0 ; break ;
			case SDLK_LEFT : key = 1 ; break ;
			case SDLK_UP : key = 2 ; break ;
			case SDLK_DOWN : key = 3 ; break ;
			case SDLK_ESCAPE : quit = true ; break ;
		}
		if (key != -1){
			KeyPressed(key) ;
		}
	}
	else if( event.type == SDL_KEYUP ){
		int key = -1 ;
		switch( event.key.keysym.sym ){
			case SDLK_a : key = 4 ; break ;
			case SDLK_s : key = 5 ; break ;
			case SDLK_RETURN : key = 7 ; break ;
			case SDLK_SPACE : key = 6; break ;
			case SDLK_RIGHT : key = 0 ; break ;
			case SDLK_LEFT : key = 1 ; break ;
			case SDLK_UP : key = 2 ; break ;
			case SDLK_DOWN : key = 3 ; break ;
			case SDLK_ESCAPE : quit = true ; break ;
		}
		if (key != -1){
			KeyReleased(key) ;
		}
	}
}
