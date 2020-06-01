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
	int branch_target[100];
	int saved_pc[100];
	int index;
	int index2;
	int valid;
}Pc_history_val;

typedef struct
{
	int jump_target[10];
	int saved_pc[10];
	int index;
	int index2;
	int valid;
}Jump_pc_history_val;

typedef struct
{
	int ghr;
	int pattern_table[0x20000];
	int pattern_index;
	int IsitTaken;
	int br_taken;
}Global_history;

typedef struct
{
	int cpc;
	int npc;
	int inst;//새로운 instruction
	int br_taken;
	int pattern_index;
}IFID_latch_;

typedef struct
{
	int npc;
	int cpc;
	int inst;
	int br_taken;
	int pattern_index;
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
	int ALU_result;
	int val_Wdata;
	int Wreg;
	Decode_val val_Rdata;
	Control_val val_control;
}MMWB_latch_;



unsigned int memory[0x400000] = { 0, };

int M_register[32] = { 0, };

unsigned int pc;
int M_index;
int R_num;
int I_num;
int J_num;
int Memory_access_num;
int Branch_inst_num;
int Branch_Taken_num;
int Branch_notTaken_num;
int Nop_num;
int Final_inst_num;
//int branch_target;
int hit;
int miss;
int taken;
int B_hit;
int Current_pc;
///////////////////////////////////Variable name

void Fetch(IFID_latch_* IFID_input_latch); //pc값을 증가시키고 해당 pc가 가르키는 instruction fetch
void Decode_func(IFID_latch_* IFID_input_latch, IDEX_latch_* IDEX_output_latch);
void Control_func(IDEX_latch_* IDEX_output_latch);
void SignExtm(IDEX_latch_* IDEX_output_latch);

void Execution(IDEX_latch_* IDEX_input_latch, EXMM_latch_* EXMM_output_latch, EXMM_latch_* EXMM_input_latch, MMWB_latch_* MMWB_input_latch);

void Memory_access(EXMM_latch_* EXMM_input_latch,MMWB_latch_* MMWB_output_latch);
void WriteBack(MMWB_latch_* MMWB_input_latch);
void Set_zero();
void Jump_addr(IDEX_latch_* IDEX_output_latch);
void Branch_addr(IDEX_latch_* IDEX_input_latch);
void Copy();
void debug();
void Pc_check(IDEX_latch_* IDEX_input_latch);
void Jump_pc_check(IDEX_latch_* IDEX_output_latch);
void IF_Pc_check(IFID_latch_* IFID_input_latch);
void initial_table();
void IF_jump_Pc_check();
//////////////////////////////////////////
IFID_latch_ IFID_latch[2];
IDEX_latch_ IDEX_latch[2];
EXMM_latch_ EXMM_latch[2];
MMWB_latch_ MMWB_latch[2];
/////////////////////////////////////////
IFID_latch_* IFID_input_latch = &IFID_latch[1];
IFID_latch_* IFID_output_latch = &IFID_latch[0];
IDEX_latch_* IDEX_input_latch = &IDEX_latch[1];
IDEX_latch_* IDEX_output_latch = &IDEX_latch[0];
EXMM_latch_* EXMM_input_latch = &EXMM_latch[1];
EXMM_latch_* EXMM_output_latch = &EXMM_latch[0];
MMWB_latch_* MMWB_input_latch = &MMWB_latch[1];
MMWB_latch_* MMWB_output_latch = &MMWB_latch[0];
/////////////////////////////////////////
Inst_info Info;
Decode_val Decode;
Register_val Register;
Control_val Control;
Data_memory_val Memory;
Address_val Address;
Pc_history_val Pc_history;
Jump_pc_history_val Jump_pc_history;
Global_history G_history;
////////////////////////////////////////
int cycle = 0;

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

	initial_table();
	while(1)
	{
	//	printf("----------cycle : %d-------------\n",cycle);
		if(pc == 0xffffffff)
		{
		WriteBack(MMWB_input_latch);
		Memory_access(EXMM_input_latch, MMWB_output_latch);
	//	debug();
		Copy();
		WriteBack(MMWB_input_latch);
	//	debug();
		cycle += 2;
		break;
		}
		
		WriteBack(MMWB_input_latch);
		Fetch(IFID_input_latch);
		Decode_func(IFID_input_latch, IDEX_output_latch);
		Execution(IDEX_input_latch, EXMM_output_latch,EXMM_input_latch, MMWB_input_latch);
		Memory_access(EXMM_input_latch,MMWB_output_latch);
	//	debug();
		Copy();
		cycle++;
	}

/////////////////////////////////////////////////////pc가 ffffffff까지 반복

		printf("*****************************************************************\n");
		printf("<%s Result>\n",file_name);
		printf(" Clock Cycle : %d\n", cycle);
		printf(" instruction num : %d\n", Final_inst_num);
		printf(" R_type : %d\n I_type : %d\n J_type : %d\n",R_num, I_num, J_num);
		printf(" Memory_Access : %d\n", Memory_access_num);
		printf(" Branch inst num : %d\n Branch Taken num : %d\n Branch notTaken num : %d\n", Branch_inst_num, Branch_Taken_num, Branch_notTaken_num);
		printf(" hit : %d miss : %d\n",hit, miss);
		printf(" HIT RATE : %f\n", (double)hit/(hit+miss)*100);
		printf(" v0(R[2]) is %d\n", M_register[2]);
		printf("*****************************************************************\n");
		return;
}

///////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
void Fetch(IFID_latch_* IFID_input_latch)
{	
	M_index = pc / 4;
	IFID_output_latch->inst = memory[M_index];

	IFID_output_latch->cpc = pc;
	Current_pc = pc;
	G_history.pattern_index = G_history.ghr ^ pc;

	if(G_history.pattern_table[G_history.pattern_index/4] >= 2)
		G_history.IsitTaken = 1;
	else
		G_history.IsitTaken = 0;
	
	IF_Pc_check(IFID_output_latch);
	G_history.br_taken = B_hit & G_history.IsitTaken;
	
	IFID_output_latch->br_taken = G_history.br_taken;
	IFID_output_latch->pattern_index = G_history.pattern_index;
	
	if(IFID_output_latch->br_taken == 1)
		pc = IFID_output_latch->npc;
	else
		pc += 4;

	IF_jump_Pc_check();
	
	
	/*printf("(fetch)\n");
	printf(" pc : %x\n", IFID_input_latch->pc4);
	printf(" instruction : %x\n", IFID_input_latch->inst);
	printf("*****************************************************************\n");
	*/
}
	

/////////////////////////////////////////////////////////////////instruction 한번 돌때 pc index 증가

void Decode_func(IFID_latch_* IFID_input_latch, IDEX_latch_* IDEX_output_latch)
{
	IDEX_output_latch->cpc = IFID_input_latch->cpc;
	IDEX_output_latch->inst = IFID_input_latch->inst;
	IDEX_output_latch->br_taken = IFID_input_latch->br_taken;
	IDEX_output_latch->pattern_index = IFID_input_latch->pattern_index;


	IDEX_output_latch->val_Rdata.opcode = ((IFID_input_latch->inst & 0xfc000000) >> 26) & 0x3f;
	IDEX_output_latch->val_Rdata.rs = (IFID_input_latch->inst & 0x03e00000) >> 21;
	IDEX_output_latch->val_Rdata.rt = (IFID_input_latch->inst & 0x001f0000) >> 16;
	IDEX_output_latch->val_Rdata.rd = (IFID_input_latch->inst & 0x0000f800) >> 11;
	IDEX_output_latch->val_Rdata.imm = (IFID_input_latch->inst & 0xffff);
	IDEX_output_latch->val_Rdata.shamt = (IFID_input_latch->inst & 0x000007c0) >>6;
	IDEX_output_latch->val_Rdata.func = (IFID_input_latch->inst & 0x0000003f);
	IDEX_output_latch->val_Rdata.address = (IFID_input_latch->inst & 0x3ffffff);

	Control_func(IDEX_output_latch);

	if(IDEX_output_latch->val_control.Jump == 1)
	{
		Jump_addr(IDEX_output_latch);
		Jump_pc_check(IDEX_output_latch);
		
		if(Jump_pc_history.valid == 1);
		else
		{
			pc = Address.Jump_address;
			Jump_pc_history.jump_target[Jump_pc_history.index2] = pc;
			Jump_pc_history.saved_pc[Jump_pc_history.index2] = IDEX_output_latch->cpc;
			Jump_pc_history.index2 += 1;
		}

	}
	
	SignExtm(IDEX_output_latch);

	IDEX_output_latch->rs_data = M_register[IDEX_output_latch->val_Rdata.rs];
	IDEX_output_latch->rt_data = M_register[IDEX_output_latch->val_Rdata.rt];

	if (IDEX_output_latch->val_control.RegDest == 1)
	{
		IDEX_output_latch->Wreg = IDEX_output_latch->val_Rdata.rd;
	}
	else
	{
		IDEX_output_latch->Wreg = IDEX_output_latch->val_Rdata.rt;
	}

/*
	printf("(decode)\n");
	printf(" pc : %x\n",IDEX_output_latch->pc4);
	printf(" instruction : %x\n", IDEX_output_latch->inst);
	printf(" rs_data = %d\n rt_data= %d\n", IDEX_output_latch->rs_data, IDEX_output_latch->rt_data);
	printf(" sp = %x\n", M_register[29]);
	printf(" Wreg = %d\n",IDEX_output_latch->Wreg);
	printf("*****************************************************************\n");
*/

}

////////////////////////////////////////////각 instruction으로 부터 계산
void Control_func(IDEX_latch_* IDEX_output_latch)
{
	if (IDEX_output_latch->val_Rdata.opcode == 0x0)
		IDEX_output_latch->val_control.RegDest = 1;
	else 
		IDEX_output_latch->val_control.RegDest = 0;

	if (IDEX_output_latch->val_Rdata.opcode != 0x0 && IDEX_output_latch->val_Rdata.opcode != 0x4 && IDEX_output_latch->val_Rdata.opcode != 0x5)
		IDEX_output_latch->val_control.ALUSrc = 1;
	else
		IDEX_output_latch->val_control.ALUSrc = 0;

	if (IDEX_output_latch->val_Rdata.opcode == 0x0)
		IDEX_output_latch->val_control.ALUOp = 0;
	else
		IDEX_output_latch->val_control.ALUOp = 1;
 
	if (IDEX_output_latch->val_Rdata.opcode == 0x23)
	{
		IDEX_output_latch->val_control.MemtoReg = 1;
		IDEX_output_latch->val_control.MemRead = 1;
	}
	else
	{
		IDEX_output_latch->val_control.MemtoReg = 0;
		IDEX_output_latch->val_control.MemRead = 0;
	}

	if (IDEX_output_latch->val_Rdata.opcode != 0x2b && IDEX_output_latch->val_Rdata.opcode != 0x4 && IDEX_output_latch->val_Rdata.opcode != 0x5 && IDEX_output_latch->val_Rdata.opcode != 0x2 && !((IDEX_output_latch->val_Rdata.opcode == 0x0 && (IDEX_output_latch->val_Rdata.func == 0x8))))
		IDEX_output_latch->val_control.RegWrite = 1;	
	
	else	
		IDEX_output_latch->val_control.RegWrite = 0;
	    
	if (IDEX_output_latch->val_Rdata.opcode == 0x2b)
		IDEX_output_latch->val_control.MemWrite = 1;
	else
		IDEX_output_latch->val_control.MemWrite = 0;

	if (IDEX_output_latch->val_Rdata.opcode == 0x2 | IDEX_output_latch->val_Rdata.opcode == 0x3)
	{
		IDEX_output_latch->val_control.Jump = 1;
	}
	else
	{
		IDEX_output_latch->val_control.Jump = 0;
	}
	
	if (IDEX_output_latch->val_Rdata.opcode == 0x4 | IDEX_output_latch->val_Rdata.opcode == 0x5)
	{
		IDEX_output_latch->val_control.Branch = 1;
	}
	else
	{
		IDEX_output_latch->val_control.Branch = 0;
	}
	
	switch(IDEX_output_latch->val_Rdata.opcode)
	{
		case 0x4:
		case 0x5:
		case 0x2b:
			IDEX_output_latch->val_control.Use_Rt = 1;
			break;
		case 0x0:
			if(IDEX_output_latch->val_Rdata.func == 0x8)
				IDEX_output_latch->val_control.Use_Rt = 0;
			else
				IDEX_output_latch->val_control.Use_Rt = 1;
			break;
		default:
			IDEX_output_latch->val_control.Use_Rt = 0;
			break;

	}

	switch(IDEX_output_latch->val_Rdata.opcode)
	{
		case 0x2:
		case 0x3:
		case 0xf:
			IDEX_output_latch->val_control.Use_Rs = 0;
			break;
		case 0x0:
			if(IDEX_output_latch->val_Rdata.func == 0x0 || IDEX_output_latch->val_Rdata.func == 0x02)
				IDEX_output_latch->val_control.Use_Rs = 0;
			
			else
				IDEX_output_latch->val_control.Use_Rs = 1;
			
			break;
		default:
			IDEX_output_latch->val_control.Use_Rs = 1;
			break;
	}
}
//////////////////////////////////////////////////////////////////////////////////
void SignExtm(IDEX_latch_* IDEX_output_latch)
{
	int msb = IDEX_output_latch->val_Rdata.imm >> 15;
	if (msb == 1)
		IDEX_output_latch->Simm = 0xffff0000 | IDEX_output_latch->val_Rdata.imm;
	else
		IDEX_output_latch->Simm = 0x00000000 | IDEX_output_latch->val_Rdata.imm;
	
	IDEX_output_latch->Zero_Simm = 0x00000000 | IDEX_output_latch->val_Rdata.imm;

}
//////////////////////////////////////////////////////////////////////////////////////

void Execution(IDEX_latch_* IDEX_input_latch, EXMM_latch_* EXMM_output_latch, EXMM_latch_* EXMM_input_latch, MMWB_latch_* MMWB_input_latch)
{
	int alu_data2;
	EXMM_output_latch->inst = IDEX_input_latch->inst;
	EXMM_output_latch->cpc = IDEX_input_latch->cpc;

	if (IDEX_input_latch->val_control.ALUSrc == 1)
	{
		alu_data2 = IDEX_input_latch->Simm;
	}
	else
	{
		alu_data2 = M_register[IDEX_input_latch->val_Rdata.rt];
	}
////////////////////////////////////////////////////////////////////////////////////
	if((IDEX_input_latch->val_Rdata.rs == EXMM_input_latch->Wreg) && IDEX_input_latch->val_Rdata.rs != 0 && (IDEX_input_latch->val_control.Use_Rs == 1) && EXMM_input_latch->val_control.RegWrite == 1)
	{
		IDEX_input_latch->rs_data = EXMM_input_latch->ALU_result;
	}
		
	else if((IDEX_input_latch->val_Rdata.rs == MMWB_input_latch->Wreg) && IDEX_input_latch->val_Rdata.rs != 0 && (IDEX_input_latch->val_control.Use_Rs == 1) && MMWB_input_latch->val_control.RegWrite == 1)
	{
		IDEX_input_latch->rs_data = MMWB_input_latch->val_Wdata;
	}

	if((IDEX_input_latch->val_Rdata.rt == EXMM_input_latch->Wreg) &&IDEX_input_latch->val_Rdata.rt != 0 && (IDEX_input_latch->val_control.Use_Rt == 1) && EXMM_input_latch->val_control.RegWrite == 1)
	{
		alu_data2 = EXMM_input_latch->ALU_result;
		IDEX_input_latch->rt_data = alu_data2;
	}
	
	else if((IDEX_input_latch->val_Rdata.rt == MMWB_input_latch->Wreg) &&IDEX_input_latch->val_Rdata.rt != 0 && (IDEX_input_latch->val_control.Use_Rt == 1) && MMWB_input_latch->val_control.RegWrite == 1)
	{
		alu_data2 = MMWB_input_latch->val_Wdata;
		IDEX_input_latch->rt_data = alu_data2;
	}
//////////////////////////////////////////////////////////////data dependancy


	if(IDEX_input_latch->val_control.ALUOp == 0)
	{
		switch (IDEX_input_latch->val_Rdata.func)
		{
			case 0x20 :
			case 0x21 :
				EXMM_output_latch->ALU_result = IDEX_input_latch->rs_data + alu_data2;
				break;
			case 0x24 :
				EXMM_output_latch->ALU_result = IDEX_input_latch->rs_data & alu_data2;
				break;
			case 0x08 :
				EXMM_output_latch->ALU_result = IDEX_input_latch->rs_data;
				pc = EXMM_output_latch->ALU_result;
				memset(&IFID_latch[0],0,sizeof(IFID_latch_));
				memset(&IDEX_latch[0],0,sizeof(IDEX_latch_));
				break;
		//	case 0x09 :
		//		Register.result = Register.reg_Data1;
		//		break;

			case 0x27 :
				EXMM_output_latch->ALU_result = ~(IDEX_input_latch->rs_data | alu_data2);
				break;
			case 0x25 :
				EXMM_output_latch->ALU_result = (IDEX_input_latch->rs_data | alu_data2);
				break;
			case 0x2a :
			case 0x2b :
				EXMM_output_latch->ALU_result = (IDEX_input_latch->rs_data < alu_data2)? 1:0;
				break;
			case 0x00 :
				EXMM_output_latch->ALU_result = (alu_data2 << IDEX_input_latch->val_Rdata.shamt);
				break;
			case 0x02 :
				EXMM_output_latch->ALU_result = (alu_data2 >> IDEX_input_latch->val_Rdata.shamt);
				break;
			case 0x22 :
			case 0x23 :
				EXMM_output_latch->ALU_result = IDEX_input_latch->rs_data - alu_data2;
				break;
		}
	}
	else
	{
		switch (IDEX_input_latch->val_Rdata.opcode)
		{	
			case 0x8 :
			case 0x9 :
				EXMM_output_latch->ALU_result = IDEX_input_latch->rs_data + IDEX_input_latch->Simm;
				break;
			case 0xc :
				EXMM_output_latch->ALU_result = IDEX_input_latch->rs_data & IDEX_input_latch->Zero_Simm;
				break;
			
			case 0x4 :
 				Branch_inst_num++;
				Branch_addr(IDEX_input_latch);
				Pc_check(IDEX_input_latch);
				if(Pc_history.valid == 1);
				else
				{
					Pc_history.branch_target[Pc_history.index2] = IDEX_input_latch->cpc + 4 + Address.Branch_address;
					Pc_history.saved_pc[Pc_history.index2] = IDEX_input_latch->cpc;
					Pc_history.index2 += 1;
				}
				//////////////////////////////////////////////////////////////////////////////////////////////////
				if(IDEX_input_latch->rs_data == alu_data2)
				{
					Branch_Taken_num++;
					IDEX_input_latch->npc = IDEX_input_latch->cpc + 4 + Address.Branch_address;
					taken = 1;
					if(G_history.pattern_table[IDEX_input_latch->pattern_index / 4] <3)
					{
						G_history.pattern_table[IDEX_input_latch->pattern_index / 4] += 1;
					}
					G_history.ghr = ((G_history.ghr << 1) + 1) & 0x0000000f;
				}
				else
				{
					Branch_notTaken_num++;
					IDEX_input_latch->npc = IDEX_input_latch->cpc + 4;
					taken = 0;
					if(G_history.pattern_table[IDEX_input_latch->pattern_index / 4] >0)
					{
						G_history.pattern_table[IDEX_input_latch->pattern_index / 4] -= 1;
					}
					G_history.ghr = (G_history.ghr << 1) & 0x0000000f;
				}
				//////////////////////////////////////////////////////////////////////////////////////////////////////
				if(IDEX_input_latch->br_taken == 1)
				{
					if(taken == 0)
					{
						pc = IDEX_input_latch->npc;
						miss += 1;
						memset(IFID_latch, 0, sizeof(IFID_latch_));
						memset(IDEX_latch, 0, sizeof(IDEX_latch_));
					}
					else
						hit += 1;
				}
				else
				{
					if(taken == 1)
					{
						pc = IDEX_input_latch->npc;
						miss += 1;
						memset(IFID_latch, 0, sizeof(IFID_latch_));
						memset(IDEX_latch, 0, sizeof(IDEX_latch_));
					}
					else
						hit += 1;
				}
				break;
				////////////////////////////////////////////////////////////////////////////////////////////////////
			case 0x5 :
				Branch_inst_num++;
				Branch_addr(IDEX_input_latch);
				Pc_check(IDEX_input_latch);

				if(Pc_history.valid == 1)
				{
					;
				}
				else
				{
					Pc_history.branch_target[Pc_history.index2] = IDEX_input_latch->cpc + 4 + Address.Branch_address;
					Pc_history.saved_pc[Pc_history.index2] = IDEX_input_latch->cpc;
					Pc_history.index2 += 1;
				}
				
				
				if(IDEX_input_latch->rs_data != alu_data2)
				{
					Branch_Taken_num++;
					IDEX_input_latch->npc = IDEX_input_latch->cpc + 4 + Address.Branch_address;
					taken = 1;
					if(G_history.pattern_table[IDEX_input_latch->pattern_index / 4] <3)
					{
						G_history.pattern_table[IDEX_input_latch->pattern_index / 4] += 1;
					}

					G_history.ghr = ((G_history.ghr << 1) + 1) & 0x0000000f;
				}
						
				else
				{
					Branch_notTaken_num++;
					IDEX_input_latch->npc = IDEX_input_latch->cpc + 4;
					taken = 0;

					if(G_history.pattern_table[IDEX_input_latch->pattern_index / 4] >0)
					{
						G_history.pattern_table[IDEX_input_latch->pattern_index / 4] -= 1;
					}

					G_history.ghr = (G_history.ghr << 1) & 0x0000000f;
				}


				if(IDEX_input_latch->br_taken == 1)
				{
					if(taken == 0)
					{
						pc = IDEX_input_latch->npc;
						miss += 1;
						memset(IFID_latch, 0, sizeof(IFID_latch_));
						memset(IDEX_latch, 0, sizeof(IDEX_latch_));
					}
					else
						hit += 1;

				}
				else
				{

					if(taken == 1)
					{
						pc = IDEX_input_latch->npc;
						miss += 1;
						memset(IFID_latch, 0, sizeof(IFID_latch_));
						memset(IDEX_latch, 0, sizeof(IDEX_latch_));
					}
					else
						hit += 1;
				}
				break;

			case 0x2 :
			/*	if(IDEX_input_latch.val_control.Jump == 1)
				{
					pc = Address.Jump_address;
					memset(IFID_latch, 0, sizeof(IFID_latch));
					memset(IDEX_latch, 0, sizeof(IDEX_latch));

				}*/
				break;
			case 0x3 :
				M_register[31] = IDEX_input_latch->cpc + 8;
				break;

			case 0x24 :
			case 0x25 :
				EXMM_output_latch->ALU_result = IDEX_input_latch->rs_data + IDEX_input_latch->Simm;
				break;
			case 0x30 :
			case 0x23 :
				EXMM_output_latch->ALU_result = IDEX_input_latch->rs_data + IDEX_input_latch->Simm;
				break;
			case 0xf :
				EXMM_output_latch->ALU_result = (IDEX_input_latch->val_Rdata.imm << 16);
				break;
			case 0xd :
				EXMM_output_latch->ALU_result = IDEX_input_latch->rs_data | IDEX_input_latch->Zero_Simm;
				break;
			case 0xa :
			case 0xb :
				EXMM_output_latch->ALU_result = (IDEX_input_latch->rs_data < IDEX_input_latch->Simm) ? 1:0;
				break;
			case 0x2b :
				EXMM_output_latch->ALU_result = IDEX_input_latch->rs_data + IDEX_input_latch->Simm;

				break;
		}
	}

	EXMM_output_latch->rt_data = IDEX_input_latch->rt_data;
	EXMM_output_latch->val_Rdata = IDEX_input_latch->val_Rdata;
	EXMM_output_latch->val_control = IDEX_input_latch->val_control;
	EXMM_output_latch->Wreg = IDEX_input_latch->Wreg;
//	EXMM_output_latch->pc4 = IDEX_input_latch->pc4;

/*
	printf("(execute)\n");
	printf(" pc : %x\n",EXMM_latch[0].pc4);
	printf(" (EX)instruction : %x\n", EXMM_latch[0].inst);
	printf(" rt_data= %d\n",EXMM_latch[0].rt_data);
	printf(" ALU_result = %d\n",EXMM_latch[0].ALU_result);
	printf(" Wreg = %d\n",EXMM_latch[0].Wreg);
	printf(" sp = %x\n", M_register[29]);
	printf("*****************************************************************\n");
*/
}
////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
void Memory_access(EXMM_latch_* EXMM_input_latch, MMWB_latch_* MMWB_output_latch)
{
	MMWB_output_latch->inst = EXMM_input_latch->inst;
	MMWB_output_latch->cpc = EXMM_input_latch->cpc;

	Memory.Memory_address = EXMM_input_latch->ALU_result;
	Memory.Memory_Wdata = EXMM_input_latch->rt_data;

	if(EXMM_input_latch->val_control.MemWrite == 1)
	{
		memory[(Memory.Memory_address/4)] = Memory.Memory_Wdata;
//		printf("SW WRITE DATA IS %d\n",Memory.Memory_Wdata);
		Memory_access_num++;
	}
	if(EXMM_input_latch->val_control.MemRead == 1)
	{
		Memory.Memory_Rdata = memory[(Memory.Memory_address/4)];
//		printf("LW READ DATA IS %d\n",Memory.Memory_Rdata);
		Memory_access_num++;
	}

	if(EXMM_input_latch->val_control.MemtoReg == 1)
		MMWB_output_latch->val_Wdata = Memory.Memory_Rdata;
	else
		MMWB_output_latch->val_Wdata = EXMM_input_latch->ALU_result;

	//MMWB_output_latch->M_data = Memory.Memory_Rdata;
	MMWB_output_latch->ALU_result = EXMM_input_latch->ALU_result;
	MMWB_output_latch->val_Rdata = EXMM_input_latch->val_Rdata;
	MMWB_output_latch->val_control = EXMM_input_latch->val_control;
	MMWB_output_latch->Wreg = EXMM_input_latch->Wreg;

/*
	printf("(memory)\n");
	printf(" pc : %x\n",MMWB_latch[0].pc4);
	printf("(MM) instruction : %x\n", MMWB_latch[0].inst);
	printf(" W_data = %d\n",MMWB_latch[0].val_Wdata);
	printf(" M_data = %d\n",MMWB_latch[0].M_data);
	printf(" ALU_result = %d\n",MMWB_latch[0].ALU_result);
	printf(" sp = %x\n", M_register[29]);
	printf(" Wreg = %d\n",MMWB_latch[0].Wreg);
	printf("*****************************************************************\n");
*/	
}

void WriteBack(MMWB_latch_* MMWB_input_latch)
{
	if(MMWB_input_latch->val_control.RegWrite == 1)
	{
		M_register[MMWB_input_latch->Wreg] = MMWB_input_latch->val_Wdata;
	}


	if(MMWB_input_latch->inst != 0x0)
	{
		Final_inst_num++;
		switch(MMWB_input_latch->val_Rdata.opcode)	
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

/*
	printf("(Writeback)\n");
	printf(" pc : %x\n",MMWB_latch[1].pc4);
	printf(" (WB)instruction : %x\n", MMWB_latch[1].inst);
	printf(" sp : %x\n s8 : %x\n", M_register[29], M_register[30]);
	printf(" R[2] : %d\n",M_register[2]);
	printf("*****************************************************************\n");
*/

	
}		
void Set_zero()
{
	memset(&Info, 0, sizeof(Inst_info));
	memset(&Decode, 0, sizeof(Decode_val));
	memset(&Control, 0, sizeof(Control_val));
	memset(&Register, 0, sizeof(Register_val));
	memset(&Address, 0, sizeof(Address_val));
	memset(&Pc_history, 0, sizeof(Pc_history_val));
	memset(&Jump_pc_history, 0 , sizeof(Jump_pc_history_val));
	memset(&G_history, 0, sizeof(Global_history));
	memset(IFID_latch, 0, sizeof(IFID_latch_));
	memset(IDEX_latch, 0, sizeof(IDEX_latch_));
	memset(EXMM_latch, 0, sizeof(EXMM_latch_));
	memset(MMWB_latch, 0, sizeof(MMWB_latch_));
}

void Jump_addr(IDEX_latch_* IDEX_output_latch)
{
	Address.Jump_address = ((IDEX_output_latch->cpc + 4) & 0xf0000000) | (IDEX_output_latch->val_Rdata.address << 2);
}


void Branch_addr(IDEX_latch_* IDEX_input_latch)
{
	int msb = IDEX_input_latch->val_Rdata.imm >> 15;

	if(msb == 1)
		Address.Branch_address = 0xfffc0000 | (IDEX_input_latch->Simm << 2);
	else
		Address.Branch_address = 0x00000000 | (IDEX_input_latch->Simm << 2) ; 

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

void Pc_check(IDEX_latch_* IDEX_input_latch)
{
	int i;
	for(i=0; i<100; i++)
	{
		if(Pc_history.saved_pc[i] == IDEX_input_latch->cpc)
		{
			Pc_history.index = i;
			Pc_history.valid = 1;
			break;
		}
		else
			Pc_history.valid = 0;
	}
	return;
}
void Jump_pc_check(IDEX_latch_* IDEX_output_latch)
{
	int i;
	for(i=0; i<10; i++)
	{
		if(Jump_pc_history.saved_pc[i] == IDEX_output_latch->cpc)
		{
			Jump_pc_history.index = i;
			Jump_pc_history.valid = 1;
			break;
		}
		else
			Jump_pc_history.valid = 0;
	}
	return;
}
void IF_Pc_check(IFID_latch_* IFID_output_latch)
{
	int i;
	for(i=0; i<100; i++)
	{
		if(pc == Pc_history.saved_pc[i] && pc != 0)
		{
			IFID_output_latch->npc = Pc_history.branch_target[i];
			B_hit = 1;
			break;
		}
		else
		{	
			B_hit = 0;
		}
	}
	return;
}	
void IF_jump_Pc_check()
{
	int i;
	for(i=0; i<10; i++)
	{
		if(Current_pc == Jump_pc_history.saved_pc[i] && Current_pc != 0)
		{
			pc = Jump_pc_history.jump_target[i];
		//	B_hit = 1;
			break;
		}
		else
		{;	
		//	B_hit = 0;
		//	pc += 4;
		}
	}
	return;
}	
void debug()
{
	printf("---------------clock %d-------------------------\n",cycle+1);
	printf("(Fetch) pc : %x inst : %x\n",Current_pc, memory[M_index]);
	printf("(Decode) inst : %x\n",IDEX_output_latch->inst);
	printf("(Execute) inst : %x\n",EXMM_output_latch->inst);
	printf("(Memory) inst : %x\n", MMWB_output_latch->inst);  
	printf("(Writeback) inst : %x\n", MMWB_input_latch->inst); 
}

void initial_table()
{
	int i;
	for(i=0; i<100; i++)
		G_history.pattern_table[i] = 3;
}
