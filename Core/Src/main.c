//********************************************************************
//*                    EEE2046F C template                           *
//*==================================================================*
//* WRITTEN BY: Jesse Arendse   	                 		               *
//* DATE CREATED: 07/04/2023                                         *
//* MODIFIED:                                                        *
//*==================================================================*
//* PROGRAMMED IN: Visual Studio Code                                *
//* TARGET:        STM32F0                                           *
//*==================================================================*
//* DESCRIPTION:                                                     *
//*                                                                  *
//********************************************************************
// INCLUDE FILES
//====================================================================
#include "stm32f0xx.h"
#include <lcd_stm32f0.c>
#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <time.h>


//====================================================================
// GLOBAL CONSTANTS
//====================================================================

char line_one[] = "score: 000      "; 
char line_two[] = "               o";
int notGameOver = 1;

//====================================================================
// GLOBAL VARIABLES
//====================================================================

//====================================================================
// FUNCTION DECLARATIONS
//====================================================================
void setScore(int score);
void up();
void down();
void update_game();
int up_bad();
int down_bad();
void new_state(uint16_t score);
void init_game();
void gameOver();
void init_interrupts();
void EXTI0_1_IRQHandler(void);
void EXTI2_3_IRQHandler(void) ;

//====================================================================
// MAIN FUNCTION
//====================================================================

#define SetClock RCC -> AHBENR |= RCC_AHBENR_GPIOAEN;
#define SW2_Not_Pressed  ( ( GPIOA -> IDR & GPIO_IDR_2) != 0)
#define SW1_Pressed  ( ( GPIOA -> IDR & GPIO_IDR_1) == 0)
#define SW3_Pressed  ( ( GPIOA -> IDR & GPIO_IDR_3) == 0)

int main (void)
{

    SetClock;

    GPIOA -> PUPDR |= GPIO_PUPDR_PUPDR2_0; //Pull up sw2
    GPIOA -> PUPDR |= GPIO_PUPDR_PUPDR1_0; //Pull up sw2
    GPIOA -> PUPDR |= GPIO_PUPDR_PUPDR3_0; //Pull up sw2

    init_LCD();
    uint16_t score = 0;

    init_game();
    update_game();
    init_interrupts();
    int maximum_speed = 100000;

    while (1){
        score++;
        delay(maximum_speed);
        new_state(score);
        setScore(score);
        if (score < 10){
            maximum_speed -=  ((float) score/100) *10000;
        }
        else if( score < 30 ){
            maximum_speed -=  ((float) score/100) *5000;

        }
        else{
            maximum_speed -=  ((float) score/100) *1000;
        }

        if (maximum_speed < 19000){ //Minimum speed
            maximum_speed = 19000;
        }
    }

}							// End of main

//====================================================================
// FUNCTION DEFINITIONS
//====================================================================
void init_interrupts(){
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN; // Enable SYSCFG clock
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI1_PA; // Map EXTI1 to PA1 (SW1)
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI3_PA; // Map EXTI3 to PA3 (SW3)

    EXTI->IMR |= EXTI_IMR_MR1 | EXTI_IMR_MR3; // Unmask EXTI1 and EXTI3
    EXTI->RTSR |= EXTI_RTSR_TR1 | EXTI_RTSR_TR3; // Trigger on rising edge

    NVIC_EnableIRQ(EXTI0_1_IRQn); // Enable EXTI0_1 interrupt
    NVIC_EnableIRQ(EXTI2_3_IRQn); // Enable EXTI2_3 interrupt
}

void setScore(int score){
    char score_str[4];
    sprintf(score_str, "%03d", score);
    line_one[7] = score_str[0];
    line_one[8] = score_str[1];
    line_one[9] = score_str[2];
    update_game();
}

void up(){
    if (up_bad()){gameOver();};
    line_one[15] ='o';
    line_two[15] = ' ';
    update_game();
}

void down(){
    if (down_bad()){gameOver();};
    line_one[15]=' ';
    line_two[15] = 'o';
    update_game();
}


void update_game(){
    lcd_command(CLEAR);
    lcd_putstring(line_one);
    lcd_command(LINE_TWO);
    lcd_putstring(line_two);

}

int up_bad(){

    if ( (line_one[15] != 'o') && (line_one[15] != ' ')){
        return 1;
    }
    return 0;
}


void gameOver(){
    notGameOver = 0;
    lcd_command(CLEAR);
    lcd_putstring("GAME OVER (^_^)");
    lcd_command(LINE_TWO);
    lcd_putstring("Replay: sw4");

    while(1);

}

int down_bad(){

    if ( (line_two[15] != 'o') && (line_two[15] != ' ')){
        return 1;
    }
    return 0;
}

void new_state(uint16_t score){


    //check if  not losing
    if ( line_one[14] == '~' && line_one[15] == 'o'){
        gameOver();
    }

    if ( line_two[14] == '~' && line_two[15] == 'o'){
        gameOver();
    }

    u_int8_t wasUp = 0;
    if (line_one[15] == 'o'){ wasUp = 1;}

    uint8_t odds = ((float)(score/200))*10;
    if (odds == 0){ odds = 1;}
    if (odds > 7){ odds =7;}  

    
    for (uint8_t i = 15 ; i > 0; i = i -1 ){ 

        if (i > 11 ){
            line_one[i] = line_one[i-1];
        }

        line_two[i] = line_two[i-1];
    }

    line_two[0] =' ';
    line_one[11] =' ';

    if (wasUp){ line_one[15] = 'o';}
    else{ line_two[15] = 'o';}

    uint8_t shouldgen = (rand() % 10) + 1;

    if (shouldgen > odds ){

        uint8_t line = rand() %4 ;
        if ( line == 0){  
            if ( line_two[11] == ' ' && line_two[10] == ' ' && line_two[12] == ' '){//Avoid two arrows at once
                line_one[11] = '~';
            }
        
        }
        else {
            if (  (line_two[1] = ' ') ){
            line_two[0] = '~'; 
            }
        }

    }

    update_game();
}


void EXTI0_1_IRQHandler(void) {
    if (notGameOver){
    if (EXTI->PR & EXTI_PR_PR1) { // Check if SW1 triggered the interrupt
        EXTI->PR |= EXTI_PR_PR1;  // Clear the interrupt flag
        up();                     // Call the `up` function
    }}
}

void EXTI2_3_IRQHandler(void) {
    if (notGameOver){
    if (EXTI->PR & EXTI_PR_PR3) { // Check if SW3 triggered the interrupt
        EXTI->PR |= EXTI_PR_PR3;  // Clear the interrupt flag
        down();                   // Call the `down` function
    }
   }
}

void init_game(){
    lcd_command(CLEAR);
    lcd_putstring("Welcome to");
    lcd_command(LINE_TWO);
    lcd_putstring("vika ingozi !!!");
    delay(300000);

    lcd_command(CLEAR);
    lcd_putstring("Evade arrows");
    lcd_command(LINE_TWO);
    lcd_putstring("and survive!!!");
    delay(300000);

    lcd_command(CLEAR);
    lcd_putstring("Up : sw1");
    lcd_command(LINE_TWO);
    lcd_putstring("Down : sw3");
    delay(300000);

    lcd_command(CLEAR);
    lcd_putstring("Ready?");
    lcd_command(LINE_TWO);
    lcd_putstring("start: sw2");
    while (SW2_Not_Pressed);

    lcd_command(CLEAR);
    lcd_putstring("Starting in");
    lcd_command(LINE_TWO);
    lcd_putstring(" 5");
    delay(150000);

    lcd_command(CLEAR);
    lcd_putstring("Starting in");
    lcd_command(LINE_TWO);
    lcd_putstring(" 4");
    delay(150000);

    lcd_command(CLEAR);
    lcd_putstring("Starting in");
    lcd_command(LINE_TWO);
    lcd_putstring(" 3");
    delay(150000);

    lcd_command(CLEAR);
    lcd_putstring("Starting in");
    lcd_command(LINE_TWO);
    lcd_putstring(" 2");
    delay(150000);

    lcd_command(CLEAR);
    lcd_putstring("Starting in");
    lcd_command(LINE_TWO);
    lcd_putstring(" 1");
    delay(150000);
}

//********************************************************************
// END OF PROGRAM
//********************************************************************
