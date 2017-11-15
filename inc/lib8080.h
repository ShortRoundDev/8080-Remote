#ifndef __LIB8080_H
#define __LIB8080_H
#include <stdio.h>
#include "libemulator.h"

//Register Constants
#define R_B 0
#define R_C 1
#define R_D 2
#define R_E 3
#define	R_H 4
#define R_L 5
#define R_M 6
#define R_A 7
#define R_F 8

//Flag Constants
#define F_C 1
#define F_P 4
#define F_A 16
#define F_Z 64
#define F_S 128

extern int ProcessId;

typedef struct Process{
	char 	free;
	Page* 	Next;
	unsigned short 	pc;
	unsigned short 	sp;
	unsigned char 	registers[9];

	char 	bank;
	char 	priority;
	char 	group;

	int Pid;
	int In;
	int Out;
	HighPageTable 	*PageTables;
} Process;

typedef void (*instruction)(char, short, Process*);
typedef unsigned int (*operation)(unsigned char, unsigned char);

void ComplementCarry(char operator, short operand, Process* Pinfo);
	
void SetCarry(char operator, short operand, Process* Pinfo);
	
void Increment(char operator, short operand, Process* Pinfo);
	
void Decrement(char operator, short operand, Process* Pinfo);
	
void ComplementAccumulator(char operator, short operand, Process* Pinfo);
	
void DecimalAdjustAccumulator(char operator, short operand, Process* Pinfo);
	
void NullInstruction(char operator, short operand, Process* Pinfo);
	
void Move(char operator, short operand, Process* Pinfo);
	
void StoreAccumulator(char operator, short operand, Process* Pinfo);
	
void LoadAccumulator(char operator, short operand, Process* Pinfo);
	
void Add(char operator, short operand, Process* Pinfo);
	
void AddCarry(char operator, short operand, Process* Pinfo);
	
void Subtract(char operator, short operand, Process* Pinfo);
	
void SubtractBorrow(char operator, short operand, Process* Pinfo);
	
void AND(char operator, short operand, Process* Pinfo);
	
void XOR(char operator, short operand, Process* Pinfo);
	
void OR(char operator, short operand, Process* Pinfo);
	
void CompareAccumulator(char operator, short operand, Process* Pinfo);
	
void RotateLeft(char operator, short operand, Process* Pinfo);
	
void RotateRight(char operator, short operand, Process* Pinfo);
	
void RotateLeftCarry(char operator, short operand, Process* Pinfo);
	
void RotateRightCarry(char operator, short operand, Process* Pinfo);
	
void Push(char operator, short operand, Process* Pinfo);
	
void Pop(char operator, short operand, Process* Pinfo);
	
void DoubleAdd(char operator, short operand, Process* Pinfo);
	
void IncrementPair(char operator, short operand, Process* Pinfo);
	
void DecrementPair(char operator, short operand, Process* Pinfo);

void ExchangeRegisters(char operator, short operand, Process* Pinfo);
	
void ExchangeStack(char operator, short operand, Process* Pinfo);
	
void LoadSP(char operator, short operand, Process* Pinfo);
	
void MoveImmediate(char operator, short operand, Process* Pinfo);
	
void AddImmediate(char operator, short operand, Process* Pinfo);
	
void AddImmediateCarry(char operator, short operand, Process* Pinfo);
	
void SubtractImmediate(char operator, short operand, Process* Pinfo);
	
void SubtractImmediateBorrow(char operator, short operand, Process* Pinfo);
	
void ANDImmediate(char operator, short operand, Process* Pinfo);
	
void XORImmediate(char operator, short operand, Process* Pinfo);
	
void ORImmediate(char operator, short operand, Process* Pinfo);
	
void CompareImmediate(char operator, short operand, Process* Pinfo);
	
void StoreDirect(char operator, short operand, Process* Pinfo);
	
void LoadDirect(char operator, short operand, Process* Pinfo);

void StoreHLDirect(char operator, short operand, Process* Pinfo);
	
void LoadHLDirect(char operator, short operand, Process* Pinfo);
	
void LoadPC(char operator, short operand, Process* Pinfo);
	
void Jump(char operator, short operand, Process* Pinfo);
	
void JumpIfCarry(char operator, short operand, Process* Pinfo);
	
void JumpNoCarry(char operator, short operand, Process* Pinfo);
	
void JumpIfZero(char operator, short operand, Process* Pinfo);
	
void JumpNotZero(char operator, short operand, Process* Pinfo);
	
void JumpIfMinus(char operator, short operand, Process* Pinfo);
	
void JumpIfPositive(char operator, short operand, Process* Pinfo);
	
void JumpIfEven(char operator, short operand, Process* Pinfo);
	
void JumpIfOdd(char operator, short operand, Process* Pinfo);
	
void Call(char operator, short operand, Process* Pinfo);
	
void CallIfCarry(char operator, short operand, Process* Pinfo);
	
void CallNoCarry(char operator, short operand, Process* Pinfo);
	
void CallIfZero(char operator, short operand, Process* Pinfo);
	
void CallNotZero(char operator, short operand, Process* Pinfo);
	
void CallIfMinus(char operator, short operand, Process* Pinfo);
	
void CallIfPlus(char operator, short operand, Process* Pinfo);
	
void CallIfEven(char operator, short operand, Process* Pinfo);
	
void CallIfOdd(char operator, short operand, Process* Pinfo);
	
void Return(char operator, short operand, Process* Pinfo);
	
void ReturnIfCarry(char operator, short operand, Process* Pinfo);
	
void ReturnNoCarry(char operator, short operand, Process* Pinfo);
	
void ReturnIfZero(char operator, short operand, Process* Pinfo);
	
void ReturnNotZero(char operator, short operand, Process* Pinfo);
	
void ReturnIfMinus(char operator, short operand, Process* Pinfo);
	
void ReturnIfPlus(char operator, short operand, Process* Pinfo);
	
void ReturnIfEven(char operator, short operand, Process* Pinfo);
	
void ReturnIfOdd(char operator, short operand, Process* Pinfo);
	
void Reset(char operator, short operand, Process* Pinfo);
	
void Interrupt(char operator, short operand, Process* Pinfo);
	
void EnableInterrupts(char operator, short operand, Process* Pinfo);
	
void DisableInterrupts(char operator, short operand, Process* Pinfo);
	
void Input(char operator, short operand, Process* Pinfo);

void Output(char operator, short operand, Process* Pinfo);
	
void Halt(char operator, short operand, Process* Pinfo);

void LoadInstructionSet();

void CheckParity(char Reg, Process *Pinfo);
void CheckCarryOut(unsigned short a, unsigned short b, Process *Pinfo, operation op);
void CheckAuxCarryOut(unsigned short a, unsigned short b, Process *Pinfo, operation op);
void CheckZero(int Reg, struct Process* Pinfo);
void CheckSign(int Reg, struct Process* Pinfo);

/*Auxiliary functions*/
unsigned int Addition(unsigned char a, unsigned char b);
unsigned int Subtraction(unsigned char a, unsigned char b);
#endif
