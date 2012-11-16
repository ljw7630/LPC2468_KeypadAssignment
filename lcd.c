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


static portTASK_FUNCTION( vLcdTask, pvParameters )
{
	/* my variables */
	int block_x = 3;
	int block_y = 4;
	int block_width = DISPLAY_WIDTH/block_x;
	int width = DISPLAY_WIDTH;
	int block_height = DISPLAY_HEIGHT/block_y;
	int height = DISPLAY_HEIGHT;
	int row,col, x0,x1,y0,y1;
	unsigned int xPos, yPos, pressure;

	unsigned char displayStrings[12][8] = {
		"1", "2", "3"
		,"4", "5", "6"
		,"7", "8", "9"
		,"OK", "0", "CANCEL"};


	/* Just to stop compiler warnings. */
	( void ) pvParameters;


	/* Initialise LCD display */
	/* NOTE: We needed to delay calling lcd_init() until here because it uses
	 * xTaskDelay to implement a delay and, as a result, can only be called from
	 * a task */
	lcd_init();

	lcd_fillScreen(MAROON);

	/* my assignment starts here */
   	// draw button
	for(row=0; row < block_y;++row)
	{
		for(col=0;col<block_x;++col)
		{
			y0 = row * block_height;
			y1 = y0 + block_height;
			x0 = col * block_width;
			x1 = x0 + block_width;
			lcd_fillRect(x0,y0,x1,y1, GREEN);
			lcd_putString( (x0+x1)/2, (y0+y1)/2, displayStrings[row*block_x+col]);

		}
	}

	// draw line
	for(row = 0; row < block_y+1; ++row)
		lcd_line(0, row * block_height, width, row * block_height, BLACK);

	for(col = 0; col < block_x+1; ++col)
		lcd_line(col * block_width, 0, col * block_width, height, BLACK);

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

		printf("waiting for event\r\n");	   	

		/* Block on a quete waiting for an event from the TS interrupt handler */		
		xQueueReceive(xTouchScreenPressedQ, NULL, portMAX_DELAY);
		
		/* Disable TS interrupt vector (VIC) (vector 17) */
		VICIntEnClr = 1 << 17;
				
		

		/* +++ This point in the code can be interpreted as a screen button push event +++ */
		/* Start polling the touchscreen pressure and position ( getTouch(...) ) */
		/* Keep polling until pressure == 0 */
		
		pressure = 1;
		getTouch(&xPos, &yPos, &pressure);
		while(pressure)
		{ 	printf("%d,%d, %d\r\n",xPos,yPos, pressure);
			getTouch(&xPos, &yPos, &pressure);
			mdelay(100);
		} 

		
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


