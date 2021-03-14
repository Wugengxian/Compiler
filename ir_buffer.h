#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "symbols.h"

#ifndef _IR_BUFFER_H_
#define _IR_BUFFER_H_
/* args_count=2:
 * 	GOTO x
 * 	RETURN x
 * 	ARG x
 * 	PARAM x
 * 	READ x
 * 	WRITE x
 * args_count=3:
 * 	LABEL x :
 * 	FUNCTION f :
 * 	x := y
 * 	x := &y
 * 	x := *y
 * 	*x := y
 * 	DEC x [size]
 * args_count=4:
 * 	x := CALL f
 * args_count=5:
 * 	x := y + z
 * 	x := y - z
 * 	x := y * z
 * 	x := y / z
 * args_count=6:
 * IF x [relop] y GOTO z
 */
//˫��ѭ�������洢�������ڴ��еı�ʾ
typedef struct code_node
{
	struct code_node* prev;		//ǰһ��
	struct code_node* next;		//��һ��
	int args_count;				//�ж��ٸ��ʡ����������ע���ڵķ��ࡣ
	char args[6][32];			//ÿ���ʶ���ʲô
}code_node;
#endif

typedef struct variable_alies
{
  char name[32];
  char othername;
  struct variable_alies* next;
}variable_alies;

char* check_name(char* name);

//�����µ�label�����Ʒ����ṩ�õ�name���档
void new_label(char* name);
//�����µ���ʱ���������Ʒ����ṩ�õ�name���档
void new_temp(char* name);
//���һ�����룬ָ����������Ĵ�����Ȼ�����������������ﶼ��char*�����������ַ���
void add_code(int args_count,...);
//���ڴ��еĴ����ӡ���ļ��У��������ļ�·������˳�������ڴ��еĴ���洢
void print_code(FILE* name);
//�ر��Ż�
void close_opt();