#include "ir_buffer.h" 

//ɾ��pָ��Ĵ����У����Ұ�pָ������һ�С��������Ωһ��һ�У���ɾ����p=NULL
#define delete_code_node(p) \
do \
{ \
	if((p)->next==(p)) \
	{ \
		free(p); \
		(p)=NULL; \
		head=NULL; \
	} \
	else \
	{ \
		code_node *temp_ptr=(p); \
		(p)=(p)->prev; \
		(p)->next=temp_ptr->next; \
		(p)->next->prev=(p); \
		free(temp_ptr); \
	} \
}while(0)

//label,temp�ļ����������������µ�label��temp
static int temp_count=0,label_count=0, v_name_count=0;
//����buff�����ͷָ�롣
static code_node* head=NULL;
//�Ƿ񱾴����Ż�
static int changed=0;
//�Ż�ѡ��
static int opt=1;

static variable_alies* v_name=NULL;

//�м��������Ż�
void optimize();
//�м��������Ż�:jump
void optimize_jump();
//�м��������Ż���exp,exp�ֳ����Ļ�����(��ʼ�ڵ�ͽ����ڵ㣬����ҿ�)
void optimize_exp();
void optimize_block(code_node* begin,code_node* end);
//�ͷ��м����buffer
void ir_buffer_destroy();
//���ļ��д�ӡһ�д���
void print_one_line(FILE* fp,code_node* p);
//һ��label�Ƿ��Ǳ�ʹ�õģ����������label����ɾ���ˣ�
int label_is_using(char* name)
{
	code_node* p=head;
	do
	{
		if(p->args_count==6 && strcmp(p->args[5],name)==0)
			return 1;
		else if(strcmp(p->args[0],"GOTO")==0 && strcmp(p->args[1],name)==0)
			return 1;
		p=p->next;
	}while(p!=head);
	return 0;
}
//һ�������Ƿ����ã�������������Ķ������ɾ���ˡ�
int var_is_using(char* name)
{
	if(name[0]=='*')return 1;
	code_node* p=head;
	do
	{
		if(strcmp(name,p->args[0])==0 && p->args_count==4)
			return 1;
		if(strcmp(name,&(p->args[0][1]))==0 && p->args[0][0]=='*')
			return 1;
		for(int i=1;i<p->args_count;i++)
			if(strcmp(name,p->args[i])==0 || strcmp(name,&(p->args[i][1]))==0)
				return 1;
		p=p->next;
	}while(p!=head);
	return 0;
}
//һ��������ֱ�����´θ�ֵ���Ƿ����ã�������������Ķ����п���ɾ���ˡ�
int var_is_using_til(code_node* begin,code_node* end)
{
	if(begin->args_count==4)return 1;
	if(begin->args[0][0]=='*')return 1;
	code_node* p=begin->next;
	while(p!=end)
	{
		if(strcmp(begin->args[0],p->args[0])==0)
		{
			for(int i=1;i<p->args_count;i++)
			{
				if(strcmp(begin->args[0],p->args[i])==0 || strcmp(begin->args[0],&(p->args[i][1]))==0)
					return 1;
			}
			return 0;
		}
		if(strcmp(begin->args[0],&(p->args[0][1]))==0 && p->args[0][0]=='*')
			return 1;
		for(int i=1;i<p->args_count;i++)
			if(strcmp(begin->args[0],p->args[i])==0 || strcmp(begin->args[0],&(p->args[i][1]))==0)
				return 1;
		p=p->next;
	}
	return 1;
}
//�鿴p�Ĳ����Ƿ���q����ֵ,����p�ڴ˴��ı���
int var_relevant(code_node* p,code_node* q)
{
	if(strcmp(q->args[1],":=")!=0)return 0;
	if(strcmp(p->args[0],q->args[0])==0)return 1;
	if(p->args_count==4)return 0;
	if(p->args[2][0]!='&' && p->args[2][0]!='#')
	{
		if(p->args[2][0]=='*' && strcmp(q->args[0],&(p->args[2][1]))==0)
			return 1;

		else if(strcmp(q->args[0],p->args[2])==0)
			return 1;
	}
	if(p->args_count==5)
	{
		if(p->args[4][0]!='&' && p->args[4][0]!='#')
		{
			if(p->args[4][0]=='*' && strcmp(q->args[0],&(p->args[4][1]))==0)
				return 1;
			
			else if(strcmp(q->args[0],p->args[4])==0)
				return 1;
        
      else if(strcmp(q->args[0],p->args[2])==0)
				return 1;
		}
	}
	return 0;
}
//����var�Ķ�����һ���ġ�
int var_equal(code_node* p,code_node* q)
{
	if(p->args_count!=q->args_count)return 0;
	if(p->args_count==4)return 0;
	if(p->args[0][0]=='*' || q->args[0][0]=='*')return 0;
	if(strcmp(q->args[1],":=")!=0)return 0;
	if(p->args_count==3)
	{
		if(strcmp(p->args[2],q->args[2])!=0)
			return 0;
		else 
			return 1;
	}
	else
	{
		if(strcmp(p->args[3],q->args[3])!=0)return 0;
		if(strcmp(p->args[3],"+")==0 || strcmp(p->args[3],"*")==0)
		{
			if(strcmp(p->args[2],q->args[2])==0 && strcmp(p->args[4],q->args[4])==0)
				return 1;
			else if(strcmp(p->args[2],q->args[4])==0 && strcmp(p->args[4],q->args[2])==0)
				return 1;
			else
				return 0;
		}
		else
		{
			if(strcmp(p->args[2],q->args[2])==0 && strcmp(p->args[4],q->args[4])==0)
				return 1;
			else
				return 0;
		}
	}
}
//���ӿ�ʼ��������old var�滻��new var,��������ҿ���ֱ�����˸ı䡣
void var_change(code_node* begin,code_node* end,char* old,char* new)
{
	code_node* p=begin;
	do
	{
		for(int i=1;i<p->args_count;i++)
		{
			if(strcmp(old,p->args[i])==0)
			{
				changed=1;
				strcpy(p->args[i],new);
			}
			else if(strcmp(old,&(p->args[i][1]))==0 && p->args[i][0]!='&')
			{
				changed=1;
				strcpy(&(p->args[i][1]),new);
			}
		}
		if(p->args[0][0]=='*' && strcmp(&(p->args[0][1]),old)==0)
		{
			changed=1;
			strcpy(&(p->args[0][1]),new);
		}
		p=p->next;
	}while(p!=end && strcmp(p->args[0],old)!=0 && strcmp(p->args[0],new)!=0);
}
//�������õ�old label�ĵط��滻��new label
void label_change(char* old,char* new)
{
	code_node* p=head;
	do
	{
		if(p->args_count==6 && strcmp(p->args[5],old)==0)
		{
			changed=1;
			strcpy(p->args[5],new);
		}
		else if(strcmp(p->args[0],"GOTO")==0 && strcmp(p->args[1],old)==0)
		{
			changed-1;
			strcpy(p->args[1],new);
		}
		p=p->next;
	}while(p!=head);
}
//�õ�realop�ķ���
void make_oppo_relop(char* op)
{
	if(strcmp(op,">")==0)
		strcpy(op,"<=");
	else if(strcmp(op,"<")==0)
		strcpy(op,">=");
	else if(strcmp(op,">=")==0)
		strcpy(op,"<");
	else if(strcmp(op,"<=")==0)
		strcpy(op,">");
	else if(strcmp(op,"==")==0)
		strcpy(op,"!=");
	else if(strcmp(op,"!=")==0)
		strcpy(op,"==");
}

void close_opt()
{
	opt=0;
}
//�����µ�label�����Ʒ����ṩ�õ�name���档
void new_label(char* name)
{
	strcpy(name,"label");
	char temp[20];
	itoa(label_count,temp,10);
	label_count++;
	strcat(name,temp);
}
//�����µ���ʱ���������Ʒ����ṩ�õ�name���档
void new_temp(char* name)
{
	strcpy(name,"t");
	char temp[20];
	itoa(temp_count,temp,10);
	temp_count++;
	strcat(name,temp);
}
//���һ�����룬ָ����������Ĵ�����Ȼ�����������������ﶼ��char*�����������ַ���
void add_code(int args_count,...)
{
	code_node* p=(code_node*)malloc(sizeof(code_node));
	p->args_count=args_count;
	va_list ap;
	va_start(ap,args_count);
	for(int i=0;i<args_count;i++)
		strcpy(p->args[i],va_arg(ap,char*));
	if(head==NULL)
	{
		p->prev=p;
		p->next=p;
		head=p;
	}
	else
	{
		p->next=head;
		p->prev=head->prev;
		head->prev->next=p;
		head->prev=p;
	}
}
//���ڴ��еĴ����ӡ���ļ��У��������ļ�·��
void print_code(FILE* fp)
{
	if(head==NULL)return;
	assert(fp!=NULL);
	if(opt)optimize();
	code_node* p=head;
	do
	{
		print_one_line(fp,p);
		p=p->next;
	}while(p!=head);
	ir_buffer_destroy();
	fclose(fp);
}
//�ͷ��м����buffer
void ir_buffer_destroy()
{
	if(head==NULL)return;
	code_node* p=head->next,*q;
	free(head);
	while(p!=head)
	{
		q=p;
		p=p->next;
		free(q);
	}
}
//���ļ��д�ӡһ�д���
void print_one_line(FILE* fp,code_node* p)
{
	if(p->args_count!=3 && p->args[0][0]=='*')
	{
		char temp[32];
		new_temp(temp);
		fprintf(fp,"%s",temp);
		for(int i=1;i<p->args_count;i++)
			fprintf(fp," %s",p->args[i]);
		fprintf(fp,"\n");
		fprintf(fp,"%s := %s\n",p->args[0],temp);
	}
	else if(p->args_count==2 && p->args[1][0]=='*' && strcmp(p->args[0],"READ")==0)
	{
		char temp[32];
		new_temp(temp);
		fprintf(fp,"READ %s\n",&temp[1]);
		fprintf(fp,"%s := %s\n",p->args[1],&temp[1]);
	}
	else
	{
		fprintf(fp,"%s",p->args[0]);
		for(int i=1;i<p->args_count;i++)
			fprintf(fp," %s",p->args[i]);
		fprintf(fp,"\n");
	}
}
//�м��������Ż�:jump
void optimize_jump()
{
	/* ���Ƚ������Ż���if xxx goto a �Ż���,�Ӻ���ǰ��
	 * 1��if xxx goto a; (���ɸ�label����); LABEL a; ==> (���ɸ�label����); LABEL a;
	 * 2��if xxx goto a; goto b; (���ɸ�label����); LABEL a; ==> if (NOT)xxx goto b; (���ɸ�label����); LABEL a;
	 */
	code_node* p=head->prev;
	//1
	do
	{
		if(p->args_count==6)
		{
			code_node* q=p->next;
			while(strcmp("LABEL",q->args[0])==0)
			{	
				if(strcmp(q->args[1],p->args[5])==0)
					break;
				q=q->next;
			}
			if(strcmp("LABEL",q->args[0])==0)
			{	
				changed=1;
				delete_code_node(p);
				p=p->next;
			}
		}
		p=p->prev;
	}while(p!=head->prev);
	//2
	p=head;
	do
	{
		if(p->args_count==6)
		{
			code_node* q=p->next;
			while(strcmp(q->args[0],"LABEL")==0)
				q=q->next;
			if(strcmp("GOTO",q->args[0])==0)
			{
				code_node* r=q;
				q=q->next;
				while(strcmp("LABEL",q->args[0])==0)
				{	
					if(strcmp(q->args[1],p->args[5])==0)
						break;
					q=q->next;
				}
				if(strcmp("LABEL",q->args[0])==0)
				{	
					make_oppo_relop(p->args[2]);
					strcpy(p->args[5],r->args[1]);
					delete_code_node(r);
					changed=1;
				}
			}
		}
		p=p->next;
	}while(p!=head);
	// �Ż� goto a; (���ɸ�label����); LABEL a; ==> (���ɸ�label����); LABEL a; �Ӻ���ǰ
	p=head->prev;
	do
	{
		if(strcmp("GOTO",p->args[0])==0)
		{
			code_node* q=p->next;
			while(strcmp("LABEL",q->args[0])==0)
			{	
				if(strcmp(q->args[1],p->args[1])==0)
					break;
				q=q->next;
			}
			if(strcmp("LABEL",q->args[0])==0)
			{	
				delete_code_node(p);
				changed=1;
				p=p->next;
			}
		}
		p=p->prev;
	}while(p!=head->prev);
	/* �Ż� goto a�� (����S��û��label)��label *����ʾ��ʵ�м�������Զ�ǲ��ɴ�ģ�
	 * ������Sֻ�ܴ�goto��ʼ˳��ִ�е��������ܴ������ط���ת������������S����ɾ����
	*/
	p=head;
	do
	{
		if(strcmp("GOTO",p->args[0])==0)
		{
			code_node* q=p->next;
			while(q!=head && strcmp("LABEL",q->args[0])!=0)
				q=q->next;
			if(q==head)
			{
				p=p->next;
				continue;
			}
			while(p->next!=q)
			{
				changed=1;
				p=p->next;
				delete_code_node(p);
			}
		}
		p=p->next;
	}while(p!=head);
	/* 1��goto a;...;LABEL a;(LABELs);goto b; ==> goto b;...;LABEL a;(LABELs);goto b;
	 * 2��goto a;...;LABEL a;(LABELs);return *; ==> return *;...;LABEL a;(LABELs);return *;
	 * 3��LABEL a;(LABELs);goto a; ==>LABEL a;(LABELs);
	 * ������滻�У�1��3���� if xxx goto a Ҳ����Ч�ġ�
	 */
	p=head;
	do
	{
		if(strcmp("GOTO",p->args[0])==0)
		{
			code_node* q=p->next;
			while(strcmp("LABEL",q->args[0])!=0 || strcmp(q->args[1],p->args[1])!=0)
				q=q->next;
			while(strcmp("LABEL",q->args[0])==0)
				q=q->next;
			if(p==q)
			{
				changed=1;
				delete_code_node(p);
			}
			else if(strcmp(q->args[0],"GOTO")==0)
			{
				changed=1;
				strcpy(p->args[1],q->args[1]);
			}
			else if(strcmp(q->args[0],"RETURN")==0)
			{
				changed=1;
				strcpy(p->args[0],q->args[0]);
				strcpy(p->args[1],q->args[1]);
			}
		}
		else if(p->args_count==6)
		{
			code_node* q=p->next;
			while(strcmp("LABEL",q->args[0])!=0 || strcmp(q->args[1],p->args[5])!=0)
				q=q->next;
			while(strcmp("LABEL",q->args[0])==0)
				q=q->next;
			if(p==q)
			{
				changed=1;
				delete_code_node(p);
			}
			else if(strcmp(q->args[0],"GOTO")==0)
			{
				changed=1;
				strcpy(p->args[5],q->args[1]);
			}
		}
		p=p->next;
	}while(p!=head);
	//��return�������䣬ֱ����һ��label/function��������Զ���ɴ�ġ�ɾ�����ǡ�
	p=head;
	do
	{
		if(strcmp(p->args[0],"RETURN")==0)
			while(p->next!=head && strcmp(p->next->args[0],"LABEL")!=0 && strcmp(p->next->args[0],"FUNCTION")!=0)
			{
				p=p->next;
				changed=1;
				delete_code_node(p);
			}
		p=p->next;
	}while(p!=head);
	//label ȥ�ء� LABEL a�� LABEL b����������õ�b�ĵط������a��ɾ��b��
	p=head;
	do
	{
		if(strcmp(p->args[0],"LABEL")==0)
			while(strcmp(p->next->args[0],"LABEL")==0)
			{
				label_change(p->next->args[1],p->args[1]);
				p=p->next;
				changed=1;
				delete_code_node(p);
			}
		p=p->next;
	}while(p!=head);
	//ȥ������ʹ�õ�label
	p=head;
	do
	{
		if(strcmp(p->args[0],"LABEL")==0)
			if(!label_is_using(p->args[1]))
			{	
				delete_code_node(p);
				changed=1;
			}
		p=p->next;
	}while(p!=head);
}
void optimize_exp()
{
	//��һ������Ƭ����Ƭ�ڲ����ֲ��Ż�
	code_node* p=head,*q;
	do
	{
		q=p->next;
		while(1)
		{
			if(strcmp(q->args[0],"LABEL")==0)
				break;
			else if(strcmp(q->args[0],"GOTO")==0 || strcmp(q->args[0],"RETURN")==0 || q->args_count==6)
			{
				q=q->next;
				break;
			}
			else if(q==head) break;
			q=q->next;
		}
		optimize_block(p,q);
		p=q;
	}while(p!=head);
	//�ڶ��������û���õı�����ֵ��
	p=head;
	do
	{
		if(strcmp(p->args[1],":=")==0)
		{	
			if(!var_is_using(p->args[0]))
			{	
				delete_code_node(p);
				changed=1;
			}
		}
		p=p->next;
	}while(p!=head);
}
void optimize_block(code_node* begin,code_node* end)
{
	//�ظ�һ���ı�����ֵģʽ�滻
	code_node* p=begin,*q;
	do
	{
		if(strcmp(p->args[1],":=")==0)
		{
			q=p->next;
			while(q!=end && !var_relevant(p,q))
			{
				if(var_equal(p,q))
				{
					var_change(q,end,q->args[0],p->args[0]);
				}
				q=q->next;
			}
		}
		p=p->next;
	}while(p!=end);
	//a=b-b==>a=0 a=b/b==>a=1
	p=begin;
	do
	{
		if(p->args_count==5)
		{
			if(strcmp(p->args[2],p->args[4])==0)
			{
				if(strcmp(p->args[3],"-")==0)
				{
					changed=1;
					p->args_count=3;
					strcpy(p->args[2],"#0");
				}
				else if(strcmp(p->args[3],"/")==0)
				{
					changed=1;
					p->args_count=3;
					strcpy(p->args[2],"#1");
				}
			}
		}
		p=p->next;
	}while(p!=end);
	//ɾ��a=a����������ĸ�ֵ��
	p=begin;
	do
	{
		if(p->args_count==3 && strcmp(p->args[1],":=")==0 && strcmp(p->args[0],p->args[2])==0)
		{
			changed=1;
			delete_code_node(p);
		}
		p=p->next;
	}while(p!=end);
	//a=...(1);...(û�õ�a);a=...(2); ==> ...(û�õ�a);a=...(2);
	p=begin;
	do
	{
		if(strcmp(p->args[1],":=")==0)
		{
			if(!var_is_using_til(p,end))
			{
				changed=1;
				delete_code_node(p);
			}
		}
		p=p->next;
	}while(p!=end);
  p=begin;
  do
  {
    q = p->next;
    if(strcmp(p->args[1],":=")==0 && strcmp(p->args[1],":=")==0)
		{
			if(!var_is_using_til(p,end))
			{
				changed=1;
				delete_code_node(p);
			}
		}
    p = p->next;
  }while(p!=end);
}
void optimize()
{
	do
	{
		changed=0;
		optimize_jump();
		optimize_exp();
	}while(changed);
}