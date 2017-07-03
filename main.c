#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <windows.h>
#include <tchar.h>
#include <commdlg.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include "simpleini/SimpleIni.h"

int loopCounter = 0;
int cyclesTime = 0;
bool quit = false;
int total = 0;
int timer = 0;
int current = 0;
int counter = 0;
bool first = true;
char saveName[200];


long int ScreenWidth = 160;
long int ScreenHeight = 144;
int pixelMultiplier = 1;

int joypad_Up = SDLK_UP;
int joypad_Down = SDLK_DOWN;
int joypad_Left = SDLK_LEFT;
int joypad_Right = SDLK_RIGHT;
int joypad_A = SDLK_s;
int joypad_B = SDLK_a;
int joypad_Start = SDLK_RETURN;
int joypad_Select = SDLK_SPACE;


int resOpenFile;
OPENFILENAME ofn;
TCHAR tmpOpenFile[1024];

CSimpleIniA ini;


#include "z80.c"
#include "NOROM.c"
#include "MBC1.c"
#include "MBC2.c"
#include "MBC3.c"

void initSDL();
void initScreen();
void quitSDL();
void initPixel();
void drawPixel(PIXEL);
void RenderScreen();
bool LoadGame(char *);
void HandleInput(SDL_Event&);
void FPSChecking();
void GameBoyASCII();
void TitleProg();
void Menu();
void apply_surface(int, int, SDL_Surface*, SDL_Surface*);
void delay(int);
void GetConfig();
void Update();


SDL_Surface *screen = NULL;
SDL_Surface *square[4];


int main(int argc, char* argv[]){

	freopen( "CON", "w", stdout );
	freopen( "CON", "w", stderr );
	
	ini.SetUnicode();
	ini.LoadFile("config.ini");
	
	GetConfig();
	
	Menu();
	//ofn.lpstrFile contient le path de la rom

    initSDL();

    initScreen();
    initPixel();
    
    bool getReady = false;

    getReady = LoadGame(ofn.lpstrFile);
	
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

	
	if(z80.m_NOROM){
		printf("ROM PCB: NO ROM\n\n");
	}
    else if(z80.m_MBC1){
        printf("ROM PCB: MBC1\n\n");
    }
    else if(z80.m_MBC2){
        printf("ROM PCB: MBC2\n\n");
    }
	else if(z80.m_MBC3){
        printf("ROM PCB: MBC3\n\n");
    }
	
	switch (z80.m_CartridgeMemory[0x149]){
		
		case 0x0 : printf("RAM SIZE : NO RAM\n\n"); break ;
        case 0x1 : printf("RAM SIZE : 2KB\n\n"); break ;
        case 0x2 : printf("RAM SIZE : 8KB\n\n"); break ;
        case 0x3 : printf("RAM SIZE : 32KB\n\n"); break ;
        case 0x4 : printf("RAM SIZE : 128KB\n\n"); break ;
        case 0x5 : printf("RAM SIZE : 64KB\n\n"); break ;
    }
	
	
	
	
	//SaveFile Name
	strcpy(saveName, ofn.lpstrFile);
	char *pch;
	pch = strstr(saveName,".gb");
	strncpy(pch,".sav",4);
	
	
	if(z80.m_ActiveBATTERY){
		if(access(saveName, F_OK) != -1){
			printf("save file found");
			FILE *ifp = fopen(saveName, "rb");
			fread(z80.m_RAMBanks, sizeof(byte), sizeof(z80.m_RAMBanks), ifp);
			fclose(ifp);
		}
	}
	
	
    if(getReady){
		SDL_Event event;
		float fps = 59.73;
		float interval = 1000;
		interval /= fps;
		
		unsigned int time2 = SDL_GetTicks();
		
		while(!quit){
			while(SDL_PollEvent(&event)){
				HandleInput(event);
				
				if (event.type == SDL_QUIT){
					quit = true;
				}
			}
			
			unsigned int current = SDL_GetTicks();
			
			if((time2 + interval) < current){
				FPSChecking();
				Update();
				time2 = current;
			}
			
			
		}
		
		if(z80.m_ActiveBATTERY){
			FILE *f = fopen(saveName, "wb");
			fwrite(z80.m_RAMBanks, sizeof(byte), sizeof(z80.m_RAMBanks), f);
			fclose(f);
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

void Menu(){
	
	bool passMenu = false;
	
	while(!passMenu){
		TitleProg();
		GameBoyASCII();
		
		char menuSelect = '0';
		printf("--------------------------------------------------------\n");
		printf("|1|-Load Rom/Game   |2|-Change Screen Resolution   |3|-Set Joypad\n\n");
		printf("Put 1,2 or 3 and press enter to choose one task : ");
		menuSelect = getchar();
		while(getchar() != '\n');
		
		while(menuSelect != '1' && menuSelect != '2' && menuSelect != '3'){
			printf("\nWrong input. Retry: ");
			menuSelect = getchar();
			while(getchar() != '\n');
		}
		
		if(menuSelect == '1'){
			tmpOpenFile[0]= '\0' ;
			ZeroMemory ( &ofn , sizeof ( OPENFILENAMEW ) );
			ofn.lStructSize = sizeof ( OPENFILENAMEW );
			ofn.lpstrFile = tmpOpenFile;
			ofn.nMaxFile = 1024;
			ofn.lpstrTitle = _T("Choose a Game");
			ofn.lpstrFilter = _T("All (*.*)\0*.*\0Gameboy (*.gb)\0*.GB\0");
			ofn.Flags = OFN_LONGNAMES | OFN_EXPLORER; // | OFN_ALLOWMULTISELECT  ;
			resOpenFile = GetOpenFileName(&ofn);
			
			if(resOpenFile == 1){
				passMenu = true;
			}
			else{
				printf("Error loading ROM\n");
				delay(2);
			}
			
		}
		else if(menuSelect == '2'){
			char screenRes = '0';
			printf("--------------------------------------------------------\n");
			printf("Resolution Config\n\n");
			printf("|1|-160x144(original size)  |2|-320x288   |3|-480x432   |4|-640x576\n\n");
			printf("Put 1,2,3 or 4 and press enter to choose one resolution : ");
			screenRes = getchar();
			while(getchar() != '\n');
			
			while(screenRes != '1' && screenRes != '2' && screenRes != '3' && screenRes != '4'){
				printf("\nWrong input. Retry: ");
				screenRes = getchar();
				while(getchar() != '\n');
			}
			
			if(screenRes == '1'){
				ScreenWidth = 160;
				ScreenHeight = 144;
				pixelMultiplier = 1;
				ini.SetValue("screen", "size", "1");
				ini.SaveFile("config.ini");
				printf("Your Config have been saved\n");
				delay(1);
			}
			else if(screenRes == '2'){
				ScreenWidth = 320;
				ScreenHeight = 288;
				pixelMultiplier = 2;
				ini.SetValue("screen", "size", "2");
				ini.SaveFile("config.ini");
				printf("Your Config have been saved\n");
				delay(1);
			}
			else if(screenRes == '3'){
				ScreenWidth = 480;
				ScreenHeight = 432;
				pixelMultiplier = 3;
				ini.SetValue("screen", "size", "3");
				ini.SaveFile("config.ini");
			}
			else if(screenRes == '4'){
				ScreenWidth = 640;
				ScreenHeight = 576;
				pixelMultiplier = 4;
				ini.SetValue("screen", "size", "4");
				ini.SaveFile("config.ini");
				printf("Your Config have been saved\n");
				delay(1);
			}
			printf("\n\n");
		}
		else if(menuSelect == '3'){
			
			SDL_Event event;
			SDL_Surface *message = NULL; 
			SDL_Surface *screen = NULL;
			TTF_Font *font;
			SDL_Color textColor = { 255, 255, 255 };
			bool waitForPressed = true;
			
			printf("--------------------------------------------------------\n");
			printf("Joypad Input Setting\n\n");
			
			if( SDL_Init( SDL_INIT_EVERYTHING ) == -1 ) { 
				fprintf(stderr,"Erreur lors de l'initialisation de la SDL %s",SDL_GetError());
				exit(EXIT_FAILURE);
			}
			
			screen = SDL_SetVideoMode( 640, 100, 32, SDL_SWSURFACE );
			
			TTF_Init();
			
			SDL_WM_SetCaption( "CONFIG INPUT", NULL );
			
			font = TTF_OpenFont( "8bit_madness.ttf", 30 );
			
			message = TTF_RenderText_Solid( font, "Press the key you want to use for | UP |", textColor );
			apply_surface( 0, 30, message, screen );
			SDL_Flip(screen);
			while(waitForPressed){
				while(SDL_PollEvent(&event)){
					if(event.type == SDL_KEYDOWN){
						joypad_Up = event.key.keysym.sym;
						char buff[10] = "";
						sprintf(buff, "%d", joypad_Up);
						ini.SetValue("joypad", "up", buff);
						ini.SaveFile("config.ini");
						waitForPressed = false;
						SDL_FillRect(screen, NULL, 0x000000);
					}
				}
			}
			message = TTF_RenderText_Solid( font, "Press the key you want to use for | DOWN |", textColor );
			apply_surface( 0, 30, message, screen );
			SDL_Flip(screen);
			waitForPressed = true;
			while(waitForPressed){
				while(SDL_PollEvent(&event)){
					if(event.type == SDL_KEYDOWN){
						joypad_Down = event.key.keysym.sym;
						char buff[10] = "";
						sprintf(buff, "%d", joypad_Down);
						ini.SetValue("joypad", "down", buff);
						ini.SaveFile("config.ini");
						waitForPressed = false;
						SDL_FillRect(screen, NULL, 0x000000);
					}
				}
			}
			message = TTF_RenderText_Solid( font, "Press the key you want to use for | LEFT |", textColor );
			apply_surface( 0, 30, message, screen );
			SDL_Flip(screen);
			waitForPressed = true;
			while(waitForPressed){
				while(SDL_PollEvent(&event)){
					if(event.type == SDL_KEYDOWN){
						joypad_Left = event.key.keysym.sym;
						char buff[10] = "";
						sprintf(buff, "%d", joypad_Left);
						ini.SetValue("joypad", "left", buff);
						ini.SaveFile("config.ini");
						waitForPressed = false;
						SDL_FillRect(screen, NULL, 0x000000);
					}
				}
			}
			message = TTF_RenderText_Solid( font, "Press the key you want to use for | RIGHT |", textColor );
			apply_surface( 0, 30, message, screen );
			SDL_Flip(screen);
			waitForPressed = true;
			while(waitForPressed){
				while(SDL_PollEvent(&event)){
					if(event.type == SDL_KEYDOWN){
						joypad_Right = event.key.keysym.sym;
						char buff[10] = "";
						sprintf(buff, "%d", joypad_Right);
						ini.SetValue("joypad", "right", buff);
						ini.SaveFile("config.ini");
						waitForPressed = false;
						SDL_FillRect(screen, NULL, 0x000000);
					}
				}
			}
			message = TTF_RenderText_Solid( font, "Press the key you want to use for | A |", textColor );
			apply_surface( 0, 30, message, screen );
			SDL_Flip(screen);
			waitForPressed = true;
			while(waitForPressed){
				while(SDL_PollEvent(&event)){
					if(event.type == SDL_KEYDOWN){
						joypad_A = event.key.keysym.sym;
						char buff[10] = "";
						sprintf(buff, "%d", joypad_A);
						ini.SetValue("joypad", "a", buff);
						ini.SaveFile("config.ini");
						waitForPressed = false;
						SDL_FillRect(screen, NULL, 0x000000);
					}
				}
			}
			message = TTF_RenderText_Solid( font, "Press the key you want to use for | B |", textColor );
			apply_surface( 0, 30, message, screen );
			SDL_Flip(screen);
			waitForPressed = true;
			while(waitForPressed){
				while(SDL_PollEvent(&event)){
					if(event.type == SDL_KEYDOWN){
						joypad_B = event.key.keysym.sym;
						char buff[10] = "";
						sprintf(buff, "%d", joypad_B);
						ini.SetValue("joypad", "b", buff);
						ini.SaveFile("config.ini");
						waitForPressed = false;
						SDL_FillRect(screen, NULL, 0x000000);
					}
				}
			}
			message = TTF_RenderText_Solid( font, "Press the key you want to use for | START |", textColor );
			apply_surface( 0, 30, message, screen );
			SDL_Flip(screen);
			waitForPressed = true;
			while(waitForPressed){
				while(SDL_PollEvent(&event)){
					if(event.type == SDL_KEYDOWN){
						joypad_Start = event.key.keysym.sym;
						char buff[10] = "";
						sprintf(buff, "%d", joypad_Start);
						ini.SetValue("joypad", "start", buff);
						ini.SaveFile("config.ini");
						waitForPressed = false;
						SDL_FillRect(screen, NULL, 0x000000);
					}
				}
			}
			message = TTF_RenderText_Solid( font, "Press the key you want to use for | SELECT |", textColor );
			apply_surface( 0, 30, message, screen );
			SDL_Flip(screen);
			waitForPressed = true;
			while(waitForPressed){
				while(SDL_PollEvent(&event)){
					if(event.type == SDL_KEYDOWN){
						joypad_Select = event.key.keysym.sym;
						char buff[10] = "";
						sprintf(buff, "%d", joypad_Select);
						ini.SetValue("joypad", "select", buff);
						ini.SaveFile("config.ini");
						waitForPressed = false;
						SDL_FillRect(screen, NULL, 0x000000);
					}
				}
			}
			
			SDL_FreeSurface( message ); 

			TTF_CloseFont( font ); 

			TTF_Quit(); 

			SDL_Quit();
			
			printf("Your Config have been saved\n");
			delay(1);
			
		}
	}
}

void FPSChecking(){
	if (first){
		first = false;
		timer = SDL_GetTicks();
	}

	counter++;
	current = SDL_GetTicks();
	if ((timer + 1000) < current){
		timer = current ;
		total = counter ;
		counter = 0 ;
	}
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
    screen = SDL_SetVideoMode(ScreenWidth, ScreenHeight, 32, SDL_HWSURFACE); //initialiser ecran
    SDL_WM_SetCaption("GameBoo by Julien MAGNIN", NULL);

    if(screen==NULL){
        fprintf(stderr,"Erreur lors du chargement du mode vidéo %s",SDL_GetError());
        exit(EXIT_FAILURE);
    }
    //PIXEL WHITE
    square[0] = SDL_CreateRGBSurface(SDL_HWSURFACE, pixelMultiplier, pixelMultiplier, 32, 0, 0, 0, 0);
    if(square[0]==NULL){
        fprintf(stderr,"Erreur lors du chargement de la surface %s",SDL_GetError());
    }
    SDL_FillRect(square[0], NULL, SDL_MapRGB(square[0]->format, 255, 255, 255));
    //PIXEL LIGHT_GREY
    square[1] = SDL_CreateRGBSurface(SDL_HWSURFACE, pixelMultiplier, pixelMultiplier, 32, 0, 0, 0, 0);
    if(square[1]==NULL){
        fprintf(stderr,"Erreur lors du chargement de la surface %s",SDL_GetError());
    }
    SDL_FillRect(square[1], NULL, SDL_MapRGB(square[1]->format, 0xCC, 0xCC, 0xCC));

    //PIXEL DARK_GREY
    square[2] = SDL_CreateRGBSurface(SDL_HWSURFACE, pixelMultiplier, pixelMultiplier, 32, 0, 0, 0, 0);
    if(square[2]==NULL){
        fprintf(stderr,"Erreur lors du chargement de la surface %s",SDL_GetError());
    }
    SDL_FillRect(square[2], NULL, SDL_MapRGB(square[2]->format, 0x77, 0x77, 0x77));
    //PIXEL BLACK
    square[3] = SDL_CreateRGBSurface(SDL_HWSURFACE, pixelMultiplier, pixelMultiplier, 32, 0, 0, 0, 0);
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
            z80.m_ScreenData[x][y].position.x = x*pixelMultiplier;
            z80.m_ScreenData[x][y].position.y = y*pixelMultiplier;
            z80.m_ScreenData[x][y].color = BLACK;
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
		
		if(event.key.keysym.sym == joypad_A){
			key = 4;
		}
		else if(event.key.keysym.sym == joypad_B){
			key = 5;
		}
		else if(event.key.keysym.sym == joypad_Start){
			key = 7;
		}
		else if(event.key.keysym.sym == joypad_Select){
			key = 6;
		}
		else if(event.key.keysym.sym == joypad_Right){
			key = 0;
		}
		else if(event.key.keysym.sym == joypad_Left){
			key = 1;
		}
		else if(event.key.keysym.sym == joypad_Up){
			key = 2;
		}
		else if(event.key.keysym.sym == joypad_Down){
			key = 3;
		}
		else if(event.key.keysym.sym == SDLK_ESCAPE){
			quit = true;
		}
		
		if (key != -1){
			KeyPressed(key) ;
		}
	}
	else if( event.type == SDL_KEYUP ){
		int key = -1 ;
		
		if(event.key.keysym.sym == joypad_A){
			key = 4;
		}
		else if(event.key.keysym.sym == joypad_B){
			key = 5;
		}
		else if(event.key.keysym.sym == joypad_Start){
			key = 7;
		}
		else if(event.key.keysym.sym == joypad_Select){
			key = 6;
		}
		else if(event.key.keysym.sym == joypad_Right){
			key = 0;
		}
		else if(event.key.keysym.sym == joypad_Left){
			key = 1;
		}
		else if(event.key.keysym.sym == joypad_Up){
			key = 2;
		}
		else if(event.key.keysym.sym == joypad_Down){
			key = 3;
		}
		else if(event.key.keysym.sym == SDLK_ESCAPE){
			quit = true;
		}
		
		if (key != -1){
			KeyReleased(key) ;
		}
	}
}

void GameBoyASCII(){
	printf(" _n_________________\n");
	printf("|_|_______________|_|\n");
	printf("|  ,-------------.  |\n");
	printf("| |  .---------.  | |\n");
	printf("| |  |         |  | |\n");
	printf("| |  |         |  | |\n");
	printf("| |  |         |  | |\n");
	printf("| |  |         |  | |\n");
	printf("| |  `---------'  | |\n");
	printf("| `---------------' |\n");
	printf("|   _ GAME BOO      |\n");
	printf("| _| |_         ,-. |\n");
	printf("||_ O _|   ,-.  ._, |\n");
	printf("|  |_|     ._,    A |\n");
	printf("|    _  _    B      |\n");
	printf("|   // //           |\n");
	printf("|  // //    \\\\\\\\\\\\  |\n");
	printf("|  `  `      \\\\\\\\\\\\ ,\n");
	printf("|________...______,'\n\n\n\n");
}
void TitleProg(){
	printf("GAMEBOO a Gameboy Emulator  BY  Julien MAGNIN a.k.a MSDOS\n\n");  
}

void apply_surface( int x, int y, SDL_Surface* source, SDL_Surface* destination)
{
    SDL_Rect offset;
 
    offset.x = x;
    offset.y = y;
 
    SDL_BlitSurface( source, NULL, destination, &offset );
}

void delay(int i)    /*Pause l'application pour i seconds*/
{
    clock_t start,end;
    start=clock();
    while(((end=clock())-start)<=i*CLOCKS_PER_SEC);
}

void GetConfig(){
	const char * tempMultiplier = ini.GetValue("screen", "size", "255");
	
	pixelMultiplier = strtol(tempMultiplier, NULL, 10);
	
	switch(pixelMultiplier){
		case 1: ScreenWidth = 160; ScreenHeight = 144; break;
		case 2: ScreenWidth = 320; ScreenHeight = 288; break;
		case 3: ScreenWidth = 480; ScreenHeight = 432; break;
		case 4: ScreenWidth = 640; ScreenHeight = 576; break;
	}
	
	const char * tempJoypadUp = ini.GetValue("joypad", "up", "1000");
	
	joypad_Up = strtol(tempJoypadUp, NULL, 10);
	
	const char * tempJoypadDown = ini.GetValue("joypad", "down", "1000");
	
	joypad_Down = strtol(tempJoypadDown, NULL, 10);
	
	const char * tempJoypadLeft = ini.GetValue("joypad", "left", "1000");
	
	joypad_Left = strtol(tempJoypadLeft, NULL, 10);
	
	const char * tempJoypadRight = ini.GetValue("joypad", "right", "1000");
	
	joypad_Right = strtol(tempJoypadRight, NULL, 10);
	
	const char * tempJoypadA = ini.GetValue("joypad", "a", "1000");
	
	joypad_A = strtol(tempJoypadA, NULL, 10);
	
	const char * tempJoypadB = ini.GetValue("joypad", "b", "1000");
	
	joypad_B = strtol(tempJoypadB, NULL, 10);
	
	const char * tempJoypadStart = ini.GetValue("joypad", "start", "1000");
	
	joypad_Start = strtol(tempJoypadStart, NULL, 10);
	
	const char * tempJoypadSelect = ini.GetValue("joypad", "select", "1000");
	
	joypad_Select = strtol(tempJoypadSelect, NULL, 10);
}
