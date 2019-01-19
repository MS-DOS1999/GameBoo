#include "emu.hpp"

int main(int argc, char **argv)
{
	Emu::Intro();
	Emu::PlayMusic();
	Emu::HideCMD();
	Emu::FileBrowser();
	Emu::StopMusic();
	Emu::Execute();

	return 0;
}

sfSprite* LoadSprite(char* _sNom, int _isCentered)
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
}

Emu& Emu::get(void)
{
	static Emu instance;
	return instance;
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

void Emu::PlayMusic()
{
	get().kidZan = sfMusic_createFromFile("MUSIC\\kid_zan.ogg");
	sfMusic_play(get().kidZan);
	Sleep(6000);
}

void Emu::HideCMD()
{
	HWND hWnd = GetConsoleWindow(); 
    ShowWindow(hWnd, SW_HIDE);
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

void Emu::StopMusic()
{
	sfMusic_stop(get().kidZan);

	//init APU
	get().buf.set_sample_rate(44100);
	get().buf.clock_rate(4194304);
	get().apu.output(get().buf.center(), get().buf.left(), get().buf.right());

	get().sound.start(44100, 2);
}

void Emu::Execute()
{
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
	get().mode = (sfVideoMode){351, 541, 32};

    get().window = sfRenderWindow_create(get().mode, "GameBoo - Chili Hot Dog Version", sfClose, NULL);

    get().myClock = sfClock_create();

    get().gameboySpr = LoadSprite("SPRITES\\gameboy.png", 0);
    sfSprite_setPosition(get().gameboySpr, (sfVector2f){ 0.0f, 0.0f });

    get().pUp = LoadSprite("SPRITES\\pushed.png", 0);
    sfSprite_setPosition(get().pUp, (sfVector2f){ 65.0f, 317.0f });

    get().pDown = LoadSprite("SPRITES\\pushed.png", 0);
    sfSprite_setPosition(get().pDown, (sfVector2f){ 65.0f, 367.0f });

    get().pRight = LoadSprite("SPRITES\\pushed.png", 0);
    sfSprite_setPosition(get().pRight, (sfVector2f){ 90.0f, 343.0f });

    get().pLeft = LoadSprite("SPRITES\\pushed.png", 0);
    sfSprite_setPosition(get().pLeft, (sfVector2f){ 40.0f, 343.0f });

    get().pSelect = LoadSprite("SPRITES\\pushed.png", 0);
    sfSprite_setPosition(get().pSelect, (sfVector2f){ 113.0f, 418.0f });

    get().pStart = LoadSprite("SPRITES\\pushed.png", 0);
    sfSprite_setPosition(get().pStart, (sfVector2f){ 166.0f, 418.0f });

    get().pAButton = LoadSprite("SPRITES\\pushed.png", 0);
    sfSprite_setPosition(get().pAButton, (sfVector2f){ 266.0f, 325.0f });

    get().pBButton = LoadSprite("SPRITES\\pushed.png", 0);
    sfSprite_setPosition(get().pBButton, (sfVector2f){ 220.0f, 349.0f });

    get().screenImg = sfImage_createFromColor(160, 144, sfBlack);
    get().screenTex = sfTexture_createFromImage(screenImg, NULL);
    get().screenSpr = sfSprite_create();
    sfSprite_setTexture(get().screenSpr, get().screenTex, sfTrue);
    sfSprite_setPosition(get().screenSpr, (sfVector2f){ 95.0f, 82.0f });
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
		get().internalMemory[0xFF0F] |= 0x10;
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
			quit = true;
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
			quit = true;
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
	double fps = 59.73;
	double interval = 1000;
	interval /= fps;

	double lastFrameTime = 0;

	while(sfRenderWindow_isOpen(window))
	{
		while(sfRenderWindow_pollEvent(get().window, &event))
		{
			if(event.type == sfEvtClosed)
			{
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
		get().cyclesTime += SharpLr::ExecuteNextOpcode();
		SharpLr::UpdateTimer(get().cyclesTime);
		Vdp::Update(get().cyclesTime);
		SharpLr::UpdateInterrupts();
		get().loopCounter += get().cyclesTime;
	}
	Vdp::RenderScreen(get().screenImg, get().screenTex, get().screenSpr, get().window);
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
	
	if(get().NOROM)
	{
		get().WriteNOROM(address, data);
	}
	else if(get().MBC1)
	{
		get().WriteMBC1(address, data);
	}
	/*else if(z80.m_MBC2)
	{
		Write_MBC2(address, data);
	}
	else if(z80.m_MBC3)
	{
		Write_MBC3(address, data);
	}*/
	
}


byte Emu::ReadMemory(word address)
{
    
	if(get().NOROM)
	{
		return get().ReadNOROM(address);
	}
	else if(get().MBC1)
	{
		return get().ReadMBC1(address);
	}
	/*else if(z80.m_MBC2)
	{
		return Read_MBC2(address);
	}
	else if(z80.m_MBC3)
	{
		return Read_MBC3(address);
	}*/
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
        if(get().activeRam)
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
 	else if((address >= 0xFEA0) && (address <= 0xFEFF))
 	{
 		//restricted area
	}
    else if(0xFF04 == address)
    {
        get().internalMemory[0xFF04] = 0;
		SharpLr::SetDividerRegister(0);
    }
	else if(0xFF07 == address)
	{
		data |= 0xF8;
		get().internalMemory[address] = data;
	}
	else if(0xFF0F == address)
	{
		data |= 0xE0;
		get().internalMemory[address] = data;
	}
	else if(address >= apu.start_addr && address <= apu.end_addr)
	{
		apu.write_register(get().loopCounter, address, data);
	}
    else if(0xFF44 == address)
    {
        get().internalMemory[0xFF44] = 0;
    }
	else if(0xFF46 == address)
	{
        get().DoDMATransfer(data);
    }
	else if ((address >= 0xFF4C) && (address <= 0xFF7F))
	{
 		//restricted area
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
	else if(0xFF00 == address)
	{
		return get().GetJoypadState();
	}
	else if(address >= 0xFF10 && address <= 0xFF26)
	{
		return apu.read_register(get().loopCounter, address);
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
			get().activeRam = true;
		}
		else if(data == 0x0)
		{
			get().activeRam = false;
		}
	}
	else if((address >= 0x2000) && (address < 0x4000))
	{
		if(data == 0x00)
		{
			data++;
		}

		data &= 0x1F;

		get().currentRomBank &= 0xE0;

		get().currentRomBank |= data;
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
		if(get().activeRam)
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
	else if((address >= 0xFEA0) && (address <= 0xFEFF))
	{
		//do not write data here, restricted area
	}
	else if(address == 0xFF04)
	{
		get().internalMemory[0xFF04] = 0;
		SharpLr::SetDividerRegister(0);
	}
	else if(address == 0xFF07)
	{
		data |= 0xF8;
		get().internalMemory[address] = data;
	}
	else if(address == 0xFF0F)
	{
		data |= 0xE0;
		get().internalMemory[address] = data;
	}
	else if((address >= 0xFF10) && (address <= 0xFF26))
	{
		apu.write_register(get().loopCounter, address, data);
	}
	else if(address == 0xFF44)
	{
		get().internalMemory[0xFF44] = 0;
	}
	else if(address == 0xFF46)
	{
		get().DoDMATransfer(data);
	}
	else if((address >= 0xFF4C) && (address <= 0xFF7F))
	{
		//do not write data here, restricted area
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
	else if(address == 0xFF00)
	{
		return get().GetJoypadState();
	}
    else if((address >= 0xFF10) && address <= 0xFF26)
	{
		return apu.read_register(get().loopCounter, address);
	}
	else
	{
		return get().internalMemory[address];
	}
}


