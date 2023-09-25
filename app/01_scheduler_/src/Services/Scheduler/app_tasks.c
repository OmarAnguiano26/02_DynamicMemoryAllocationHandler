/****************************************************************************************************/
/**
\file       app_tasks.c
\brief      Multi-thread Task scheduler - list of tasks.
\author     Abraham Tezmol
\version    1.0
\date       07/09/2013
*/
/****************************************************************************************************/

/*****************************************************************************************************
* Include files
*****************************************************************************************************/

/** Scheduler function prototypes definitions */
#include    "app_tasks.h"
/**For output function*/
#include "pio_it.h" 
#include "pio.h"
#include "app_scheduler.h"

/*****************************************************************************************************
* Definition of  VARIABLEs - 
*****************************************************************************************************/
Pin pinPC09 = {  0x40u, //Pin mask
                PIOA, //pio register direction
	            PIOA_IRQn, //Port id
	            PIO_OUTPUT_0, //type
	            PIO_PULLUP}; /**attribute*/

void vfnPIO_Output( void );


/*****************************************************************************************************
* Definition of module wide (CONST-) CONSTANTs 
*****************************************************************************************************/

/*****************************************************************************************************
* Code of module wide FUNCTIONS
*****************************************************************************************************/


void vfnPIO_Output( void )
{
    /**Toogles GPIO*/
    if(PIO_Get(&pinPC09))
    {
        PIO_Clear(&pinPC09);
    }
    else
    {
    PIO_Set(&pinPC09);
    }
}

/* List of tasks to be executed @ 1ms */
void TASKS_LIST_1MS( void )
{
    
    vfnLedCtrl_BlinkingPattern();
    //vfnSchedulePoint();
    //printf("1MS\n\n");
}

/* List of tasks to be executed @ 2ms, first group */
void TASKS_LIST_2MS_A(void)
{
    //vfnSchedulePoint();
    //printf("2MS_A\n\n");;
}
/* List of tasks to be executed @ 2ms, second group */
void TASKS_LIST_2MS_B( void )
{
    //vfnSchedulePoint();
    //printf("2MS_B\n\n");;
}
    
/* List of tasks to be executed @ 10ms */
void TASKS_LIST_10MS( void )
{
    //vfnSchedulePoint();
    //printf("10MS\n\n"); ;
}
/* List of tasks to be executed @ 50ms */
void TASKS_LIST_50MS( void )
{
    //vfnSchedulePoint();
    //printf("50MS\n\n");;
}
/* List of tasks to be executed @ 100ms */
void TASKS_LIST_100MS( void )
{
    //vfnSchedulePoint();
    //printf("100MS\n\n");;
}

/**Event Task to be exevuted each push button*/
void TASK_LIST_PB( void )
{
    printf("Event-Activated task priority = %d\n", EVENT_PRIORITY);
    //PIO_Set(&pinPC09);
    //vfnPIO_Output();
}


