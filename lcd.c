/* 
	Sample task that initialises the EA QVGA LCD display
	with touch screen controller and processes touch screen
	interrupt events.

	Jonathan Dukes (jdukes@scss.tcd.ie)
*/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lcd.h"
#include "lcd_hw.h"
#include "lcd_grph.h"
#include "serial.h"
#include <stdio.h>
#include <string.h>

/* Maximum task stack size */
#define lcdSTACK_SIZE			( ( unsigned portBASE_TYPE ) 256 )

/* Interrupt handlers */
extern void vLCD_ISREntry( void );
void vLCD_ISRHandler( void );

/* The LCD task. */
static void vLcdTask( void *pvParameters );

/* my assignment code */
extern xComPortHandle xConsolePortHandle(void);
static xQueueHandle xTouchScreenPressedQ;

void vStartLcd( unsigned portBASE_TYPE uxPriority )
{
	/* my assignment code */
	xTouchScreenPressedQ = xQueueCreate(1,0);		

	/* Spawn the console task . */
	xTaskCreate( vLcdTask, ( signed char * ) "Lcd", lcdSTACK_SIZE, NULL, uxPriority, ( xTaskHandle * ) NULL );
}

struct ButtonRectangle
{
	int x0;
	int x1;
	int y0;
	int y1;
};

int inWhichButton(int x, int y, struct ButtonRectangle rects[], const int button_num)
{
	int i;
	for(i=0;i<button_num;++i)
	{
		if( rects[i].x0 < x 
			&& rects[i].x1 > x
			&& rects[i].y0 < y
			&& rects[i].y1 > y)
		{
			return i;
		}
	}
	return -1;
}

void displayResult(short digit[], int len)
{
	int i;
	for(i = 0; i < len; ++i)
	{
		printf("%hd", digit[i]);
	}
	printf("\r\n");
}

static portTASK_FUNCTION( vLcdTask, pvParameters )
{
	/* my variables */
	const int digit_len = 50;
	short digit[digit_len];
	int flag;
	int value;
	int digit_current_index = 0;
	const int block_num_x = 3;
	const int block_num_y = 4;
	const int line_border = 12;
	const int button_num = 12;
	int block_width = (DISPLAY_WIDTH-(block_num_x+1) * line_border)/block_num_x;
	//int width = DISPLAY_WIDTH;
	int block_height = (DISPLAY_HEIGHT-(block_num_y+1)*line_border)/block_num_y;
	//int height = DISPLAY_HEIGHT;
	int row,col, x0,x1,y0,y1, index;
	unsigned int x_pos, y_pos, pressure;
	struct ButtonRectangle buttonRects[button_num];
	unsigned char displayStrings[button_num][8] = {
		"1", "2", "3"
		,"4", "5", "6"
		,"7", "8", "9"
		,"OK", "0", "CANCEL"};
	short displayNumbers[button_num] = {
		1, 2, 3
		, 4, 5, 6
		, 7, 8, 9
		, -1, 0, -1
	};

	const int ok_index = 9, cancel_index = 11;


	/* Just to stop compiler warnings. */
	( void ) pvParameters;


	/* Initialise LCD display */
	/* NOTE: We needed to delay calling lcd_init() until here because it uses
	 * xTaskDelay to implement a delay and, as a result, can only be called from
	 * a task */
	lcd_init();

	lcd_fillScreen(BLACK);

	/* my assignment starts here */
   	// draw button
	y_pos = line_border;
	index = 0;
	for(row=0; row < block_num_y;++row)
	{
		x_pos = 0;
		for(col=0;col<block_num_x;++col)
		{
			x_pos += line_border;
			x0 = x_pos;
			x1 = x0 + block_width;
			y0 = y_pos;
			y1 = y0 + block_height;

			buttonRects[index].x0 = x0;
			buttonRects[index].x1 = x1;
			buttonRects[index].y0 = y0;
			buttonRects[index].y1 = y1;
			

			x_pos = x1;

			lcd_fillRect(x0,y0,x1,y1, GREEN);
			lcd_putString( (x0+x1)/2, (y0+y1)/2, displayStrings[index]);

			++index;
		}

		y_pos += block_height + line_border;
	}

	for(index = 0; index < button_num;++index)
	{
		printf("%d, %d, %d, %d, %d\r\n"
		, buttonRects[index].x0, buttonRects[index].x1
		, buttonRects[index].y0, buttonRects[index].y1
		, displayNumbers[index]);
	}

	// draw line
	/*
	for(row = 0; row < block_y+1; ++row)
		lcd_line(0, row * block_height, width, row * block_height, BLACK);

	for(col = 0; col < block_x+1; ++col)
		lcd_line(col * block_width, 0, col * block_width, height, BLACK);
	*/

	/* Infinite loop blocks waiting for a touch screen interrupt event from
	 * the queue. */
	for( ;; )
	{
		/* Clear TS interrupts (EINT3) */
		EXTINT = 8;

		/* Enable TS interrupt vector (VIC) (vector 17) */
		// VICIntSelect &= ~(1 << 17);	/* Configure vector 17 (EINT3) for IRQ */
		// VICVectPriority17 = 8;		/* Set priority 8 for vector 17 */
		// VICVectAddr17 = (unsigned long)vLCD_ISRHandler;	/* Set handler vector */
		VICIntEnable |= 1 << 17;	/* Enable interrupts on vector 17 */

		// printf("waiting for event\r\n");	   	

		/* Block on a quete waiting for an event from the TS interrupt handler */		
		xQueueReceive(xTouchScreenPressedQ, NULL, portMAX_DELAY);
		
		/* Disable TS interrupt vector (VIC) (vector 17) */
		VICIntEnClr = 1 << 17;
				
		

		/* +++ This point in the code can be interpreted as a screen button push event +++ */
		/* Start polling the touchscreen pressure and position ( getTouch(...) ) */
		/* Keep polling until pressure == 0 */
		
		pressure = 1;
		getTouch(&x_pos, &y_pos, &pressure);
		flag = 0;
		while(pressure)
		{ 	
			value = inWhichButton(x_pos, y_pos, buttonRects, button_num);

			// printf("%d,%d, %d\r\n",x_pos,y_pos, value);

			if(!flag)
			{
				if(0 <= value)
				{
					if(ok_index == value)
					{
						printf("OK pressed, you typed:\r\n");
						displayResult(digit, digit_current_index);
						digit_current_index = 0;
					}
					else if(cancel_index == value)
					{
						printf("CANCEL pressed, all type in disgarded\r\n");
						digit_current_index = 0;					
					}
					else
					{
						digit[digit_current_index] = displayNumbers[value];
						printf("value: %d, index: %d\r\n",displayNumbers[value], digit_current_index);
						++digit_current_index;
						
						if(digit_current_index >= digit_len)
						{
							printf("Buffer is full, you typed:\r\n");
							displayResult(digit, digit_len);
							digit_current_index = 0;		
							printf("Buffer clear\r\n");				
						}						
					}
				}
				flag = 1;
			}
			getTouch(&x_pos, &y_pos, &pressure);
			mdelay(100);
		} 

		mdelay(100);
		/* +++ This point in the code can be interpreted as a screen button release event +++ */
	}
}


void vLCD_ISRHandler( void )
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	/* Process the touchscreen interrupt */
	/* We would want to indicate to the task above that an event has occurred */
	xQueueSendFromISR(xTouchScreenPressedQ, 0, &xHigherPriorityTaskWoken);

	EXTINT = 8;					/* Reset EINT3 */
	VICVectAddr = 0;			/* Clear VIC interrupt */

	/* Exit the ISR.  If a task was woken by either a character being received
	or transmitted then a context switch will occur. */
	portEXIT_SWITCHING_ISR( xHigherPriorityTaskWoken );
}


