#define CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct
{
	unsigned int inst_data;
	char type;
}Inst_info;

typedef struct
{
	unsigned int opcode;
	unsigned int rs;
	unsigned int rt;
	unsigned int rd;
	signed int imm;
	signed int Ext_imm;
	signed int Ext_imm_zero;
	unsigned int func;
	unsigned int shamt;
	unsigned int address;
}Decode_val;


typedef struct
{
	int reg_Data1;
	int reg_Data2;
	int Wreg;
	int Wdata;
	int result;
}Register_val;

typedef struct
{
	unsigned int RegDest;
	unsigned int ALUSrc;
	unsigned int ALUOp;
	unsigned int MemtoReg;
	unsigned int RegWrite;
	unsigned int MemRead;
	unsigned int MemWrite;
	unsigned int Jump;
	unsigned int Branch;
}Control_val;

typedef struct
{
	int Memory_Wdata;
	int Memory_Rdata;
	int Memory_address;
}Data_memory_val;

typedef struct
{
	int Jump_address;
	int Branch_address;
}Address_val;

unsigned int memory[0x400000] = { 0, };

int M_register[32] = { 0, };

unsigned int pc;
int M_index;
int R_num;
int I_num;
int J_num;
int Memory_access_num;
int Branch_inst_num;
int Branch_satisfy_num;
int Nop_num;
///////////////////////////////////Variable name

void Fetch(); //pc값을 증가시키고 해당 pc가 가르키는 instruction fetch
void Decode_func();
void Control_func();
void Evaluate();
void SignExtm();
void Execution();
void Memory_access();
void WriteBack();
void Set_zero();
void Jump_addr();
void Branch_addr();

///////////////////////////////////////
Inst_info Info;
Decode_val Decode;
Register_val Register;
Control_val Control;
Data_memory_val Memory;
Address_val Address;
////////////////////////////////////////

void main()
{
	FILE* fp = NULL;
	int inst_num = 0;
	int cycle_num = 0;
	char Want_Result;

	char* file_name;

	file_name = (char*)malloc(sizeof(char)*20);
	
	printf("Are you want to print All of Cycle results? Y or N\n");
	scanf("%c",&Want_Result);

	printf("Please Write File Name [ex) simple.bin]\n");
	scanf("%s",file_name);

	fp = fopen(file_name, "rb");
//	fp = fopen("simple3.bin", "rb");

	if (fp == NULL)
	{
		printf("no-file : %s\n", file_name);
		return;
	}

//////////////////////////////////////////파일 오픈

	Set_zero(); //구조체초기화
	M_register[31] = 0xFFFFFFFF;
	M_register[29] = 0x100000;

///////////////////////////////////////레지스터 초기화
	while (1)
	{
		fread(&memory[inst_num], sizeof(int), 1, fp);
		if (feof(fp))
			break;

		Info.inst_data = ((memory[inst_num] & 0xff) << 24) | (((memory[inst_num] & 0xff000000) >> 24) & 0xff) | ((memory[inst_num] & 0xff00) << 8) | (((memory[inst_num] & 0xff0000) >> 8) & 0xff00);

		memory[inst_num] = Info.inst_data;
		inst_num++;
	}

	fclose(fp);
///////////////////////////////////////////메모리에 instruction 저장


	while (pc != 0xffffffff)
	{
		Fetch();
		Decode_func();
		Execution();
		Memory_access();
		WriteBack();
			
		if(Want_Result == 'Y' || Want_Result == 'y')
		{
		printf("---------------cycle%d-----------------\n",cycle_num+1);
		printf(" changed val : 0x%08x\n", memory[M_index]);
		printf(" index : %x\n sp : %x\n s8 : %x\n",M_index, M_register[29], M_register[30]);
		printf(" type is %c \n",Info.type);
		printf(" opcode : %08x rs : %d rt: %d rd : %d imm : %08x shamt : %08x func : %08x address : %08x\n", Decode.opcode, Decode.rs, Decode.rt, Decode.rd, Decode.imm, Decode.shamt, Decode.func, Decode.address);
		printf(" R[rs] : %x R[rt] : %x R[rd] : %x\n",M_register[Decode.rs], M_register[Decode.rt], M_register[Decode.rd]);
		printf("R[2] : %d\n",M_register[2]);
		printf("---------------------------------------\n");
		}
	
		cycle_num++;
	}

/////////////////////////////////////////////////////pc가 ffffffff까지 반복

		printf("*****************************************************************\n");
		printf("<%s Result>\n",file_name);
		printf(" instruction num : %d\n", cycle_num);
		printf(" R_type : %d\n I_type : %d\n J_type : %d\n",R_num, I_num, J_num);
		printf(" Memory_Access : %d\n", Memory_access_num);
		printf(" Branch inst num : %d\n Branch satisfy num : %d\n", Branch_inst_num, Branch_satisfy_num);
		printf(" Nop num : %d\n",Nop_num);
		printf(" v0(R[2]) is %d\n", M_register[2]);
		printf("*****************************************************************\n");
		return;
}

///////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
void Fetch()
{
//	printf(" pc : %x\n",pc);
	M_index = pc / 4;
	pc += 4;
}
/////////////////////////////////////////////////////////////////instruction 한번 돌때 pc index 증가

void Decode_func()
{

	Decode.opcode = ((memory[M_index] & 0xfc000000) >> 26) & 0x3f;
	Decode.rs = (memory[M_index] & 0x03e00000) >> 21;
	Decode.rt = (memory[M_index] & 0x001f0000) >> 16;
	Decode.rd = (memory[M_index] & 0x0000f800) >> 11;
	Decode.imm = (memory[M_index] & 0xffff);
	Decode.shamt = (Decode.imm & 0x7ff) >>6;
	Decode.func = (Decode.imm & 0x3f);
	Decode.address = (memory[M_index] & 0x3ffffff);

	if(memory[M_index] != 0x00000000)
	{
		switch(Decode.opcode)
		{
			case 0x0: 
				Info.type = 'R';
				R_num++;
				break;
			case 0x2:
			case 0x3:
				Info.type = 'J';
				J_num++;
				break;
			default :
				Info.type = 'I';
				I_num++;
				break;
		}
	}
	else
		Nop_num++;

	Control_func();
	SignExtm();
	Branch_addr();
	Jump_addr();
}



////////////////////////////////////////////각 instruction으로 부터 계산
void Control_func()
{
	if (Decode.opcode == 0x0)
		Control.RegDest = 1;
	else 
		Control.RegDest = 0;

	if (Decode.opcode != 0x0 && Decode.opcode != 0x4 && Decode.opcode != 0x5)
		Control.ALUSrc = 1;
	else
		Control.ALUSrc = 0;

	if (Decode.opcode == 0x0)
		Control.ALUOp = 0;
	else
		Control.ALUOp = 1;
 
	if (Decode.opcode == 0x23)
	{
		Control.MemtoReg = 1;
		Control.MemRead = 1;
	}
	else
	{
		Control.MemtoReg = 0;
		Control.MemRead = 0;
	}

	if (Decode.opcode != 0x2b && Decode.opcode != 0x4 && Decode.opcode != 0x5 && Decode.opcode != 0x2 && !((Decode.opcode == 0x0 && (Decode.func == 0x8))))
		Control.RegWrite = 1;	
	
	else	
		Control.RegWrite = 0;
	    
	if (Decode.opcode == 0x2b)
		Control.MemWrite = 1;
	else
		Control.MemWrite = 0;

	if (Decode.opcode == 0x2 | Decode.opcode == 0x3)
		Control.Jump = 1;
	else
		Control.Jump = 0;
	
	if (Decode.opcode == 0x4 | Decode.opcode == 0x5)
		Control.Branch = 1;
	else
		Control.Branch = 0;
	
//	printf("Control signal part!!\n RegDst : %d // ALUSrc : %d // ALUOp : %d // MEMtoReg : %d // MemRead : %d // RegWrite : %d // MemWrite : %d // Jump : %d // Branch : %d\n",Control.RegDest, Control.ALUSrc, Control.ALUOp, Control.MemtoReg, Control.MemRead, Control.RegWrite, Control.MemWrite, Control.Jump, Control.Branch);
}
//////////////////////////////////////////////////////////////////////////////////
void SignExtm()
{
	int msb = Decode.imm >> 15;
	if (msb == 1)
		Decode.Ext_imm = 0xffff0000 | Decode.imm;
	else
		Decode.Ext_imm = 0x00000000 | Decode.imm;
	
	Decode.Ext_imm_zero = 0x00000000 | Decode.imm;
//	printf("SignExtmm part\n Ext_imm result : %x & %d // Ext_imm_zero result : %x & %d\n", Decode.Ext_imm, Decode.Ext_imm, Decode.Ext_imm_zero, Decode.Ext_imm_zero);
}
//////////////////////////////////////////////////////////////////////////////////////
void Evaluate()
{

	Register.reg_Data1 = M_register[Decode.rs];

	////////////////////////////////////////
	if (Control.RegDest == 1)
	{
		Register.Wreg = Decode.rd;
	}
	else
	{
		Register.Wreg = Decode.rt;
	}
	/////////////////////////////////////MUX1

	if (Control.ALUSrc == 1)
	{
		Register.reg_Data2 = Decode.Ext_imm;
	}
	else
	{
		Register.reg_Data2 = M_register[Decode.rt];
	}

	/////////////////////////////////////MUX2
}
///////////////////////////////////////////////////////////////////////////////////////
void Execution()
{
	Evaluate();

	if(Control.ALUOp == 0)
	{
		switch (Decode.func)
		{
			case 0x20 :
			case 0x21 :
				Register.result = Register.reg_Data1 + Register.reg_Data2;
				break;
			case 0x24 :
				Register.result = Register.reg_Data1 & Register.reg_Data2;
				break;
			case 0x08 :
				Register.result = Register.reg_Data1;
				pc = Register.result;
				break;
			case 0x09 :
				Register.result = Register.reg_Data1;
				M_register[Register.Wreg] = pc;
				pc = Register.result;
				break;

			case 0x27 :
				Register.result = ~(Register.reg_Data1 | Register.reg_Data2);
				break;
			case 0x25 :
				Register.result = (Register.reg_Data1 | Register.reg_Data2);
				break;
			case 0x2a :
			case 0x2b :
				Register.result = (Register.reg_Data1 < Register.reg_Data2)? 1:0;
				break;
			case 0x00 :
				Register.result = (Register.reg_Data2 << Decode.shamt);
				break;
			case 0x02 :
				Register.result = (Register.reg_Data2 >> Decode.shamt);
				break;
			case 0x22 :
			case 0x23 :
				Register.result = Register.reg_Data1 - Register.reg_Data2;
				break;
		}
	}
	else
	{
		switch (Decode.opcode)
		{	
			case 0x8 :
			case 0x9 :
				Register.result = Register.reg_Data1 + Decode.Ext_imm;
				break;
			case 0xc :
				Register.result = Register.reg_Data1 & Decode.Ext_imm_zero;
				break;
			case 0x4 :
 				Branch_inst_num++;
				if(M_register[Decode.rs] == M_register[Decode.rt])
					{
						pc = pc + Address.Branch_address;
						Branch_satisfy_num++;
					}
				else;
				
				break;

			case 0x5 :
				Branch_inst_num++;
				if(M_register[Decode.rs] != M_register[Decode.rt])
					{
						pc = pc + Address.Branch_address;
						Branch_satisfy_num++;
					}
				else;

				break;

			case 0x2 :
				if(Control.Jump == 1)
				{
					pc = Address.Jump_address;
				}
				break;
			case 0x3 :
				if(Control.Jump == 1)
				{
					M_register[31] = pc+4;
					pc = Address.Jump_address;
				}
				break;
			case 0x24 :
			case 0x25 :
				Register.result = Register.reg_Data1 + Decode.Ext_imm;
				break;
			case 0x30 :
			case 0x23 :
				Register.result = Register.reg_Data1 + Decode.Ext_imm;
				break;
			case 0xf :
				Register.result = (Decode.imm << 16);
				break;
			case 0xd :
				Register.result = Register.reg_Data1 | Decode.Ext_imm_zero;
				break;
			case 0xa :
			case 0xb :
				Register.result = (Register.reg_Data1 < Decode.Ext_imm) ? 1:0;
				break;
			case 0x28 :
				
				break;
			case 0x38 :
				break;
			case 0x29 :
				break;
			case 0x2b :
				Register.result = Register.reg_Data1 + Decode.Ext_imm;
				break;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
void Memory_access()
{
	if(Control.MemWrite == 0 && Control.MemRead == 0)
		return;
	else
	{
		Memory.Memory_address = Register.result;
		Memory.Memory_Wdata = M_register[Decode.rt];

		if(Control.MemWrite == 1)
			memory[Memory.Memory_address/4] = Memory.Memory_Wdata;
		if(Control.MemRead == 1)
			Memory.Memory_Rdata = memory[Memory.Memory_address/4];
	}
	Memory_access_num++;
}

void WriteBack()
{
	if(Control.MemWrite == 1 | Control.Jump == 1)
		return;
	
	else{
	if(Control.MemtoReg == 1)
	{
		Register.Wdata = Memory.Memory_Rdata;
	}
	else
	{
		Register.Wdata = Register.result;
	}	

	if(Control.RegWrite == 1)
	{
		if(Decode.opcode == 0x0 && Decode.func == 0x9);
		else
		{
		M_register[Register.Wreg] = Register.Wdata;
		}
	}

	else;
	}
}		
void Set_zero()
{
	memset(&Info, 0, sizeof(Inst_info));
	memset(&Decode, 0, sizeof(Decode_val));
	memset(&Control, 0, sizeof(Control_val));
	memset(&Register, 0, sizeof(Register_val));
	memset(&Address, 0, sizeof(Address_val));
}

void Jump_addr()
{
	Address.Jump_address = (pc & 0xf0000000) | (Decode.address << 2);
}


void Branch_addr()
{
	int msb = Decode.imm >> 15;

	if(msb == 1)
		Address.Branch_address = 0xfffc0000 | ( Decode.Ext_imm << 2);
	else
		Address.Branch_address = 0x00000000 | (Decode.Ext_imm <<2) ; 

}
