/*******************************************************************************/
/**
\file       app_scheduler.c
\brief      Multi-thread Task scheduler.
\author     Abraham Tezmol
\version    0.1
\date       09/09/2008
*/

/** Variable types and common definitions */
#include "system_samv71.h"

/** Scheduler function prototypes definitions */
#include "app_scheduler.h"
/** Tasks definition */
#include "app_tasks.h"
/** Real Time timer resource assigned as scheduler tick */
#include "systick.h"
/**For push button IRQ*/
#include "pio_it.h" 
#include "pio.h"


/*****************************************************************************************************
* Definition of  VARIABLEs - 
*****************************************************************************************************/

UINT8 gu8Scheduler_Status;
UINT8 gu8Scheduler_Counter;

tSchedulerTasks_ID TaskScheduler_Task_ID_Activated;
tSchedulerTasks_ID TaskScheduler_Task_ID_Running;
tSchedulerTasks_ID TasksScheduler_Task_ID_Backup;

UINT8 u8_10ms_Counter;
UINT8 u8_50ms_Counter;
UINT8 u8_100ms_Counter;

UINT8 TASK_PRIORITY;
UINT8 TASK_PRIORITY_PB;
UINT8 TASK_PRIORITY_BACKUP;
UINT8 PB_flag = 0;


tSchedulingTask TimeTriggeredTasks[TASK_SCH_MAX_NUMBER_TIME_TASKS] =
{ 
    {TASKS_1_MS,  TASKS_LIST_1MS,  SUSPENDED, 5},
    {TASKS_2_MS_A,TASKS_LIST_2MS_A,SUSPENDED, 4},
    {TASKS_2_MS_B,TASKS_LIST_2MS_B,SUSPENDED, 4},
    {TASKS_10_MS, TASKS_LIST_10MS, SUSPENDED, 3},
    {TASKS_50_MS, TASKS_LIST_50MS, SUSPENDED, 2},
    {TASKS_100_MS,TASKS_LIST_100MS,SUSPENDED, 1},
};
/**Event TASK*/
tSchedulingTask EVENT_TASK_PIO = {TASKS_PB,TASK_LIST_PB,SUSPENDED,EVENT_PRIORITY};

/** PinA09(SW0)*/
Pin pinPA09 = { PINA09_MASK, /**Pin mask*/
                PIOA, /**pio register direction*/
	            PIOA_IRQn, /**Port id*/
	            PIO_INPUT, /**type*/
	            PIO_DEBOUNCE | PIO_IT_FALL_EDGE}; /**attribute*/

Pin PC09 = {  0x40u, //Pin mask
                PIOA, //pio register direction
	            PIOA_IRQn, //Port id
	            PIO_OUTPUT_0, //type
	            PIO_PULLUP}; /**attribute*/

/*****************************************************************************************************
* Definition of module wide (CONST-) CONSTANTs 
*****************************************************************************************************/


/*****************************************************************************************************
* Code of module wide private FUNCTIONS
*****************************************************************************************************/
void vfnScheduler_Callback(void);
void _Button1_Handler(const Pin* pPin);

/*****************************************************************************************************
* Code of public FUNCTIONS
*****************************************************************************************************/
/**
* \brief    Events Initialization
* \author   Omar Anguiano
* \param    void
* \return   void
* \todo     
*/
void vfnInit_Event_Task( void )
{
    /* Configure PIO as inputs. */
	PIO_Configure( &pinPA09, 1) ;

	/* Adjust PIO denounce filter parameters, uses 10 Hz filter. */
	PIO_SetDebounceFilter( &pinPA09, CUTOFF_FREQ);

	PIO_InitializeInterrupts(EVENT_PRIORITY);
	/* Enable PIO controller IRQs. */
	NVIC_EnableIRQ(PIOA_IRQn);

/* Initialize PIO interrupt handlers, see PIO definition in board.h. */
	PIO_ConfigureIt( &pinPA09, _Button1_Handler) ; /* Interrupt on rising edge  */

	/* Enable PIO line interrupts. */
	PIO_EnableIt(&pinPA09);
}



/****************************************************************************************************/
/**
* \brief    Scheduler - Initialization
* \author   Abraham Tezmol
* \param    void
* \return   void
* \todo     
*/
void vfnScheduler_Init(void)
{    
    /* Init Global and local Task Scheduler variables */
    gu8Scheduler_Counter   = 0u;
    TaskScheduler_Task_ID_Activated = TASK_NULL;
    TaskScheduler_Task_ID_Running = TASK_NULL;
    TasksScheduler_Task_ID_Backup = TASK_NULL;
    u8_10ms_Counter        = 0u;
    u8_50ms_Counter        = 0u;
    u8_100ms_Counter       = 0u;
    gu8Scheduler_Status    = TASK_SCHEDULER_INIT;
}

/*******************************************************************************/
/**
* \brief    Scheduler Start - Once time base is armed, start execution of   \n
            Multi-thread Round Robin scheduling scheme.                     \n
            This function requires prior execution of "vfnScheduler_Init"
* \author   Abraham Tezmol
* \param    void
* \return   void
* \todo     
*/
void vfnScheduler_Start(void)
{
	if (sysTick_init(TASK_SCHEDULER_BASE_FREQ, vfnScheduler_Callback)) 
	{
		while (1);
	}
    gu8Scheduler_Status = TASK_SCHEDULER_RUNNING;
}

/*******************************************************************************/
/**
* \brief    Scheduler Stop - stop execution of Multi-thread Round Robin scheduling scheme.
* \author   Abraham Tezmol
* \param    void
* \return   void
* \todo     
*/
void vfnScheduler_Stop(void)
{  
    /* Update scheduler status accordingly */
    gu8Scheduler_Status = TASK_SCHEDULER_HALTED;
}

/***************************************************************************************************/
/**
* \brief    Scheduler - execution of time or event driven tasks
* \author   Abraham Tezmol
* \param    void
* \return   void
* \todo     
*/
void vfnScheduler_TaskStart( tSchedulingTask * Task )
{
    /* Indicate that this Task has gained CPU allocation */ 
    Task->enTaskState = RUNNING;
    TaskScheduler_Task_ID_Running =  Task->TaskId;
    /* Perform actual execution of task */
    Task->ptrTask();
    /* Indicate that Task execution has completed */ 
    Task->enTaskState = SUSPENDED;
}

/***************************************************************************************************/
/**
* \brief    Scheduler - activation of time or event driven tasks
* \author   Abraham Tezmol
* \param    Task - pointer to task
* \return   void
* \todo     
*/
void vfnScheduler_TaskActivate( tSchedulingTask * Task )
{ 
    TaskScheduler_Task_ID_Activated = Task->TaskId;
    Task->enTaskState = READY;
}

/***************************************************************************************************/
/**
* \brief    Scheduler - check priority of task or event driven tasks
* \author   Diego Reyna
* \param    void
* \return   void
* \todo     
*/
void vfnScheduler_TaskPriority( tSchedulingTask * Task )
{
    /* Check priority of current task */
    TASK_PRIORITY =  Task->u8Priority;
}

/*******************************************************************************/
/**
* \brief    This function goes through the list of event-driven activated tasks  \n
            If priority of these tasks is higher than priority of the           \n
            currently executed task, then it proceeds to execution of           \n
            tasks and removes them from the list                                \n
* \author   Diego Reyna
* \param    void
* \return   void
* \todo
*/
void vfnSchedulePoint(void)
{
    if( ( TaskScheduler_Task_ID_Activated == TASKS_1_MS )
         || ( TaskScheduler_Task_ID_Activated == TASKS_100_MS ) 
         || ( TaskScheduler_Task_ID_Activated == TASKS_PB ))
    {
        /* Make a copy of scheduled task ID */
        TasksScheduler_Task_ID_Backup = TaskScheduler_Task_ID_Activated;

        /* Check priority of 1 MS Task*/
        vfnScheduler_TaskPriority(&TimeTriggeredTasks[TASKS_1_MS]); 
        /* Make a copy of 1 MS Task Priority*/
        TASK_PRIORITY_BACKUP = TASK_PRIORITY;
        /* Check priority of 100 MS Task*/
        vfnScheduler_TaskPriority(&TimeTriggeredTasks[TASKS_100_MS]); 
        
        
        if(TaskScheduler_Task_ID_Activated == TASKS_PB ){
        /* Compare priorities of 1 MS Task and 100 MS Task*/
            if(TASK_PRIORITY_PB >= TASK_PRIORITY){
                vfnScheduler_TaskStart(&TimeTriggeredTasks[TASKS_PB]);
                PIO_Clear(&PC09);
                PB_flag = 0;
                printf("Event-Activated Clear\n");
            }
        }
        else if(TaskScheduler_Task_ID_Activated == TASKS_1_MS ){
        /* Compare priorities of 1 MS Task and 100 MS Task*/
            if(TASK_PRIORITY_BACKUP >= TASK_PRIORITY){
                vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_1_MS]);
                
            }
        }
        else if( TaskScheduler_Task_ID_Activated == TASKS_100_MS ){
            if(TASK_PRIORITY > TASK_PRIORITY_BACKUP){
                vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_100_MS]);
                
            }
            else{
                vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_1_MS]);
                vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_100_MS]);
                
            }
        }
        /* Verify that thread execution took less than 500 us */
        if( TasksScheduler_Task_ID_Backup == TaskScheduler_Task_ID_Activated )
        {
             /* In case execution of all thread took less than 500us */
            TaskScheduler_Task_ID_Activated = TASK_NULL;
        }
        else
        {
            gu8Scheduler_Status = TASK_SCHEDULER_OVERLOAD_1MS;
        }
    }
    else
    {
        /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
        /*  2ms execution thread - used to derive two execution threads:                */
        /*  a) 2ms group A thread (high priority tasks)                                 */
        /*  b) 50ms thread (second lowest priority tasks)                               */
        /*  As any other thread on this scheduler, all tasks must be executed in <=500us*/
        /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
        if( ( TaskScheduler_Task_ID_Activated == TASKS_2_MS_A )
             || ( TaskScheduler_Task_ID_Activated == TASKS_50_MS ) 
             || ( TaskScheduler_Task_ID_Activated == TASKS_PB ))
        {
            /* Make a copy of scheduled task ID */
            TasksScheduler_Task_ID_Backup = TaskScheduler_Task_ID_Activated;

            /* Check priority of 2 MS A Task*/
            vfnScheduler_TaskPriority(&TimeTriggeredTasks[TASKS_2_MS_A]); 
            /* Make a copy of 2 MS A Task Priority*/
            TASK_PRIORITY_BACKUP = TASK_PRIORITY;
            /* Check priority of 50 MS Task*/
            vfnScheduler_TaskPriority(&TimeTriggeredTasks[TASKS_50_MS]); 

            if(TaskScheduler_Task_ID_Activated == TASKS_PB ){
            /* Compare priorities of 1 MS Task and 100 MS Task*/
                if(TASK_PRIORITY_PB >= TASK_PRIORITY){
                    vfnScheduler_TaskStart(&TimeTriggeredTasks[TASKS_PB]);
                    PIO_Clear(&PC09);
                    PB_flag = 0;
                    printf("Event-Activated Clear\n");
                }
            }
            else if(TaskScheduler_Task_ID_Activated == TASKS_2_MS_A ){
                /* Compare priorities of 2 MS  A Task and 50 MS Task*/
                if(TASK_PRIORITY_BACKUP >= TASK_PRIORITY)
                {
                    vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_2_MS_A]);
                                 
                }
            }
            else if( TaskScheduler_Task_ID_Activated == TASKS_50_MS ){
                if(TASK_PRIORITY > TASK_PRIORITY_BACKUP)
                    vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_50_MS]);
                    
                else {
                    vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_2_MS_A]);
                    vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_50_MS]);
                    
                }
            }
            /* Verify that thread execution took less than 500 us */
            if( TasksScheduler_Task_ID_Backup == TaskScheduler_Task_ID_Activated )
            {
                 /* In case execution of all thread took less than 500us */
                TaskScheduler_Task_ID_Activated = TASK_NULL;
            }
            else
            {
                gu8Scheduler_Status = TASK_SCHEDULER_OVERLOAD_2MS_A;
            }
        }
        else
        {
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
            /*  2ms execution thread - used to derive two execution threads:                */
            /*  a) 2ms group B thread (high priority tasks)                                 */
            /*  b) 10ms thread (medium priority tasks)                                      */
            /*  As any other thread on this scheduler, all tasks must be executed in <=500us*/
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
            if( ( TaskScheduler_Task_ID_Activated == TASKS_2_MS_B )
                 || ( TaskScheduler_Task_ID_Activated == TASKS_10_MS ) 
                 || ( TaskScheduler_Task_ID_Activated == TASKS_PB ))
            {
                /* Make a copy of scheduled task ID */
                TasksScheduler_Task_ID_Backup = TaskScheduler_Task_ID_Activated;

                /* Check priority of 2 MS B Task*/
                vfnScheduler_TaskPriority(&TimeTriggeredTasks[TASKS_2_MS_B]); 
                /* Make a copy of 2 MS B Task Priority*/
                TASK_PRIORITY_BACKUP = TASK_PRIORITY;
                /* Check priority of 10 MS Task*/
                vfnScheduler_TaskPriority(&TimeTriggeredTasks[TASKS_10_MS]); 

                 if(TaskScheduler_Task_ID_Activated == TASKS_PB ){
                /* Compare priorities of 1 MS Task and 100 MS Task*/
                    if(TASK_PRIORITY_PB >= TASK_PRIORITY){
                        vfnScheduler_TaskStart(&TimeTriggeredTasks[TASKS_PB]);
                        PIO_Clear(&PC09);
                        PB_flag = 0;
                        printf("Event-Activated Clear\n");
                    }
                }
                else if(TaskScheduler_Task_ID_Activated == TASKS_2_MS_B ){
                    /* Compare priorities of 2 MS  B Task and 10 MS Task*/
                    if(TASK_PRIORITY_BACKUP >= TASK_PRIORITY)
                    {
                        vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_2_MS_B]);
                        
                    }
                }
                else if( TaskScheduler_Task_ID_Activated == TASKS_10_MS ){
                    if(TASK_PRIORITY > TASK_PRIORITY_BACKUP)
                        vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_10_MS]);
                        
                    else{
                        vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_2_MS_B]);
                        vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_10_MS]);
                        
                    }
                }

                
                 /* Verify that thread execution took less than 500 us */
                if( TasksScheduler_Task_ID_Backup == TaskScheduler_Task_ID_Activated )
                {
                    /* In case execution of all thread took less than 500us */
                    TaskScheduler_Task_ID_Activated = TASK_NULL;
                }
                else
                {
                    gu8Scheduler_Status = TASK_SCHEDULER_OVERLOAD_2MS_B;
                }
            }
        }
    }            
}
/***************************************************************************************************/


/*******************************************************************************/
/**
* \brief    Multi-thread round robin task Scheduler  (non-preemtive)        \n
            It calls the different tasks based on the status of             \n   
            "gu8Scheduler_Thread_ID". This variable is modified by          \n
            ISR "vfnScheduler_PIT_Isr"                                        \n
            List of tasks shall be defined @ "tasks.h" file
* \author   Abraham Tezmol
* \param    void
* \return   void
* \todo     
*/
void vfnTask_Scheduler(void)
{
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    /*  1ms execution thread - used to derive two execution threads:                */
    /*  a) 1ms thread (high priority tasks)                                         */
    /*  b) 100ms thread (lowest priority tasks)                                     */
    /*  As any other thread on this scheduler, all tasks must be executed in <=500us*/
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    if( ( TaskScheduler_Task_ID_Activated == TASKS_1_MS )
         || ( TaskScheduler_Task_ID_Activated == TASKS_100_MS ) )
    {
        /* Make a copy of scheduled task ID */
        TasksScheduler_Task_ID_Backup = TaskScheduler_Task_ID_Activated;

        //vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_1_MS]);
        if( TaskScheduler_Task_ID_Activated == TASKS_100_MS )
        {
            //vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_100_MS]);
        }
        /* Verify that thread execution took less than 500 us */
        if( TasksScheduler_Task_ID_Backup == TaskScheduler_Task_ID_Activated )
        {
             /* In case execution of all thread took less than 500us */
            TaskScheduler_Task_ID_Activated = TASK_NULL;
        }
        else
        {
            gu8Scheduler_Status = TASK_SCHEDULER_OVERLOAD_1MS;
        }
    }
    else
    {
        /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
        /*  2ms execution thread - used to derive two execution threads:                */
        /*  a) 2ms group A thread (high priority tasks)                                 */
        /*  b) 50ms thread (second lowest priority tasks)                               */
        /*  As any other thread on this scheduler, all tasks must be executed in <=500us*/
        /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
        if( ( TaskScheduler_Task_ID_Activated == TASKS_2_MS_A )
             || ( TaskScheduler_Task_ID_Activated == TASKS_50_MS ) )
        {
            /* Make a copy of scheduled task ID */
            TasksScheduler_Task_ID_Backup = TaskScheduler_Task_ID_Activated;

            //vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_2_MS_A]);
            if( TaskScheduler_Task_ID_Activated == TASKS_50_MS )
            {
                //vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_50_MS]);
            }
            /* Verify that thread execution took less than 500 us */
            if( TasksScheduler_Task_ID_Backup == TaskScheduler_Task_ID_Activated )
            {
                 /* In case execution of all thread took less than 500us */
                TaskScheduler_Task_ID_Activated = TASK_NULL;
            }
            else
            {
                gu8Scheduler_Status = TASK_SCHEDULER_OVERLOAD_2MS_A;
            }
        }
        else
        {
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
            /*  2ms execution thread - used to derive two execution threads:                */
            /*  a) 2ms group B thread (high priority tasks)                                 */
            /*  b) 10ms thread (medium priority tasks)                                      */
            /*  As any other thread on this scheduler, all tasks must be executed in <=500us*/
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
            if( ( TaskScheduler_Task_ID_Activated == TASKS_2_MS_B )
                 || ( TaskScheduler_Task_ID_Activated == TASKS_10_MS ) )
            {
                /* Make a copy of scheduled task ID */
                TasksScheduler_Task_ID_Backup = TaskScheduler_Task_ID_Activated;

                //vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_2_MS_B]);
                if( TaskScheduler_Task_ID_Activated == TASKS_10_MS )
                {
                    //vfnScheduler_TaskStart (&TimeTriggeredTasks[TASKS_10_MS]);
                }
                 /* Verify that thread execution took less than 500 us */
                if( TasksScheduler_Task_ID_Backup == TaskScheduler_Task_ID_Activated )
                {
                    /* In case execution of all thread took less than 500us */
                    TaskScheduler_Task_ID_Activated = TASK_NULL;
                }
                else
                {
                    gu8Scheduler_Status = TASK_SCHEDULER_OVERLOAD_2MS_B;
                }
            }
        }
    }
    vfnSchedulePoint();        
}

/*******************************************************************************/
/**
* \brief    Periodic Interrupt Timer Service routine.                            \n
            This interrupt is the core of the task scheduler.                   \n
            It is executed every 500us                                          \n
            It defines 3 basic threads from which other 3 threads are derived:  \n
            a) 1ms thread (basic) ->  100ms thread (derived)                    \n
            b) 2ms A thread (basic)-> 50ms thread (derived)                     \n
            c) 2ms B thread (basic)-> 10ms thread (derived)                     \n
            It partitions core execution time into time slices (500us each one).\n 
            This arrangement assures core will have equal task loading across time.\n   
            For more information on how time slice is assigned to each thread,  \n
            refer to file "S12X Task Scheduler Layout.xls"
* \author   Abraham Tezmol
* \param    void
* \return   void
* \todo
*/

void vfnScheduler_Callback(void)
{
    /*-- Update scheduler control variable --*/
    gu8Scheduler_Counter++;
    //printf("gu8Scheduler_Counter %d ", gu8Scheduler_Counter);

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    /*  1ms execution thread - used to derive two execution threads:                */
    /*  a) 1ms thread (highest priority tasks)                                      */
    /*  b) 100ms thread (lowest priority tasks)                                     */
    /*  As any other thread on this scheduling scheme,                              */
    /*  all tasks must be executed in <= 500us                                      */
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    if( ( gu8Scheduler_Counter & 0x01u ) == 0x01u )
    {
        u8_100ms_Counter++;
        /*-- Allow 100 ms periodic tasks to be executed --*/
        if( u8_100ms_Counter >= 100u )
        {
            /* Indicate that Task is Ready to be executed */ 
            vfnScheduler_TaskActivate(&TimeTriggeredTasks[TASKS_100_MS]);
            u8_100ms_Counter       = 0u;
        }
        /*-- Allow 1 ms periodic tasks to be executed --*/
        else
        {
            vfnScheduler_TaskActivate(&TimeTriggeredTasks[TASKS_1_MS]);
        }
    }
    else
    {
        /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
        /*  2ms execution thread - used to derive two execution threads:                */
        /*  a) 2ms group A thread (high priority tasks)                                 */
        /*  b) 50ms thread (second lowest priority tasks)                               */
        /*  As any other thread on this scheduling scheme,                              */
        /*  all tasks must be executed in <= 500us                                      */
        /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
        if( ( gu8Scheduler_Counter & 0x02u ) == 0x02u )
        {
            u8_50ms_Counter++;
            /*-- Allow 50 ms periodic tasks to be executed --*/
            if( u8_50ms_Counter >= 25u )
            {
                vfnScheduler_TaskActivate(&TimeTriggeredTasks[TASKS_50_MS]);
                u8_50ms_Counter        = 0u;
            }
            /*-- Allow 2 ms group A periodic tasks to be executed --*/
            else
            {
                vfnScheduler_TaskActivate(&TimeTriggeredTasks[TASKS_2_MS_A]);
            }
        }
        else
        {
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
            /*  2ms execution thread - used to derive two execution threads:                */
            /*  a) 2ms group B thread (high priority tasks)                                 */
            /*  b) 10ms thread (medium priority tasks)                                      */
            /*  As any other thread on this scheduling scheme,                              */
            /*  all tasks must be executed in <= 500us                                      */
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
            if( ( gu8Scheduler_Counter & 0x03u ) == 0x00u )
            {
                u8_10ms_Counter++;
                /*-- Allow 10 ms periodic tasks to be executed --*/
                if( u8_10ms_Counter >= 5u )
                {
                    vfnScheduler_TaskActivate(&TimeTriggeredTasks[TASKS_10_MS]);
                    u8_10ms_Counter        = 0u;
                }
                /*-- Allow 2 ms group B periodic tasks to be executed --*/
                else
                {
                    vfnScheduler_TaskActivate(&TimeTriggeredTasks[TASKS_2_MS_B]);
                }
            }
        }
    }
}



/**Button Handler*/
void _Button1_Handler(const Pin* pPin)
{
if ( pPin == &pinPA09 ) {
        /* Activate flag of PB Event*/
        PB_flag = 1;
        /* Indicate that PB Event Task is Ready to be executed */ 
        vfnScheduler_TaskActivate(&TimeTriggeredTasks[TASKS_PB]);
        /* Check priority of 1 MS Task*/
        vfnScheduler_TaskPriority(&TimeTriggeredTasks[TASKS_PB]); 
        /* Make a copy of 1 MS Task Priority*/
        TASK_PRIORITY_PB = TASK_PRIORITY;
        //vfnScheduler_TaskStart(&TimeTriggeredTasks[TASK_PB]);
        PIO_Set(&PC09);
	}
}