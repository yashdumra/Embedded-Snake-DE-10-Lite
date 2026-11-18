
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "address_map_niosv.h"

typedef uint16_t pixel_t;

volatile pixel_t *pVGA = (pixel_t *)FPGA_PIXEL_BUF_BASE;

//buttons
volatile int *pKEY = (int *)KEY_BASE;
//hex
volatile int *pHEX = (int *)HEX3_HEX0_BASE;

int sbodyX[MAX_X * MAX_Y];
int sbodyY[MAX_X * MAX_Y];

const pixel_t blk = 0x0000;
const pixel_t wht = 0xffff;
const pixel_t red = 0xf800;
const pixel_t grn = 0x07e0;
const pixel_t blu = 0x001f;

void delay(int N)
{
    for (int i = 0; i < N; i++)
        *pVGA;
}

void drawPixel(int y, int x, pixel_t colour)
{
    *(pVGA + (y << YSHIFT) + x) = colour;
}

pixel_t makePixel(uint8_t r8, uint8_t g8, uint8_t b8)
{
    const uint16_t r5 = (r8 & 0xf8) >> 3;
    const uint16_t g6 = (g8 & 0xfc) >> 2;
    const uint16_t b5 = (b8 & 0xf8) >> 3;
    return (pixel_t)((r5 << 11) | (g6 << 5) | b5);
}

void rect(int y1, int y2, int x1, int x2, pixel_t c)
{
    for (int y = y1; y < y2; y++)
        for (int x = x1; x < x2; x++)
            drawPixel(y, x, c);
}

void displayScore(int score, int highscore) {

  	  int numbers[10] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66,
        0x6D, 0x7D, 0x07, 0x7F, 0x6F
    	};

	int current = numbers[score];
	int highest = numbers[highscore];

	highest = highest << 8;
	*pHEX = current | highest;

}

int isTaken(int x, int y, int length) {

    if ((x >= 30 && x < 33 && y >= 30 && y < 90) ||
        (x >= 127 && x < 130 && y >= 30 && y < 90))
    {
        return 1;
    }

    for(int i = 0; i < length; i++) {
	if(x == sbodyX[i] && y == sbodyY[i]) {
		return 1;
	}
	
     }
	return 0;

}

int main()
{
int highscore = 0;
while(1){

    int snakeX = MAX_X / 2;
    int snakeY = MAX_Y / 2;

    sbodyX[0] = snakeX;
    sbodyY[0] = snakeY;

    int length = 1;
    int direction = 0;

    int score = 0;
    displayScore(score, highscore);

    int growthRemaining = 0;
    int slowCounter = 0;

    int foodX;
    int foodY;
    int validFood;

    rect(0, MAX_Y, 0, MAX_X, wht);
    rect(1, MAX_Y - 1, 1, MAX_X - 1, blk);
    rect(30, 90, 30, 33, wht);
    rect(30, 90, 127, 130, wht);

    drawPixel(snakeY, snakeX, grn);

    do
    {
        foodX = 1 + rand() % (MAX_X - 2);
        foodY = 1 + rand() % (MAX_Y - 2);
    } while (isTaken(foodX, foodY, length));

    drawPixel(foodY, foodX, red);

    int previousKeys = *pKEY;

    int speed = 1000000;

    while (1)
    {
        delay(speed);

        int keys = *pKEY;

        if ((previousKeys & 1) != 0 && (keys & 1) == 0)
        {
            direction = (direction + 3) % 4;
        }
        else if ((previousKeys & 2) != 0 && (keys & 2) == 0)
        {
            direction = (direction + 1) % 4;
        }

        previousKeys = keys;

        if (direction == 0)
        {
            snakeX++;
        }
        else if (direction == 1)
        {
            snakeY++;
        }
        else if (direction == 2)
        {
            snakeX--;
        }
        else if (direction == 3)
        {
            snakeY--;
        }
	if (snakeX <= 0 || snakeX >= MAX_X - 1 || snakeY <= 0 || snakeY >= MAX_Y - 1)
	{
    	break;
	}

	if ((snakeX >= 30 && snakeX < 33 && snakeY >= 30 && snakeY < 90) || (snakeX >= 127 && snakeX < 130 && snakeY >= 30 && snakeY < 90))
        {
            break;
        }

        int grow = 0;

        if (length < 10)
        {
            grow = 1;
        }
        else if (growthRemaining > 0)
        {
            grow = 1;
            growthRemaining--;
        }
        else
        {
            slowCounter++;

            if (slowCounter >= 2)
            {
                grow = 1;
                slowCounter = 0;
            }
        }

        if (length >= MAX_X * MAX_Y)
        {
            grow = 0;
        }

        int collision = 0;
        int first = grow ? 0 : 1;

        for (int i = first; i < length; i++)
        {
            if (snakeX == sbodyX[i] && snakeY == sbodyY[i])
            {
                collision = 1;
                break;
            }
        }

        if (collision == 1)
        {
            break;
        }

        int ateFood = (snakeX == foodX && snakeY == foodY);

        if (grow)
        {
            length++;
        }
        else
        {
            drawPixel(sbodyY[0], sbodyX[0], blk);

            for (int i = 0; i < length - 1; i++)
            {
                sbodyX[i] = sbodyX[i + 1];
                sbodyY[i] = sbodyY[i + 1];
            }
        }

        sbodyX[length - 1] = snakeX;
        sbodyY[length - 1] = snakeY;

        drawPixel(snakeY, snakeX, grn);

        if (ateFood)
        {
            score++;
	   
            if(score > highscore) {
		
		highscore = score;
	    }
		
	    if (score == 9){

 	   break;
	}
	    
	    displayScore(score,highscore);
            printf("Food eaten! Score: %d\n", score);

            growthRemaining += 10 * score;
	    speed = speed * 0.9;
            slowCounter = 0;

do
{
    foodX = 1 + rand() % (MAX_X - 2);
    foodY = 1 + rand() % (MAX_Y - 2);
} while (isTaken(foodX, foodY, length));

            drawPixel(foodY, foodX, red);
        }
    }
	if (score == 9){

    rect(0, MAX_Y, 0, MAX_X, grn);
    delay(3000000);
    rect(0, MAX_Y, 0, MAX_X, blk);
    delay(3000000);
    rect(0, MAX_Y, 0, MAX_X, grn);
    delay(3000000);
    rect(0, MAX_Y, 0, MAX_X, blk);
    delay(3000000);
    rect(0, MAX_Y, 0, MAX_X, grn);
    delay(6000000);
    printf("YOU WIN!\n");

	}

	else {
    rect(0, MAX_Y, 0, MAX_X, red);
    delay(6000000);
    printf("Game over! Score: %d\n", score);
}
}
}