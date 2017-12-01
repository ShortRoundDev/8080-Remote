#include "lib8080.h"
#include "libscheduler.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

extern instruction InstructionSet[256];

#define CMC ComplementCarry
void ComplementCarry(char operator, short operand, struct Process* Pinfo){
	//Page 14
	Pinfo->registers[R_F] ^= F_C;
	Pinfo->pc++;
}

#define STC SetCarry
void SetCarry(char operator, short operand, struct Process* Pinfo){
	//Page 14
	Pinfo->registers[R_F] |= F_C;
	Pinfo->pc++;
}

#define INR Increment
void Increment(char operator, short operand, struct Process* Pinfo){
	//Page 15
	int Reg = GetRegister(operator, 5, 3);
	int RegOut = Pinfo->registers[Reg];
	
	int _operand = 0;
	if(!Reg != 6){
		_operand = Pinfo->registers[Reg];
		Pinfo->registers[Reg] = _operand + 1;
	}
	else if(Reg == 6){
		short MemPtr = (Pinfo->registers[R_H] << 8) | Pinfo->registers[R_L];
		_operand = GetMemory(MemPtr, Pinfo);
		SetMemory(_operand + 1, MemPtr, Pinfo);
	}
	
	CheckZero(Reg, Pinfo);
	CheckSign(Reg, Pinfo);
	CheckParity(Reg, Pinfo);
	CheckAuxCarryOut(RegOut, 1, Pinfo, Addition);
	Pinfo->pc++;
}

#define DCR Decrement
void Decrement(char operator, short operand, struct Process* Pinfo){
	//Page 15
	int Reg = GetRegister(operator, 5, 3);
	int RegA = Pinfo->registers[R_A];
	
	if(!(Reg == 6 || Reg == 7)){
		Pinfo->registers[Reg]++;
	}
	else if(Reg == 6){
		short MemPtr = ((short)(Pinfo->registers[R_H]) << 8) | Pinfo->registers[R_L];
		SetMemory(GetMemory(MemPtr, Pinfo) - 1, MemPtr, Pinfo);
	}else if(Reg == 7){
		Pinfo->registers[R_A]--;
	}

	CheckZero(Reg, Pinfo);
	CheckSign(Reg, Pinfo);
	CheckParity(Reg, Pinfo);
	CheckAuxCarryOut(Pinfo->registers[R_A], 1, Pinfo, Subtraction);
	Pinfo->pc++;
}

#define CMA ComplementAccumulator
void ComplementAccumulator(char operator, short operand, struct Process* Pinfo){
	//Page 15
	Pinfo->registers[R_A] = ~(Pinfo->registers[R_A]);
	Pinfo->pc++;
}

#define DAA DecimalAdjustAccumulator
void DecimalAdjustAccumulator(char operator, short operand, struct Process* Pinfo){
	//Page 15
	if(Pinfo->registers[R_F] & F_A || (Pinfo->registers[R_A] & 0xf) > 9){
		if(Pinfo->registers[R_A] & 0xf > 9)
			Pinfo->registers[R_F] |= F_A;
		else
			Pinfo->registers[R_F] &= ~F_A;

		Pinfo->registers[R_A] += 6;
			
		if(((Pinfo->registers[R_A] & 0xf0) >> 4) > 9 || Pinfo->registers[R_F] & F_C){
			if((Pinfo->registers[R_A] >> 4) > 9)
				Pinfo->registers[R_F] |= F_C;
			else
				Pinfo->registers[R_F] &= ~F_C;
			Pinfo->registers[R_A] += 0x60;
		}
	}
	
	CheckZero(R_A, Pinfo);
	CheckSign(R_A, Pinfo);
	CheckParity(R_A, Pinfo);
	Pinfo->pc++;
}

#define NOP NullInstruction
void NullInstruction(char operator, short operand, struct Process* Pinfo){
	//Page 16
	Pinfo->pc++;
}

#define MOV Move
void Move(char operator, short operand, struct Process* Pinfo){
	//Page 1A
	int dst = GetRegister(operator, 5, 3);
	int src = operator & 0x7;
	
	int value = 0;
	if(src != 6){
		value = Pinfo->registers[src];
	}else if(src == 6){
		short MemPtr = (Pinfo->registers[R_H] << 8) | Pinfo->registers[R_L];
		value = GetMemory(MemPtr, Pinfo);
	}

	if(dst != 6){
		Pinfo->registers[dst] = value;
	}else if(dst == 6){
		short MemPtr = (Pinfo->registers[R_H] << 8) | Pinfo->registers[R_L];
		SetMemory(value, MemPtr, Pinfo);
	}
	Pinfo->pc++;

}

#define STAX StoreAccumulator
void StoreAccumulator(char operator, short operand, struct Process* Pinfo){
	//Page 17
	if(operator & 16){
		unsigned short MemPtr = Concatenate(R_D, R_E, Pinfo);
		unsigned char value = Pinfo->registers[R_A];
		SetMemory(value, MemPtr, Pinfo);
//		Pinfo->memory[Concatenate(R_D, R_E, Pinfo)] = Pinfo->registers[R_A];
	}else{
		unsigned short MemPtr = Concatenate(R_B, R_C, Pinfo);
		unsigned char value = Pinfo->registers[R_A];
		SetMemory(value, MemPtr, Pinfo);
//		Pinfo->memory[Concatenate(R_B, R_C, Pinfo)] = Pinfo->registers[R_A];
	}
	Pinfo->pc++;
}

#define LDAX LoadAccumulator
void LoadAccumulator(char operator, short operand, struct Process* Pinfo){
	//Page 17
	if(operator & 16){
		unsigned char value = GetMemory(Concatenate(R_D, R_E, Pinfo), Pinfo);
		Pinfo->registers[R_A] = value;
	}else{
		unsigned char value = GetMemory(Concatenate(R_B, R_C, Pinfo), Pinfo);
		Pinfo->registers[R_A] = value;
	}
	Pinfo->pc++;
}

#define ADD Add
void Add(char operator, short operand, struct Process* Pinfo){
	//Page 17
	int Reg = operator & 7;
	unsigned char value = 0;
	if(Reg != 6)
		value = Pinfo->registers[Reg];
	else
		value = GetMemory(Concatenate(R_H, R_L, Pinfo), Pinfo);
	unsigned char before = Pinfo->registers[R_A];
	Pinfo->registers[R_A] += value;
	CheckCarryOut(before, value, Pinfo, Addition);
	CheckAuxCarryOut(before, value, Pinfo, Addition);
	CheckZero(R_A, Pinfo);
	CheckParity(R_A, Pinfo);
	CheckSign(R_A, Pinfo);
	Pinfo->pc++;
}
#define ADC AddCarry
void AddCarry(char operator, short operand, struct Process* Pinfo){
	int Reg = operator & 7;
	unsigned char value = 0;
	int Carry = 0;
	if(Pinfo->registers[R_F] & F_C)
		Carry = 1;
	if(Reg != 6)
		value = Pinfo->registers[Reg];
	else
		value = GetMemory(Concatenate(R_H, R_L, Pinfo), Pinfo);
	
	unsigned char before = Pinfo->registers[R_A];
	Pinfo->registers[R_A] += value + Carry;
	CheckCarryOut(before, value + Carry, Pinfo, Addition);
	CheckAuxCarryOut(before, value + Carry, Pinfo, Addition);
	CheckZero(R_A, Pinfo);
	CheckParity(R_A, Pinfo);
	CheckSign(R_A, Pinfo);
	Pinfo->pc++;
}

#define SUB Subtract
void Subtract(char operator, short operand, struct Process* Pinfo){
	int Reg = operator & 7;
	char value = 0;
	if(Reg != 6)
		value = Pinfo->registers[Reg];
	else
		value = GetMemory(Concatenate(R_H, R_L, Pinfo), Pinfo);
	value = (~value) + 1;
	unsigned char before = Pinfo->registers[R_A];
	Pinfo->registers[R_A] += value;
	CheckCarryOut(before, value, Pinfo, Addition);
	Pinfo->registers[R_F] ^= F_C;
	CheckAuxCarryOut(before, value, Pinfo, Addition);
	CheckZero(R_A, Pinfo);
	CheckParity(R_A, Pinfo);
	CheckSign(R_A, Pinfo);
	Pinfo->pc++;
}

#define SBB SubtractBorrow
void SubtractBorrow(char operator, short operand, struct Process* Pinfo){
	int Reg = operator & 7;
	unsigned char value = 0;
	int Carry = 0;
	if(Pinfo->registers[R_F] & F_C)
		Carry = 1;
	if(Reg != 6)
		value = Pinfo->registers[Reg];
	else
		value = GetMemory(Concatenate(R_H, R_L, Pinfo), Pinfo);
	
	unsigned char before = Pinfo->registers[R_A];
	Pinfo->registers[R_A] -= (value + Carry);
	CheckCarryOut(before, value + Carry, Pinfo, Subtraction);
	CheckAuxCarryOut(before, value + Carry, Pinfo, Subtraction);
	CheckZero(R_A, Pinfo);
	CheckParity(R_A, Pinfo);
	CheckSign(R_A, Pinfo);
	Pinfo->pc++;
}

#define ANA AND
void AND(char operator, short operand, struct Process* Pinfo){
	//Page 19
	int Reg = operator & 7;
	unsigned char value = 0;
	if(Reg != 6)
		value = Pinfo->registers[Reg];
	else
		value = GetMemory(Concatenate(R_H, R_L, Pinfo), Pinfo);
	
	Pinfo->registers[R_A] &= value;
	Pinfo->registers[R_F] &= ~F_C;
	CheckZero(R_A, Pinfo);	
	CheckParity(R_A, Pinfo);
	CheckSign(R_A, Pinfo);
	Pinfo->pc++;
}

#define XRA XOR
void XOR(char operator, short operand, struct Process* Pinfo){
	//Page 19
	int Reg = operator & 7;
	unsigned char value = 0;
	if(Reg != 6)
		value = Pinfo->registers[Reg];
	else
		value = GetMemory(Concatenate(R_H, R_L, Pinfo), Pinfo);
	
	Pinfo->registers[R_A] ^= value;
	Pinfo->registers[R_F] &= ~F_C;
	Pinfo->registers[R_F] |= F_A;
	CheckZero(R_A, Pinfo);	
	CheckParity(R_A, Pinfo);
	CheckSign(R_A, Pinfo);
	Pinfo->pc++;
}

#define OR OR
void OR(char operator, short operand, struct Process* Pinfo){
	//Page 20
	int Reg = operator & 7;
	unsigned char value = 0;
	if(Reg != 6){
		value = Pinfo->registers[Reg];
		if(Reg == 7){
			
		}
	}
	else{
		value = GetMemory(Concatenate(R_H, R_L, Pinfo), Pinfo);
	}
	Pinfo->registers[R_A] |= value;
	Pinfo->registers[R_F] &= ~F_C;
	Pinfo->registers[R_F] |= F_A;
	CheckZero(R_A, Pinfo);	
	CheckParity(R_A, Pinfo);
	CheckSign(R_A, Pinfo);
	Pinfo->pc++;
}

#define CMP CompareAccumulator
void CompareAccumulator(char operator, short operand, struct Process* Pinfo){
	//Page 20
	int Reg = operator & 7;
	unsigned char value = 0;
	if(Reg != 6){
		value = Pinfo->registers[Reg];
	}else{
		value = GetMemory(Concatenate(R_H, R_L, Pinfo), Pinfo);
	}
	unsigned char Acc = Pinfo->registers[R_A];
	
	unsigned int diff = Acc - value;
	if(diff == 0){
		Pinfo->registers[R_F] |= F_Z;
	}else{
		Pinfo->registers[R_F] &= ~F_Z;
	}

	if(value > Acc){
		Pinfo->registers[R_F] |= F_C;
	}else{
		Pinfo->registers[R_F] &= ~F_C;
	}

		//Different signs, switch F_C
	if((Acc & 0x70) ^ (diff & 0x80)){
		Pinfo->registers[R_F] ^= F_C;
	}
	Pinfo->pc++;
}

#define RLC RotateLeft
void RotateLeft(char operator, short operand, struct Process* Pinfo){
	//Page 21
	unsigned char Acc = Pinfo->registers[R_A];
	Pinfo->registers[R_F] |= (Acc & 0x80) && 1 ? F_C : 0;
	Pinfo->registers[R_A] = (Acc << 1) | Pinfo->registers[R_F] & F_C;
	Pinfo->pc++;
}

#define RRC RotateRight
void RotateRight(char operator, short operand, struct Process* Pinfo){
	//Page 21
	unsigned char Acc = Pinfo->registers[R_A];
	Pinfo->registers[R_F] |= (Acc & 0x01);
	Pinfo->registers[R_A] = (Acc >> 1) | ((Pinfo->registers[R_F] & F_C) << 7);
	Pinfo->pc++;
}

#define RAL RotateLeftCarry
void RotateLeftCarry(char operator, short operand, struct Process* Pinfo){
	//Page 22
	unsigned char Acc = Pinfo->registers[R_A];
	unsigned char Carry = Pinfo->registers[R_F] & F_C;
	Pinfo->registers[R_F] |= (Acc & 0x80) && 1;
	Pinfo->registers[R_A] = (Acc << 1) | Carry;
	Pinfo->pc++;
}

#define RAR RotateRightCarry
void RotateRightCarry(char operator, short operand, struct Process* Pinfo){
	//Page 22
	unsigned char Acc = Pinfo->registers[R_A];
	unsigned char Carry = Pinfo->registers[R_F] & F_C;
	Pinfo->registers[R_F] &= (Acc & 1);
	Pinfo->registers[R_A] = (Acc >> 1) | Carry << 7;
	Pinfo->pc++;
}

#define PUSH Push
void Push(char operator, short operand, struct Process* Pinfo){
	//Page 22
	int RP = GetRegister(operator, 5, 4);
	unsigned short value = 0;

	if(RP == 0)
		value = Concatenate(R_B, R_C, Pinfo);
	else if(RP == 1)
		value = Concatenate(R_D, R_E, Pinfo);
	else if(RP == 2)
		value = Concatenate(R_H, R_L, Pinfo);
	else if(RP == 3)
		value = Concatenate(R_F, R_A, Pinfo);
	
	SetMemory((unsigned char)(value & 0xff), Pinfo->sp - 2, Pinfo);
	SetMemory((unsigned char)(value >> 8), Pinfo->sp - 1, Pinfo);
	
	Pinfo->sp -= 2;
	Pinfo->pc++;
}

#define POP Pop
void Pop(char operator, short operand, struct Process* Pinfo){
	//Page 23
	int RP = GetRegister(operator, 5, 4);
	unsigned short value = 0;
	if(RP == 0){
		Pinfo->registers[R_B] = GetMemory(Pinfo->sp + 1, Pinfo);
		Pinfo->registers[R_C] = GetMemory(Pinfo->sp, Pinfo);
	}else if(RP == 1){
		Pinfo->registers[R_D] = GetMemory(Pinfo->sp + 1, Pinfo);
		Pinfo->registers[R_E] = GetMemory(Pinfo->sp, Pinfo);
	}else if(RP == 2){
		Pinfo->registers[R_H] = GetMemory(Pinfo->sp + 1, Pinfo);
		Pinfo->registers[R_L] = GetMemory(Pinfo->sp, Pinfo);
	}else if(RP == 3){
		Pinfo->registers[R_A] = GetMemory(Pinfo->sp + 1, Pinfo);
		Pinfo->registers[R_F] = GetMemory(Pinfo->sp, Pinfo);
			//2nd flag bit always 1
		Pinfo->registers[R_F] |= 0x02;
			//4th & 6th flag bits always 0
		Pinfo->registers[R_F] &= ~(0x08 | 0x20);
	}
	Pinfo->sp += 2;
	Pinfo->pc++;
}

#define DAD DoubleAdd
void DoubleAdd(char operator, short operand, struct Process* Pinfo){
	//Page 24
	int RP = GetRegister(operator, 5, 4);
	unsigned int value = 0;
	if(RP == 0)
		value = Concatenate(R_B, R_C, Pinfo);
	else if(RP == 1)
		value = Concatenate(R_D, R_E, Pinfo);
	else if(RP == 2)
		value = Concatenate(R_H, R_L, Pinfo);
	else if(RP == 3)
		value = Pinfo->sp;

	unsigned int _operand = Concatenate(R_H, R_L, Pinfo);
	unsigned int sum = value + _operand;
	if(sum > 0xffff)
		Pinfo->registers[R_F] |= F_C;

	Pinfo->registers[R_L] = (unsigned char)(sum & 0xff);
	Pinfo->registers[R_H] = (unsigned char)((sum >> 8) & 0xff);
	Pinfo->pc++;
}

#define INX IncrementPair
void IncrementPair(char operator, short operand, struct Process* Pinfo){
	//Page 24
	int RP = GetRegister(operator, 5, 4);
	unsigned int value = 0;
	if(RP == 0){
		value = Concatenate(R_B, R_C, Pinfo);
		value++;
		Pinfo->registers[R_C] = value & 0xff;
		Pinfo->registers[R_C] = (value >> 8) & 0xff;
	}else if(RP == 1){
		value = Concatenate(R_D, R_E, Pinfo);
		value++;
		Pinfo->registers[R_D] = value & 0xff;
		Pinfo->registers[R_E] = (value >> 8) & 0xff;
	}else if(RP == 2){
		value = Concatenate(R_H, R_L, Pinfo);
		value++;
		Pinfo->registers[R_L] = value & 0xff;
		Pinfo->registers[R_H] = (value >> 8) & 0xff;
	}else if(RP == 3){
		Pinfo->sp++;
	}
	Pinfo->pc++;
}

#define DCX DecrementPair
void DecrementPair(char operator, short operand, struct Process* Pinfo){
	//Page 24
	int RP = GetRegister(operator, 5, 4);
	unsigned int value = 0;
	if(RP == 0){
		value = Concatenate(R_B, R_C, Pinfo);
		value--;
		Pinfo->registers[R_C] = value & 0xff;
		Pinfo->registers[R_C] = (value >> 8) & 0xff;
	}else if(RP == 1){
		value = Concatenate(R_D, R_E, Pinfo);
		value--;
		Pinfo->registers[R_D] = value & 0xff;
		Pinfo->registers[R_E] = (value >> 8) & 0xff;
	}else if(RP == 2){
		value = Concatenate(R_H, R_L, Pinfo);
		value--;
		Pinfo->registers[R_H] = value & 0xff;
		Pinfo->registers[R_L] = (value >> 8) & 0xff;
	}else if(RP == 3){
		Pinfo->sp--;
	}
	Pinfo->pc++;
}

#define XCHG ExchangeRegisters
void ExchangeRegisters(char operator, short operand, struct Process* Pinfo){
	unsigned short HL = Concatenate(R_H, R_L, Pinfo);
	unsigned short DE = Concatenate(R_D, R_E, Pinfo);
	unsigned short tmp = HL;
	HL = DE;
	DE = tmp;
	Pinfo->pc++;
}

#define XTHL ExchangeStack
void ExchangeStack(char operator, short operand, struct Process* Pinfo){
	//Page 25
	unsigned char H = Pinfo->registers[R_H];
	unsigned char L = Pinfo->registers[R_L];
	Pinfo->registers[R_L] = GetMemory(Pinfo->sp, Pinfo);
	Pinfo->registers[R_H] = GetMemory(Pinfo->sp + 1, Pinfo);
	SetMemory(L, Pinfo->sp, Pinfo);
	SetMemory(H, Pinfo->sp + 1, Pinfo);
	Pinfo->pc++;
}

#define SPHL LoadSP
void LoadSP(char operator, short operand, struct Process* Pinfo){
	//Page 25
	Pinfo->sp = Concatenate(R_H, R_L, Pinfo);
	Pinfo->pc++;
}

#define LXI LoadImmediate
void LoadImmediate(char operator, short operand, struct Process* Pinfo){
	//Page 26
	int RP = GetRegister(operator, 5, 4);
	if(RP == 0){
		Pinfo->registers[R_C] = operand & 0xff;
		Pinfo->registers[R_B] = (operand >> 8) & 0xff;
	}else if(RP == 1){
		Pinfo->registers[R_E] = operand & 0xff;
		Pinfo->registers[R_D] = (operand >> 8) & 0xff;
	}else if(RP == 2){
		Pinfo->registers[R_L] = operand & 0xff;
		Pinfo->registers[R_H] = (operand >> 8) & 0xff;
	}else if(RP == 3){
		Pinfo->sp = operand;
	}
	Pinfo->pc += 3;
}

#define MVI MoveImmediate
void MoveImmediate(char operator, short operand, struct Process* Pinfo){
	char reg = GetRegister(operator, 5, 3);
	if(reg == R_M){
			//TODO: Abstract memory access to support pages
//		Pinfo->memory[Pinfo->registers[R_M]] = operand;
		SetMemory(operand, Concatenate(R_H, R_L, Pinfo), Pinfo);
	}else{
		Pinfo->registers[reg] = operand;
	}
	Pinfo->pc += 3;
}

#define ADI AddImmediate
void AddImmediate(char operator, short operand, struct Process* Pinfo){
	//Page 27

	int value = Pinfo->registers[R_A];
	int _operand = (operand >> 8) & 0xff;
	Pinfo->registers[R_A] = value + _operand;

	CheckCarryOut(value, _operand, Pinfo, Addition);
	CheckAuxCarryOut(value, _operand, Pinfo, Addition);
	CheckZero(R_A, Pinfo);
	CheckParity(R_A, Pinfo);
	CheckSign(R_A, Pinfo);

	Pinfo->pc += 2;
}

#define ACI AddImmediateCarry
void AddImmediateCarry(char operator, short operand, struct Process* Pinfo){
	//Page 27
	unsigned int carry = Pinfo->registers[R_F] & F_C;
	
	int value = Pinfo->registers[R_A];
	int _operand = (operand >> 8) & 0xff;
	Pinfo->registers[R_A] = value + _operand + carry;

	CheckCarryOut(value, _operand + carry, Pinfo, Addition);
	CheckAuxCarryOut(value, _operand + carry, Pinfo, Addition);
	CheckZero(R_A, Pinfo);
	CheckParity(R_A, Pinfo);
	CheckSign(R_A, Pinfo);

	Pinfo->pc += 2;
}

#define SUI SubtractImmediate
void SubtractImmediate(char operator, short operand, struct Process* Pinfo){
	//Page 27
	
	int value = Pinfo->registers[R_A];
	int _operand = ~((operand >> 8) & 0xff);
	Pinfo->registers[R_A] = value + _operand;

	CheckCarryOut(value, _operand, Pinfo, Subtraction);
	CheckAuxCarryOut(value, _operand, Pinfo, Subtraction);
	CheckZero(R_A, Pinfo);
	CheckParity(R_A, Pinfo);
	CheckSign(R_A, Pinfo);

	Pinfo->pc += 2;
}

#define SBI SubtractImmediateBorrow
void SubtractImmediateBorrow(char operator, short operand, struct Process* Pinfo){
	//Page 28
	int carry = Pinfo->registers[R_F] & F_C;
	int value = Pinfo->registers[R_A];
	int _operand = ~(((operand >> 8) & 0xff) + carry);
	Pinfo->registers[R_A] = value + _operand;

	CheckCarryOut(value, _operand, Pinfo, Subtraction);
	CheckAuxCarryOut(value, _operand, Pinfo, Subtraction);
	CheckZero(R_A, Pinfo);
	CheckParity(R_A, Pinfo);
	CheckSign(R_A, Pinfo);

	Pinfo->pc += 2;
}

#define ANI ANDImmediate
void ANDImmediate(char operator, short operand, struct Process* Pinfo){
	//Page 28
	int value = (operand >> 8) & 0xff;
	Pinfo->registers[R_A] &= value;
	Pinfo->registers[R_F] &= ~F_C;
	CheckZero(R_A, Pinfo);
	CheckParity(R_A, Pinfo);
	CheckSign(R_A, Pinfo);

	Pinfo->pc += 2;
}

#define XRI XORImmediate
void XORImmediate(char operator, short operand, struct Process* Pinfo){
	//Page 29
	int value = (operand >> 8) & 0xff;
	Pinfo->registers[R_A] ^= value;
	Pinfo->registers[R_F] &= ~F_C;
	CheckZero(R_A, Pinfo);
	CheckParity(R_A, Pinfo);
	CheckSign(R_A, Pinfo);
	
	Pinfo->pc += 2;
}

#define ORI ORImmediate
void ORImmediate(char operator, short operand, struct Process* Pinfo){
	//Page 29
	int value = (operand >> 8) & 0xff;
	Pinfo->registers[R_A] |= value;
	Pinfo->registers[R_F] &= ~F_C;
	CheckZero(R_A, Pinfo);
	CheckParity(R_A, Pinfo);
	CheckSign(R_A, Pinfo);
	
	Pinfo->pc += 2;
}

#define CPI CompareImmediate
void CompareImmediate(char operator, short operand, struct Process* Pinfo){
	//Page 29
	unsigned char value = (operand >> 8) & 0xff;
	unsigned char Acc = Pinfo->registers[R_A];
	
	unsigned int diff = Acc - value;
	if(diff == 0){
		Pinfo->registers[R_F] |= F_Z;
	}else{
		Pinfo->registers[R_F] &= ~F_Z;
	}

	if(value > Acc){
		Pinfo->registers[R_F] |= F_C;
	}else{
		Pinfo->registers[R_F] &= ~F_C;
	}

		//Different signs, switch F_C
	if((Acc & 0x70) ^ (diff & 0x80)){
		Pinfo->registers[R_F] ^= F_C;
	}
	
	Pinfo->pc += 2;
}

#define STA StoreDirect
void StoreDirect(char operator, short operand, struct Process* Pinfo){
	//Page 30
	SetMemory(Pinfo->registers[R_A], operand, Pinfo);
	
	Pinfo->pc += 3;
}

#define LDA LoadDirect
void LoadDirect(char operator, short operand, struct Process* Pinfo){
	//Page 30
	Pinfo->registers[R_A] = GetMemory(operand, Pinfo);
	
	Pinfo->pc += 3;
}

#define SHLD StoreHLDirect
void StoreHLDirect(char operator, short operand, struct Process* Pinfo){
	//Page 30
	SetMemory(Pinfo->registers[R_L], operand, Pinfo);
	SetMemory(Pinfo->registers[R_H], operand + 1, Pinfo);
	
	Pinfo->pc += 3;
}

#define LHLD LoadHLDirect
void LoadHLDirect(char operator, short operand, struct Process* Pinfo){
	//Page 31
	unsigned char L = GetMemory(operand, Pinfo);
	unsigned char H = GetMemory(operand + 1, Pinfo);
	
	Pinfo->registers[R_H] = H;
	Pinfo->registers[R_L] = L;
	
	Pinfo->pc += 3;
}

#define PCHL LoadPC
void LoadPC(char operator, short operand, struct Process* Pinfo){
	//Page 31
	Pinfo->pc = Concatenate(R_H, R_L, Pinfo);
	
	Pinfo->pc++;
}

#define JMP Jump
void Jump(char operator, short operand, struct Process* Pinfo){
	//Page 32
	Pinfo->pc = operand;
}

#define JC JumpIfCarry
void JumpIfCarry(char operator, short operand, struct Process* Pinfo){
	//Page 32
	if(Pinfo->registers[R_F] & F_C){
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}
}

#define JNC JumpNoCarry
void JumpNoCarry(char operator, short operand, struct Process* Pinfo){
	//Page 32
	if(!(Pinfo->registers[R_F] & F_C)){
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}
}

#define JZ JumpIfZero
void JumpIfZero(char operator, short operand, struct Process* Pinfo){
	//Page 32
	if(Pinfo->registers[R_F] & F_Z){
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}
}

#define JNZ JumpNotZero
void JumpNotZero(char operator, short operand, struct Process* Pinfo){
	//Page 33
	if(!(Pinfo->registers[R_F] & F_Z)){
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}
}

#define JM JumpIfMinus
void JumpIfMinus(char operator, short operand, struct Process* Pinfo){
	//Page 33
	if(Pinfo->registers[R_F] & F_S){
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}

}

#define JP JumpIfPositive
void JumpIfPositive(char operator, short operand, struct Process* Pinfo){
	//Page 33
	if(!(Pinfo->registers[R_F] & F_S)){
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}
}

#define JPE JumpIfEven
void JumpIfEven(char operator, short operand, struct Process* Pinfo){
	//Page 33
	if(Pinfo->registers[R_F] & F_P){
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}
}

#define JPO JumpIfOdd
void JumpIfOdd(char operator, short operand, struct Process* Pinfo){
	//Page 33
	if(!(Pinfo->registers[R_F] & F_P)){
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}
}

#define CALL Call
void Call(char operator, short operand, struct Process* Pinfo){
	//Page 34
	unsigned short value = Pinfo->pc;
	SetMemory(value & 0xff, Pinfo->sp - 2, Pinfo);
	SetMemory((value >> 8) & 0xff, Pinfo->sp - 1, Pinfo);
	Pinfo->sp -= 2;
	Pinfo->pc = operand;
}

#define CC CallIfCarry
void CallIfCarry(char operator, short operand, struct Process* Pinfo){
	//Page 34
	if(Pinfo->registers[R_F] & F_C){
		unsigned short value = Pinfo->pc;
		SetMemory(value & 0xff, Pinfo->sp - 2, Pinfo);
		SetMemory((value >> 8) & 0xff, Pinfo->sp - 1, Pinfo);
		Pinfo->sp -= 2;
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}
}

#define CNC CallNoCarry
void CallNoCarry(char operator, short operand, struct Process* Pinfo){
	//Page 34
	if(!(Pinfo->registers[R_F] & F_C)){
		unsigned short value = Pinfo->pc;
		SetMemory(value & 0xff, Pinfo->sp - 2, Pinfo);
		SetMemory((value >> 8) & 0xff, Pinfo->sp - 1, Pinfo);
		Pinfo->sp -= 2;
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}
}

#define CZ CallIfZero
void CallIfZero(char operator, short operand, struct Process* Pinfo){
	//Page 35
	if(Pinfo->registers[R_F] & F_Z){
		unsigned short value = Pinfo->pc;
		SetMemory(value & 0xff, Pinfo->sp - 2, Pinfo);
		SetMemory((value >> 8) & 0xff, Pinfo->sp - 1, Pinfo);
		Pinfo->sp -= 2;
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}
}

#define CNZ CallNotZero
void CallNotZero(char operator, short operand, struct Process* Pinfo){
	//Page 35
	if(!(Pinfo->registers[R_F] & F_Z)){
		unsigned short value = Pinfo->pc;
		SetMemory(value & 0xff, Pinfo->sp - 2, Pinfo);
		SetMemory((value >> 8) & 0xff, Pinfo->sp - 1, Pinfo);
		Pinfo->sp -= 2;
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}
}

#define CM CallIfMinus
void CallIfMinus(char operator, short operand, struct Process* Pinfo){
	//Page 35
	if(Pinfo->registers[R_F] & F_S){
		unsigned short value = Pinfo->pc;
		SetMemory(value & 0xff, Pinfo->sp - 2, Pinfo);
		SetMemory((value >> 8) & 0xff, Pinfo->sp - 1, Pinfo);
		Pinfo->sp -= 2;
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}
}

#define CP CallIfPlus
void CallIfPlus(char operator, short operand, struct Process* Pinfo){
	//Page 35
	if(!(Pinfo->registers[R_F] & F_S)){
		unsigned short value = Pinfo->pc;
		SetMemory(value & 0xff, Pinfo->sp - 2, Pinfo);
		SetMemory((value >> 8) & 0xff, Pinfo->sp - 1, Pinfo);
		Pinfo->sp -= 2;
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}
}

#define CPE CallIfEven
void CallIfEven(char operator, short operand, struct Process* Pinfo){
	//Page 35
	if(Pinfo->registers[R_F] & F_P){
		unsigned short value = Pinfo->pc;
		SetMemory(value & 0xff, Pinfo->sp - 2, Pinfo);
		SetMemory((value >> 8) & 0xff, Pinfo->sp - 1, Pinfo);
		Pinfo->sp -= 2;
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}
}

#define CPO CallIfOdd
void CallIfOdd(char operator, short operand, struct Process* Pinfo){
	//Page 35
	if(!(Pinfo->registers[R_F] & F_P)){
		unsigned short value = Pinfo->pc;
		SetMemory(value & 0xff, Pinfo->sp - 2, Pinfo);
		SetMemory((value >> 8) & 0xff, Pinfo->sp - 1, Pinfo);
		Pinfo->sp -= 2;
		Pinfo->pc = operand;
	}else{
		Pinfo->pc += 3;
	}
}

#define RET Return
void Return(char operator, short operand, struct Process* Pinfo){
	//Page 36
	unsigned short high = GetMemory(Pinfo->sp + 1, Pinfo);
	unsigned char low  = GetMemory(Pinfo->sp, Pinfo);
	unsigned short ReturnAddress = (high << 8) | low;
	
	Pinfo->pc = ReturnAddress;
	Pinfo->sp += 2;
}

#define RC ReturnIfCarry
void ReturnIfCarry(char operator, short operand, struct Process* Pinfo){
	//Page 36
	if(Pinfo->registers[R_F] & F_C){
		unsigned short high = GetMemory(Pinfo->sp + 1, Pinfo);
		unsigned char low  = GetMemory(Pinfo->sp, Pinfo);
		unsigned short ReturnAddress = (high << 8) | low;
		
		Pinfo->pc = ReturnAddress;
		Pinfo->sp += 2;
	}else{
		Pinfo->pc += 3;
	}
}

#define RNC ReturnNoCarry
void ReturnNoCarry(char operator, short operand, struct Process* Pinfo){
	//Page 36
	if(!(Pinfo->registers[R_F] & F_C)){
		unsigned short high = GetMemory(Pinfo->sp + 1, Pinfo);
		unsigned char low  = GetMemory(Pinfo->sp, Pinfo);
		unsigned short ReturnAddress = (high << 8) | low;
		
		Pinfo->pc = ReturnAddress;
		Pinfo->sp += 2;
	}else{
		Pinfo->pc += 3;
	}
}

#define RZ ReturnIfZero
void ReturnIfZero(char operator, short operand, struct Process* Pinfo){
	//Page 36
	if(Pinfo->registers[R_F] & F_Z){
		unsigned short high = GetMemory(Pinfo->sp + 1, Pinfo);
		unsigned char low  = GetMemory(Pinfo->sp, Pinfo);
		unsigned short ReturnAddress = (high << 8) | low;
		
		Pinfo->pc = ReturnAddress;
		Pinfo->sp += 2;
	}else{
		Pinfo->pc += 3;
	}
}

#define RNZ ReturnNotZero
void ReturnNotZero(char operator, short operand, struct Process* Pinfo){
	//Page 36
	if(!(Pinfo->registers[R_F] & F_Z)){
		unsigned short high = GetMemory(Pinfo->sp + 1, Pinfo);
		unsigned char low  = GetMemory(Pinfo->sp, Pinfo);
		unsigned short ReturnAddress = (high << 8) | low;
		
		Pinfo->pc = ReturnAddress;
		Pinfo->sp += 2;
	}else{
		Pinfo->pc += 3;
	}
}

#define RM ReturnIfMinus
void ReturnIfMinus(char operator, short operand, struct Process* Pinfo){
	//Page 37
	if(Pinfo->registers[R_F] & F_S){
		unsigned short high = GetMemory(Pinfo->sp + 1, Pinfo);
		unsigned char low  = GetMemory(Pinfo->sp, Pinfo);
		unsigned short ReturnAddress = (high << 8) | low;
		
		Pinfo->pc = ReturnAddress;
		Pinfo->sp += 2;
	}else{
		Pinfo->pc += 3;
	}
}

#define RP ReturnIfPlus
void ReturnIfPlus(char operator, short operand, struct Process* Pinfo){
	//Page 37
	if(!(Pinfo->registers[R_F] & F_S)){
		unsigned short high = GetMemory(Pinfo->sp + 1, Pinfo);
		unsigned char low  = GetMemory(Pinfo->sp, Pinfo);
		unsigned short ReturnAddress = (high << 8) | low;
		
		Pinfo->pc = ReturnAddress;
		Pinfo->sp += 2;
	}else{
		Pinfo->pc += 3;
	}
}

#define RPE ReturnIfEven
void ReturnIfEven(char operator, short operand, struct Process* Pinfo){
	//Page 37
	if(Pinfo->registers[R_F] & F_P){
		unsigned short high = GetMemory(Pinfo->sp + 1, Pinfo);
		unsigned char low  = GetMemory(Pinfo->sp, Pinfo);
		unsigned short ReturnAddress = (high << 8) | low;
		
		Pinfo->pc = ReturnAddress;
		Pinfo->sp += 2;
	}else{
		Pinfo->pc += 3;
	}
}

#define RPO ReturnIfOdd
void ReturnIfOdd(char operator, short operand, struct Process* Pinfo){
	//Page 37
	if(!(Pinfo->registers[R_F] & F_P)){
		unsigned short high = GetMemory(Pinfo->sp + 1, Pinfo);
		unsigned char low  = GetMemory(Pinfo->sp, Pinfo);
		unsigned short ReturnAddress = (high << 8) | low;
		
		Pinfo->pc = ReturnAddress;
		Pinfo->sp += 2;
	}else{
		Pinfo->pc += 3;
	}
}

#define RST Reset
void Reset(char operator, short operand, struct Process* Pinfo){
	//Page 37
	if(Pinfo->inte == 0)
		return;
	int RstLocation = (GetRegister(operator, 5, 3) << 3);
	unsigned short value = Pinfo->pc;
	SetMemory(value & 0xff, Pinfo->sp - 2, Pinfo);
	SetMemory((value >> 8) & 0xff, Pinfo->sp - 1, Pinfo);
	Pinfo->sp -= 2;
	Pinfo->pc = RstLocation;
}

#define EI EnableInterrupts
void EnableInterrupts(char operator, short operand, struct Process* Pinfo){
	//Page 38
	Pinfo->inte = 1;
	
	Pinfo->pc++;
}

#define DI DisableInterrupts
void DisableInterrupts(char operator, short operand, struct Process* Pinfo){
	//Page 38
	Pinfo->inte = 0;
	
	Pinfo->pc++;
}

#define IN Input
void Input(char operator, short operand, struct Process* Pinfo){
	//Page 38
	int BytesAvailable = 0;
	int err = ioctl(Pinfo->In, FIONREAD, &BytesAvailable);
	if(BytesAvailable == 0){
		Pinfo->registers[R_A] = 0;
	}else{
		char Out = 0;
		read(Pinfo->In, &Out, 1);
		Pinfo->registers[R_A] = Out;
	}
	
	Pinfo->pc+=2;
}

#define OUT Output
void Output(char operator, short operand, struct Process* Pinfo){
	//Page 39
	write(Pinfo->Out, &Pinfo->registers[R_A], 1);
	Pinfo->pc+=2;
}

#define HLT Halt
#define HALT Halt
void Halt(char operator, short operand, struct Process* Pinfo){
	//Page 39
	Pinfo->state = -1;
}

void Fork(char operator, short operand, struct Process *Pinfo){
	Process *Child = ProcessAlloc();
	Child->PageTables = Pinfo->PageTables;
	Child->state = Pinfo->state;
	Child->inte = Pinfo->inte;
	Child->pc = Pinfo->pc;
	Child->sp = Pinfo->sp;

	Child->registers[R_A] = Pinfo->registers[R_A];
	Child->registers[R_B] = Pinfo->registers[R_B];
	Child->registers[R_C] = Pinfo->registers[R_C];
	Child->registers[R_D] = Pinfo->registers[R_D];
	Child->registers[R_E] = Pinfo->registers[R_E];
	Child->registers[R_H] = Pinfo->registers[R_H];
	Child->registers[R_L] = Pinfo->registers[R_L];
	Child->registers[R_F] = Pinfo->registers[R_F];
	Child->registers[R_M] = Pinfo->registers[R_M];
	
	Child->registers[R_F] |= F_Z;
	Pinfo->registers[R_F] &= ~F_Z;
	Child->bank = Pinfo->bank;
	Child->priority = Pinfo->priority;
	Child->group = Pinfo->group;
	
	Child->Pid = ProcessId++;
	Child->In = Pinfo->In;
	Child->Out = Pinfo->Out;
	
	AddToProcessTable(Child);
	QueueInsert(Child);
	Pinfo->pc++;
	Child->pc++;
}

void LoadInstructionSet(){

	InstructionSet[0x0] = NOP;
	InstructionSet[0x1] = LXI;
	InstructionSet[0x2] = STAX;
	InstructionSet[0x3] = INX;
	InstructionSet[0x4] = INR;
	InstructionSet[0x5] = DCR;
	InstructionSet[0x6] = MVI;
	InstructionSet[0x7] = RLC;
	InstructionSet[0x9] = DAD;
	InstructionSet[0xa] = LDAX;
	InstructionSet[0xb] = DCX;
	InstructionSet[0xc] = INR;
	InstructionSet[0xd] = DCR;
	InstructionSet[0xe] = MVI;
	InstructionSet[0xf] = RRC;
	InstructionSet[0x11] = LXI;
	InstructionSet[0x12] = STAX;
	InstructionSet[0x13] = INX;
	InstructionSet[0x14] = INR;
	InstructionSet[0x15] = DCR;
	InstructionSet[0x16] = MVI;
	InstructionSet[0x17] = RAL;
	InstructionSet[0x19] = DAD;
	InstructionSet[0x1a] = LDAX;
	InstructionSet[0x1b] = DCX;
	InstructionSet[0x1c] = INR;
	InstructionSet[0x1d] = DCR;
	InstructionSet[0x1e] = MVI;
	InstructionSet[0x1f] = RAR;
	InstructionSet[0x21] = LXI;
	InstructionSet[0x22] = SHLD;
	InstructionSet[0x23] = INX;
	InstructionSet[0x24] = INR;
	InstructionSet[0x25] = DCR;
	InstructionSet[0x26] = MVI;
	InstructionSet[0x27] = DAA;
	InstructionSet[0x29] = DAD;
	InstructionSet[0x2a] = LHLD;
	InstructionSet[0x2b] = DCX;
	InstructionSet[0x2c] = INR;
	InstructionSet[0x2d] = DCR;
	InstructionSet[0x2e] = MVI;
	InstructionSet[0x2f] = CMA;
	InstructionSet[0x31] = LXI;
	InstructionSet[0x32] = STA;
	InstructionSet[0x33] = INX;
	InstructionSet[0x34] = INR;
	InstructionSet[0x35] = DCR;
	InstructionSet[0x36] = MVI;
	InstructionSet[0x37] = STC;
	InstructionSet[0x39] = DAD;
	InstructionSet[0x3a] = LDA;
	InstructionSet[0x3b] = DCX;
	InstructionSet[0x3c] = INR;
	InstructionSet[0x3d] = DCR;
	InstructionSet[0x3e] = MVI;
	InstructionSet[0x3f] = CMC;
	InstructionSet[0x40] = MOV;
	InstructionSet[0x41] = MOV;
	InstructionSet[0x42] = MOV;
	InstructionSet[0x43] = MOV;
	InstructionSet[0x44] = MOV;
	InstructionSet[0x45] = MOV;
	InstructionSet[0x46] = MOV;
	InstructionSet[0x47] = MOV;
	InstructionSet[0x48] = MOV;
	InstructionSet[0x49] = MOV;
	InstructionSet[0x4a] = MOV;
	InstructionSet[0x4b] = MOV;
	InstructionSet[0x4c] = MOV;
	InstructionSet[0x4d] = MOV;
	InstructionSet[0x4e] = MOV;
	InstructionSet[0x4f] = MOV;
	InstructionSet[0x50] = MOV;
	InstructionSet[0x51] = MOV;
	InstructionSet[0x52] = MOV;
	InstructionSet[0x53] = MOV;
	InstructionSet[0x54] = MOV;
	InstructionSet[0x55] = MOV;
	InstructionSet[0x56] = MOV;
	InstructionSet[0x57] = MOV;
	InstructionSet[0x58] = MOV;
	InstructionSet[0x59] = MOV;
	InstructionSet[0x5a] = MOV;
	InstructionSet[0x5b] = MOV;
	InstructionSet[0x5c] = MOV;
	InstructionSet[0x5d] = MOV;
	InstructionSet[0x5e] = MOV;
	InstructionSet[0x5f] = MOV;
	InstructionSet[0x60] = MOV;
	InstructionSet[0x61] = MOV;
	InstructionSet[0x62] = MOV;
	InstructionSet[0x63] = MOV;
	InstructionSet[0x64] = MOV;
	InstructionSet[0x65] = MOV;
	InstructionSet[0x66] = MOV;
	InstructionSet[0x67] = MOV;
	InstructionSet[0x68] = MOV;
	InstructionSet[0x69] = MOV;
	InstructionSet[0x6a] = MOV;
	InstructionSet[0x6b] = MOV;
	InstructionSet[0x6c] = MOV;
	InstructionSet[0x6d] = MOV;
	InstructionSet[0x6e] = MOV;
	InstructionSet[0x6f] = MOV;
	InstructionSet[0x70] = MOV;
	InstructionSet[0x71] = MOV;
	InstructionSet[0x72] = MOV;
	InstructionSet[0x73] = MOV;
	InstructionSet[0x74] = MOV;
	InstructionSet[0x75] = MOV;
	InstructionSet[0x76] = HLT;
	InstructionSet[0x77] = MOV;
	InstructionSet[0x78] = MOV;
	InstructionSet[0x79] = MOV;
	InstructionSet[0x7a] = MOV;
	InstructionSet[0x7b] = MOV;
	InstructionSet[0x7c] = MOV;
	InstructionSet[0x7d] = MOV;
	InstructionSet[0x7e] = MOV;
	InstructionSet[0x7f] = MOV;
	InstructionSet[0x80] = ADD;
	InstructionSet[0x81] = ADD;
	InstructionSet[0x82] = ADD;
	InstructionSet[0x83] = ADD;
	InstructionSet[0x84] = ADD;
	InstructionSet[0x85] = ADD;
	InstructionSet[0x86] = ADD;
	InstructionSet[0x87] = ADD;
	InstructionSet[0x88] = ADC;
	InstructionSet[0x89] = ADC;
	InstructionSet[0x8a] = ADC;
	InstructionSet[0x8b] = ADC;
	InstructionSet[0x8c] = ADC;
	InstructionSet[0x8d] = ADC;
	InstructionSet[0x8e] = ADC;
	InstructionSet[0x8f] = ADC;
	InstructionSet[0x90] = SUB;
	InstructionSet[0x91] = SUB;
	InstructionSet[0x92] = SUB;
	InstructionSet[0x93] = SUB;
	InstructionSet[0x94] = SUB;
	InstructionSet[0x95] = SUB;
	InstructionSet[0x96] = SUB;
	InstructionSet[0x97] = SUB;
	InstructionSet[0x98] = SBB;
	InstructionSet[0x99] = SBB;
	InstructionSet[0x9a] = SBB;
	InstructionSet[0x9b] = SBB;
	InstructionSet[0x9c] = SBB;
	InstructionSet[0x9d] = SBB;
	InstructionSet[0x9e] = SBB;
	InstructionSet[0x9f] = SBB;
	InstructionSet[0xa0] = ANA;
	InstructionSet[0xa1] = ANA;
	InstructionSet[0xa2] = ANA;
	InstructionSet[0xa3] = ANA;
	InstructionSet[0xa4] = ANA;
	InstructionSet[0xa5] = ANA;
	InstructionSet[0xa6] = ANA;
	InstructionSet[0xa7] = ANA;
	InstructionSet[0xa8] = XRA;
	InstructionSet[0xa9] = XRA;
	InstructionSet[0xaa] = XRA;
	InstructionSet[0xab] = XRA;
	InstructionSet[0xac] = XRA;
	InstructionSet[0xad] = XRA;
	InstructionSet[0xae] = XRA;
	InstructionSet[0xaf] = XRA;
	InstructionSet[0xb0] = OR;
	InstructionSet[0xb1] = OR;
	InstructionSet[0xb2] = OR;
	InstructionSet[0xb3] = OR;
	InstructionSet[0xb4] = OR;
	InstructionSet[0xb5] = OR;
	InstructionSet[0xb6] = OR;
	InstructionSet[0xb7] = OR;
	InstructionSet[0xb8] = CMP;
	InstructionSet[0xb9] = CMP;
	InstructionSet[0xba] = CMP;
	InstructionSet[0xbb] = CMP;
	InstructionSet[0xbc] = CMP;
	InstructionSet[0xbd] = CMP;
	InstructionSet[0xbe] = CMP;
	InstructionSet[0xbf] = CMP;
	InstructionSet[0xc0] = RNZ;
	InstructionSet[0xc1] = POP;
	InstructionSet[0xc2] = JNZ;
	InstructionSet[0xc3] = JMP;
	InstructionSet[0xc4] = CNZ;
	InstructionSet[0xc5] = PUSH;
	InstructionSet[0xc6] = ADI;
	InstructionSet[0xc7] = RST;
	InstructionSet[0xc8] = RZ;
	InstructionSet[0xc9] = RET;
	InstructionSet[0xca] = JZ;
	InstructionSet[0xcb] = JMP;
	InstructionSet[0xcc] = CZ;
	InstructionSet[0xcd] = CALL;
	InstructionSet[0xce] = ACI;
	InstructionSet[0xcf] = RST;
	InstructionSet[0xd0] = RNC;
	InstructionSet[0xd1] = POP;
	InstructionSet[0xd2] = JNC;
	InstructionSet[0xd3] = OUT;
	InstructionSet[0xd4] = CNC;
	InstructionSet[0xd5] = PUSH;
	InstructionSet[0xd6] = SUI;
	InstructionSet[0xd7] = RST;
	InstructionSet[0xd8] = RC;
	InstructionSet[0xd9] = Fork;
	InstructionSet[0xda] = JC;
	InstructionSet[0xdb] = IN;
	InstructionSet[0xdc] = CC;
	InstructionSet[0xde] = SBI;
	InstructionSet[0xdf] = RST;
	InstructionSet[0xe0] = RPO;
	InstructionSet[0xe1] = POP;
	InstructionSet[0xe2] = JPO;
	InstructionSet[0xe3] = XTHL;
	InstructionSet[0xe4] = CPO;
	InstructionSet[0xe5] = PUSH;
	InstructionSet[0xe6] = ANI;
	InstructionSet[0xe7] = RST;
	InstructionSet[0xe8] = RPE;
	InstructionSet[0xe9] = PCHL;
	InstructionSet[0xea] = JPE;
	InstructionSet[0xeb] = XCHG;
	InstructionSet[0xec] = CPE;
	InstructionSet[0xee] = XRI;
	InstructionSet[0xef] = RST;
	InstructionSet[0xf0] = RP;
	InstructionSet[0xf1] = POP;
	InstructionSet[0xf2] = JP;
	InstructionSet[0xf3] = DI;
	InstructionSet[0xf4] = CP;
	InstructionSet[0xf5] = PUSH;
	InstructionSet[0xf6] = ORI;
	InstructionSet[0xf7] = RST;
	InstructionSet[0xf8] = RM;
	InstructionSet[0xf9] = SPHL;
	InstructionSet[0xfa] = JM;
	InstructionSet[0xfb] = EI;
	InstructionSet[0xfc] = CM;
	InstructionSet[0xfe] = CPI;
	InstructionSet[0xff] = RST;
}

/**Helpers**/

void CheckParity(char Reg, Process *Pinfo){
	int result = -1;
	if(Reg == 6){
		short MemPtr = (Pinfo->registers[R_H] << 8) | Pinfo->registers[R_L];
		result = GetMemory(MemPtr, Pinfo);
	}else{
		result = Pinfo->registers[Reg];
	}
	char x = result & 1;
	for(int i = 1; i < 8; i++){
		x ^= ((result >> i) & 1);
	}
	
	if(x == 0)
		Pinfo->registers[R_F] |= F_P;
	else
		Pinfo->registers[R_F] &= ~F_P;
}

void CheckCarryOut(unsigned short a, unsigned short b, Process *Pinfo, operation op){
	unsigned int r = op(a, b);
	if(r > 0xff){
		if(op == Subtraction)
			Pinfo->registers[R_F] &= ~F_C;
		else
			Pinfo->registers[R_F] |= F_C;
		
	}else{
		if(op == Subtraction)
			Pinfo->registers[R_F] |= F_C;
		else
			Pinfo->registers[R_F] &= ~F_C;
	}
}

void CheckAuxCarryOut(unsigned short a, unsigned short b, Process *Pinfo, operation op){
	unsigned int r = op(a & 0x0f, b & 0x0f);
	if(r > 0xf){
		Pinfo->registers[R_F] |= F_A;
	}else{
		Pinfo->registers[R_F] &= ~F_A;
	}
}

void CheckZero(int Reg, struct Process* Pinfo){
	int result = -1;
	if(Reg == 6){
		short MemPtr = (Pinfo->registers[R_H] << 8) | Pinfo->registers[R_L];
		result = GetMemory(MemPtr, Pinfo);
	}else{
		result = Pinfo->registers[Reg];
	}
	if(result == 0){
		Pinfo->registers[R_F] |= F_Z;
	}else{
		Pinfo->registers[R_F] &= ~F_Z;
	}
}

void CheckSign(int Reg, struct Process* Pinfo){
	int result = -1;
	if(Reg == 6){
		short MemPtr = (Pinfo->registers[R_H] << 8) | Pinfo->registers[R_L];
		result = GetMemory(MemPtr, Pinfo);
	}else{
		result = Pinfo->registers[Reg];
	}
	if(result & 0x80)
		Pinfo->registers[R_F] |= F_S;
	else
		Pinfo->registers[R_F] &= ~F_S;
}

unsigned int Addition(unsigned char a, unsigned char b){
	return (unsigned int) a + b;
}

unsigned int Subtraction(unsigned char a, unsigned char b){
	return (unsigned int) a + ((unsigned char)~b);
}
