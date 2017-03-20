#include <hidef.h> /* for EnableInterrupts macro */
#include "derivative.h" /* include peripheral declarations */

/*
RTOS Switch Context
Joel Langer - joellanger@gmail.com
*/
//--------------------------
typedef unsigned long U32;//32 bits unsigned
typedef unsigned int U16;//16 bits unsigned
typedef unsigned char U8;//8 bits unsigned
typedef const unsigned char U8FLASH;//8 bits const in FLASH
typedef char PTR;//pointer
typedef char CAR8;//chars ASCII
typedef int INT;//integer
//--------------------------
U16 oCount=0;//for monitoring

U8 Stack1[50];
U16 Sp1;

void Task1(void)
{
    for(;;)
    {
        oCount++;
    }
}
//--------------------------
U16 tCount=0;//for monitoring

U8 Stack2[50];
U16 Sp2;

void Task2(void)
{
    for(;;)
    {
        tCount++;
    }
}
//--------------------------
U16 IntCounter=0;//for monitoring

U16 Sp;//storing the SP during the context switching
U8 nTasks=0;//number of installed tasks
U8 eTask=0;//task that was running
U16 *SpTasks[20];//stores the address of the task Stack pointers

//switching the context in each 8ms
void interrupt VectorNumber_Vrti SwitchContext(void)
{
    /*
        the CPU registers are stored automatically by the processor
        the H register content is not automatically saved, but the interrupt directive
        causes the compiler to add the instruction that stores content in Stack
    */
    asm//at the start of the interrupt
    {
        
        TSX//copy the content of SP to HX
        STHX Sp//store the content of HX in memory (Sp)
    }

    SRTISC_RTIACK=1;//clear the interrupt flag
    IntCounter++;
    //--------------------------------
    //switching
    //store the SP of current task
    *SpTasks[eTask]=Sp;
    //go to the next task to be running
    eTask++;
    if(eTask>=nTasks)
    {
        eTask=0;
    }
    //load the SP with the next task 
    Sp=*SpTasks[eTask];
    //--------------------------------
    asm//at the end of the interrupt
    {
        LDHX Sp//load the HX with an immediate value or memory (Sp)
        TXS//copy the content of HX to SP
    }
}

//it is added as a return address of a task that ended
void TaskEnd(void)
{
    DisableInterrupts;
    //remove the task (pending todo)
    EnableInterrupts;
    for(;;)//remove when implemented
    {
    }
}
//--------------------------
typedef struct
{
    U8 H;//INDEX REGISTER (HIGH), stored by firmware
    union
    {
        U8 Byte;
        struct
        {
            U8 C:1;//CARRY
            U8 Z:1;//ZERO
            U8 N:1;//NEGATIVE
            U8 I:1;//INTERRUPT MASK
            U8 H:1;//HALF-CARRY (FROM BIT 3)
            U8  :1;
            U8  :1;
            U8 V:1;//TWO'S COMPLEMENT OVERFLOW
        }Bits;
    }CCR;//CONDITION CODE REGISTER
    U8 A;//ACCUMULATOR
    U8 X;//INDEX REGISTER (LOW)
    U8 PCH;//PROGRAM COUNTER HIGH
    U8 PCL;//PROGRAM COUNTER LOW
}IntStackFrame;

typedef struct
{
    U8 PCH;//PROGRAM COUNTER HIGH
    U8 PCL;//PROGRAM COUNTER LOW
}RTSStackFrame;

//initialize the task
void InstallTask(void (*pTask)(void),U8 *pStack,U16 *pSp,U16 StackSize)
{
    IntStackFrame *ISFp;
    RTSStackFrame *RTSSFp;
    //initialize local SP
    *pSp=((U16)pStack)+(StackSize-sizeof(IntStackFrame)-(sizeof(RTSStackFrame)));//loads the local SP with the memory address of the local Stack
    //initialize the pointer of StackFrame
    ISFp=(IntStackFrame*)((U16)pStack+(StackSize-1)-(sizeof(IntStackFrame)-1)-(sizeof(RTSStackFrame)));
    //initialize the return pointer
    RTSSFp=(RTSStackFrame*)((U16)pStack+(StackSize-1)-(sizeof(RTSStackFrame)-1));
    
    //initialize the new Stack
    //context switching occurs in an interrupt
    //this load matches the information on the return of an interrupt
    //H
    ISFp->H=0;
    //CCR
    ISFp->CCR.Byte=0;
    //A
    ISFp->A=0;
    //X
    ISFp->X=0;
    //PCH
    ISFp->PCH=((U16)(pTask)) >> 8;//load the PCH with the most significant part of routine start address
    //PCL
    ISFp->PCL=((U16)(pTask)) & 0xFF;//load the PCL with the least significant part of routine start address
    
    RTSSFp->PCH=((U16)(&TaskEnd)) >> 8;
    RTSSFp->PCL=((U16)(&TaskEnd)) & 0xFF;
    
    DisableInterrupts;
    //add to context manager
    SpTasks[nTasks]=pSp;//loads 
    nTasks++;//increase the number of installed tasks
    EnableInterrupts;
}
//--------------------------
U16 mCount=0;//for monitoring

U16 SpMain;//stores the return pointer to the main task
void main(void)
{
    //initialize the CPU
    //initialize the hardware
    
    //so that the main program also becomes a running task
    SpTasks[0]=&SpMain;//load with the address of the Stack pointer of the main program
    nTasks=1;//now has one task
    //so the STACKSIZE defined in Project.prm is for the main task and the interrupts

    SRTISC = 0x51;//initialize the timer interrupt
    EnableInterrupts;//enable interrupts
    
    InstallTask(&Task1,Stack1,&Sp1,sizeof(Stack1));
    InstallTask(&Task2,Stack2,&Sp2,sizeof(Stack2));
    for(;;)
    {
        mCount++;
        __RESET_WATCHDOG();
    }
}
