#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <util/delay.h>
#include "ruota.h"
#include "rios.h"
#include "image.h"

#define LED_INIT    DDRB  |=  _BV(PINB7)
#define LED_ON      PORTB |=  _BV(PINB7)
#define LED_OFF     PORTB &= ~_BV(PINB7)
#define LED_TOGGLE  PINB  |=  _BV(PINB7)

#define FRAME_TIME 25

#define TITLE_FRAME 0
#define RESTART_FRAME 1
#define WIN_FRAME 2
#define TOWER_FRAME 3
#define RUN_FRAME 4
#define CHARACTER_FRAME 5
#define DEMO_FRAME 6
int gameState;

#define HERO_WIDTH BIG_WIDTH
#define HERO_HEIGHT BIG_HEIGHT
image hero;

typedef enum { Linh,Luke} character;

character currentCharacter=Linh;

void showCharacter(image i, character c, int frame){
    switch(c){
        case Linh:
            if(frame==0){
                draw_image_indexed(i,LinhRight);
            } else {
                draw_image_indexed(i,LinhLeft);
            }
            break;
        case Luke:
            if(frame==0){
                draw_image_indexed(i,LukeRight);
            } else {
                draw_image_indexed(i,LukeLeft);
            }
            break;
    }
}

uint16_t random_seed;

/* functions rand_init and rand are taken from Giacomo Meanti's
   Space Invaders game from 2014-2015
*/

uint16_t rand_init(void) {
    //ADC conversion from unused pins should give random results.
    //an amplification factor of 200x is used.
    ADMUX |= _BV(REFS0) | _BV(MUX3) | _BV(MUX1) | _BV(MUX0);
    //Prescaler
    ADCSRA |= _BV(ADPS2) | _BV(ADPS1);
    //Enable and start
    ADCSRA |= _BV(ADEN) | _BV(ADSC);
    //Wait until complete
    while(! (ADCSRA & _BV(ADIF))) {
        _delay_ms(2);
    }
    //Read result
    uint16_t res = ADCL;
    res |= ADCH << 8;
    //Disable
    ADCSRA &= ~_BV(ADEN);
    
    //The XOR should increase the randomness?
    //since the converted number is only 10 bits
    return res ^ 0xACE1u;
}

//http://en.wikipedia.org/wiki/Linear_feedback_shift_register
uint16_t rand(void) {
    unsigned lsb = random_seed & 1;  /* Get lsb (i.e., the output bit). */
    random_seed >>= 1;               /* Shift register */
    if (lsb == 1)             /* Only apply toggle mask if output bit is 1. */
        random_seed ^= 0xB400u;        /* Apply toggle mask, value has 1 at bits corresponding
                             * to taps, 0 elsewhere. */
    return random_seed;
}

uint8_t get_switch_state_mine(uint8_t mask);


// ------------------------------------------------------------------------------------------------
// ---------------------------------------- Frames ------------------------------------------------
// ------------------------------------------------------------------------------------------------

void titleFrame();
void restartFrame();
void winFrame();
void towerFrame();
void runFrame();
void characterFrame();
void demoFrame();

void setupTowerFrame();
void setupRunFrame();

#define TIME_TO_DEMO 15000/FRAME_TIME
volatile counter=0;

int firstTitle=1;
void titleFrame(){
    if(!(get_switch_state(_BV(SWN)) || get_switch_state(_BV(SWE)) || get_switch_state(_BV(SWW)) || get_switch_state(_BV(SWS))) && firstTitle){
        clear_screen();
        display_string("Left - Tower\nRight - Run\nDown - Change your character");
        firstTitle=0;
        counter=0;
    } else if(!firstTitle){
        if (counter>=TIME_TO_DEMO) {
                setupTowerFrame();
			    gameState=DEMO_FRAME;
			    firstTitle=1;
	    } else if (get_switch_state_mine(_BV(SWW))) {
                setupTowerFrame();
			    gameState=TOWER_FRAME;
			    firstTitle=1;
	    } else if (get_switch_state_mine(_BV(SWE))) {
                setupRunFrame();
			    gameState=RUN_FRAME;
			    firstTitle=1;
	    } else if (get_switch_state_mine(_BV(SWS))) {
			    gameState=CHARACTER_FRAME;
			    firstTitle=1;
	    }
	}
}

int firstRestart=1;
void restartFrame(){
    if(firstRestart){
        clear_screen();
        display_string("You Lost. Press Up to return to menu");
        firstRestart=0;
    }
    if (get_switch_state_mine(_BV(SWN))) {
			gameState=TITLE_FRAME;
			firstRestart=1;
	}
}

int firstWin=1;
void winFrame(){
    if(firstWin){
        clear_screen();
        display_string("You won. Press Up to return to menu");
        firstWin=0;
    }
    if (get_switch_state_mine(_BV(SWN))) {
			gameState=TITLE_FRAME;
			firstWin=1;
	}
}

int firstCharacter=1;
int chFrame;
image LinhImage,LukeImage,NickImage;

void characterFrame(){
    if(firstCharacter){
        clear_screen();
        display_string("Choose a character by pressing one of the keys...");
        firstCharacter=0;
        chFrame=0;
        
        double midHeight=HEIGHT/2;
        double midWidth=WIDTH/2;
        LinhImage.left=midWidth-50-BIG_WIDTH;
        LinhImage.top=midHeight+BIG_HEIGHT/2;
        LinhImage.height=BIG_HEIGHT;
        LinhImage.width=BIG_WIDTH;
        LinhImage.colour=WHITE;
        
        LukeImage.left=midWidth+50;
        LukeImage.top=midHeight+BIG_HEIGHT/2;
        LukeImage.height=BIG_HEIGHT;
        LukeImage.width=BIG_WIDTH;
        LukeImage.colour=WHITE;
        
        showCharacter(LinhImage,Linh,0);
        showCharacter(LukeImage,Luke,0);
    } else {
        showCharacter(LinhImage,Linh,(chFrame/5)%2);
        showCharacter(LukeImage,Luke,(chFrame/5)%2);
    }
    chFrame++;
    if (get_switch_state_mine(_BV(SWW))) {
            currentCharacter=Linh;
			gameState=TITLE_FRAME;
			firstCharacter=1;
	} else if (get_switch_state_mine(_BV(SWE))) {
	        currentCharacter=Luke;
			gameState=TITLE_FRAME;
			firstCharacter=1;
	}
}

void demoFrame(){
    if(get_switch_state(_BV(SWN)) || get_switch_state(_BV(SWE)) || get_switch_state(_BV(SWW)) || get_switch_state(_BV(SWS))){
        gameState=TITLE_FRAME;
        return;
    }
    towerFrame();
    if(gameState!=DEMO_FRAME){
        setupTowerFrame();
		gameState=DEMO_FRAME;
    }
    display_string_xy("Press to play", WIDTH/3, HEIGHT/2);
}


// ------------------------------------------------------------------------------------------------
// ---------------------------------------- Main --------------------------------------------------
// ------------------------------------------------------------------------------------------------

int displayFrame(int state);

volatile flag=0;

int displayFrame(int state){
    counter++;
    flag=1;
    return state;
}

int main(void) {
    /* 8MHz clock, no prescaling (DS, p. 48) */
    CLKPR = (1 << CLKPCE);
    CLKPR = 0;
      
    init_lcd();
    os_init_scheduler();
    os_init_ruota();
   random_seed = rand_init();
   gameState=TITLE_FRAME;
   os_add_task(displayFrame,            25, 1);
   sei();

   while(1){
            if(flag){
                cli();
                flag=0;
                sei();
                switch(gameState){
                    case TITLE_FRAME:
                        titleFrame();
                        break;
                    case RESTART_FRAME:
                        restartFrame();
                        break;
                    case TOWER_FRAME:
                        towerFrame();
                        break;
                    case WIN_FRAME:
                        winFrame();
                        break;
                    case RUN_FRAME:
                        runFrame();
                        break;
                    case CHARACTER_FRAME:
                        characterFrame();
                        break;
                    case DEMO_FRAME:
                        demoFrame();
                        break;
                }
            }
   };

}

// ------------------------------------------------------------------------------------------------
// ---------------------------------------- TOWER -------------------------------------------------
// ------------------------------------------------------------------------------------------------

#define TOWER_PLATFORM_HEIGHT 10
#define TOWER_PLATFORM_DISTANCE 100

#define TOWER_WIN_COLOUR_PLATFORM GOLD
#define TOWER_MIDHEIGHT HEIGHT/2
#define TOWER_PLATFORMS 5
#define TOWER_LEVELS 10
#define TOWER_PLATFORMS_PER_LEVEL 10


#define TOWER_maxVX 15
#define TOWER_maxVY 15
#define TOWER_SPEED_INITIAL 0
#define TOWER_SPEED_INCREMENT 0.4

int currentPlatform;
image platforms[TOWER_PLATFORMS];

int vx,vy,atPlatform,highestPlatform,created;


double speed;
int tower_frame;
const uint16_t platformColours[TOWER_LEVELS+1]={INDIGO, DARK_VIOLET,BLUE,LIGHT_BLUE,LIGHT_GREEN, GREEN,YELLOW,ORANGE,RED,DARK_RED,TOWER_WIN_COLOUR_PLATFORM};

void setupTowerFrame();
void createRandomPlatform(int i);
void draw_platforms();
void init_platforms();
void towerFrame();

void setupTowerFrame(){
    currentPlatform=-1;
    highestPlatform=HEIGHT+TOWER_PLATFORM_DISTANCE;
    created=0;
    vx=0;
    vy=0;
    atPlatform=0;
    clear_screen();
    init_platforms();
   draw_platforms();
   speed=0;
   tower_frame=0;
   hero.left=(WIDTH-HERO_WIDTH)/2;
   hero.top=HEIGHT-HERO_HEIGHT-50;
   hero.width=HERO_WIDTH;
   hero.height=HERO_HEIGHT;
   hero.colour=WHITE;
   showCharacter(hero,currentCharacter,0);
}

void createRandomPlatform(int i){
    if(created<=TOWER_LEVELS*TOWER_PLATFORMS_PER_LEVEL){
        platforms[i].height=TOWER_PLATFORM_HEIGHT;
        platforms[i].top=highestPlatform-TOWER_PLATFORM_HEIGHT-TOWER_PLATFORM_DISTANCE;
        if(created%TOWER_PLATFORMS_PER_LEVEL==0){
            platforms[i].left=0;
            platforms[i].width=WIDTH;
            speed+=TOWER_SPEED_INCREMENT;
        } else {
            platforms[i].width=75+rand()%50;
            platforms[i].left=rand()%(WIDTH-((uint16_t)platforms[i].width));
        }
        platforms[i].colour=platformColours[created/TOWER_PLATFORMS_PER_LEVEL];
        highestPlatform=platforms[i].top;
        created++;
    }
}

void draw_platforms(){
    int i=0;
    for(;i<TOWER_PLATFORMS;i++){
        fill_image(platforms[i],platforms[i].colour);
    }
}

void init_platforms(){
    int i=0;
    for(;i<TOWER_PLATFORMS;i++){
        createRandomPlatform(i);
    }
}

uint8_t get_switch_state_mine(uint8_t mask){
    if(gameState==DEMO_FRAME){
        if(atPlatform && mask == _BV(SWN)){
            return 1;
        }
        if(mask==_BV(SWE) || mask==_BV(SWW)){
            int x=platforms[(currentPlatform+1)%TOWER_PLATFORMS].left+platforms[(currentPlatform+1)%TOWER_PLATFORMS].width/2;
            int x2=hero.left+hero.width/2;
            if(abs(x-x2)<=vx*(vx-1)){
                if(vx==0){
                    return 0;
                } else if(vx>0 && mask==_BV(SWW)){
                    return 1;
                } else if(vx<0 && mask==_BV(SWE)){
                    return 1;
                } else if(vx>0 && mask==_BV(SWE)){
                    return 0;
                } else if(vx<0 && mask==_BV(SWW)){
                    return 0;
                } 
            }
            if(mask==_BV(SWE)){
                return x>x2;
            } else if(mask==_BV(SWW)){
                return x<x2;
            }
        }
    }
    return get_switch_state(mask);
}

void towerFrame(){
    tower_frame++;
    image temp=hero;
    if (get_switch_state_mine(_BV(SWW)) && vx>-TOWER_maxVX) {
		vx--;
	} else if (get_switch_state_mine(_BV(SWE)) && vx<TOWER_maxVX) {
		vx++;
	} else if(vx<0){
	    vx++;
	} else if(vx>0){
	    vx--;
	}
	if (get_switch_state_mine(_BV(SWN)) && atPlatform) {
		vy=-TOWER_maxVY;
		atPlatform=0;
	} else if(!atPlatform && vy<TOWER_maxVY/2){
	    vy++;
	} 
	if(vx<0 && hero.left!=0){
	    temp.left= hero.left+vx<0 ? 0 : hero.left+vx;
	} else if(vx>0 && hero.left!=WIDTH-HERO_WIDTH){
	    temp.left= hero.left+vx+HERO_WIDTH>=WIDTH? WIDTH-HERO_WIDTH : hero.left+vx;
	}
	double offset=0;
	if(!atPlatform){
        temp.top=hero.top+vy;
        if(temp.top<TOWER_MIDHEIGHT){
            offset=TOWER_MIDHEIGHT-temp.top;
            temp.top=TOWER_MIDHEIGHT;
        }
        if(vy>=0){
            int i=0;
            for(;i<TOWER_PLATFORMS;i++){
                if(temp.left+HERO_WIDTH>=platforms[i].left && platforms[i].left+platforms[i].width>=temp.left
                && hero.top+HERO_HEIGHT<=platforms[i].top && temp.top+HERO_HEIGHT>platforms[i].top){
                    temp.top=(platforms[i].top-HERO_HEIGHT)-1;
                    atPlatform=1;
                    currentPlatform=i;
                    if(platforms[i].colour==TOWER_WIN_COLOUR_PLATFORM){
                        gameState=WIN_FRAME;
                        return;
                    }
                    break;
                }
            }
        }
	}else if(atPlatform){
	    if(temp.left+HERO_WIDTH<platforms[currentPlatform].left || 
	    platforms[currentPlatform].left+platforms[currentPlatform].width<temp.left){
                atPlatform=0;
        }
	}
	//if(change || frame%5==0){
	    fill_image(hero,BLACK);
	        offset+=speed;
	        temp.top+=speed;
	    //}
	    if(offset!=0){
            int i=0;
            highestPlatform+=offset;
	        for(;i<TOWER_PLATFORMS;i++){
	            
	            if(offset>=TOWER_PLATFORM_HEIGHT){
	                fill_image(platforms[i],BLACK);
                    platforms[i].top+=offset;
                    fill_image(platforms[i],platforms[i].colour);
	            } else {
	                image im=platforms[i];
	                im.height=offset;
                    fill_image(im,BLACK);
                    im.top+=platforms[i].height;
                    fill_image(im,im.colour);
                    platforms[i].top+=offset;
                }
                if(platforms[i].top>=HEIGHT+HERO_HEIGHT){
                    createRandomPlatform(i);
                }
                //fill_image(platforms[i],platforms[i].colour);
            }
	    } else if(!atPlatform){
	        draw_platforms();
	    }
	    hero=temp;
        if(hero.top>HEIGHT){
            gameState=RESTART_FRAME;
            return;
        }
        showCharacter(hero,currentCharacter,(tower_frame/5)%2);
	//}
}

// ------------------------------------------------------------------------------------------------
// ----------------------------------------- RUN --------------------------------------------------
// ------------------------------------------------------------------------------------------------

#define RUN_PLATFORMS 5
#define RUN_PLATFORM_HEIGHT 10

#define RUN_maxVX 15
#define RUN_maxVY 15

#define RUN_OBSTACLES 5
#define RUN_OBSTACLES_PER_PLATFORM 10
#define RUN_OBSTACLE_WIDTH 10

#define RUN_WIN_COLOUR_PLATFORM

#define RUN_SPEED_INITIAL 3.6
#define RUN_SPEED_INCREMENT 0.4

//const uint16_t* heroRun[2]={LinhRight, LinhLeft};

int runVx;
int runVy;

int currentRunPlatform;
image runPlatform;

image obstacles[RUN_OBSTACLES];

int runAtPlatform;

int frame;
double runSpeed;
const uint16_t runColours[RUN_PLATFORMS]={INDIGO,BLUE,GREEN,ORANGE,RED};

int passedObstacles;
int createdObstacles;
double lastObstacle;
void createRandomObstacle(int i){
    
        obstacles[i].height=30+rand()%50;
        obstacles[i].top=HEIGHT-RUN_PLATFORM_HEIGHT-obstacles[i].height;
        obstacles[i].left=lastObstacle+RUN_OBSTACLE_WIDTH+120+rand()%100;
        obstacles[i].width=RUN_OBSTACLE_WIDTH;
        obstacles[i].colour=runColours[currentRunPlatform%RUN_PLATFORMS];
        lastObstacle=obstacles[i].left;
        createdObstacles++;
        if(createdObstacles>RUN_PLATFORMS*RUN_OBSTACLES_PER_PLATFORM){
            obstacles[i].left=100000;
        }
}

void draw_obstacles(){
    int i=0;
    for(;i<RUN_OBSTACLES;i++){
        fill_image(obstacles[i],obstacles[i].colour);
    }
}

void init_obstacles(){
    int i=0;
    for(;i<RUN_OBSTACLES;i++){
        createRandomObstacle(i);
    }
}

void setupRunFrame(){
   clear_screen();
   passedObstacles=0;
   createdObstacles=0;
   frame=0;
   currentRunPlatform=0;
   runAtPlatform=1;
   hero.left=0;
   runVx=0;
   runVy=0;
   lastObstacle=0;
   runSpeed=RUN_SPEED_INITIAL;
   hero.left=0;
   hero.top=HEIGHT-HERO_HEIGHT-RUN_PLATFORM_HEIGHT;
   hero.width=HERO_WIDTH;
   hero.height=HERO_HEIGHT;
   hero.colour=WHITE;
   runPlatform.left=0;
   runPlatform.width=WIDTH;
   runPlatform.top=HEIGHT-RUN_PLATFORM_HEIGHT;
   runPlatform.height=RUN_PLATFORM_HEIGHT;
   runPlatform.colour=runColours[0];
   showCharacter(hero,currentCharacter,0);
   fill_image(runPlatform,runPlatform.colour);
   init_obstacles();
   draw_obstacles();
}

void runFrame(){
    frame++;
    image temp=hero;
    double offset=runSpeed;
    lastObstacle-=offset;
    if (get_switch_state_mine(_BV(SWW)) && runVx>-RUN_maxVX) {
		runVx--;
	} else if (get_switch_state_mine(_BV(SWE)) && runVx<RUN_maxVX) {
		runVx++;
	} else if(runVx<0){
	    runVx++;
	} else if(runVx>0){
	    runVx--;
	}
	if (get_switch_state_mine(_BV(SWN)) && runAtPlatform) {
		runVy=-RUN_maxVY;
		runAtPlatform=0;
	} else if(!runAtPlatform && runVy<RUN_maxVY/2){
	    runVy++;
	}
	if(runVx<0 && hero.left!=0){
	    temp.left= hero.left+runVx<0 ? 0 : hero.left+runVx;
	    if(temp.left==0){
	        runVx=0;
	    }
	}else if(runVx>0 && hero.left!=WIDTH-HERO_WIDTH){
	    temp.left= hero.left+runVx+HERO_WIDTH>=WIDTH? WIDTH-HERO_WIDTH : hero.left+runVx;
	    if(temp.left==WIDTH-HERO_WIDTH){
	        runVx=0;
	    }
	}
	if(!runAtPlatform){
        temp.top=hero.top+runVy;
        if(runVy>=0){
                if(hero.top+HERO_HEIGHT<=runPlatform.top && temp.top+HERO_HEIGHT>runPlatform.top){
                    temp.top=(runPlatform.top-HERO_HEIGHT);
                    runAtPlatform=1;
                }
        }
	}
	temp.left=temp.left-offset<0?0:temp.left-offset;
	int i=0;
	for(;i<RUN_OBSTACLES;i++){     
        if(offset<=-RUN_OBSTACLE_WIDTH){
            fill_image(obstacles[i],BLACK);
            obstacles[i].left-=offset;
            fill_image(obstacles[i],obstacles[i].colour);
        } else {
            image im=obstacles[i];
            im.width=offset;
            im.left=obstacles[i].left+obstacles[i].width-offset;
            fill_image(im,BLACK);
            im.left-=obstacles[i].width;
            fill_image(im,im.colour);
            obstacles[i].left-=offset;
        }
        if(((temp.left<=obstacles[i].left && temp.left+temp.width>=obstacles[i].left) ||
            (temp.left<=obstacles[i].left + obstacles[i].width && temp.left+temp.width>=obstacles[i].left + obstacles[i].width)) && 
            ((temp.top<=obstacles[i].top && temp.top+temp.height>=obstacles[i].top) ||
            (temp.top<=obstacles[i].top + obstacles[i].height && temp.top+temp.height>=obstacles[i].top + obstacles[i].height))){
                gameState=RESTART_FRAME;
                return;
            }
        if(obstacles[i].left+obstacles[i].width<=0){
            passedObstacles++;
            if(passedObstacles%RUN_OBSTACLES_PER_PLATFORM==0){
                if(passedObstacles==RUN_PLATFORMS*RUN_OBSTACLES_PER_PLATFORM){
                    gameState=WIN_FRAME;
                    return;
                }
                currentRunPlatform+=1;
                runPlatform.colour=runColours[currentRunPlatform%RUN_PLATFORMS];
                runSpeed+=RUN_SPEED_INCREMENT;
                int j=0;
	            for(;j<RUN_OBSTACLES;j++){
	                obstacles[j].colour=runColours[currentRunPlatform%RUN_PLATFORMS];
	            }
                fill_image(runPlatform,runPlatform.colour);
            }
            createRandomObstacle(i);
        }
    }
    fill_image(hero,BLACK);
    hero=temp;
    showCharacter(hero,currentCharacter,(frame/5)%2);
}


/*
  Copyright 2016 Nikola Bozhinov
  At your option this work is licensed under a Creative Commons
  Attribution-NonCommercial 3.0 Unported License [1], or under a
  Creative Commons Attribution-ShareAlike 3.0 Unported License [2].
  [1]: See: http://creativecommons.org/licenses/by-nc/3.0/
  [2]: See: http://creativecommons.org/licenses/by-sa/3.0/
  =================================================================
*/
