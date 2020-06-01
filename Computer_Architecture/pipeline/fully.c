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
	unsigned int Use_Rt;
	unsigned int Use_Rs;
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

typedef struct
{
	int cpc;
	int inst;//새로운 instruction
	int valid;
}IFID_latch_;

typedef struct
{
	int npc;
	int cpc;
	int inst;
	int rs_data;//v1
	int rt_data;//v2
	int Simm;
	int Zero_Simm;
	int Wreg;
	Decode_val val_Rdata;
	Control_val val_control;
}IDEX_latch_;

typedef struct
{
	int cpc;
	int npc2;
	int npc1;//pc4 & branch adress
	int inst;
	int ALU_result;//v3
	int rt_data;//v2
	int Wreg;
	Decode_val val_Rdata;
	Control_val val_control;
}EXMM_latch_;

typedef struct
{
	int cpc;
	int inst;
	int npc2;
	int npc1;
	int ALU_result;
	int M_data;
	int val_Wdata;
	int Wreg;
	Decode_val val_Rdata;
	Control_val val_control;
}MMWB_latch_;

typedef struct
{
	unsigned int tag;
	unsigned int valid;
	unsigned int dirty;
	unsigned int Sca;
	int data[16];
}Cache_line;

unsigned int memory[0x100000] = { 0, };

int M_register[32] = { 0, };

unsigned int pc;
int M_index;
int Memory_access_num;
int cycle_num;

int coldmiss_num;
int miss_num;
int hit_num;
int hit_rate;
/////////////////////////////////////////////
int oldest;
int Full;
///////////////////////////////////Variable name

void Fetch(); //pc값을 증가시키고 해당 pc가 가르키는 instruction fetch
void Decode_func(IFID_latch_ IFID_input_latch);
void Control_func();

void SignExtm();
void Execution(IDEX_latch_ IDEX_input_latch);
void Memory_access(EXMM_latch_ EXMM_input_latch);
void WriteBack(MMWB_latch_ MMWB_input_latch);
void Set_zero();
void Jump_addr();
void Branch_addr();
void Copy();
void debug();

//////////////////////////////////////////

int ReadMem(int addr);
int WriteMem(int addr, int value);

void StoreMem(int drain_addr, int index);
void StoreCache(int dest_addr, int index);
int FindOldest();

//////////////////////////////////////////
IFID_latch_ IFID_latch[2];
IDEX_latch_ IDEX_latch[2];
EXMM_latch_ EXMM_latch[2];
MMWB_latch_ MMWB_latch[2];
Cache_line cache[64];
/////////////////////////////////////////

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
//	int cycle_num = 0;
	char Want_Result;

	char* file_name;

	file_name = (char*)malloc(sizeof(char)*20);
	
	printf("Please Write File Name [ex) simple.bin]\n");
	scanf("%s",file_name);

	fp = fopen(file_name, "rb");
//	fp = fopen("input4.bin", "rb");
	if (fp == NULL)
	{
		printf("no-file : %s\n", file_name);
		return;
	}

//////////////////////////////////////////파일 오픈

	Set_zero(); //구조체초기화
	M_register[31] = 0xFFFFFFFF;
	M_register[29] = 0x400000;

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


	while(1)
	{
//		printf("---------------cycle%d-----------------\n",cycle_num+1);
		if(pc == 0xffffffff)
		{
		WriteBack(MMWB_latch[1]);
		Memory_access(EXMM_latch[1]);
	//	debug();
		Copy();
		WriteBack(MMWB_latch[1]);
	//	debug();
		cycle_num += 2;
		break;
		}

		Fetch();
		WriteBack(MMWB_latch[1]);
		Decode_func(IFID_latch[1]);
		Execution(IDEX_latch[1]);
		Memory_access(EXMM_latch[1]);
	//	debug();
		Copy();
		cycle_num++;

	}
	
/////////////////////////////////////////////////////pc가 ffffffff까지 반복

		printf("*****************************************************************\n");
		printf("<%s Result>\n",file_name);
		printf(" instruction num : %d\n", cycle_num);
		printf(" Memory_Access : %d\n", Memory_access_num);
		printf(" ColdMiss_num : %d\n Miss_num : %d\n Hit_num : %d\n",coldmiss_num, miss_num, hit_num);
		printf(" Hit Rate : %f\n", (double)hit_num / (double)(hit_num + miss_num) * 100);
		printf(" v0(R[2]) is %d\n", M_register[2]);
		printf("*****************************************************************\n");
		
		return;
}

///////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
void Fetch()
{
	IFID_latch[0].cpc = pc;
	IFID_latch[0].inst = ReadMem(pc);
	pc += 4;

}
/////////////////////////////////////////////////////////////////instruction 한번 돌때 pc index 증가

void Decode_func(IFID_latch_ IFID_input_latch)
{
	IDEX_latch[0].inst = IFID_input_latch.inst;
	IDEX_latch[0].val_Rdata.opcode = ((IFID_input_latch.inst & 0xfc000000) >> 26) & 0x3f;
	IDEX_latch[0].val_Rdata.rs = (IFID_input_latch.inst & 0x03e00000) >> 21;
	IDEX_latch[0].val_Rdata.rt = (IFID_input_latch.inst & 0x001f0000) >> 16;
	IDEX_latch[0].val_Rdata.rd = (IFID_input_latch.inst & 0x0000f800) >> 11;
	IDEX_latch[0].val_Rdata.imm = (IFID_input_latch.inst & 0xffff);
	IDEX_latch[0].val_Rdata.shamt = (IFID_input_latch.inst & 0x000007c0) >>6;
	IDEX_latch[0].val_Rdata.func = (IFID_input_latch.inst & 0x0000003f);
	IDEX_latch[0].val_Rdata.address = (IFID_input_latch.inst & 0x3ffffff);

	Control_func();
	IDEX_latch[0].val_control = Control;

	if(IDEX_latch[0].val_control.Jump == 1)
	{
		Jump_addr();
		pc = Address.Jump_address;
	}
	
	SignExtm();

	IDEX_latch[0].cpc = IFID_input_latch.cpc;
	IDEX_latch[0].rs_data = M_register[IDEX_latch[0].val_Rdata.rs];
	IDEX_latch[0].rt_data = M_register[IDEX_latch[0].val_Rdata.rt];

	if (IDEX_latch[0].val_control.RegDest == 1)
	{
		IDEX_latch[0].Wreg = IDEX_latch[0].val_Rdata.rd;
	}
	else
	{
		IDEX_latch[0].Wreg = IDEX_latch[0].val_Rdata.rt;
	}
}



////////////////////////////////////////////각 instruction으로 부터 계산
void Control_func()
{
	if (IDEX_latch[0].val_Rdata.opcode == 0x0)
		Control.RegDest = 1;
	else 
		Control.RegDest = 0;

	if (IDEX_latch[0].val_Rdata.opcode != 0x0 && IDEX_latch[0].val_Rdata.opcode != 0x4 && IDEX_latch[0].val_Rdata.opcode != 0x5)
		Control.ALUSrc = 1;
	else
		Control.ALUSrc = 0;

	if (IDEX_latch[0].val_Rdata.opcode == 0x0)
		Control.ALUOp = 0;
	else
		Control.ALUOp = 1;
 
	if (IDEX_latch[0].val_Rdata.opcode == 0x23)
	{
		Control.MemtoReg = 1;
		Control.MemRead = 1;
	}
	else
	{
		Control.MemtoReg = 0;
		Control.MemRead = 0;
	}

	if (IDEX_latch[0].val_Rdata.opcode != 0x2b && IDEX_latch[0].val_Rdata.opcode != 0x4 && IDEX_latch[0].val_Rdata.opcode != 0x5 && IDEX_latch[0].val_Rdata.opcode != 0x2 && !((IDEX_latch[0].val_Rdata.opcode == 0x0 && (IDEX_latch[0].val_Rdata.func == 0x8))))
		Control.RegWrite = 1;	
	
	else	
		Control.RegWrite = 0;
	    
	if (IDEX_latch[0].val_Rdata.opcode == 0x2b)
		Control.MemWrite = 1;
	else
		Control.MemWrite = 0;

	if (IDEX_latch[0].val_Rdata.opcode == 0x2 | IDEX_latch[0].val_Rdata.opcode == 0x3)
	{
		Control.Jump = 1;
	}
	else
	{
		Control.Jump = 0;
	}
	
	if (IDEX_latch[0].val_Rdata.opcode == 0x4 | IDEX_latch[0].val_Rdata.opcode == 0x5)
	{
		Control.Branch = 1;
	}
	else
	{
		Control.Branch = 0;
	}

	switch(IDEX_latch[0].val_Rdata.opcode)
	{
		case 0x4:
		case 0x5:
		case 0x2b:
			Control.Use_Rt = 1;
			break;
		case 0x0:
			if(IDEX_latch[0].val_Rdata.func == 0x8)
				Control.Use_Rt = 0;
			else
				Control.Use_Rt = 1;
			break;

		default:
			Control.Use_Rt = 0;
			break;
	}

	switch(IDEX_latch[0].val_Rdata.opcode)
	{
		case 0x2:
		case 0x3:
		case 0xf:
			Control.Use_Rs = 0;
			break;
		case 0x0:
			if(IDEX_latch[0].val_Rdata.func == 0x0 || IDEX_latch[0].val_Rdata.func == 0x02)
				Control.Use_Rs = 0;
			else
				Control.Use_Rs = 1;
			break;
		default:
			Control.Use_Rs = 1;
			break;
	}


}
//////////////////////////////////////////////////////////////////////////////////
void SignExtm()
{
	int msb = IDEX_latch[0].val_Rdata.imm >> 15;
	if (msb == 1)
		IDEX_latch[0].Simm = 0xffff0000 | IDEX_latch[0].val_Rdata.imm;
	else
		IDEX_latch[0].Simm = 0x00000000 | IDEX_latch[0].val_Rdata.imm;
	
	IDEX_latch[0].Zero_Simm = 0x00000000 | IDEX_latch[0].val_Rdata.imm;
}
//////////////////////////////////////////////////////////////////////////////////////

void Execution(IDEX_latch_ IDEX_input_latch)
{
	int alu_data2;
	EXMM_latch[0].inst = IDEX_input_latch.inst;

	Branch_addr();
	if (IDEX_input_latch.val_control.ALUSrc == 1)
	{
		alu_data2 = IDEX_input_latch.Simm;
	}
	else
	{
		alu_data2 = M_register[IDEX_input_latch.val_Rdata.rt];
	}
////////////////////////////////////////////////////////////////////////////////////
	if((IDEX_input_latch.val_Rdata.rs == EXMM_latch[1].Wreg) && (IDEX_input_latch.val_Rdata.rs != 0) && (IDEX_input_latch.val_control.Use_Rs == 1) && EXMM_latch[1].val_control.RegWrite == 1)
	{
		IDEX_input_latch.rs_data = EXMM_latch[1].ALU_result;
	}
		
	else if((IDEX_input_latch.val_Rdata.rs == MMWB_latch[1].Wreg) && (IDEX_input_latch.val_Rdata.rs != 0) && (IDEX_input_latch.val_control.Use_Rs == 1) && MMWB_latch[1].val_control.RegWrite == 1)
	{
		IDEX_input_latch.rs_data = MMWB_latch[1].val_Wdata;
	}
 
	
	if((IDEX_input_latch.val_Rdata.rt == EXMM_latch[1].Wreg) && (IDEX_input_latch.val_Rdata.rt != 0) && (IDEX_input_latch.val_control.Use_Rt == 1) && EXMM_latch[1].val_control.RegWrite == 1)
	{
		alu_data2 = EXMM_latch[1].ALU_result;
		IDEX_input_latch.rt_data = alu_data2;
	}
	

	else if((IDEX_input_latch.val_Rdata.rt == MMWB_latch[1].Wreg) && (IDEX_input_latch.val_Rdata.rt != 0) && (IDEX_input_latch.val_control.Use_Rt == 1)&& MMWB_latch[1].val_control.RegWrite == 1)
	{
		alu_data2 = MMWB_latch[1].val_Wdata;
		IDEX_input_latch.rt_data = alu_data2;
	}
//////////////////////////////////////////////////////////////data dependancy

	if(IDEX_input_latch.val_control.ALUOp == 0)
	{
		switch (IDEX_input_latch.val_Rdata.func)
		{
			case 0x20 :
			case 0x21 :
				EXMM_latch[0].ALU_result = IDEX_input_latch.rs_data + alu_data2;
				break;
			case 0x24 :
				EXMM_latch[0].ALU_result = IDEX_input_latch.rs_data & alu_data2;
				break;
			case 0x08 :
				EXMM_latch[0].ALU_result = IDEX_input_latch.rs_data;
				pc = EXMM_latch[0].ALU_result;
				memset(&IFID_latch[0], 0, sizeof(IFID_latch_));
				memset(&IDEX_latch[0], 0, sizeof(IDEX_latch_));
				break;
			case 0x27 :
				EXMM_latch[0].ALU_result = ~(IDEX_input_latch.rs_data | alu_data2);
				break;
			case 0x25 :
				EXMM_latch[0].ALU_result = (IDEX_input_latch.rs_data | alu_data2);
				break;
			case 0x2a :
			case 0x2b :
				EXMM_latch[0].ALU_result = (IDEX_input_latch.rs_data < alu_data2)? 1:0;
				break;
			case 0x00 :
				EXMM_latch[0].ALU_result = (alu_data2 << IDEX_input_latch.val_Rdata.shamt);
				break;
			case 0x02 :
				EXMM_latch[0].ALU_result = (alu_data2 >> IDEX_input_latch.val_Rdata.shamt);
				break;
			case 0x22 :
			case 0x23 :
				EXMM_latch[0].ALU_result = IDEX_input_latch.rs_data - alu_data2;
				break;
		}
	}
	else
	{
		switch (IDEX_input_latch.val_Rdata.opcode)
		{	
			case 0x8 :
			case 0x9 :
				EXMM_latch[0].ALU_result = IDEX_input_latch.rs_data + IDEX_input_latch.Simm;
				break;
			case 0xc :
				EXMM_latch[0].ALU_result = IDEX_input_latch.rs_data & IDEX_input_latch.Zero_Simm;
				break;
			
			case 0x4 :
 				if(IDEX_input_latch.rs_data == alu_data2)
				{
						pc = IDEX_input_latch.cpc + 4 + Address.Branch_address;
						memset(IFID_latch, 0, sizeof(IFID_latch_));
						memset(IDEX_latch, 0, sizeof(IDEX_latch_));
				}
				break;

			case 0x5 :
				if(IDEX_input_latch.rs_data != alu_data2)
					{
						pc = IDEX_input_latch.cpc + 4 + Address.Branch_address;
						memset(IFID_latch, 0, sizeof(IFID_latch_));
						memset(IDEX_latch, 0, sizeof(IDEX_latch_));

					}
			
				break;

			case 0x2 :
				break;
			case 0x3 :
					M_register[31] = IDEX_input_latch.cpc + 8;
				break;

			case 0x24 :
			case 0x25 :
				EXMM_latch[0].ALU_result = IDEX_input_latch.rs_data + IDEX_input_latch.Simm;
				break;
			case 0x30 :
			case 0x23 :
				EXMM_latch[0].ALU_result = IDEX_input_latch.rs_data + IDEX_input_latch.Simm;
				break;
			case 0xf :
				EXMM_latch[0].ALU_result = (IDEX_input_latch.val_Rdata.imm << 16);
				break;
			case 0xd :
				EXMM_latch[0].ALU_result = IDEX_input_latch.rs_data | IDEX_input_latch.Zero_Simm;
				break;
			case 0xa :
			case 0xb :
				EXMM_latch[0].ALU_result = (IDEX_input_latch.rs_data < IDEX_input_latch.Simm) ? 1:0;
				break;
			case 0x2b :
				EXMM_latch[0].ALU_result = IDEX_input_latch.rs_data + IDEX_input_latch.Simm;

				break;
		}
	}
	EXMM_latch[0].rt_data = IDEX_input_latch.rt_data;
	EXMM_latch[0].val_Rdata = IDEX_input_latch.val_Rdata;
	EXMM_latch[0].val_control = IDEX_input_latch.val_control;
	EXMM_latch[0].Wreg = IDEX_input_latch.Wreg;
	EXMM_latch[0].cpc = IDEX_input_latch.cpc;

		

}
////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
void Memory_access(EXMM_latch_ EXMM_input_latch)
{
	MMWB_latch[0].inst = EXMM_input_latch.inst;
	MMWB_latch[0].cpc = EXMM_input_latch.cpc;

	Memory.Memory_address = EXMM_input_latch.ALU_result;
	Memory.Memory_Wdata = EXMM_input_latch.rt_data;

	if(EXMM_input_latch.val_control.MemWrite == 1)
	{
		WriteMem(Memory.Memory_address, Memory.Memory_Wdata);
	}
	if(EXMM_input_latch.val_control.MemRead == 1)
	{
		Memory.Memory_Rdata = ReadMem(Memory.Memory_address);
	}

	if(EXMM_input_latch.val_control.MemtoReg == 1)
		MMWB_latch[0].val_Wdata = Memory.Memory_Rdata;
	else
		MMWB_latch[0].val_Wdata = EXMM_input_latch.ALU_result;

	MMWB_latch[0].ALU_result = EXMM_input_latch.ALU_result;
	MMWB_latch[0].val_Rdata = EXMM_input_latch.val_Rdata;
	MMWB_latch[0].val_control = EXMM_input_latch.val_control;
	MMWB_latch[0].Wreg = EXMM_input_latch.Wreg;

}

void WriteBack(MMWB_latch_ MMWB_input_latch)
{
	if(MMWB_input_latch.val_control.RegWrite == 1)
	{
		M_register[MMWB_input_latch.Wreg] = MMWB_input_latch.val_Wdata;
	}
	
	
}		
void Set_zero()
{
	memset(&Info, 0, sizeof(Inst_info));
	memset(&Decode, 0, sizeof(Decode_val));
	memset(&Control, 0, sizeof(Control_val));
	memset(&Register, 0, sizeof(Register_val));
	memset(&Address, 0, sizeof(Address_val));
	memset(IFID_latch, 0, sizeof(IFID_latch_));
	memset(IDEX_latch, 0, sizeof(IDEX_latch_));
	memset(EXMM_latch, 0, sizeof(EXMM_latch_));
	memset(MMWB_latch, 0, sizeof(MMWB_latch_));
	memset(cache, 0, sizeof(Cache_line));
}

void Jump_addr()
{
	Address.Jump_address = ((IDEX_latch[0].cpc + 4) & 0xf0000000) | (IDEX_latch[0].val_Rdata.address << 2);
//	pc = Address.Jump_address;
}


void Branch_addr()
{
	int msb = IDEX_latch[1].val_Rdata.imm >> 15;

	if(msb == 1)
		Address.Branch_address = 0xfffc0000 | (IDEX_latch[1].Simm << 2);
	else
		Address.Branch_address = 0x00000000 | (IDEX_latch[1].Simm << 2) ; 

}

void Copy()
{
	IFID_latch[1] = IFID_latch[0];
	memset(&IFID_latch[0], 0, sizeof(IFID_latch[0]));

	IDEX_latch[1] = IDEX_latch[0];
	memset(&IDEX_latch[0], 0, sizeof(IDEX_latch[0]));

	EXMM_latch[1] = EXMM_latch[0];
	memset(&EXMM_latch[0], 0, sizeof(EXMM_latch[0]));

	MMWB_latch[1] = MMWB_latch[0];
	memset(&MMWB_latch[0], 0, sizeof(MMWB_latch[0]));
}

int ReadMem(int addr)
{
	unsigned int tag = (addr & 0xffffffc0) >> 6;
	unsigned int offset = (addr & 0x0000003f);
	int dest_addr = (addr & 0xffffffc0);
//	int drain_addr = (cache[index].tag << 6); //tag + index
	
	int index;
	int overwrite;

	for(index = 0; index < 64; index++)
	{
		if(cache[index].valid == 0)
		{
			StoreCache(dest_addr, index);
			cache[index].tag = tag;
			cache[index].valid = 1;
			cache[index].dirty = 0;
			
			coldmiss_num++;
			miss_num++;
			Memory_access_num++;
			Full++;

			return cache[index].data[offset/4];
		}

		if(cache[index].tag == tag)
		{
			hit_num++;
			
			if(cache[index].Sca != 1)
				cache[index].Sca++;
			
			return cache[index].data[offset/4];
		}
	}
		if(Full == 64)
		{
			overwrite = FindOldest();
			
			StoreCache(dest_addr, overwrite);
			cache[overwrite].tag = tag;
			cache[overwrite].valid = 1;
			cache[overwrite].dirty = 0;

			miss_num++;
			Memory_access_num++;

			return cache[overwrite].data[offset/4];
		}

//	}
}

int WriteMem(int addr, int value)
{
	unsigned int tag = (addr & 0xffffffc0) >> 6;
	unsigned int offset = (addr & 0x0000003f);
	int dest_addr = (addr & 0xffffffc0);
//	int drain_addr = (cache[index].tag << 6); //tag + index

	int index;
	int overwrite;

	for(index = 0; index < 64; index++)
	{
		if(cache[index].valid == 0)
		{
			StoreCache(dest_addr, index);
			
			cache[index].data[offset/4] = value;
			cache[index].tag = tag;
			cache[index].valid = 1;
			cache[index].dirty = 1;
			
			coldmiss_num++;
			miss_num++;
			Memory_access_num++;
			Full++;

			return 0;
		}

		if(cache[index].tag == tag)
		{
			cache[index].data[offset/4] = value;
			cache[index].dirty = 1;
			hit_num++;
			
			if(cache[index].Sca != 1)
				cache[index].Sca++;
			
			return 0;
		}
	}
		if(Full == 64)
		{
			overwrite = FindOldest();
			
			StoreCache(dest_addr, overwrite);

			cache[overwrite].data[offset/4] = value;
			cache[overwrite].tag = tag;
			cache[overwrite].valid = 1;
			cache[overwrite].dirty = 1;

			miss_num++;
			Memory_access_num++;

			return 0;
		}
//	}
}
void StoreMem(int drain_addr, int index)
{
	for(int i=0; i<16; i++)
	{
		memory[(drain_addr/4) + i] = cache[index].data[i];
	}

}
void StoreCache(int dest_addr, int index)
{	for(int i=0; i<16; i++)
	{
		cache[index].data[i] = memory[(dest_addr/4) + i];
	}
}

int FindOldest()
{
	int old;
	
	if(cache[oldest].Sca == 0)
	{
		int drain_addr = (cache[oldest].tag << 6);
		
		if(cache[oldest].dirty == 1)
		{
			StoreMem(drain_addr, oldest);
		}
		old = oldest;

		if(oldest < Full-1)
			oldest++;
		else
			oldest = 0;

		return old;
	}
	
	else if(cache[oldest].Sca == 1)
	{
		cache[oldest].Sca = 0;
		hit_num++;

		if(oldest < Full-1)
			oldest++;
		else
			oldest = 0;

		FindOldest();
	}
}


	void debug()
	{
	        printf("---------------clock %d-------------------------\n",cycle_num+1);
		printf("(Fetch) pc : %x inst : %x\n",IFID_latch[0].cpc, IFID_latch[0].inst);
		printf("(Decode) inst : %x\n",IDEX_latch[0].inst);
		printf("(Execute) inst : %x\n",EXMM_latch[0].inst);
		printf("(Memory) inst : %x\n", MMWB_latch[0].inst);
	        printf("(Writeback) inst : %x\n", MMWB_latch[1].inst);
	}

