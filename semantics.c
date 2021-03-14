#include "semantics.h"
#include "symbols.h"
static val_kind kind=USER_DEFINED;			//��ǰ�������ͣ�int��float���û���������
static type_d* val_type=NULL;				//��ǰ������û��������͵Ķ���ṹ��ָ�루�����Ҫ��
static func_d* current_func=NULL;			//��ǰ���ڴ���ĺ�������ָ�롣
static int para_count=0;					//������Ŀ��
static val_d* paras[512];					//���������塣�����������ˡ�һ���ṹ�壬һ�������ı���������������512����
static int need_count=0;					//�Ƿ���Ҫ��¼����ı�����
static int structing=0;					//�Ƿ����ڶ���ṹ�壬���ڱ�־��ǰ���������Ƿ��ǽṹ���ڵ�����
static int do_not_push=0;					//�����ں����ս�����CompSt����Ҫpush���ű�
static val_d* last_val=NULL;				//�������ı�����
static int error_occured=0;
typedef struct stack
{
	val_kind kind;						//��ǰ�������ͣ�int��float���û���������
	struct type_d* val_type;			//��ǰ������û��������͵Ķ���ṹ��ָ�루�����Ҫ��
	int para_count;						//������Ŀ��
	val_d* paras[512];					//���������塣����Щ�����ˡ�һ���ṹ�塢һ�������ı���������������512����
	int need_count;						//�Ƿ���Ҫ��¼����ı�����
	int structing;						//�Ƿ����ڶ���ṹ�壬���ڱ�־��ǰ���������Ƿ��ǽṹ���ڵ�����
	struct stack* next;
}stack;
static stack* st_head=NULL;
typedef enum id_type
{
	id_val,id_func
}id_type;
//����exp�ڵ�ĺ�������exp_kind��exp_type�з��ط������exp���͡����exp_kind=USER_DEFINED��exp_type=NULL����˴�EXP�Ѿ��������������Ѿ������
void ana_exp(val_kind* exp_kind,type_d** exp_type,Node* h);
//���ݴ��������(h->name)�����ͣ����id�Ƿ���ڡ��������򱨴���null�������򷵻ض���ָ�롣
void* check_id(Node* h,id_type identity);
//��鴫���Node*�Ƿ��������ֵ
int check_left(Node* h);
int get_error_occured()
{
	return error_occured;
}
void push()
{
	stack* p=(stack*)malloc(sizeof(stack));
	p->kind=kind;
	p->val_type=val_type;
	p->para_count=para_count;
	p->need_count=need_count;
	p->structing=structing;
	for(int i=0;i<para_count;i++)
		p->paras[i]=paras[i];
	para_count=0;
	insert_head(st_head,p);
}
void pop()
{
	stack* p=st_head;
	st_head=st_head->next;
	kind=p->kind;
	val_type=p->val_type;
	para_count=p->para_count;
	structing=p->structing;
	for(int i=0;i<p->para_count;i++)
		paras[i]=p->paras[i];
	need_count=p->need_count;
	free(p);
}
void semantic_analysis(Node* h)
{
	//printf("begin: %s %d\n",get_type_name(h->type),h->line);
	if(h==NULL)return;	
	switch(h->type)
	{
		case Specifier:
		{			
			//�������õ�ǰ��������ͣ�����ȫ�ֱ���
			if(h->child[0]->type==_TYPE)
			{
				if(strcmp(h->child[0]->name,"int")==0)kind=_int;
				else kind=_float;
				val_type=NULL;
			}
			else
			{
				kind=USER_DEFINED;
				semantic_analysis(h->child[0]);
			}
			break;
		}
		case StructSpecifier:
		{
			if(h->child_count==2)
			{
				//������ʹ���Ѿ�����Ľṹ��
				val_type=find_type(h->child[1]->child[0]->name);
				if(val_type==NULL)
				{	
					printf("Error type 17 at Line %d: undefined struct %s.\n",h->line,h->child[1]->child[0]->name);
					error_occured=1;
				}
			}
			else
			{
				//����ṹ�壬���ܻ�ֱ��ʹ���ڶ������������ʹ�����ṹ�嶨�塣
				char name[33];
				if(h->child[1]->child[0]->type==None)
					get_a_name(name);
				else
					strcpy(name,h->child[1]->child[0]->name);
				val_type=find_type(name);
				if(val_type!=NULL || find_value(name)!=NULL)
				{
					printf("Error type 16 at Line %d: name %s is used.\n",h->line,name);
					error_occured=1;
					val_type=NULL;
				}
				else
				{
					val_type=new_type(name);
					add_type_declaration(val_type);
					push();
					structing=1;
					need_count=1;
					para_count=0;
					semantic_analysis(h->child[3]);
					st_head->val_type->def.s->define_count=para_count;
					st_head->val_type->def.s->def_list=(val_d**)malloc(sizeof(val_d*)*para_count);
					for(int i=0;i<para_count;i++)
						st_head->val_type->def.s->def_list[i]=paras[i];
					need_count=0;
					structing=0;
					pop();
				}
			}
			break;
		}
		case CompSt:
		{
			if(do_not_push)
				do_not_push=0;
			else
				value_stack_push();
			semantic_analysis(h->child[1]);
			semantic_analysis(h->child[2]);
			value_stack_pop();
			break;
		}
		case ExtDef:
		{
			/* �˴��Ƕ��壬���ݵڶ����ӽ����ExtDecList,SEMI,FunDec,
			 * �ֱ�ȷ���Ƕ���ȫ�ֱ����������Ķ��壬�ֱ�����ͬ����
			 */
			if(h->child[1]->type==FunDec)
			{
				semantic_analysis(h->child[0]);
				func_d* temp=find_function(h->child[1]->child[0]->name);
				if(temp!=NULL)
				{
					printf("Error type 4 at Line %d: function %s is redefined.\n",h->line,h->child[1]->child[0]->name);
					error_occured=1;
				}
				else
				{
					temp=new_function(h->child[1]->child[0]->name);
					add_function_declaration(temp);
					current_func=temp;
					temp->return_type=val_type;
					temp->return_kind=kind;
					value_stack_push();
					do_not_push=1;
					need_count=1;
					para_count=0;
					semantic_analysis(h->child[1]->child[2]);
					temp->parameter_count=para_count;
					if(para_count!=0)
					{
						temp->kinds=(val_kind*)malloc(sizeof(val_kind)*para_count);
						temp->parameters=(type_d**)malloc(sizeof(type_d*)*para_count);
						temp->parameter_list=(val_d**)malloc(sizeof(val_d*)*para_count);
					}
					for(int i=0;i<para_count;i++)
					{
						temp->kinds[i]=paras[i]->kind;
						temp->parameters[i]=paras[i]->val_type;
						temp->parameter_list[i]=paras[i];
					}
					need_count=0;
					semantic_analysis(h->child[2]);
					do_not_push=0;
					current_func=NULL;
				}
			}
			else
				for(int i=0;i<h->child_count;i++)
					semantic_analysis(h->child[i]);
			break;
		}
		case VarDec:
		{
			Node* temp=h;
			while(temp->child_count!=1)
				temp=temp->child[0];
			temp=temp->child[0];
			int check=value_stack_check(temp->name);
			if(!check)
			{
				val_d* che=find_value(temp->name);
				int i=-1;
				if(structing)
					for(i=0;i<para_count;i++)
						if(paras[i]==che)
							break;
				if(i==para_count||i==-1)
				{
					error_occured=1;
					printf("Error type 3 at Line %d: variable %s is redefined.\n",h->line,temp->name);
				}
				else
				{
					error_occured=1;
					printf("Error type 15 at Line %d: Redefined field %s.\n",h->line,temp->name);
				}
				last_val=NULL;
			}
			else if(!(kind==USER_DEFINED && val_type==NULL))
			{
				val_d* v=new_value(temp->name);
				v->is_true_value=!structing;
				if(h->child_count==1)
				{
					v->kind=kind;
					v->val_type=val_type;
				}
				else
				{
					v->kind=USER_DEFINED;
					v->val_type=new_type(NULL);
					add_type_declaration(v->val_type);
					temp=h->child[0];
					array_generate_basic_dimension(v->val_type,h->child[2]->value_i,kind,val_type);
					while(temp->child_count!=1)
					{
						array_expand_dimension(v->val_type,temp->child[2]->value_i);
						temp=temp->child[0];
					}
				}
				add_value_declaration(v);
				last_val=v;
				if(need_count)
				{
					paras[para_count]=v;
					para_count++;
				}
			}
			break;
		}
		case Stmt:
		{
			//���ݵ�һ���ӽ��Ĳ�ͬ���ֱ�����ͬ�Ĵ���
			switch(h->child[0]->type)
			{
				case Exp:
				{
					val_kind temp1;
					type_d* temp2;
					ana_exp(&temp1,&temp2,h->child[0]);
					break;
				}
				case CompSt:
				{
					semantic_analysis(h->child[0]);
					break;
				}
				case _RETURN:
				{
					val_kind temp1;
					type_d* temp2;
					ana_exp(&temp1,&temp2,h->child[1]);
					if(!(temp1==USER_DEFINED && temp2==NULL))
						if(current_func->return_kind!= temp1 || current_func->return_type!=temp2)
						{
							printf("Error type 8 at Line %d: invalid return type\n",h->line);
							error_occured=1;
						}
					break;
				}
				default:
				{
					val_kind temp1;
					type_d* temp2;
					ana_exp(&temp1,&temp2,h->child[2]);
					if(temp1!=_int && !(temp1==USER_DEFINED && temp2==NULL))
					{
						printf("Error type 7 at Line %d: only int can be used as boolean\n",h->line);
						error_occured=1;
					}
					for(int i=4;i<h->child_count;i++)
						semantic_analysis(h->child[i]);
					break;
				}
			}
			break;
		}
		case Dec:
		{
			semantic_analysis(h->child[0]);
			if(h->child_count==3)
			{
				if(structing)
				{
					error_occured=1;
					printf("Error type 15 at Line %d: can't initialize a field while defining the struct\n",h->line);
				}
				else
				{
					val_kind temp1;
					type_d* temp2;
					ana_exp(&temp1,&temp2,h->child[2]);
					if(!(temp1==USER_DEFINED && temp2==NULL) && last_val!=NULL)
						if(last_val->kind!=temp1 || !type_equal(last_val->val_type,temp2))
						{
							error_occured=1;
							printf("Error type 5 at Line %d: incompatible type near =\n",h->line);
						}
				}
			}
			break;
		}
		default:
		{
			for(int i=0;i<h->child_count;i++)
				semantic_analysis(h->child[i]);
			break;
		}
	}
	//printf("fin: %s %d\n",get_type_name(h->type),h->line);
}
void ana_exp(val_kind* exp_kind,type_d** exp_type,Node* h)
{
	if(h->child_count==1)
	{
		/* ID
		 * INT
		 * FLOAT
		 */
		switch(h->child[0]->type)
		{
			case _ID:
			{
				val_d* temp=(val_d*)check_id(h->child[0],id_val);
				if(temp!=NULL)
				{
					if(temp->is_true_value==0)
					{
						error_occured=1;
						printf("Error type 1 at Line %d: %s is a field of a struct\n",h->line,temp->name);
						*exp_kind=USER_DEFINED;
						*exp_type=NULL;
					}
					else
					{
						*exp_kind=temp->kind;
						*exp_type=temp->val_type;
					}
				}
				else
				{
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
				}
				break;
			}
			case _INT:
			{
				*exp_kind=_int;
				*exp_type=NULL;
				break;
			}
			case _FLOAT:
			{
				*exp_kind=_float;
				*exp_type=NULL;
				break;
			}
		}
	}
	else if(h->child_count==2)
	{
		/* NOT Exp
		 * MINUS Exp
		 */
		val_kind temp1;
		type_d* temp2;
		ana_exp(&temp1,&temp2,h->child[1]);
		if(h->child[0]->type==_NOT)
		{
			if((temp1==USER_DEFINED && temp2!=NULL) || temp1==_float)
			{
				error_occured=1;
				printf("Error type 7 at Line %d: only int can use \"!\"(not)\n",h->line);
				*exp_kind=USER_DEFINED;
				*exp_type=NULL;
			}
			else 
			{
				*exp_kind=temp1;
				*exp_type=temp2;
			}
		}
		else
		{
			if(temp1==USER_DEFINED && temp2!=NULL)
			{
				error_occured=1;
				printf("Error type 7 at Line %d: only int or float can use \"-\"(minus)\n",h->line);
				*exp_kind=USER_DEFINED;
				*exp_type=NULL;
			}
			else 
			{
				*exp_kind=temp1;
				*exp_type=temp2;
			}
		}
	}
	else if(h->child_count==3)
	{
		/*
		 *  Exp ASSIGNOP Exp
		 *  Exp AND Exp
		 *  Exp OR Exp
		 *  Exp (cal) Exp : cal requires both EXPs are int or (both are) float 
		 *  LP Exp RP 
		 *  ID LP RP
		 *  Exp DOT ID
		 */
		switch(h->child[1]->type)
		{
			case Exp:
			{
				ana_exp(exp_kind,exp_type,h->child[1]);
				break;
			}
			case _LP:
			{
				if(find_value(h->child[0]->name)!=NULL)
				{
					error_occured=1;
					printf("Error type 11 at Line %d: %s is a variable, not a function\n",h->line,h->child[0]->name);
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
					return;
				}
				func_d* temp=(func_d*)check_id(h->child[0],id_func);
				if(temp==NULL)
				{
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
				}
				else if(temp->parameter_count==0)
				{
					*exp_kind=temp->return_kind;
					*exp_type=temp->return_type;
				}
				else
				{
					error_occured=1;
					printf("Error type 9 at Line %d: unmatched parameters for function %s\n",h->line,temp->name);
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
				}
				break;
			}
			case _DOT:
			{
				val_kind temp1;
				type_d* temp2;
				ana_exp(&temp1,&temp2,h->child[0]);
				if(temp1!=USER_DEFINED)
				{
					error_occured=1;
					printf("Error type 13 at Line %d: use \".\" on none struct variable\n",h->line);
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
				}
				else if(temp2==NULL)
				{
					error_occured=1;
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
				}
				else if(temp2->kind==_array)
				{
					error_occured=1;
					printf("Error type 13 at Line %d: use \".\" on none struct variable\n",h->line);
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
				}
				else
				{
					val_d* temp=find_value(h->child[2]->name);
					for(int i=0;i<temp2->def.s->define_count;i++)
						if(temp2->def.s->def_list[i]==temp)
						{
							*exp_kind=temp->kind;
							*exp_type=temp->val_type;
							return;
						}
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
					error_occured=1;
					printf("Error type 14 at Line %d: undefined field %s for struct %s\n",h->line,h->child[2]->name,temp2->name);
				}
				break;
			}
			case _AND:
			case _OR:
			{
				val_kind temp1,temp3;
				type_d* temp2,*temp4;
				ana_exp(&temp1,&temp2,h->child[0]);
				ana_exp(&temp3,&temp4,h->child[2]);
				if((temp1==USER_DEFINED && temp2==NULL) || (temp3==USER_DEFINED && temp4==NULL))
				{
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
				}
				else if(temp1!=_int || temp3!=_int)
				{
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
					error_occured=1;
					printf("Error type 7 at Line %d: only int can be used as boolean\n",h->line);
				}
				else
				{
					*exp_kind=_int;
					*exp_type=NULL;
				}
				break;
			}
			case _ASSIGNOP:
			{
				if(check_left(h->child[0])==0)
				{
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
					error_occured=1;
					printf("Error type 6 at Line %d: the left-hand side of \"=\" must have left side value\n",h->line);
				}
				else
				{
					val_kind temp1,temp3;
					type_d *temp2,*temp4;
					ana_exp(&temp1,&temp2,h->child[0]);
					ana_exp(&temp3,&temp4,h->child[2]);
					if((temp1==USER_DEFINED && temp2==NULL) || (temp3==USER_DEFINED && temp4==NULL))
					{
						*exp_kind=USER_DEFINED;
						*exp_type=NULL;
					}
					else if(temp1==temp3 && type_equal(temp2,temp4))
					{
						*exp_kind=temp1;
						*exp_type=temp2;
					}
					else
					{
						*exp_kind=USER_DEFINED;
						*exp_type=NULL;
						error_occured=1;
						printf("Error type 5 at Line %d: both sides of \"=\" must be the same type\n",h->line);
					}
				}
				break;
			}
			default:
			{
				val_kind temp1,temp3;
				type_d *temp2,*temp4;
				ana_exp(&temp1,&temp2,h->child[0]);
				ana_exp(&temp3,&temp4,h->child[2]);
				if((temp1==USER_DEFINED && temp2==NULL) || (temp3==USER_DEFINED && temp4==NULL))
				{
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
				}
				else if(temp1==temp3 && (temp1==_int || temp1==_float))
				{
					*exp_kind=temp1;
					*exp_type=NULL;
				}
				else
				{
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
					error_occured=1;
					printf("Error type 7 at Line %d: calculation requires both EXPs are int or (both are) float\n",h->line);
				}
				break;
			}
		}
	}
	else if(h->child_count==4)
	{
		/* Exp LB Exp RB
		 * ID LP Args RP
		 */
		if(h->child[0]->type==Exp)
		{
			//�ҵ�ͷ����������Ķ���
			Node* head=h;
			while(head->child_count==4 && head->child[0]->type==Exp)
			{
				head=head->child[0];
			}
			if(!(head->child_count==1 && head->child[0]->type==_ID))
			{
				error_occured=0;
				//printf("Error type 10 at Line %d: use \"[]\" on none array variable\n",h->line);
				*exp_kind=USER_DEFINED;
				*exp_type=NULL;
				return;
			}
			val_d* temp=(val_d*)check_id(head->child[0],id_val);
			if(temp==NULL)
			{
				*exp_kind=USER_DEFINED;
				*exp_type=NULL;
				return;
			}
			if(temp->kind!=USER_DEFINED || temp->val_type->kind!=_array)
			{
				error_occured=1;
				printf("Error type 10 at Line %d: use \"[]\" on none array variable\n",h->line);
				*exp_kind=USER_DEFINED;
				*exp_type=NULL;
				return;
			}
			//˳��ƫ�ƣ�һ��[]��Ӧ���鶨������һ��
			array_def_list* current=temp->val_type->def.a;
			head=head->father;
			while(head!=h && current->dimension!=0)
			{
				//���[]�����ֺϷ���
				val_kind temp1;
				type_d* temp2;
				ana_exp(&temp1,&temp2,head->child[2]);
				if(temp1==USER_DEFINED && temp2==NULL)
				{
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
					return;
				}
				else if(temp1!=_int)
				{
					error_occured=1;
					printf("Error type 12 at Line %d: the type of exp between \"[]\" should be int\n",h->line);
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
					return;
				}
				head=head->father;
				current=current->next;
			}
			if(current->dimension==0 && head!=h)
			{
				//[]����
				error_occured=1;
				printf("Error type 10 at Line %d: use \"[]\" on none array variable\n",h->line);
				*exp_kind=USER_DEFINED;
				*exp_type=NULL;
				return;
			}
			//������һ��[]�����ֵĺϷ���
			val_kind temp1;
			type_d* temp2;
			ana_exp(&temp1,&temp2,head->child[2]);
			if(temp1==USER_DEFINED && temp2==NULL)
			{
				*exp_kind=USER_DEFINED;
				*exp_type=NULL;
				return;
			}
			else if(temp1!=_int)
			{
				error_occured=1;
				printf("Error type 12 at Line %d: the type of exp between \"[]\" should be int\n",h->line);
				*exp_kind=USER_DEFINED;
				*exp_type=NULL;
				return;
			}
			if(head==h && current->dimension==0)
			{
				//�պö��룬ȡ������Ԫ�ض���
				*exp_kind=current->kind;
				*exp_type=current->val_type;
				return;
			}
			else
			{
				//ȡ�ò������顣���ƣ��½�һ����Ӧ�����ͷ����ϲ㡣
				type_d* p=new_type(NULL);
				add_type_declaration(p);
				current=current->next;
				array_def_list* copy_current=p->def.a;
				while(current!=NULL)
				{
					array_def_list* copy_temp=(array_def_list*)malloc(sizeof(array_def_list));
					memcpy(copy_temp,current,sizeof(array_def_list));
					copy_temp->next=NULL;
					if(copy_current==NULL)
						p->def.a=copy_temp;
					else
						copy_current->next=copy_temp;
					copy_current=copy_temp;
					current=current->next;
				}
				*exp_kind=USER_DEFINED;
				*exp_type=p;
			}
		}
		else
		{
			if(find_value(h->child[0]->name)!=NULL)
			{
				error_occured=1;
				printf("Error type 11 at Line %d: %s is a variable, not a function\n",h->line,h->child[0]->name);
				*exp_kind=USER_DEFINED;
				*exp_type=NULL;
				return;
			}
			func_d* temp=(func_d*)check_id(h->child[0],id_func);
			if(temp==NULL)
			{
				*exp_kind=USER_DEFINED;
				*exp_type=NULL;
				return;
			}
			//����args�����м�������
			Node* args_temp=h->child[2];
			int count=1;
			while(args_temp->child_count==3)
			{
				count++;
				args_temp=args_temp->child[2];
			}
			if(count!=temp->parameter_count)
			{
				error_occured=1;
				printf("Error type 9 at Line %d: unmatched parameters for function %s\n",h->line,temp->name);
				*exp_kind=USER_DEFINED;
				*exp_type=NULL;
				return;
			}
			//���μ���������
			args_temp=h->child[2];
			for(int i=0;i<count;i++)
			{
				val_kind temp1;
				type_d* temp2;
				ana_exp(&temp1,&temp2,args_temp->child[0]);
				if(temp1==USER_DEFINED && temp2==NULL)
				{
					*exp_kind=USER_DEFINED;
					*exp_type=NULL;
					return;
				}
//				if(temp1!=temp->kinds[i] || temp2!=temp->parameters[i])
//				{
//					error_occured=1;
//					printf("Error type 9 at Line %d: unmatched parameters for function %s???\n",h->line,temp->name);
//					*exp_kind=USER_DEFINED;
//				*exp_type=NULL;
//					return;
//				}
				if(args_temp->child_count==3)
					args_temp=args_temp->child[2];
			}
			*exp_kind=temp->return_kind;
			*exp_type=temp->return_type;
		}
	}
}
void* check_id(Node* h,id_type identity)
{
	if(identity==id_func)
	{
		func_d* temp=find_function(h->name);
		if(temp==NULL)
		{
			printf("Error type 2 at Line %d: undefined function %s\n",h->line,h->name);
			error_occured=1;
		}
		return temp;
	}
	else
	{
		val_d* temp=find_value(h->name);
		if(temp==NULL)
		{
			error_occured=1;
			printf("Error type 1 at Line %d: undefined variable %s\n",h->line,h->name);
		}
		return temp;
	}
}
int check_left(Node* h)
{
	if(h->child_count==1 && h->child[0]->type==_ID)
		return 1;
	if(h->child_count==3 && h->child[1]->type==_DOT)
		return 1;
	if(h->child_count==4 && h->child[1]->type==_LB)
		return 1;
	return 0;
}