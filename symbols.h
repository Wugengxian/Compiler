#include <stdlib.h>
#include <string.h>
#include "tree.h"

#ifndef _SYMBOLS_H_
#define _SYMBOLS_H_

//����ͷ������ꡣheadΪͷָ�룬pΪҪ����Ԫ�ص�ָ�롣
#define insert_head(head,p) \
	do \
	{ \
		(p)->next=(head); \
		(head)=(p); \
	}while(0)

struct type_d;
struct val_d;

//���������ͣ�int��float���û��Զ������ͣ����飬�ṹ�壩
typedef enum val_kind
{
	_int,_float,USER_DEFINED
}val_kind;
//�û��Զ������ͣ��ṹ�壬����
typedef enum type_kind
{
	_struct,_array
}type_kind;

//���鶨��ĵ�Ԫ�����鶨���������ʾ��
typedef struct array_def_list
{
	int dimension;					//�ò��ά�ȡ���0�㿪ʼ����һ������int a[]����0�㡣
	int number;						//�ò��м���Ԫ��
	int size_of_each;				//ÿ��Ԫ�ض�󡣣�Ŀǰ��Ϊ��struct�Ĵ�С��intһ������4�ֽڣ�
	val_kind kind;					//����Ԫ����ʲô��int,float,struct
	struct type_d* val_type;		//struct����ָ�룬�����Ҫ
	struct array_def_list* next;	//��һά��ָ��
}array_def_list;
//�ṹ�嶨�嵥Ԫ���ýṹ���м�����ÿ����Ķ���
typedef struct struct_def_list
{
	int define_count;			//�ýṹ���м�����
	struct val_d** def_list;	//ÿ����Ķ���
}struct_def_list;

//�������嵥Ԫ���к����������û��������ṹ���ڱ�����
typedef struct val_d
{
	char name[MAX_LEN_OF_NAME];		//����
	int is_true_value;				//�Ƿ�����ı��������Ƿ����ֱ��ʹ�ã���Ϊ�ṹ���е�����ֱ��ʹ��
	val_kind kind;					//���ͣ�int��float���û���������
	struct type_d* val_type;		//�û��������͵Ķ���ṹ��ָ�루�����Ҫ��
	struct val_d* next;				//��һ����Ԫ��ַ
}val_d;
//���Ͷ��嵥Ԫ��
typedef struct type_d
{
	char name[MAX_LEN_OF_NAME];	//���ơ�����û������
	type_kind kind;				//���ͣ��ṹ�壬����
	union
	{
		array_def_list* a;
		struct_def_list* s;
	}def;						//���;��嶨�塣aΪ���鶨���ַ��sΪ�ṹ�嶨���ַ
	struct type_d* next;		//��һ����Ԫ��ַ
}type_d;
//�������嵥Ԫ��
typedef struct func_d
{
	char name[MAX_LEN_OF_NAME];	//����
	int parameter_count;		//��������
	val_kind* kinds;			//�������ͱ�־
	type_d** parameters;			//���������б�(���ĳ������Ҫ)
	val_kind return_kind;
	struct val_d** parameter_list;
	type_d* return_type;		//����ֵ���Ͷ���
	struct func_d* next;		//��һ����Ԫ��ַ
}func_d;
//Ϊ���������ʵ�ֵı��������ջ������ͷ�����룬ȡͷ����ͷ��ɾ����ʵ�֡�
typedef struct value_stack
{
	val_d* values;				//���������
	struct value_stack* next;	//��һ��
}value_stack;
#endif

//ջ����������push��pop���鿴ĳһ�������ܷ���ջ�����壨true��ʾջ��û������������壬false��ʾջ����������������壩��
void value_stack_push();
void value_stack_pop();
int value_stack_check(const char* name);

//�����ʼ���������������ű������������
void init_symbol_table();
void destroy_symbol_table();

//������3�����캯������̬������µı���������ַ��
type_d* new_type(const char* name);
func_d* new_function(const char* name);
val_d* new_value(const char* name);

//�����������������鶨��ʱ��ʹ�á��ֱ�Ϊ����һ�������Ͳ㣬�Լ����Ѿ��л����Ͳ��������չһ��ά�ȡ�
void array_generate_basic_dimension(type_d* t,int number,val_kind kind,type_d* val_type);
void array_expand_dimension(type_d* t,int number);

//�����������ж����������Ƿ���ȡ�
int type_equal(type_d* p,type_d* q);

//���������������һ������,���������ظ���һ����Բ������û����������������������struct�ȳ���ʹ�á���������dest�С�
void get_a_name(char* name);

//������3����ѯ��������ѯ�ɹ��򷵻ض�Ӧ����ָ�룬ʧ���򷵻�NULL��
type_d* find_type(const char* name);
func_d* find_function(const char* name);
val_d* find_value(const char* name);

//������3����Ӻ��������Ѿ�����ñ���󣬽�����ӽ����ű��ж�Ӧ��λ�á����ظոձ���ӽ�ȥ�ı��
type_d* add_type_declaration(type_d* r);
func_d* add_function_declaration(func_d* r);
val_d* add_value_declaration(val_d* r);

//�õ��ṹ��Ĵ�С���õ�һ����(һ���ڸýṹ���д���)�ڽṹ���ڴ�ͷ����ʼ��ƫ������
int struct_get_size(type_d* s);
int struct_get_offset(type_d* s,char* field);

//int to string
void itoa(unsigned long val, char* buf,unsigned radix);