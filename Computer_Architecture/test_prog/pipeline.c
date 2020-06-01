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
	unsigned int Rt_forwarding;
	unsigned int C_dependency;
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
	int pc4;
	int inst;//새로운 instruction
	int valid;
}IFID_latch_;

typedef struct
{
	int npc;
	int pc4;
	int inst;
	int valid;
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
	int pc4;
	int valid;
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
	int pc4;
	int inst;
	int valid;
	int npc2;
	int npc1;
	int ALU_result;
	int M_data;
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
int Branch_satisfy_num;
int Nop_num;
int branch_target;
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
IFID_latch_ IFID_latch[2];
IDEX_latch_ IDEX_latch[2];
EXMM_latch_ EXMM_latch[2];
MMWB_latch_ MMWB_latch[2];
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
	int cycle_num = 0;
	char Want_Result;

	char* file_name;

	file_name = (char*)malloc(sizeof(char)*20);
	
//	printf("Are you want to print All of Cycle results? Y or N\n");
//	scanf("%c",&Want_Result);

	printf("Please Write File Name [ex) simple.bin]\n");
	scanf("%s",file_name);

	fp = fopen(file_name, "rb");
//	fp = fopen("simple4.bin", "rb");

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


	while(1)
	{
//		printf("---------------cycle%d-----------------\n",cycle_num+1);
		if(pc == 0xffffffff)
		{
		WriteBack(MMWB_latch[1]);
		Memory_access(EXMM_latch[1]);
		Copy();
		WriteBack(MMWB_latch[1]);
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
	M_index = pc / 4;
	pc += 4;
//	printf("next pc is : %x\n",pc);
//	if(EXMM_latch[1].val_control.C_dependency == 1)
//	{
//		pc = npc + 4;
//		IFID_latch[0].pc4 = pc;
//		IFID_latch[0].inst = memory[M_index];


	IFID_latch[0].pc4 = pc;
	IFID_latch[0].inst = memory[M_index];

/*
	printf("(fetch) : %d번째\n", (IFID_latch[0].pc4 - 4) / 4 + 1);
	printf(" pc : %x\n", IFID_latch[0].pc4-4);
	printf(" instruction : %x\n", IFID_latch[0].inst);
	printf("*****************************************************************\n");
	*/
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

//	printf("val_rt is %d\n",IDEX_latch[0].val_Rdata.rt);
	if(IFID_input_latch.inst != 0x00000000)
	{
		switch(IDEX_latch[0].val_Rdata.opcode)	
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
	IDEX_latch[0].val_control = Control;

	if(IDEX_latch[0].val_control.Jump == 1)
	{
		Jump_addr();
		pc = Address.Jump_address;
	}
	
	SignExtm();
//	IDEX_latch[0].Simm = IDEX_latch[0].val_Rdata.Ext_imm;
//	IDEX_latch[0].Zero_Simm = IDEX_latch[0].val_Rdata.Ext_imm_zero;

	IDEX_latch[0].pc4 = IFID_input_latch.pc4;
	IDEX_latch[0].rs_data = M_register[IDEX_latch[0].val_Rdata.rs];
	IDEX_latch[0].rt_data = M_register[IDEX_latch[0].val_Rdata.rt];

//	printf("R[rt] is R[%d] = %d\n",IDEX_latch[0].val_Rdata.rt, M_register[IDEX_latch[0].val_Rdata.rt]);

	if (IDEX_latch[0].val_control.RegDest == 1)
	{
		IDEX_latch[0].Wreg = IDEX_latch[0].val_Rdata.rd;
	}
	else
	{
		IDEX_latch[0].Wreg = IDEX_latch[0].val_Rdata.rt;
	}

/*
	printf("(decode) : %d번째\n", (IDEX_latch[0].pc4) / 4 + 1);
	printf(" pc : %x\n",IDEX_latch[0].pc4-4);
	printf(" current pc is : %x\n",pc);
	printf(" instruction : %x\n", IDEX_latch[0].inst);
	printf(" rs_data = %d\n rt_data= %d\n", IDEX_latch[0].rs_data, IDEX_latch[0].rt_data);
	printf(" sp = %x\n", M_register[29]);
	printf(" Wreg = %d\n",IDEX_latch[0].Wreg);
	printf("*****************************************************************\n");
	
*/
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

	if (IDEX_latch[0].val_Rdata.opcode == 0x2 | IDEX_latch[0].val_Rdata.opcode == 0x3)//(IDEX_latch[0].val_Rdata.opcode==0 && IDEX_latch[0].val_Rdata.func == 0x08))
	{
		Control.Jump = 1;
		Control.C_dependency = 1;
	}
	else
	{
		Control.Jump = 0;
		Control.C_dependency = 0;
	}
	
	if (IDEX_latch[0].val_Rdata.opcode == 0x4 | IDEX_latch[0].val_Rdata.opcode == 0x5)
	{
		Control.Branch = 1;
		Control.C_dependency = 1;
	}
	else
	{
		Control.Branch = 0;
		Control.C_dependency = 0;
	}

	switch(IDEX_latch[0].val_Rdata.opcode)
	{
		case 0x4:
		case 0x5:
		case 0x2b:
			Control.Rt_forwarding = 1;
			break;
		case 0x0:
		switch(IDEX_latch[0].val_Rdata.func)
		{
			case 0x8:
				Control.Rt_forwarding = 0;
				break;
			default :
				Control.Rt_forwarding = 1;
				break;
		}

		default:
			Control.Rt_forwarding = 0;
			break;

	}
//	printf("Control signal part!!\n RegDst : %d // ALUSrc : %d // ALUOp : %d // MEMtoReg : %d // MemRead : %d // RegWrite : %d // MemWrite : %d // Jump : %d // Branch : %d\n",Control.RegDest, Control.ALUSrc, Control.ALUOp, Control.MemtoReg, Control.MemRead, Control.RegWrite, Control.MemWrite, Control.Jump, Control.Branch);
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

//	printf("SignExtmm part\n Ext_imm result : %x & %d // Ext_imm_zero result : %x & %d\n", IDEX_latch[0].val_Rdata.Ext_imm, IDEX_latch[0].val_Rdata.Ext_imm, IDEX_latch[0].val_Rdata.Ext_imm_zero, IDEX_latch[0].val_Rdata.Ext_imm_zero);
}
//////////////////////////////////////////////////////////////////////////////////////

void Execution(IDEX_latch_ IDEX_input_latch)
{
	int alu_data2;
	EXMM_latch[0].inst = IDEX_input_latch.inst;

	Branch_addr();

//f(IDEX_latch[1].val_Rdata.opcode == 0x2)
//		EXMM_latch[1].npc2 = Address.Jump_address;
//	else
//		EXMM_latch[0].npc2 = IDEX_input_latch.rs_data;

	if (IDEX_input_latch.val_control.ALUSrc == 1)
	{
		alu_data2 = IDEX_input_latch.Simm;
	}
	else
	{
		alu_data2 = M_register[IDEX_input_latch.val_Rdata.rt];
	}
////////////////////////////////////////////////////////////////////////////////////
	if((IDEX_input_latch.val_Rdata.rs == EXMM_latch[1].Wreg) && (IDEX_input_latch.val_Rdata.rs != 0) && EXMM_latch[1].val_control.RegWrite == 1)
	{
		IDEX_input_latch.rs_data = EXMM_latch[1].ALU_result;
//		printf("*1**data dependency 1***\n");
//		printf("After dependency rs_data = %d\n",IDEX_input_latch.rs_data);
	}
		
	else if((IDEX_input_latch.val_Rdata.rs == MMWB_latch[1].Wreg) && (IDEX_input_latch.val_Rdata.rs != 0) && MMWB_latch[1].val_control.RegWrite == 1)
	{
		
		if(MMWB_latch[1].val_control.MemtoReg == 1)
			IDEX_input_latch.rs_data = MMWB_latch[1].M_data;
		else
			IDEX_input_latch.rs_data = MMWB_latch[1].ALU_result;

//		printf("*1**data dependency 2***\n");
//		printf("After dependency rs_data = %d\n",IDEX_input_latch.rs_data);
	}
 
	
	if((IDEX_input_latch.val_Rdata.rt == EXMM_latch[1].Wreg) && (IDEX_input_latch.val_Rdata.rt != 0) && EXMM_latch[1].val_control.RegWrite == 1)
	{
		alu_data2 = EXMM_latch[1].ALU_result;
//		printf("*2**data dependency 1***\n");
//		printf("After dependency rt_data = %d\n",alu_data2);
		IDEX_input_latch.rt_data = alu_data2;
	}
	

	else if((IDEX_input_latch.val_Rdata.rt == MMWB_latch[1].Wreg) && (IDEX_input_latch.val_Rdata.rt != 0) && MMWB_latch[1].val_control.RegWrite == 1)
	{
		if(MMWB_latch[1].val_control.MemtoReg == 1)
			alu_data2 = MMWB_latch[1].M_data;
		else
			alu_data2 = MMWB_latch[1].ALU_result;
//		printf("*2**data dependency 2***\n");
//		printf("After dependency rt_data = %d\n",alu_data2);
		IDEX_input_latch.rt_data = alu_data2;
	}
//////////////////////////////////////////////////////////////data dependancy

//	printf("alu_data2 is %d\n",alu_data2);

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
				break;
		//	case 0x09 :
		//		Register.result = Register.reg_Data1;
		//		break;

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
 				Branch_inst_num++;
				if(IDEX_input_latch.rs_data == alu_data2)
					{
						pc = IDEX_input_latch.pc4 + Address.Branch_address;
						Branch_satisfy_num++;
						memset(IFID_latch, 0, sizeof(IFID_latch));
					}
			
				
				break;

			case 0x5 :
				Branch_inst_num++;
				if(IDEX_input_latch.rs_data != alu_data2)
					{
						pc = IDEX_input_latch.pc4 + Address.Branch_address;
						Branch_satisfy_num++;
						memset(IFID_latch, 0, sizeof(IFID_latch));
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
			//	if(IDEX_input_latch.val_control.Jump == 1)
			//	{
					M_register[31] = IDEX_input_latch.pc4 + 4;
			//		pc = Address.Jump_address;
			//		memset(IFID_latch, 0, sizeof(IFID_latch));
			//		memset(IDEX_latch, 0, sizeof(IDEX_latch));

			//	}
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
			case 0x28 :
				
				break;
			case 0x38 :
				break;
			case 0x29 :
				break;
			case 0x2b :
				EXMM_latch[0].ALU_result = IDEX_input_latch.rs_data + IDEX_input_latch.Simm;

				break;
		}
	}

//	if(IDEX_input_latch.val_Rdata.opcode == 0x4 || IDEX_input_latch.val_Rdata.opcode == 0x5)
//		EXMM_latch[0].npc1 = branch_target;
//	else
//		EXMM_latch[0].npc1 = IDEX_input_latch.pc4;


	EXMM_latch[0].rt_data = IDEX_input_latch.rt_data;
	EXMM_latch[0].val_Rdata = IDEX_input_latch.val_Rdata;
	EXMM_latch[0].val_control = IDEX_input_latch.val_control;
	EXMM_latch[0].Wreg = IDEX_input_latch.Wreg;
	EXMM_latch[0].pc4 = IDEX_input_latch.pc4;

		
//	printf("*****************************************************************\n");
/*
	printf("(execute)\n");
	printf(" pc : %x\n",EXMM_latch[0].pc4-4);
	printf(" current pc is : %x\n",pc);
	printf(" instruction : %x\n", EXMM_latch[0].inst);
	printf(" rt_data= %d\n",EXMM_latch[0].rt_data);
	printf(" ALU_result = %d\n",EXMM_latch[0].ALU_result);
	printf(" Wreg = %d\n",EXMM_latch[0].Wreg);
	printf(" sp = %x\n", M_register[29]);
	printf("*****************************************************************\n");
	
*/
}
////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
void Memory_access(EXMM_latch_ EXMM_input_latch)
{
	MMWB_latch[0].inst = EXMM_input_latch.inst;
	MMWB_latch[0].pc4 = EXMM_input_latch.pc4;

	Memory.Memory_address = EXMM_input_latch.ALU_result;
	Memory.Memory_Wdata = EXMM_input_latch.rt_data;

	if(EXMM_input_latch.val_control.MemWrite == 1)
	{
		memory[(Memory.Memory_address/4)+1] = Memory.Memory_Wdata;
//		printf("SW WRITE DATA IS %d\n",Memory.Memory_Wdata);
	}
	if(EXMM_input_latch.val_control.MemRead == 1)
	{
		Memory.Memory_Rdata = memory[(Memory.Memory_address/4)+1];
//		printf("LW READ DATA IS %d\n",Memory.Memory_Rdata);

	}
	Memory_access_num++;
	MMWB_latch[0].npc2 = EXMM_input_latch.npc2;
	MMWB_latch[0].npc1 = EXMM_input_latch.npc1;

	if(EXMM_input_latch.val_control.MemtoReg == 1)
		MMWB_latch[0].val_Wdata = Memory.Memory_Rdata;
	else
		MMWB_latch[0].val_Wdata = EXMM_input_latch.ALU_result;

	MMWB_latch[0].M_data = Memory.Memory_Rdata;
	MMWB_latch[0].ALU_result = EXMM_input_latch.ALU_result;
	MMWB_latch[0].val_Rdata = EXMM_input_latch.val_Rdata;
	MMWB_latch[0].val_control = EXMM_input_latch.val_control;
	MMWB_latch[0].Wreg = EXMM_input_latch.Wreg;

//	printf("*****************************************************************\n");
/*
	printf("(memory) : %d번째\n", (MMWB_latch[0].pc4 - 4) / 4 + 1);
	printf(" pc : %x\n",MMWB_latch[0].pc4-4);
	printf(" current pc is : %x\n",pc);
	printf(" instruction : %x\n", MMWB_latch[0].inst);
	printf(" W_data = %d\n",MMWB_latch[0].val_Wdata);
	printf(" M_data = %d\n",MMWB_latch[0].M_data);
	printf(" ALU_result = %d\n",MMWB_latch[0].ALU_result);
	printf(" sp = %x\n", M_register[29]);
	printf(" Wreg = %d\n",MMWB_latch[0].Wreg);
	printf("*****************************************************************\n");
*/
}

void WriteBack(MMWB_latch_ MMWB_input_latch)
{
	if(MMWB_input_latch.val_control.RegWrite == 1)
	{
//		if(MMWB_input_latch.val_Rdata.opcode == 0x0 && MMWB_input_latch.val_Rdata.func == 0x9);
//		else
//		{
		M_register[MMWB_input_latch.Wreg] = MMWB_input_latch.val_Wdata;
//		}
	}
	
	//	printf("*****************************************************************\n");
	/*
	printf("(Writeback) : %d번째\n", (MMWB_latch[1].pc4 - 4) / 4 + 1);
	printf(" pc : %x\n",MMWB_latch[1].pc4-4);
	printf(" current pc is : %x\n",pc);
	printf(" instruction : %x\n", MMWB_latch[1].inst);
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
	memset(IFID_latch, 0, sizeof(IFID_latch_));
	memset(IDEX_latch, 0, sizeof(IDEX_latch_));
	memset(EXMM_latch, 0, sizeof(EXMM_latch_));
	memset(MMWB_latch, 0, sizeof(MMWB_latch_));
}

void Jump_addr()
{
	Address.Jump_address = (IDEX_latch[0].pc4 & 0xf0000000) | (IDEX_latch[0].val_Rdata.address << 2);
	pc = Address.Jump_address;
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

void debug()
{
	printf("next pc is %x\n",pc);
	printf("IFID_latch inst = %x  ",IFID_latch[1].inst);
	printf("IDEX_latch inst = %x rs_data = %d rt_data = %d Wreg = %d\n",IDEX_latch[1].inst,IDEX_latch[1].rs_data,IDEX_latch[1].rt_data,IDEX_latch[1].Wreg);
	printf("EXMM_latch inst = %x ALU_result = %x \n",EXMM_latch[1].inst, EXMM_latch[1].ALU_result);
	printf("MMWB_latch inst = %x\n",MMWB_latch[1].inst);
}
