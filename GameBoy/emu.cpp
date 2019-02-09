#include "emu.hpp"

int main(int argc, char **argv)
{
	Emu::Intro();
	Emu::GetConfig();
	Emu::FileBrowser();
	Emu::Execute();

	return 0;
}

sfSprite* LoadSprite(const char* _sNom, int _isCentered)
{
    sfSprite* spriteTemp;
    sfTexture* textureTemp;
    spriteTemp = sfSprite_create();
    textureTemp = sfTexture_createFromFile(_sNom, NULL);
    sfSprite_setTexture(spriteTemp, textureTemp, sfTrue);
    if (_isCentered == 1)
    {
        sfVector2u taileTexture = sfTexture_getSize(textureTemp);
        sfVector2f centreTexture = { taileTexture.x / 2.0f, taileTexture.y / 2.0f };
        sfSprite_setOrigin(spriteTemp, centreTexture);
    }

    return spriteTemp;
}

Emu::Emu()
{
	this->bpUp = false;
    this->bpDown = false;
    this->bpRight = false;
    this->bpLeft = false;
    this->bpSelect = false;
    this->bpStart = false;
    this->bpAButton = false;
    this->bpBButton = false;

	this->loopCounter = 0;
	this->cyclesTime = 0;
	this->quit = false;
	this->total = 0;
	this->timer = 0;
	this->current = 0;
	this->counter = 0;
	this->first = false;
	this->romLoaded = false;
	this->currentRomBank = 1;
	this->currentRamBank = 0;
	this->joypadState = 0xFF;
	this->pixelSize = 1;

	memset(this->cartridgeMemory, 0, sizeof(this->cartridgeMemory));
	memset(&this->ramBanks, 0, sizeof(this->ramBanks));
	memset(this->internalMemory, 0, sizeof(this->internalMemory));

	this->internalMemory[0xFF05] = 0x00;
    this->internalMemory[0xFF06] = 0x00;
    this->internalMemory[0xFF07] = 0x00;
    this->internalMemory[0xFF10] = 0x80;
    this->internalMemory[0xFF11] = 0xBF;
    this->internalMemory[0xFF12] = 0xF3;
    this->internalMemory[0xFF14] = 0xBF;
    this->internalMemory[0xFF16] = 0x3F;
    this->internalMemory[0xFF17] = 0x00;
    this->internalMemory[0xFF19] = 0xBF;
    this->internalMemory[0xFF1A] = 0x7F;
    this->internalMemory[0xFF1B] = 0xFF;
    this->internalMemory[0xFF1C] = 0x9F;
    this->internalMemory[0xFF1E] = 0xBF;
    this->internalMemory[0xFF20] = 0xFF;
    this->internalMemory[0xFF21] = 0x00;
    this->internalMemory[0xFF22] = 0x00;
    this->internalMemory[0xFF23] = 0xBF;
    this->internalMemory[0xFF24] = 0x77;
    this->internalMemory[0xFF25] = 0xF3;
    this->internalMemory[0xFF26] = 0xF1;
    this->internalMemory[0xFF40] = 0x91;
    this->internalMemory[0xFF42] = 0x00;
    this->internalMemory[0xFF43] = 0x00;
    this->internalMemory[0xFF45] = 0x00;
    this->internalMemory[0xFF47] = 0xFC;
    this->internalMemory[0xFF48] = 0xFF;
    this->internalMemory[0xFF49] = 0xFF;
    this->internalMemory[0xFF4A] = 0x00;
    this->internalMemory[0xFF4B] = 0x00;
	this->internalMemory[0xFF0F] = 0xE1;
    this->internalMemory[0xFFFF] = 0x00;

    this->enableRam = false;

    char path[2048];
	GetCurrentDirectory(2048, path);
	this->fullPath = path;
}

Emu& Emu::get(void)
{
	static Emu instance;
	return instance;
}
char Emu::GetLcdColor()
{
	return get().lcdColor;
}

void Emu::GetConfig()
{
	std::string add = get().fullPath + "/CONFIG/config.txt";
	std::ifstream input(add);
	for(int i = 0; i < 2; i++)
	{
		std::string id;
		std::string value;
		std::string empty;
		std::getline(input, id, '=');
		std::getline(input, value, ';');
		std::getline(input, empty, '\n');

		if(id.compare("ScreenSize") == 0)
		{
			if(value.at(0) == '1')
			{
				get().screenConfig = 1;
			}
			else if(value.at(0) == '2')
			{
				get().screenConfig = 2;
			}
			else if(value.at(0) == '3')
			{
				get().screenConfig = 3;
			}
		}

		if(id.compare("LcdColor") == 0)
		{
			if(value.at(0) == '0')
			{
				get().lcdColor = 0;
			}
			else if(value.at(0) == '1')
			{
				get().lcdColor = 1;
			}
			else if(value.at(0) == '2')
			{
				get().lcdColor = 2;
			}
		}
	}

	input.close();
}

void Emu::Intro()
{
	freopen("CON", "w", stdout);
	freopen("CON", "w", stderr);

	std::string raw_str=R"(
        GGGGGGGGGGGGG                                                              
     GGG::::::::::::G                                                              
   GG:::::::::::::::G                                                              
  G:::::GGGGGGGG::::G                                                              
 G:::::G       GGGGGG  aaaaaaaaaaaaa      mmmmmmm    mmmmmmm       eeeeeeeeeeee    
G:::::G                a::::::::::::a   mm:::::::m  m:::::::mm   ee::::::::::::ee  
G:::::G                aaaaaaaaa:::::a m::::::::::mm::::::::::m e::::::eeeee:::::ee
G:::::G    GGGGGGGGGG           a::::a m::::::::::::::::::::::me::::::e     e:::::e
G:::::G    G::::::::G    aaaaaaa:::::a m:::::mmm::::::mmm:::::me:::::::eeeee::::::e
G:::::G    GGGGG::::G  aa::::::::::::a m::::m   m::::m   m::::me:::::::::::::::::e 
G:::::G        G::::G a::::aaaa::::::a m::::m   m::::m   m::::me::::::eeeeeeeeeee  
 G:::::G       G::::Ga::::a    a:::::a m::::m   m::::m   m::::me:::::::e           
  G:::::GGGGGGGG::::Ga::::a    a:::::a m::::m   m::::m   m::::me::::::::e          
   GG:::::::::::::::Ga:::::aaaa::::::a m::::m   m::::m   m::::m e::::::::eeeeeeee  
     GGG::::::GGG:::G a::::::::::aa:::am::::m   m::::m   m::::m  ee:::::::::::::e  
        GGGGGG   GGGG  aaaaaaaaaa  aaaammmmmm   mmmmmm   mmmmmm    eeeeeeeeeeeeee  
                                                                                   
 .-.
(o o) boo!
| O \
 \   \
  `~~~'
)";
	std::cout << raw_str << std::endl;

	std::cout << "Programmed by Julien Magnin (MS-DOS1999)" << std::endl;
	std::cout << "Sound Emulation programmed by blargg" << std::endl;
	std::cout << "Chiptune used for intro and file selection composed by Kid Zan" << std::endl;
	std::cout << "Original Gameboy pixel art by ENSELLITIS" << std::endl;
}

void Emu::FileBrowser()
{
	OPENFILENAME ofn;
	memset(&get().romName, 0, sizeof(get().romName));
    memset(&ofn,      0, sizeof(ofn));
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = NULL;
    ofn.lpstrFilter  = "Gameboy Rom\0*.gb\0";
    ofn.lpstrFile    = get().romName;
    ofn.nMaxFile     = 200;
    ofn.lpstrTitle   = "Select a Rom, hell yeah !";
    ofn.Flags        = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

    GetOpenFileNameA(&ofn);
}

void Emu::Execute()
{
	//init APU
	get().buf.set_sample_rate(44100);
	get().buf.clock_rate(4194304);
	get().apu.output(get().buf.center(), get().buf.left(), get().buf.right());

	get().sound.start(44100, 2);

	get().InitSFML();
	get().LoadGame();
	SharpLr::Init();
	get().GetRomInfo();
	get().GetSave();
	get().Run();
	get().WriteSave();
}

void Emu::InitSFML()
{
	if(get().screenConfig == 1 || get().screenConfig == 2)
	{
		get().mode = (sfVideoMode){351 * get().screenConfig, 541 * get().screenConfig, 32};
		get().window = sfRenderWindow_create(get().mode, "GameBoo - Chili Hot Dog Version", sfClose, NULL);
	}
	else if(get().screenConfig == 3)
	{
		sfVideoMode screenParam = sfVideoMode_getDesktopMode();

		get().mode = (sfVideoMode){screenParam.width, screenParam.height, 32};
		get().window = sfRenderWindow_create(get().mode, "GameBoo - Chili Hot Dog Version", sfFullscreen, NULL);
	}

	get().pixelSize = get().get().screenConfig;
	
    get().myClock = sfClock_create();

    if(get().screenConfig == 1)
    {
    	std::string add = fullPath + "/SPRITES/gameboy.png";
    	get().gameboySpr = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().gameboySpr, (sfVector2f){ 0.0f, 0.0f });

	    add = fullPath + "/SPRITES/pushed.png";
	    get().pUp = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pUp, (sfVector2f){ 65.0f, 317.0f });

	    get().pDown = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pDown, (sfVector2f){ 65.0f, 367.0f });

	    get().pRight = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pRight, (sfVector2f){ 90.0f, 343.0f });

	    get().pLeft = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pLeft, (sfVector2f){ 40.0f, 343.0f });

	    get().pSelect = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pSelect, (sfVector2f){ 113.0f, 418.0f });

	    get().pStart = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pStart, (sfVector2f){ 166.0f, 418.0f });

	    get().pAButton = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pAButton, (sfVector2f){ 266.0f, 325.0f });

	    get().pBButton = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pBButton, (sfVector2f){ 220.0f, 349.0f });
    }
    else if(get().screenConfig == 2)
    {
    	std::string add = fullPath + "/SPRITES/gameboyX2.png";
    	get().gameboySpr = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().gameboySpr, (sfVector2f){ 0.0f, 0.0f });

	    add = fullPath + "/SPRITES/pushedX2.png";
	    get().pUp = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pUp, (sfVector2f){ 65.0f * 2.0f, 317.0f * 2.0f });

	    get().pDown = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pDown, (sfVector2f){ 65.0f * 2.0f, 367.0f * 2.0f});

	    get().pRight = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pRight, (sfVector2f){ 90.0f * 2.0f, 343.0f * 2.0f });

	    get().pLeft = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pLeft, (sfVector2f){ 40.0f * 2.0f, 343.0f * 2.0f});

	    get().pSelect = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pSelect, (sfVector2f){ 113.0f * 2.0f, 418.0f * 2.0f});

	    get().pStart = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pStart, (sfVector2f){ 166.0f * 2.0f, 418.0f * 2.0f});

	    get().pAButton = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pAButton, (sfVector2f){ 266.0f * 2.0f, 325.0f * 2.0f });

	    get().pBButton = LoadSprite(add.c_str(), 0);
	    sfSprite_setPosition(get().pBButton, (sfVector2f){ 220.0f * 2.0f, 349.0f * 2.0f});
    }

    
    get().screenImg = sfImage_createFromColor(160 * get().pixelSize, 144 * get().pixelSize, sfBlack);
    
    get().screenTex = sfTexture_createFromImage(get().screenImg, NULL);
    get().screenSpr = sfSprite_create();
    sfSprite_setTexture(get().screenSpr, get().screenTex, sfTrue);

    if(get().screenConfig == 1)
    {
    	sfSprite_setPosition(get().screenSpr, (sfVector2f){ 95.0f, 82.0f });
    }
    else if(get().screenConfig == 2)
    {
    	sfSprite_setPosition(get().screenSpr, (sfVector2f){ 95.0f * 2.0f, 82.0f * 2.0f });
    }
    else if(get().screenConfig == 3)
    {
    	int widthGBScreen = get().pixelSize * 160;
    	int heightGBScreen = get().pixelSize * 144;
    	sfVideoMode screenParam = sfVideoMode_getDesktopMode();
    	int x = (screenParam.width / 2) - (widthGBScreen / 2);
    	int y = (screenParam.height / 2) - (heightGBScreen / 2);
    	sfSprite_setPosition(get().screenSpr, (sfVector2f){ (float)x, (float)y });
    }
}

unsigned int Emu::GetPixelSize()
{
	return get().pixelSize;
}

void Emu::LoadGame()
{
	FILE *rom = NULL;
	rom = fopen(get().romName, "rb");

	if(rom != NULL)
	{
		fread(get().cartridgeMemory, 1, 0x200000, rom);
		fclose(rom);
		memcpy(&get().internalMemory[0x0], &get().cartridgeMemory[0x0], 0x8000);
	}
	else
	{
		printf("error opening file");
		//error, can't open file
	}

	get().currentRomBank = 1;
}

void Emu::GetRomInfo()
{
	get().activeRam = false;
	get().activeBattery = false;
	get().NOROM = false;
	get().MBC1 = false;
	get().MBC2 = false;
	get().MBC3 = false;
	get().MBC5 = false;
	get().rtc = false;

	switch(get().cartridgeMemory[0x147])
	{
		case 0x0: get().NOROM = true; break;
        case 0x1: get().MBC1 = true; break;
        case 0x2: get().MBC1 = true; get().activeRam = true; break;
        case 0x3: get().MBC1 = true; get().activeRam = true; get().activeBattery = true; break;
        case 0x5: get().MBC2 = true; break;
        case 0x6: get().MBC2 = true; get().activeBattery = true; break;
		case 0x8: get().NOROM = true; get().activeRam = true; break;
		case 0x9: get().NOROM = true; get().activeRam = true; get().activeBattery = true; break;
		case 0xF: get().MBC3 = true; get().activeBattery = true; get().rtc = true; break;
		case 0x10: get().MBC3 = true; get().activeRam = true; get().activeBattery = true; get().rtc = true; break;
		case 0x11: get().MBC3 = true; break;
		case 0x12: get().MBC3 = true; get().activeRam = true; break;
		case 0x13: get().MBC3 = true; get().activeRam = true; get().activeBattery = true; break;
		case 0x19: get().MBC5 = true; break;
		case 0x1A: get().MBC5 = true; get().activeRam = true; break;
		case 0x1B: get().MBC5 = true; get().activeRam = true; get().activeBattery = true; break;
		case 0x1C: get().MBC5 = true; break;
		case 0x1D: get().MBC5 = true; get().activeRam = true; break;
		case 0x1E: get().MBC5 = true; get().activeRam = true; get().activeBattery = true; break;
        default: break;
	}
}

void Emu::GetSave()
{
	strcpy(get().saveName, get().romName);
	char* savePointer;
	savePointer = strstr(saveName, ".gb");
	strncpy(savePointer, ".sav", 4);

	if(get().activeBattery)
	{
		if(access(saveName, F_OK) != 1)
		{
			//save found
			FILE* saveFile = fopen(saveName, "rb");
			fread(get().ramBanks, sizeof(byte), sizeof(get().ramBanks), saveFile);
			fclose(saveFile);
		}
	}
}

void Emu::WriteSave()
{
	if(get().activeBattery)
	{
		FILE* saveWFile = fopen(saveName, "wb");
		fwrite(get().ramBanks, sizeof(byte), sizeof(get().ramBanks), saveWFile);
		fclose(saveWFile);
	}
}

void Emu::KeyPressed(int key)
{
   bool previouslyUnset = false; 

   if(!TestBit(get().joypadState, key))
   {
   		previouslyUnset = true ;
   }

   get().joypadState = BitClear(get().joypadState, key);

   bool button = true;

   if(key > 3)
   { 
		button = true;
   }
   else
   { 
		button = false;
   }
   
   byte keyReq = get().internalMemory[0xFF00];
   bool requestInterupt = false; 

   if(button && !TestBit(keyReq, 5))
   {
		requestInterupt = true;
   }
   else if(!button && !TestBit(keyReq, 4))
   {
		requestInterupt = true;
   }

   if(requestInterupt && !previouslyUnset)
   {
		SharpLr::RequestInterrupt(4);
   }
}

void Emu::KeyReleased(int key)
{
   get().joypadState = BitSet(get().joypadState, key);
}

byte Emu::GetJoypadState()
{
   byte res = get().internalMemory[0xFF00];
   
   res ^= 0xFF;

   if(!TestBit(res, 4))
   {
     byte topJoypad = get().joypadState >> 4;
     topJoypad |= 0xF0; 
     res &= topJoypad; 
   }
   else if (!TestBit(res, 5))
   {
     byte bottomJoypad = get().joypadState & 0xF;
     bottomJoypad |= 0xF0; 
     res &= bottomJoypad; 
   }

   return res;
}

void Emu::HandleInput(){
	
	if(event.type == sfEvtKeyPressed)
	{
		int key = -1;
		
		if(event.key.code == sfKeyO)
		{
			key = 4;
			get().bpAButton = true;
		}
		else if(event.key.code == sfKeyK)
		{
			key = 5;
			get().bpBButton = true;
		}
		else if(event.key.code == sfKeyA)
		{
			key = 7;
			get().bpStart = true;
		}
		else if(event.key.code == sfKeySpace)
		{
			key = 6;
			get().bpSelect = true;
		}
		else if(event.key.code == sfKeyD)
		{
			key = 0;
			get().bpRight = true;
		}
		else if(event.key.code == sfKeyQ)
		{
			key = 1;
			get().bpLeft = true;
		}
		else if(event.key.code == sfKeyZ)
		{
			key = 2;
			get().bpUp = true;
		}
		else if(event.key.code == sfKeyS)
		{
			key = 3;
			get().bpDown = true;
		}
		else if(event.key.code == sfKeyEscape)
		{
			get().quit = true;
		}
		
		if (key != -1)
		{
			get().KeyPressed(key);
		}
	}
	else if(event.type == sfEvtKeyReleased)
	{
		int key = -1;
		
		if(event.key.code == sfKeyO)
		{
			key = 4;
			get().bpAButton = false;
		}
		else if(event.key.code == sfKeyK)
		{
			key = 5;
			get().bpBButton = false;
		}
		else if(event.key.code == sfKeyA)
		{
			key = 7;
			get().bpStart = false;
		}
		else if(event.key.code == sfKeySpace)
		{
			key = 6;
			get().bpSelect = false;
		}
		else if(event.key.code == sfKeyD)
		{
			key = 0;
			get().bpRight = false;
		}
		else if(event.key.code == sfKeyQ)
		{
			key = 1;
			get().bpLeft = false;
		}
		else if(event.key.code == sfKeyZ)
		{
			key = 2;
			get().bpUp = false;
		}
		else if(event.key.code == sfKeyS)
		{
			key = 3;
			get().bpDown = false;
		}
		else if(event.key.code == sfKeyEscape)
		{
			get().quit = true;
		}
		
		if (key != -1){
			get().KeyReleased(key);
		}
	}
}

void Emu::FPSChecking()
{
	if(get().first)
	{
		get().first = false;
		get().timer = sfTime_asMilliseconds(sfClock_getElapsedTime(get().myClock));
	}

	get().counter++;

	get().current = sfTime_asMilliseconds(sfClock_getElapsedTime(get().myClock));

	if((get().timer + 1000) < get().current)
	{
		get().timer = get().current;
		get().total = get().counter;
		get().counter = 0;
	}
}

void Emu::Run()
{
	double fps = 60;
	double interval = 1000;
	interval /= fps;

	double lastFrameTime = 0;

	while(sfRenderWindow_isOpen(window) && !quit)
	{
		while(sfRenderWindow_pollEvent(get().window, &event))
		{
			if(event.type == sfEvtClosed)
			{
				get().WriteSave();
				sfRenderWindow_close(get().window);
			}

			get().HandleInput();
		}

		double current = sfTime_asMilliseconds(sfClock_getElapsedTime(get().myClock));

		if((lastFrameTime + interval) <= current)
		{
			lastFrameTime = current;
			get().FPSChecking();
			get().Update();
		}
	}
}

void Emu::Update()
{
	get().loopCounter = 0;

	while(get().loopCounter < MAX_LOOP)
	{
		get().cyclesTime = 0;
		
		get().cyclesTime = SharpLr::ExecuteNextOpcode();
		get().loopCounter += get().cyclesTime;

		Ppu::Update(get().cyclesTime);
		SharpLr::UpdateTimer(get().cyclesTime);
		get().loopCounter += SharpLr::UpdateInterrupts();
	}
	Ppu::RenderScreen(get().screenImg, get().screenTex, get().screenSpr, get().window);
	get().RenderGameboy();

	bool stereo = get().apu.end_frame(get().loopCounter);
	get().buf.end_frame(get().loopCounter, stereo);

	if (get().buf.samples_avail() >= 4096)
	{
		size_t count = get().buf.read_samples(get().out_buf, 4096);

		get().sound.write(get().out_buf, count);
	}
}

void Emu::RenderGameboy()
{	
	sfRenderWindow_clear(window, sfBlack);
	if(get().screenConfig == 1 || get().screenConfig == 2)
	{
		sfRenderWindow_drawSprite(get().window, get().gameboySpr, NULL);
		if(get().bpUp)
		{
			sfRenderWindow_drawSprite(get().window, get().pUp, NULL);
		}
		if(get().bpDown)
		{
			sfRenderWindow_drawSprite(get().window, get().pDown, NULL);
		}
		if(get().bpLeft)
		{
			sfRenderWindow_drawSprite(get().window, get().pLeft, NULL);
		}
		if(get().bpRight)
		{
			sfRenderWindow_drawSprite(get().window, get().pRight, NULL);
		}
		if(get().bpStart)
		{
			sfRenderWindow_drawSprite(get().window, get().pStart, NULL);
		}
		if(get().bpSelect)
		{
			sfRenderWindow_drawSprite(get().window, get().pSelect, NULL);
		}
		if(get().bpAButton)
		{
			sfRenderWindow_drawSprite(get().window, get().pAButton, NULL);
		}
		if(get().bpBButton)
		{
			sfRenderWindow_drawSprite(get().window, get().pBButton, NULL);
		}
	}
    sfRenderWindow_drawSprite(window, get().screenSpr, NULL);
	sfRenderWindow_display(get().window);
}



//MMU

void Emu::WriteDirectMemory(word address, byte data)
{
	get().internalMemory[address] = data;
}

byte Emu::ReadDirectMemory(word address)
{
	return get().internalMemory[address];
}

void Emu::WriteMemory(word address, byte data)
{
	get().WriteMapper(address, data);
}

void Emu::WriteMapper(word address, byte data)
{
	if((address >= 0xFEA0) && (address <= 0xFEFF))
	{
		//do not write data here, restricted area
	}
	else if(address >= apu.start_addr && address <= apu.end_addr)
	{
		apu.write_register(get().loopCounter, address, data);
	}
	else if(address == 0xFF04)
	{
		SharpLr::SetClockFrequence();
		get().internalMemory[0xFF04] = 0;
		SharpLr::SetDividerRegister(0);
	}
	else if(address == 0xFF07)
	{
		byte currentFreq = SharpLr::GetClockFrequence();
		data |= 0xF8;
		get().internalMemory[address] = data;
		byte newFreq = SharpLr::GetClockFrequence();

		if(currentFreq != newFreq)
		{
			SharpLr::SetClockFrequence();
		}
	}
	else if(address == 0xFF41)
	{
		get().internalMemory[0xFF41] = data | 0x80;
	}
	else if(address == 0xFF44)
    {
        get().internalMemory[0xFF44] = 0;
    }
    else if(0xFF46 == address)
	{
        get().DoDMATransfer(data);
    }
    else if((address >= 0xFF4C) && (address <= 0xFF7F))
	{
 		//restricted area
	}
	else if(get().NOROM)
	{
		get().WriteNOROM(address, data);
	}
	else if(get().MBC1)
	{
		get().WriteMBC1(address, data);
	}
	else if(get().MBC2)
	{
		get().WriteMBC2(address, data);
	}
	else if(get().MBC3)
	{
		get().WriteMBC3(address, data);
	}
	else if(get().MBC5)
	{
		get().WriteMBC5(address, data);
	}
}


byte Emu::ReadMemory(word address)
{
	return get().ReadMapper(address);	
}

byte Emu::ReadMapper(word address)
{
	if(address == 0xFF00)
	{
		return get().GetJoypadState();
	}
	else if(address >= apu.start_addr && address <= apu.end_addr)
	{
		return apu.read_register(get().loopCounter, address);
	}
	else if(address == 0xFF0F)
	{
		return get().internalMemory[address] |= 0xE0;
	}
	else if(get().NOROM)
	{
		return get().ReadNOROM(address);
	}
	else if(get().MBC1)
	{
		return get().ReadMBC1(address);
	}
	else if(get().MBC2)
	{
		return get().ReadMBC2(address);
	}
	else if(get().MBC3)
	{
		return get().ReadMBC3(address);
	}
	else if(get().MBC5)
	{
		return get().ReadMBC5(address);
	}
	return 0;
}

void Emu::DoDMATransfer(byte data)
{
    word address = data << 8;
    int i;
    for(i = 0; i < 0xA0; i++)
    {
       get().WriteMemory(0xFE00+i, get().ReadMemory(address+i));
    }
}

void Emu::WriteNOROM(word address, byte data)
{
	if(address < 0x8000)
	{
	//restricted area	
	}
	else if((address >= 0xA000) && (address < 0xC000))
	{
        if(get().enableRam)
        {
			word newAddress = address - 0xA000;
			get().ramBanks[newAddress] = data;
        }
    }
	else if((address >= 0xE000) && (address <= 0xFDFF))
	{
		get().internalMemory[address] = data ;
		get().internalMemory[address-0x2000] = data ; // echo data into ram address
	}
	else
	{
		get().internalMemory[address] = data;
	}
}

byte Emu::ReadNOROM(word address)
{	
    if((address >= 0xA000) && (address <= 0xBFFF))
    {
        word newAddress = address - 0xA000;
        return get().ramBanks[newAddress + (get().currentRamBank * 0x2000)];
    }
	else
	{
		return get().internalMemory[address];
	}
}

void Emu::WriteMBC1(word address, byte data)
{
	if(address < 0x2000)
	{
		if((data & 0xF) == 0xA)
		{
			get().enableRam = true;
		}
		else if((data & 0xF) == 0x0)
		{
			get().enableRam = false;
		}
	}
	else if((address >= 0x2000) && (address < 0x4000))
	{
		data &= 0x1F;

		get().currentRomBank &= 0xE0;

		get().currentRomBank |= data;

		if(get().currentRomBank == 0x00 || get().currentRomBank == 0x20 || get().currentRomBank == 0x40 || get().currentRomBank == 0x60)
		{
			get().currentRomBank++;
		}
	}
	else if((address >= 0x4000) && (address < 0x6000))
	{
		if(get().romBanking)
		{
			get().currentRamBank = 0;

			data &= 0x3;
			data <<= 5;

			if((get().currentRomBank & 0x1F) == 0)
			{
				data++;
			}

			get().currentRomBank &= 0x1F;

			get().currentRomBank |= data;
		}
		else
		{
			get().currentRamBank = data & 0x3;
		}
	}
	else if((address >= 0x6000) && (address < 0x8000))
	{
		byte newData = data & 0x1;

		if(newData == 0)
		{
			get().romBanking = true;
		}
		else
		{
			get().romBanking = false;
		}

		if(!get().romBanking)
		{
			get().currentRamBank = 0;
		}
	}
	else if((address >= 0xA000) && (address < 0xC000))
	{
		if(get().enableRam)
		{
			word newAdress = address - 0xA000;
			get().ramBanks[newAdress + (get().currentRamBank * 0x2000)] = data;
		}
	}
	else if((address >= 0xE000) && (address <= 0xFDFF))
	{
		get().internalMemory[address] = data;
		get().internalMemory[address - 0x2000] = data; // mirroring
	}
	else
	{
		get().internalMemory[address] = data;
	}
}

byte Emu::ReadMBC1(word address)
{
	if((address >= 0x4000) && (address <= 0x7FFF))
	{
		word newAdress = address - 0x4000;
		return get().cartridgeMemory[newAdress +(get().currentRomBank * 0x4000)];
	}
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		word newAdress = address - 0xA000;
		return get().ramBanks[newAdress + (get().currentRamBank * 0x2000)];
	}
	else
	{
		return get().internalMemory[address];
	}
}

void Emu::WriteMBC2(word address, byte data)
{
	if(address < 0x2000)
	{
		if(TestBit(address, 8) == false)
		{
			if((data & 0xF) == 0xA)
			{
				get().enableRam = true;
			}
			else if(data == 0x0)
			{
				get().enableRam = false;
			}
		}
	}
	else if((address >= 0x2000) && (address < 0x4000))
	{
		data &= 0xF;
		get().currentRomBank = data;
	}
	else if((address >= 0x4000) && (address < 0x8000))
	{
		//restricted area
	}
	else if((address >= 0xA000) && (address < 0xC000))
	{
		if((address >= 0xA000) && (address < 0xA200))
		{
			word newAdress = address - 0xA000;
			get().ramBanks[newAdress + get().currentRamBank * 0x2000] = data;
		}
		else
		{
			//restricted area
		}
	}
	else if((address >= 0xE000) && (address <= 0xFDFF))
	{
		get().internalMemory[address] = data;
		get().internalMemory[address - 0x2000] = data;
	}
	else
	{
		get().internalMemory[address] = data;
	}
}

byte Emu::ReadMBC2(word address)
{
	if((address >= 0x4000) && (address <= 0x7FFF))
	{
		word newAdress = address - 0x4000;
		return get().cartridgeMemory[newAdress + (get().currentRomBank * 0x4000)];
	}
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		word newAdress = address - 0xA000;
		return get().ramBanks[newAdress + (get().currentRamBank * 0x2000)];
	}
	else
	{
		return get().internalMemory[address];
	}
}

void Emu::GrabTime()
{
	time_t systemTime = time(0);
	tm* currentTime = localtime(&systemTime);

	get().rtcReg[0] = currentTime->tm_sec;

	if(get().rtcReg[0] > 59)
	{
		get().rtcReg[0] = 59;
	}

	get().rtcReg[0] = (get().rtcReg[0] % 60);

	get().rtcReg[1] = currentTime->tm_min;
	get().rtcReg[1] = (get().rtcReg[1] % 60);

	get().rtcReg[2] = currentTime->tm_hour;
	get().rtcReg[2] = (get().rtcReg[1] % 24);

	word tempDay = currentTime->tm_yday;
	tempDay = (tempDay % 366);

	get().rtcReg[3] = tempDay & 0xFF;
	tempDay >>= 8;

	if(tempDay == 1)
	{
		get().rtcReg[4] |= 0x1;
	}
	else
	{
		get().rtcReg[4] &= ~0x1;
	}

	for(int x = 0; x < 5; x++)
	{
		get().latchReg[5] = get().rtcReg[5];
	}
}

void Emu::WriteMBC3(word address, byte data)
{
	if(address < 0x2000)
	{
		if((data & 0xF) == 0xA)
		{
			get().enableRam = true;
			get().rtcEnabled = true;
		}
		else
		{
			get().enableRam = false;
			get().rtcEnabled = false;
		}
	}
	else if((address >= 0x2000) && (address < 0x4000))
	{
		if((data & 0x7F) == 0x00)
		{
			get().currentRomBank = 0x01;
		}
		else
		{
			get().currentRomBank = (data & 0x7F);
		}
	}
	else if((address >= 0x4000) && (address < 0x6000))
	{
		if((data >= 0x08) && (data <= 0x0C))
		{
			get().rtc = true;
			get().currentRamBank = data;
		}
		if(data <= 0x03)
		{
			get().rtc = false;
			get().currentRamBank = data;
		}
	}
	else if((address >= 0x6000) && (address < 0x8000))
	{
		if(get().rtcEnabled)
		{
			if((get().rtcLatch1 == 0xFF) && (data == 0))
			{
				get().rtcLatch1 = 0;
			}
			else if((get().rtcLatch2 == 0xFF) && (data == 1))
			{
				GrabTime();
				get().rtcLatch1 = get().rtcLatch2 = 0xFF;	
			}
		}
	}
	else if((address >= 0xA000) && (address < 0xC000))
	{
        if(get().enableRam && (get().currentRamBank <= 3))
        {
			word newAddress = address - 0xA000;
			get().ramBanks[newAddress + (get().currentRamBank * 0x2000)] = data;
        }
		else if(get().rtcEnabled && (get().currentRamBank >= 8) && (get().currentRamBank <= 12))
		{
			get().rtcReg[get().currentRamBank - 8] = data;
		}
    }
	else if((address >= 0xE000) && (address <= 0xFDFF))
	{
		get().internalMemory[address] = data ;
		get().internalMemory[address - 0x2000] = data ; // echo data into ram address
	}
	else
	{
		get().internalMemory[address] = data;
	}
}

byte Emu::ReadMBC3(word address)
{
	if((address >= 0x4000) && (address <= 0x7FFF))
	{
			word newAddress = address - 0x4000;
			return get().cartridgeMemory[newAddress + (get().currentRomBank * 0x4000)];
    }
    else if((address >= 0xA000) && (address <= 0xBFFF))
    {
		if((get().enableRam) && (get().currentRamBank <= 3))
		{
			word newAddress = address - 0xA000;
			return get().ramBanks[newAddress + (get().currentRamBank * 0x2000)];
		}
		else if((get().rtcEnabled) && (get().currentRamBank >= 8) && (get().currentRamBank <= 12))
		{
			return get().rtcReg[get().currentRamBank - 8];
		}
		else
		{
			return 0x00;
		}
    }
	else
	{
		return get().internalMemory[address];
	}
}

void Emu::WriteMBC5(word address, byte data)
{
	if(address < 0x2000)
	{
		if((data & 0xF) == 0xA)
		{
			get().enableRam = true;
		}
		else if((data & 0xF) == 0x0)
		{
			get().enableRam = false;
		}
	}
	else if((address >= 0x2000) && (address < 0x3000))
	{
		get().currentRomBank = (get().currentRomBank & 0x100) | data;
	}
	else if((address >= 0x3000) && (address < 0x4000))
	{
		get().currentRomBank = (get().currentRomBank & 0xFF) | (data & 0x01) << 8;
	}
	else if((address >= 0x4000) && (address < 0x6000))
	{
		get().currentRamBank = data & 0xF;
	}
	else if((address >= 0xA000) && (address < 0xC000))
	{
		if(get().enableRam)
		{
			word newAdress = address - 0xA000;
			get().ramBanks[newAdress + (get().currentRamBank * 0x2000)] = data;
		}
	}
	else if((address >= 0xE000) && (address <= 0xFDFF))
	{
		get().internalMemory[address] = data;
		get().internalMemory[address - 0x2000] = data; // mirroring
	}
	else
	{
		get().internalMemory[address] = data;
	}
}

byte Emu::ReadMBC5(word address)
{
	if((address >= 0x4000) && (address <= 0x7FFF))
	{
		word newAdress = address - 0x4000;
		return get().cartridgeMemory[newAdress +(get().currentRomBank * 0x4000)];
	}
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		word newAdress = address - 0xA000;
		return get().ramBanks[newAdress + (get().currentRamBank * 0x2000)];
	}
	else
	{
		return get().internalMemory[address];
	}
}

