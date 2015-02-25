#define SDL_MAIN_HANDLED
#define WINDOWWIDTH 450 //600
#define WINDOWHEIGHT 663 //884
#define TILENUM 4
#define DEFAULTBAGWIDTH 64
#define DEFAULTBAGHEIGHT 64
#define BGNUM 4
#include<SDL.h>
#include<GL/glew.h>
#include<stdio.h>
#include<string.h>
#include<vector>
#include"fmod.h"
#include"DrawUtils.h"

struct AnimFrameDef {
	GLuint texture;
	int frameNum;
	float frameTime;
};

struct AnimDef {
	const char* name;
	std::vector<AnimFrameDef> frames;
};

struct AnimData {
	AnimDef* def;
	int curFrame;
	float timeToNextFrame;
	bool isPlaying;
};

struct ColBox {
	int x;
	int y;
	int w;
	int h;
};

struct Actor {
	float x;
	float y;
	int width;
	int height;
	AnimData anim;
	ColBox box;
	float moveSpeed;
};

struct Background {
	GLuint bgarray[TILENUM][TILENUM];
};

enum EventType {
	ET_COLLISION_BIG,
	ET_COLLISION_EAT,
	ET_MODE_EASY,
	ET_MODE_HARD,
	ET_LOSE,
	ET_TIMER_COUNTDOWN,
	ET_TIME_OVER,
	ET_NEXT_LEVEL,
	ET_START,
	ET_RESET_GAMESTATE
};

struct Event {
	EventType type;
	Actor* object;
	Actor* subject;
	Actor* subject2;
};

struct GLFontChar {
	GLuint tex;
	int w;
	int h;
};

struct GLFont{
	GLFontChar chars[256];
};

Actor makeActor( float x, float y, int w, int h, AnimData anim, float speed);
void moveActor( Actor* Actor);
void animTick( AnimData* data, float dt);
void makeAABB( Actor* actor);
bool boxCollision(ColBox a, ColBox b);
bool collisionRes(Actor* a, ColBox b, Actor* bag);
void initEventQueue(void);
Event makeEvent(EventType type, Actor* object, Actor* subject, Actor* subject2);
void queueEvent(Event* ev);
void updateEventQueue(void);
void handleEvent(Event* ev);
GLFontChar makeFontChar( GLuint tex, int w, int h);
void glDrawString(GLFont* font, const char* str, int x, int y);
void chooseBG(void);

std::vector<Event> eventQueue;
std::vector<Actor> allSprites;
bool easyMode = true;
bool gameOver = false;
bool levelTransition = false;
bool titleScreen = true;
bool stopMovement = true;
bool timerLock = true;
int healthLeft = 2;
int level = 0;
int time = 1000;
int curBG = -1;
int prevBG = -1;


int main( void )
{
	bool shouldExit = false;
	int w = WINDOWWIDTH;
	int h = WINDOWHEIGHT;
	std::vector<Actor> particles;
	std::vector<Actor> explosions;
	int dustDirection = 0;
	GLFont font;

	// Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
		return 1;
	}

	/// The previous frame's keyboard state
	unsigned char kbPrevState[SDL_NUM_SCANCODES] = { 0 };
	/// The current frame's keyboard state
	const unsigned char* kbState = NULL;
	/// Tick Initialization
	int ticksPerFrame = 1000/60;
	int prevTick = SDL_GetTicks();
	int ticksPerPhysics = 1000/100;
	int prevPhysicsTick = prevTick;
	int ticksPerDust = 1000/(3 + (level+2));
	int prevDustTick = prevTick;
	int ticksPerTimer = 1000;
	int prevTimerTick = prevTick;
	int ticksPerExplosion = 1000/15;
	int prevExplosionTick = prevTick;
	int moveSpeed = 3;
	bool paused = true;

	int particleCounter = 0;
	int particleX = 0;
	int particleY = 0;


	//Init Event Queue
	initEventQueue();

	// Create the window, OpenGL context
	SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE, 32 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_Window* window = SDL_CreateWindow(
		"Vacuums are the Enemy!",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		w, h,
		SDL_WINDOW_OPENGL );
	if( !window ) {
		SDL_Quit();
		return 1;
	}
	SDL_GL_CreateContext( window );

	//Make sure we have a recent version of OpenGL
	GLenum glewError = glewInit();
	if( glewError != GLEW_OK ) {
		SDL_Quit();
		return 1;
	}
	if( !GLEW_VERSION_1_5 ) {
		SDL_Quit();
		return 1;
	}

	// Keep a pointer to SDL's internal keyboard state
	kbState = SDL_GetKeyboardState( NULL );

	glViewport( 0, 0, WINDOWWIDTH, WINDOWHEIGHT );
	glMatrixMode( GL_PROJECTION );
	glOrtho( 0, w, h, 0, 0, 1 );
	glEnable( GL_TEXTURE_2D );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_ALPHA_TEST );
	glAlphaFunc( GL_GREATER, 0.04 );
	
	//Load textures and make sprite structs
	GLuint bgtex = glTexImageTGAFile("woodbg.tga",NULL,NULL);
	GLuint bgtex2 = glTexImageTGAFile("glassbg.tga",NULL,NULL);
	GLuint bgtex3 = glTexImageTGAFile("greenbg.tga",NULL,NULL);
	GLuint bgtex4 = glTexImageTGAFile("lavabg.tga",NULL,NULL);
	GLuint tex = glTexImageTGAFile("dust1.tga",NULL,NULL);
	GLuint tex2 = glTexImageTGAFile("dust1red.tga",NULL,NULL);
	GLuint texalt = glTexImageTGAFile("pin.tga",NULL,NULL);
	GLuint texalt2 = glTexImageTGAFile("pinred.tga",NULL,NULL);
	GLuint texaltalt = glTexImageTGAFile("band.tga",NULL,NULL);
	GLuint texaltalt2 = glTexImageTGAFile("bandred.tga",NULL,NULL);
	GLuint dustLeftTex = glTexImageTGAFile("dustproto1.tga",NULL,NULL);
	GLuint dustRightTex = glTexImageTGAFile("dustproto1.tga",NULL,NULL);
	GLuint bagTex = glTexImageTGAFile("bagproto1.tga",NULL,NULL);
	GLuint vacuumTopTex = glTexImageTGAFile("vacuumtop.tga",NULL,NULL);
	GLuint vacuumBotTex = glTexImageTGAFile("vacuumbot.tga",NULL,NULL);
	GLuint particleTex = glTexImageTGAFile("particle1.tga",NULL,NULL);
	GLuint titleTex = glTexImageTGAFile("title.tga",NULL, NULL);
	GLuint loseTex = glTexImageTGAFile("lose.tga",NULL, NULL);
	GLuint nextLevelTex = glTexImageTGAFile("nextlevel.tga",NULL, NULL);
	GLuint sizeTex = glTexImageTGAFile("size.tga",NULL, NULL);
	GLuint targetTex = glTexImageTGAFile("target.tga",NULL, NULL);
	GLuint vacDestTex = glTexImageTGAFile("vacuumsdestroyed.tga",NULL, NULL);
	GLuint vacDestBackTex = glTexImageTGAFile("destback.tga",NULL, NULL);
	GLuint exploTex = glTexImageTGAFile("explosion.tga",NULL, NULL);
	GLuint exploBlankTex = glTexImageTGAFile("explosionBlank.tga",NULL, NULL);
	GLuint brokeVacTopTex = glTexImageTGAFile("brokenvacuumtop.tga",NULL, NULL);
	for(int i = 0; i <256; i++){
		font.chars[i] = makeFontChar(glTexImageTGAFile("fonts2/Blank.tga",NULL,NULL), 30,30);
	}
	font.chars[48] = makeFontChar(glTexImageTGAFile("fonts2/Num0.tga",NULL,NULL), 16,30);
	font.chars[49] = makeFontChar(glTexImageTGAFile("fonts2/Num1.tga",NULL,NULL), 16,30);
	font.chars[50] = makeFontChar(glTexImageTGAFile("fonts2/Num2.tga",NULL,NULL), 16,30);
	font.chars[51] = makeFontChar(glTexImageTGAFile("fonts2/Num3.tga",NULL,NULL), 16,30);
	font.chars[52] = makeFontChar(glTexImageTGAFile("fonts2/Num4.tga",NULL,NULL), 16,30);
	font.chars[53] = makeFontChar(glTexImageTGAFile("fonts2/Num5.tga",NULL,NULL), 16,30);
	font.chars[54] = makeFontChar(glTexImageTGAFile("fonts2/Num6.tga",NULL,NULL), 16,30);
	font.chars[55] = makeFontChar(glTexImageTGAFile("fonts2/Num7.tga",NULL,NULL), 16,30);
	font.chars[56] = makeFontChar(glTexImageTGAFile("fonts2/Num8.tga",NULL,NULL), 16,30);
	font.chars[57] = makeFontChar(glTexImageTGAFile("fonts2/Num9.tga",NULL,NULL), 16,30);
	font.chars[42] = makeFontChar(glTexImageTGAFile("fonts2/heart.tga",NULL,NULL), 30,30);

	//Creating the tiles for background
	Background bg[BGNUM];
	for(int i = 0; i<TILENUM; i++){
		for(int j = 0; j<TILENUM; j++){
			bg[0].bgarray[i][j] = bgtex;
			bg[1].bgarray[i][j] = bgtex2;
			bg[2].bgarray[i][j] = bgtex3;
			bg[3].bgarray[i][j] = bgtex4;
		}
	}
	
	AnimDef expl;
	AnimFrameDef explo;
	AnimFrameDef exploBlank;
	expl.name = "Explosion";
	explo.texture = exploTex;
	exploBlank.texture = exploBlankTex;
	explo.frameNum = 0;
	exploBlank.frameNum = 1;
	explo.frameTime = 20;
	exploBlank.frameTime = 20;
	expl.frames.push_back(explo);
	expl.frames.push_back(exploBlank);
	AnimData explosionData;
	explosionData.def = &expl;
	explosionData.curFrame = 0;
	explosionData.timeToNextFrame = 1000/60;

	//Animation Initialization
	AnimDef fallDust;
	AnimDef fallBand;
	AnimDef fallPin;
	AnimFrameDef fallObjF1;
	AnimFrameDef fallObjF2;
	AnimFrameDef fallObjF3;
	AnimFrameDef fallObjF4;
	AnimFrameDef fallObjF5;
	AnimFrameDef fallObjF6;
	fallDust.name = "Falling Dust";
	fallBand.name = "Falling Band";
	fallPin.name = "Falling Pin";
	fallObjF1.texture = tex;
	fallObjF2.texture = tex2;
	fallObjF3.texture = texalt;
	fallObjF4.texture = texalt2;
	fallObjF5.texture = texaltalt;
	fallObjF6.texture = texaltalt2;
	fallObjF1.frameNum = 0;
	fallObjF2.frameNum = 3;
	fallObjF3.frameNum = 1;
	fallObjF4.frameNum = 4;
	fallObjF5.frameNum = 2;
	fallObjF6.frameNum = 5;
	fallDust.frames.push_back(fallObjF1);
	fallDust.frames.push_back(fallObjF2);
	fallPin.frames.push_back(fallObjF3);
	fallPin.frames.push_back(fallObjF4);
	fallBand.frames.push_back(fallObjF5);
	fallBand.frames.push_back(fallObjF6);
	AnimData fallingDust;
	AnimData fallingPin;
	AnimData fallingBand;
	fallingDust.def = &fallDust;
	fallingPin.def = &fallPin;
	fallingBand.def = &fallBand;

	//Actor Data 
	AnimDef dustMove;
	AnimFrameDef dustLeft;
	AnimFrameDef dustRight;
	dustMove.name = "Dust Move";
	dustLeft.texture = dustLeftTex;
	dustRight.texture = dustRightTex;
	dustLeft.frameNum = 0;
	dustRight.frameNum = 1;
	dustLeft.frameTime = 20;
	dustRight.frameTime = 20;
	dustMove.frames.push_back(dustLeft);
	dustMove.frames.push_back(dustRight);
	AnimDef bagAnimDef;
	AnimFrameDef bagFrameDef;
	bagAnimDef.name = "Bag";
	bagFrameDef.texture = bagTex;
	bagFrameDef.frameNum = 0;
	bagFrameDef.frameTime = 20;
	bagAnimDef.frames.push_back(bagFrameDef);
	AnimData dustData;
	AnimData bagData;
	bagData.def = &bagAnimDef;
	bagData.curFrame = 0;
	bagData.timeToNextFrame = 1000/60;
	dustData.def = &dustMove;
	dustData.curFrame = 0;
	dustData.timeToNextFrame = 1000/60;
	Actor actor = makeActor(WINDOWWIDTH/2,WINDOWWIDTH/2,32,32,dustData, NULL); 
	Actor bag = makeActor((actor.x + (actor.width/2)) - (DEFAULTBAGWIDTH/2),(actor.y + actor.height),DEFAULTBAGWIDTH,DEFAULTBAGHEIGHT,bagData, 5);

	//Play BG music
	FMOD_SYSTEM* fmod;
	FMOD_System_Create(&fmod);
	FMOD_System_Init(fmod, 100, FMOD_INIT_NORMAL,0);
	FMOD_SOUND* bgMusic;
	FMOD_SOUND* collect;
	FMOD_System_CreateSound(fmod, "boop.mp3", FMOD_DEFAULT,0, &collect);
	FMOD_System_CreateStream(fmod, "bgmusic.mp3",FMOD_DEFAULT,0,&bgMusic);
	FMOD_CHANNEL* bgChan;
	FMOD_System_PlaySound(fmod,FMOD_CHANNEL_FREE, NULL, true, &bgChan);	
	FMOD_System_PlaySound(fmod,FMOD_CHANNEL_REUSE,bgMusic,true,&bgChan);
	FMOD_Channel_SetVolume(bgChan, 1.0);
	FMOD_Channel_SetMode(bgChan,FMOD_LOOP_NORMAL);
	FMOD_Channel_SetPaused(bgChan, false);	

	// The game loop
	while( !shouldExit ) {
		// kbState is updated by the message pump. Copy over the old state before the pump!
		memcpy ( kbPrevState, kbState, sizeof( kbPrevState ));
		// Handle OS message pump
		SDL_Event event;
		while( SDL_PollEvent( &event )) {
			switch( event.type ) {
				case SDL_QUIT:
				return 0;
			}
		}

	// Throttle Framerate
	int tick = SDL_GetTicks();
	do {
		SDL_Delay( std::max( 0, ticksPerFrame - (tick - prevTick) ));
		tick = SDL_GetTicks();
	} while( ticksPerFrame - (tick - prevTick) > 0 );
	prevTick = tick;
	
	// Game logic goes here
		
	//BagLimit
	if(bag.width > WINDOWWIDTH/3){
		bag.width = WINDOWWIDTH/3;
		bag.height = WINDOWWIDTH/3;
	}

	//Music FMOD Update
	FMOD_System_Update(fmod); 

	//Timer Once per Second
	while( tick > prevTimerTick + ticksPerTimer) {
		queueEvent( &makeEvent(ET_TIMER_COUNTDOWN,NULL,NULL,NULL));
		prevTimerTick += ticksPerTimer;
	}

	if(healthLeft > 5){
		healthLeft = 5;
	}
	if(healthLeft == 0){
		queueEvent( &makeEvent(ET_LOSE,NULL,NULL,NULL));
	}
	if(time < 0){
		queueEvent( &makeEvent(ET_TIME_OVER,&bag,&actor,NULL));
	}


	//Make Falling Objects
	while( tick > prevDustTick + ticksPerDust) {
		int size = rand()%(bag.width - (bag.width/3)) + ((bag.width/3) + (bag.width/6));
		int objectChoice = rand()%3;
		float randSpeed = rand()%4 + (3 + (float)level/4);
		if(objectChoice == 2){
			allSprites.push_back(makeActor(rand()%WINDOWWIDTH+1-(size/2),-size,size,size,fallingBand,randSpeed));
		}else if(objectChoice == 1){
			allSprites.push_back(makeActor(rand()%WINDOWWIDTH+1-(size/2),-size,size,size,fallingPin,randSpeed));
		}else{
			allSprites.push_back(makeActor(rand()%WINDOWWIDTH+1-(size/2),-size,size,size,fallingDust,randSpeed));
		}
		prevDustTick += ticksPerDust;
	}

	//Draw background
	for(int i = 0; i < TILENUM; i++){
		for(int j = 0; j < TILENUM; j++){
			glDrawSprite(bg[curBG].bgarray[i][j],(i*(w/2)),(j*(w/2)),(w/2),(w/2));
		}//Y drawing for loop
	}//X drawing for loop

	//Draw bottom of vacuum
	glDrawSprite(vacuumBotTex, 0, h - 180, w, 210);
	//Draw ambient particles
	if(particleCounter == 30){
		particleX = rand() % w - 128;
	}	
	if(particleCounter == 60){
		particleCounter = 0;
	}
	if(particleY > WINDOWHEIGHT){
		particleY = 0;
	}
	particleY = particleY + 25;	
	particleCounter++;
	glDrawSprite(particleTex, particleX, particleY, 256, 256);
	

	//Draw Falling Objects
	for(std::vector<Actor>::size_type i = 0; i != allSprites.size(); i++) {
		//This is Easy Mode, colors what you can grab and what you have to avoid
		if(easyMode){
			if(allSprites.at(i).width > bag.width){
				glDrawSprite((allSprites.at(i).anim.def->frames.at(1).texture),allSprites.at(i).x,allSprites.at(i).y,allSprites.at(i).width,allSprites.at(i).height);	
			}else{
				glDrawSprite((allSprites.at(i).anim.def->frames.at(0).texture),allSprites.at(i).x,allSprites.at(i).y,allSprites.at(i).width,allSprites.at(i).height);	
			}
		}else{
			glDrawSprite((allSprites.at(i).anim.def->frames.at(0).texture),allSprites.at(i).x,allSprites.at(i).y,allSprites.at(i).width,allSprites.at(i).height);
		}
	}

	//Move Dust
	if(!stopMovement){
		for(std::vector<Actor>::size_type i = 0; i != allSprites.size(); i++) {
			moveActor(&allSprites[i]);
		}
	}

	//Draw Main Character
	glDrawSprite(actor.anim.def->frames.at(dustDirection).texture, actor.x, actor.y, actor.width, actor.height);
	glDrawSprite(bag.anim.def->frames.at(0).texture, bag.x, bag.y, bag.width, bag.height);

	//Draw top of vacuum
	glDrawSprite(vacuumTopTex, 0, h - 125, w, 155);
	
	//Physics Section
	while( tick > prevPhysicsTick + ticksPerPhysics) {
		//Physics Logic

		//Update a new bounding box as actor moves
		makeAABB(&actor);
		makeAABB(&bag);

		//Actor Bump Detection
		for(std::vector<Actor>::size_type i = 0; i != allSprites.size(); i++) {
			makeAABB(&allSprites.at(i));
			if(boxCollision(actor.box, allSprites.at(i).box)){
				//Collision With Dust Bunny
				if(allSprites.at(i).box.w < bag.width){
					if(collisionRes(&actor,allSprites.at(i).box, &bag)){ 
						FMOD_System_PlaySound(fmod, FMOD_CHANNEL_FREE, collect, false, NULL);
						allSprites.erase(allSprites.begin()+i);
						break;
					}
				}else if(allSprites.at(i).box.w > bag.width){
					queueEvent(&makeEvent(ET_COLLISION_BIG,NULL,&actor,&bag));
					allSprites.erase(allSprites.begin()+i);
					break;
				}
			}
			//Collision With Bag
			if(boxCollision(bag.box, allSprites.at(i).box)){
				if(allSprites.at(i).box.w > bag.width){
					queueEvent(&makeEvent(ET_COLLISION_BIG,NULL,&actor,&bag));
					allSprites.erase(allSprites.begin()+i);
					break;
				}
			}
		}

		//Movement for Actor
		if(!stopMovement){
			if(kbState[SDL_SCANCODE_D] || kbState[SDL_SCANCODE_RIGHT]){	
				if(actor.x < w - actor.width - (26/3)){	
					actor.x = actor.x + moveSpeed;
					bag.x = bag.x + moveSpeed;
				}
			}
			if(kbState[SDL_SCANCODE_A] || kbState[SDL_SCANCODE_LEFT]){
				if(actor.x > 0 ){
					actor.x = actor.x - moveSpeed;
					bag.x = bag.x - moveSpeed;
				}
			}
			if(kbState[SDL_SCANCODE_W] || kbState[SDL_SCANCODE_UP]){	
				if(actor.y >0){	
					actor.y = actor.y - moveSpeed;
					bag.y = bag.y - moveSpeed;
				}
			}
			if(kbState[SDL_SCANCODE_S] || kbState[SDL_SCANCODE_DOWN]){
				if(actor.y < h - 100 - (actor.height + bag.height) ){
					actor.y = actor.y + moveSpeed;
					bag.y = bag.y + moveSpeed;
				}
			}
		}

		prevPhysicsTick += ticksPerPhysics;
	}


	//Delete Before Reaches Bottom of Screen
	for(int i = 0; i < allSprites.size(); i++){
		if(allSprites.at(i).y > WINDOWHEIGHT){
			allSprites.erase(allSprites.begin()+i);
			break;
		}
	}

	if(!timerLock){
		//Draw The Timer
		char buf[5];
		sprintf_s(buf, "%d", time);
		glDrawString(&font, buf, WINDOWWIDTH - 70, 40); 
	}
	//Draw Health
	char health[11] = "";
	for(int i = 0; i < healthLeft; i++){
		strcat_s(health,"*");
	}
	glDrawString(&font, health, 5, 40);
	if(!levelTransition && !gameOver){
		//Draw Current Size
		glDrawSprite(sizeTex,5,80,75,40);
		char buf2[7];
		sprintf_s(buf2, "%d", bag.width);
		glDrawString(&font, buf2, 80, 85); 
		//Draw Target Size
		glDrawSprite(targetTex,5,120,75,40);
		char buf3[7];
		sprintf_s(buf3, "%d", DEFAULTBAGWIDTH + (((2+(level/2)) * 10)));
		glDrawString(&font, buf3, 80, 125); 
	}

	//Easy/Hard Difficulty
	if(kbState[SDL_SCANCODE_P]){		
		queueEvent (&makeEvent(ET_MODE_EASY,NULL,NULL,NULL));
	}
	if(kbState[SDL_SCANCODE_O]){		
		queueEvent (&makeEvent(ET_MODE_HARD,NULL,NULL,NULL));
	}

	
	//Title Screen
	if(titleScreen){
		glDrawSprite(titleTex, 0, 0, WINDOWWIDTH, WINDOWHEIGHT);

		if(kbState[SDL_SCANCODE_SPACE]){		
			queueEvent (&makeEvent(ET_START,NULL,&actor,&bag));
		}
	}
	
	//Next Round Screen
	if(levelTransition){
		glDrawSprite(nextLevelTex, 0, 120, WINDOWWIDTH, 300);
		moveActor(&bag);
		if(bag.y > WINDOWHEIGHT){
			glDrawSprite(brokeVacTopTex, 0, h - 125, w, 155);		
			while( tick > prevExplosionTick + ticksPerExplosion) {
				if(explosions.size() > 20){
					explosions.erase(explosions.begin());	
				}
				explosions.push_back(makeActor(rand()%WINDOWWIDTH+1,rand()%210 + (h - 200),70,70,explosionData,NULL));
				prevExplosionTick += ticksPerExplosion;
			}
			for(int i = 0; i < explosions.size(); i++){
					glDrawSprite((explosions.at(i).anim.def->frames.at(explosionData.curFrame).texture),explosions.at(i).x,explosions.at(i).y,explosions.at(i).width,explosions.at(i).height);
			}
		}
		if(kbState[SDL_SCANCODE_SPACE]){	
			timerLock = true;
			queueEvent( &makeEvent(ET_NEXT_LEVEL,NULL,&actor,&bag));
			explosions.clear();
		}		
	}
	
	//Lose Screen
	if(gameOver){
		glDrawSprite(loseTex, 0, 0, WINDOWWIDTH, WINDOWHEIGHT);
		glDrawSprite(vacDestBackTex, WINDOWWIDTH - 130, WINDOWHEIGHT-300, 150, 130);	
		glDrawSprite(vacDestTex, WINDOWWIDTH - 120, WINDOWHEIGHT-275, 120, 40);		
		char buf4[7];
		sprintf_s(buf4, "%d", level-1);
		glDrawString(&font, buf4, WINDOWWIDTH - 75, WINDOWHEIGHT-225); 

		if(kbState[SDL_SCANCODE_R]){	
			timerLock = true;
			queueEvent (&makeEvent(ET_RESET_GAMESTATE,NULL,&actor,&bag));
		}
	}

	updateEventQueue();

	SDL_GL_SwapWindow( window );
	}
	SDL_Quit();
	return 0;
}

Actor makeActor( float x, float y, int width, int height, AnimData anim, float speed){
	Actor sprite;
	sprite.x = x;
	sprite.y = y;
	sprite.width = width;
	sprite.height = height;
	sprite.anim = anim;
	sprite.moveSpeed = speed;

	return sprite;
}//makeSprite

void moveActor( Actor* s){
	s->y += s->moveSpeed;
}//moveSprite

void makeAABB( Actor* actor){
	actor->box.x = actor->x;
	actor->box.y = actor->y;
	actor->box.w = actor->width; 
	actor->box.h = actor->height;
}//makeAABB

bool boxCollision(ColBox a, ColBox b){
	return !(b.x > a.x + a.w 
        || b.x + b.w < a.x 
        || b.y > a.y + a.h 
		|| b.y + b.h < a.y); 
}//boxCollision

bool collisionRes(Actor* a, ColBox b, Actor* bag){	
	if(b.y + b.h <= a->box.y && b.y < a->box.y + a->box.h ||
		b.x <= a->box.x + a->box.w && b.x > a->box.x ||
		b.x + b.w >= a->box.x && b.x < a->box.x + a->box.w){
		queueEvent(&makeEvent(ET_COLLISION_EAT,NULL,a,bag));
		return true;
	}else{
		return false;
	}
}//collisionRes

void initEventQueue(void){
	eventQueue.reserve(16);
}//initEventQueue

Event makeEvent(EventType type, Actor* object, Actor* subject, Actor* subject2){
	Event ev;
	ev.type = type;
	ev.object = object;
	ev.subject = subject;
	ev.subject2 = subject2;
	return ev;
}//makeEvent

void queueEvent(Event* ev){
	eventQueue.push_back(*ev);
}//queueEvent

void updateEventQueue(void){
	int i;
	for(i = 0; i < eventQueue.size(); i++){
		handleEvent(&eventQueue.at(i));
	}
	eventQueue.clear();
}//updateEventQueue

void handleEvent(Event* ev){
	if( ev->type == ET_COLLISION_BIG){
		healthLeft--;
	}
	if( ev->type == ET_COLLISION_EAT){
		ev->subject2->width += 2;
		ev->subject2->height += 2;
		ev->subject2->x = (ev->subject->x + (ev->subject->width/2)) - (ev->subject2->width/2);
	}
	if( ev->type == ET_MODE_EASY){
		easyMode = true;
	}
	if( ev->type == ET_MODE_HARD){
		easyMode = false;
	}
	if( ev->type == ET_TIMER_COUNTDOWN){
		if( time >= 0){
			if(!timerLock){
				time--;
			}
		}
	}
	if( ev->type == ET_LOSE){
		gameOver = true;
		stopMovement = true;
		time = 0;
		timerLock = true;
	}
	if( ev->type == ET_TIME_OVER){
		if(ev->object->width < (DEFAULTBAGWIDTH + (((2+(level/2)) * 10)))){
			queueEvent( &makeEvent(ET_LOSE,NULL,NULL,NULL));
		}else{
			levelTransition = true;
			stopMovement = true;
			time = 0;
			timerLock = true;
		}
	}
	if( ev->type == ET_START){
		titleScreen = false;
		stopMovement = false;
		timerLock = false;
		queueEvent( &makeEvent(ET_NEXT_LEVEL,NULL,ev->subject,ev->subject2));		
	}
	if( ev->type == ET_NEXT_LEVEL){		
		//Increment Level
		ev->subject->x = WINDOWWIDTH/2;
		ev->subject->y = WINDOWWIDTH/2;
		ev->subject2->x = (ev->subject->x + (ev->subject->width/2)) - (ev->subject2->width/2);
		ev->subject2->y = (ev->subject->y + ev->subject->height);
		ev->subject2->width = DEFAULTBAGWIDTH;
		ev->subject2->height = DEFAULTBAGHEIGHT;
		ev->subject2->x = (ev->subject->x + (ev->subject->width/2)) - (ev->subject2->width/2);
		allSprites.clear();
		healthLeft = healthLeft + 1;
		level++;
		time = 15 + ((level - 1) * 5);	
		chooseBG();
		levelTransition = false;
		stopMovement = false;
		timerLock = false;
	}
	if( ev->type == ET_RESET_GAMESTATE){
		//Reset Gamesate
		healthLeft = 2;
		level = 0;
		time = 1000;
		allSprites.clear();
		titleScreen = true;
		gameOver = false;
		levelTransition = false;
		timerLock = true;
		stopMovement = true;
		curBG = -1;
		prevBG = -1;
	}
}//handleEvent

void animTick( AnimData* data, float dt){
	if(!data->isPlaying){
		return;
	}
	
	int numFrames = data->def->frames.size();
	data->timeToNextFrame -= dt;
	if( data->timeToNextFrame < 0){
		++(data->curFrame);
		if( data->curFrame >= numFrames) {
			//LoopAnim
			data->curFrame = 0;
			data->timeToNextFrame = 1000/30;
		}else{
			AnimFrameDef* curFrame = &data->def->frames[data->curFrame];			
			data->timeToNextFrame += curFrame ->frameTime;
		}
	}
}//animTick

GLFontChar makeFontChar( GLuint tex, int w, int h){
	GLFontChar fontChar;
	fontChar.tex = tex;
	fontChar.w = w;
	fontChar.h = h;
	return fontChar;
}//makeFont

void glDrawString( GLFont* font, const char* str, int x, int y){
	while( *str != '\0'){
		char curChar = *str;
		GLFontChar* fontChar = &font->chars[curChar];
		glDrawSprite(fontChar->tex, x, y, fontChar->w, fontChar->h);
		x += fontChar ->w;
		++str;
	}
}//drawString

void chooseBG(void){
	prevBG = curBG;
	curBG = rand()%BGNUM;
	if(curBG == prevBG){
		chooseBG();
	}
}