#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include "lexan.h"

/* Grammer
(0)  E -> T E��
(1)  E�� -> + T E��
(2)  E�� -> - T E��
(3)  E�� ->  e(Epsilon)
(4)  T -> F T��
(5)  T�� -> * F T��
(6)  T�� -> / F T��
(7)  T�� ->   e(Epsilon)
(8)  F -> ( E )
(9)  F ->   id
(10) F ->  num
*/

/* Each terminal or non-terminal symbol that grammer have. { Symbol(symbol-number) }
Non-Terminal -> E(0), E'(1), T(2), T'(3), F(4)
Terminal -> +(0), -(1), e(2), *(3), /(4), '(' (5), ')' (6), id(7), num(8), $(9)(EOF identifier)
*/

#define MaxTerminal		10
#define MaxNonTerminal	5

/* Identify one symbol is terminal or non-terminal and contain information about symbol name.*/
typedef struct Symbol {
	/* Terminal : 0 , Nonterminal : 1 */
	int kind;
	int sym_no;
}sym;

/* This structure contain each rule's information as using sym strcuture. */
typedef struct orule {

	sym left;
	sym righthand[10];
	int rightnum; // Count of right hand side.
}onerule;

/* lev�� �Ľ�Ʈ���� ��Ÿ�� �� ��� */
typedef struct RuleList {
	sym a_rule;
	struct RuleList* next;
	int lev;
	struct RuleList* prt; //�θ��� ����
	int y; 
}list;

typedef struct TokenSTA {
	char num[10];
	int uniNum;
	char type[10];
	char token[10];
	char att[20];
}tok;

/* Using this array to make rules structure. */
/* i= id, n=num */
char NameTerminals[MaxTerminal] = { '+','-','e','*','/','(',')','i','n','$' };
char NameNonTerminals[MaxNonTerminal][2] = { {'E'},{'E','\''},{'T'},{'T','\''},{'F'} };

/* Make rules structure from grammer */
onerule Rules[11] = {
	{ {1,0}, { {1,2},{1,1} }, 2 },			//E->TE'
	{ {1,1}, { {0,0},{1,2},{1,1} }, 3 },	//E'->+TE'
	{ {1,1}, { {0,1},{1,2},{1,1} }, 3 },	//E'->-TE'
	{ {1,1}, { {0,2} }, 1 },				//E'->e
	{ {1,2}, { {1,4},{1,3}}, 2 },			//T->FT'
	{ {1,3}, { {0,3},{1,4},{1,3} }, 3 },	//T'->*FT'
	{ {1,3}, { {0,4},{1,4},{1,3} }, 3 },	//T'->/FT'
	{ {1,3}, { {0,2} }, 1 },				//T'->e
	{ {1,4}, { {0,5},{1,0},{0,6} }, 3 },	//F->( E )
	{ {1,4}, { {0,7} }, 1 },				//F->id
	{ {1,4}, { {0,8} }, 1 }					//F-num
};
FILE* fp;
list* head;
list* ohead;


/* This arrays will contain information about each Non-Termianl symbol's first and follow. */
char FirstArray[MaxNonTerminal][MaxTerminal + 2];
char FollowArray[MaxNonTerminal][MaxTerminal + 1];
/* �Ľ����̺��� ����� �� �迭�� ���ҿ� �ش�Ǵ� Rule Number�� �����Ѵ�.
   �� ��� ���� index number�� NameNonTerminals �� NameTerminals�� �� index ��ȣ�� �����Ѵ�.
   ������ Maxterminal������ ParsingTable�� �ϼ� ���θ� ���� (�ϼ� : 1)*/
int ParsingTable[MaxNonTerminal][MaxTerminal + 1];

gotoxy(int x, int y)//���� ���ϴ� ��ġ�� Ŀ�� �̵�
{
	COORD pos = { x - 1, y - 1 };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}


void InitiateParsingTable() {
	int i, j;
	for (i = 0; i < MaxNonTerminal; i++) {
		for (j = 0; j < MaxTerminal + 1; j++) {
			ParsingTable[i][j] = 99;
		}
	}
}

/* This function will copy from source's First to FirstArray[target][] */
/* If flag=0 -> There is epsilon in First Array.
   If flag=1 -> There is no epsilon in First Array. */
void InsertFirst(int target, sym source, int flag) {
	int i, j, k;
	if (flag == 0) {
		for (i = 0; i < MaxTerminal; i++) {
			if (FirstArray[target][i] == NULL) {
				break;
			}
		}
		/* Copy all First from source to target without epsilon. */
		for (j = 0; FirstArray[source.sym_no][j] != NULL; j++) {
			if (FirstArray[source.sym_no][j] != 'e') {
				FirstArray[target][i] = FirstArray[source.sym_no][j];
				i++;
			}
		}
	}
	/* If source is non-terminal then copy all elements in First Array. */
	else if (source.kind == 1) {
		for (i = 0; i < MaxTerminal; i++) {
			if (FirstArray[target][i] == NULL) {
				break;
			}
		}
		for (j = 0; FirstArray[source.sym_no][j] != NULL; j++) {
			FirstArray[target][i] = FirstArray[source.sym_no][j];
			i++;
		}
	}
	/* If souce is terminal then allocate to the target's First Array only one terminal symbol */
	else {
		for (i = 0; i < MaxTerminal; i++) {
			if (FirstArray[target][i] == NULL) {
				break;
			}
		}
		FirstArray[target][i] = NameTerminals[source.sym_no];
	}
}


/* First function will receive one symbol. Then fill up it's FirstArray and associated symbol's First. */
void First(sym symbol) {
	sym eps = { 0,2 };
	int ruleNumber = 0;
	int index = 0;
	/* If First Array already finish then return. */
	if (FirstArray[symbol.sym_no][MaxTerminal + 1] == '1') {
		return;
	}
	/* Find a rule number whose LHS equal symbol paramater. */
	for (ruleNumber; ruleNumber < 11; ruleNumber++) {
		if (Rules[ruleNumber].left.sym_no == symbol.sym_no) {

			/* A symbol is classified terminal or non-terminal.  */
			if (Rules[ruleNumber].righthand[index].kind == 1) { //Non-terminal
				if (FirstArray[Rules[ruleNumber].righthand[index].sym_no][MaxTerminal + 1] == '1') { //Already finish First
					if (FirstArray[Rules[ruleNumber].righthand[index].sym_no][MaxTerminal] == '1') { //There is epsilon
						InsertFirst(symbol.sym_no, Rules[ruleNumber].righthand[index], 0);

						/* In E->FT case, if F has epsilon in First then find T's First. */
						index++;
						if (index <= Rules[symbol.sym_no].rightnum) {
							First(Rules[ruleNumber].righthand[index]);
						}

					}
					else {
						InsertFirst(symbol.sym_no, Rules[ruleNumber].righthand[index], 1);
					}
				}
				else {

					First(Rules[ruleNumber].righthand[index]);
					if (FirstArray[Rules[ruleNumber].righthand[index].sym_no][MaxTerminal] == '1') { //There is epsilon
						InsertFirst(symbol.sym_no, Rules[ruleNumber].righthand[index], 0);
						index++;
						if (index <= Rules[symbol.sym_no].rightnum) {
							First(Rules[ruleNumber].righthand[index]);
						}

					}
					else {
						InsertFirst(symbol.sym_no, Rules[ruleNumber].righthand[index], 1);
					}
				}

			}
			else { //Terminal
				if (Rules[ruleNumber].righthand[index].sym_no == 2) { //Epsilon
					InsertFirst(symbol.sym_no, eps, 1);
					FirstArray[symbol.sym_no][MaxTerminal] = '1';
				}
				else {
					InsertFirst(symbol.sym_no, Rules[ruleNumber].righthand[index], 1);
				}
			}
		}
	}
	/* If finish First process then express it as FirstArray's last element change to '1'.*/
	FirstArray[symbol.sym_no][MaxTerminal + 1] = '1';

}

/* This function will check if there are already exist input character at the Follow Array.*/
int CloneCheck(int target, char input) {
	int i;
	for (i = 0; i < MaxTerminal; i++) {
		if (FollowArray[target][i] == input) {
			return -1;
		}
	}
	return 1;
}

/* ���� ��ȣ �ϳ��� �޾� First�� Epsilon�� ���ԵǾ� �ִ����� Ȯ��.
   �ִٸ� -1�� ��ȯ ������ 1�� ��ȯ*/
int EpsilonCheck(int rowNum) {
	int i;
	for (i = 0; i < MaxTerminal; i++) {
		if (FirstArray[rowNum][i] == 'e') {
			return -1;
		}
	}
	return 1;
}

/* This function will copy source's Follow to target's Follow array.
   Flag is represent what to do.
   Flag:0 -> Copy from First Array.
   Flag:1 -> Copy from NameTerminals.
   Flag:2 -> Copy from Follow Array.*/
void InsertFollow(int target, int flag, int src_no) {
	int i, j;
	int clnchk;
	switch (flag) {
		/*First*/
	case 0:
		for (i = 0; i < MaxTerminal; i++) {
			if (FollowArray[target][i] == NULL) {
				break;
			}
		}
		for (j = 0; j < MaxTerminal; j++) {
			if (FirstArray[src_no][j] != 'e') {
				if (CloneCheck(target, FirstArray[src_no][j]) == 1) {
					FollowArray[target][i] = FirstArray[src_no][j];
					i++;
				}
			}
		}
		break;

		/*Terminal*/
	case 1:
		for (i = 0; i < MaxTerminal; i++) {
			if (FollowArray[target][i] == NULL) {
				break;
			}
		}
		if (CloneCheck(target, NameTerminals[src_no]) == 1) {
			FollowArray[target][i] = NameTerminals[src_no];
		}
		break;

		/*Follow*/
	case 2:
		for (i = 0; i < MaxTerminal; i++) {
			if (FollowArray[target][i] == NULL) {
				break;
			}
		}
		for (j = 1; j < MaxTerminal; j++) {
			if (CloneCheck(target, FollowArray[src_no][j]) == 1) {
				FollowArray[target][i] = FollowArray[src_no][j];
				i++;
			}
		}
		break;

	}
}

/* This function will fill up symbol's Follow Array.
   A->aBb : Input all elements in First(b) without epsilon to Follow(B).
   A->aB, A->aBb(First(b) has epsilon)
   : Input all elements in Follow(A) to Follow(B).*/
void Follow(sym symbol) {
	int index;
	int i;
	int ruleNumber;

	/* At the beginning we must input symbol '$' at the first of Follow Array.*/
	FollowArray[symbol.sym_no][0] = '$';

	for (ruleNumber = 0; ruleNumber < 11; ruleNumber++) {
		for (index = 0; index < Rules[ruleNumber].rightnum; index++) {
			/* Find a index which forms look like 'aBb' at the RHS.*/
			if (symbol.kind == Rules[ruleNumber].righthand[index].kind && symbol.sym_no == Rules[ruleNumber].righthand[index].sym_no) {

				/* Checking if index is out of bound. */
				if (index + 1 < Rules[ruleNumber].rightnum) {
					if (Rules[ruleNumber].righthand[index + 1].kind == 1) {
						/* Non-terminal, Input all elements in First(b) without epsilon.*/
						InsertFollow(symbol.sym_no, 0, Rules[ruleNumber].righthand[index + 1].sym_no);
						for (i = 0; i < MaxTerminal; i++) {

							/* If First(b) has epsilon then add Follow(A) in Follow(B). */
							if (FirstArray[Rules[ruleNumber].righthand[index + 1].sym_no][i] == 'e') {
								if (symbol.sym_no != Rules[ruleNumber].left.sym_no) {
									InsertFollow(symbol.sym_no, 2, Rules[ruleNumber].left.sym_no);
								}
								break;
							}
						}
					}
					else { //Terminal symbol.
						InsertFollow(symbol.sym_no, 1, Rules[ruleNumber].righthand[index + 1].sym_no);
					}
				}
				else { // Form is look like 'aB'
					if (symbol.sym_no != Rules[ruleNumber].left.sym_no) {
						InsertFollow(symbol.sym_no, 2, Rules[ruleNumber].left.sym_no);
					}
				}
			}
		}
	}
	FollowArray[symbol.sym_no][MaxTerminal] = '1';

}

/* FirstArray�� Nonterminal Symbol�� Row�� epsilon�� ������ ��� ���Ҹ�
   ParsingTable�� �ش�Ǵ� Nonterminal Symbol�� Row�� �����Ѵ�.
   Flag���� 0�̸�, �ش� Nonterminal Symbol�� First���� ParsingTable�� �����ϰ�,
   Flag���� 1�̸�, �ش� Nonterminal Symbol�� Follow���� ParsingTable�� �����Ѵ�.*/
void InsertParsing(int sym_no, int ruleNumber, int flag) {
	int index = 0;
	int i;
	int j;
	if (flag == 0) { //First �Ҵ�
		for (i = 0; i < MaxTerminal; i++) {
			if (FirstArray[sym_no][i] != 'e'&&FirstArray[sym_no][i] != NULL) {
				for (j = 0; j < MaxTerminal; j++) {
					if (FirstArray[sym_no][i] == NameTerminals[j]) {
						break;
					}
				}
				ParsingTable[Rules[ruleNumber].left.sym_no][j] = ruleNumber;
			}
		}
	}
	else { // Follow �Ҵ�
		for (i = 0; i < MaxTerminal; i++) {
			if (FollowArray[sym_no][i] != NULL) {
				for (j = 0; j < MaxTerminal; j++) {
					if (FollowArray[sym_no][i] == NameTerminals[j]) {
						break;
					}
				}
				ParsingTable[Rules[ruleNumber].left.sym_no][j] = ruleNumber;
			}
		}
	}
}

/* �̹� ������� First�� Follow Array�� �̿��Ͽ� ParsingTable�� ����.
   A->a�� ���ؼ� First(a)�� �ִ� �� Terminal symbol�� Ȯ���Ͽ� ParsingTable[A][a]�� �ش�Ǵ� Rule��ȣ�� �Է�.
   ���� e�� ������ Follow(A)���� �� Terminal symbol�� Ȯ���Ͽ� �����۾�.*/
void MakeParsingTable() {
	int index = 0;
	int ruleNumber;
	int i;

	for (ruleNumber = 0; ruleNumber < 11; ruleNumber++) {
		//Non-terminal�� ��� + Parsingtable�� �ϼ����� ���� Non-terminal Symbol�� ���ؼ��� ParsingTable �����۾��� ��.
		if (ParsingTable[Rules[ruleNumber].left.sym_no][MaxTerminal] != '1'&&Rules[ruleNumber].righthand[index].kind == 1) {
			InsertParsing(Rules[ruleNumber].righthand[index].sym_no, ruleNumber, 0);
			if (EpsilonCheck(Rules[ruleNumber].righthand[index].sym_no) == -1) {
				index++;
				if (Rules[ruleNumber].rightnum > index) {
					InsertParsing(Rules[ruleNumber].left.sym_no, ruleNumber, 1);
				}
			}

		}
		//Terminal Symbol�� ��� ParsingTable�� �ش�Ǵ� Terminal Symbol ���� RuleNumber�� �߰����ش�.
		else if (Rules[ruleNumber].righthand[index].kind == 0) {
			ParsingTable[Rules[ruleNumber].left.sym_no][Rules[ruleNumber].righthand[index].sym_no] = ruleNumber;
			//���� RHS�� epsilon�� ��� LHS�� Follow�� ParsingTable�� �߰��Ѵ�.
			if (Rules[ruleNumber].righthand[index].sym_no == 2) {
				InsertParsing(Rules[ruleNumber].left.sym_no, ruleNumber, 1);
			}
		}
	}
	ParsingTable[Rules[ruleNumber].left.sym_no][MaxTerminal] == '1';

}

/* �Ľ� �� ������ ���� �Լ�. ������ ù ���� �з��̹Ƿ� �� �Լ����� ���� �����͸� 2��° �ٺ��ͷ� �ű��.
   ���� ���ο� list�� ����� �� ����Ʈ�� ù ��° symbol�� E�� start symbol�� �����Ѵ�.*/
void InitiateParsing() {
	char tmp[100];
	LexicalAnalyzer();
	fp = fopen("outputfile.txt", "r");
	fgets(tmp, 100, fp);
	head = (list*)malloc(sizeof(list));
	head->a_rule.kind = 1;
	head->a_rule.sym_no = 0;
	head->next = NULL;
	head->lev = 0;
	ohead = (list*)malloc(sizeof(list));
	ohead->a_rule.kind = 1;
	ohead->a_rule.sym_no = 0;
	ohead->next = NULL;
	ohead->lev = 0;
}

/* �Ľ����̺�� ���� sym_no ��� index���� �ִ� ruleNumber�� �̿��Ͽ� �� ���� Linked List�� �����Ѵ�.
   olist�� �Ľ� ������ �����Ѵ� ����Ʈ�̰� list�� �Ľ� �������� �ʿ��� ����Ʈ�̴�.
   lev���� level�� ���ϰ� �� �Ľ̰����ܰ踦 ���� ���µ� ���ȴ�. */
int InsertList(int sym_no, int index,int lev) {
	int ruleNumber = ParsingTable[sym_no][index];
	int i = 0;
	list* tmp;
	list* otmp;
	list* cur;
	list* ocur;
	list* par;
	if (ruleNumber == 99) {
		system("cls");
		printf("������ ���� ���� �߸��� ������ �Է��Ͽ����ϴ�.\n���α׷� ����\n");
		while (head != NULL) {
			tmp = head;
			head = head->next;
			free(tmp);
		}
		while (ohead != NULL) {
			tmp = ohead;
			ohead = ohead->next;
			free(tmp);
		}
		exit(0);
	}
	if (ruleNumber == 7 || ruleNumber == 3) {
		return -1;
	}
	cur = head;
	ocur = ohead;
	par = ohead;
	while (par->next != NULL) {
		par = par->next;
	}
	for (i = Rules[ruleNumber].rightnum - 1; i >= 0; i--) {
		tmp = (list*)malloc(sizeof(list));
		otmp = (list*)malloc(sizeof(list));
		tmp->a_rule = Rules[ruleNumber].righthand[i];
		tmp->next = head->next;
		tmp->lev = lev;
		head->next = tmp;
		otmp->a_rule = Rules[ruleNumber].righthand[Rules[ruleNumber].rightnum - 1 - i];
		otmp->lev = lev;
		otmp->prt = par;
		while (ocur->next != NULL) {
			ocur = ocur->next;
		}
		otmp->next = ocur->next;
		ocur->next = otmp;
	}
return 1;
}

/* outputfile�� ���� ��ū�� �Է¹޾� atok ����ü�� ������ �� �� ������ ���� ��ū�� ���� �������� ���Ǵ�
   ��ū������ ���� �Ѵ�. �� �� Start symbol�� E, input�� ���Ϸκ��� ���� ��ū���� �Ͽ� �Ľ��� �Ѵ�.*/
void Parsing() {
	static int lev = 0;
	tok atok;
	int index;
	list* tmp;
	sym start = { 1,0 };
	char c;
	while (1) {
		/* ���� ���Ŀ� �°� ��ū ������ �Է¹��� */
		fscanf(fp, "%s\t%d\t%s\t%s", atok.num, &atok.uniNum, atok.type, atok.token);
		c = fgetc(fp);
		if (c == '\n') {
			ungetc(c, fp);
		}
		else if (c == EOF) {
			break;
		}
		else {
			fgets(atok.att, 20, fp);
		}
		for (index = 0; index < MaxTerminal; index++) {
			/* ���� ��ū�� ���� �������� ���Ǵ� Terminal ��ū���� Ȯ��*/
			if (atok.type[0] == NameTerminals[index]) {
				break;
			}
			else if (atok.uniNum == 0) {
				index = 7;
				break;
			}
			else if (atok.uniNum == 1) {
				index = 8;
				break;
			}
		}
		/* Linked List�� ���� �Ľ̰����� ���� */
		if (index < MaxTerminal) {
			if (head != NULL) {
				while (head->a_rule.kind == 1) {
					while (head->lev != lev&&head->lev <= lev) {
						lev--;
					}
					lev++;
					InsertList(head->a_rule.sym_no, index, lev);
					tmp = head;
					head = head->next;
					free(tmp);
					if (head == NULL) {
						break;
					}
				}
			}
			if (head != NULL) {
				tmp = head;
				head = head->next;
				free(tmp);
				if (head->next->lev != head->lev) {
					lev--;
				}
			}

		}
		if (feof(fp) != 0) {
			break;
		}
	}
}
void initiatePrint() {
	list* 
}
/* �Ľ�Ʈ�� ��� */
void PrintParsingTree(list* current) {
	InitiatePrint();
	
}

/* ���� ȭ�� ��� */
void PrintGrammer() {
	int x = 40;
	gotoxy(x, 1);
	printf("�־��� ����");
	gotoxy(x, 2);
	printf("(0)  E -> T E��");
	gotoxy(x, 3);
	printf("(1)  E�� -> + T E��");
	gotoxy(x, 4);
	printf("(2)  E�� -> - T E��");
	gotoxy(x, 5);
	printf("(3)  E��->e(Epsilon)");
	gotoxy(x, 6);
	printf("(4)  T->F T��");
	gotoxy(x, 7);
	printf("(5)  T�� -> * F T��");
	gotoxy(x, 8);
	printf("(6)  T�� -> / F T��");
	gotoxy(x, 9);
	printf("(7)  T��->e(Epsilon)");
	gotoxy(x, 10);
	printf("(8)  F -> (E)");
	gotoxy(x, 11);
	printf("(9)  F->id");
	gotoxy(x, 12);
	printf("(10) F->num");
	gotoxy(1, 30);
}

int main() {
	/* NonTerminal symbol�� ���� */
	sym e = { 1,0 };	//E
	sym ep = { 1,1 };	//E'
	sym t = { 1,2 };	//T
	sym tp = { 1,3 };	//T'
	sym f = { 1,4 };	//F
	sym symbolArray[MaxNonTerminal] = { e,ep,t,tp,f };
	char* symbol[MaxNonTerminal] = { "E","E\'","T","T\'","F" };
	char c = '0';
	int i, j;
	list* tmp;
	/* �� NonTerminal Symbol���� First ���ϱ� */
	for (i = 0; i < MaxNonTerminal; i++) {
		First(symbolArray[i]);
	}
	/* �� NonTerminal Symbol���� Follow ���ϱ� */
	for (i = 0; i < MaxNonTerminal; i++) {
		Follow(symbolArray[i]);
	}

	/* First, Follow ��� */
	printf("First Array\n");
	for (i = 0; i < MaxNonTerminal; i++) {
		printf("%s\t", symbol[i]);
		for (j = 0; j < MaxTerminal; j++) {
			printf("%c", FirstArray[i][j]);
		}
		printf("\n");
	}
	printf("\nFollow Array\n");
	for (i = 0; i < MaxNonTerminal; i++) {
		printf("%s\t", symbol[i]);
		for (j = 0; j < MaxTerminal; j++) {
			printf("%c", FollowArray[i][j]);
		}
		printf("\n");
	}

	/* �Ľ����̺� �ʱ�ȭ �� �Ľ����̺� ���� */
	InitiateParsingTable();
	MakeParsingTable();


	/* �Ľ����̺� ��� */
	printf("\nParsingTable('*'ǥ�ô� ����̴� ������ ����)\n\t");
	for (i = 0; i < MaxTerminal; i++) {
		printf("%c ", NameTerminals[i]);
	}
	printf("\n");
	for (i = 0; i < MaxNonTerminal; i++) {
		printf("%s\t", symbol[i]);
		for (j = 0; j < MaxTerminal; j++) {
			if (ParsingTable[i][j] != 99) {
				printf("%d ", ParsingTable[i][j]);
			}
			else {
				printf("* ");
			}
		}
		printf("\n");
	}
	printf("\n");


	

	/* �Ľ�Ʈ�� ��� */
	printf("���� Ű�� �Է��Ͻø� �Ľ�Ʈ���� ����մϴ�.");
	PrintGrammer();
	getchar(c);
	/* �Ľ��� �����ϱ� ���� ���� �ʱ�ȭ �� �Ľ� */
	InitiateParsing();
	Parsing();
	system("cls");
	printf("�Ľ�Ʈ�� ���!\n");
	//PrintParsingTree();
	tmp = ohead;
	while (tmp->next != NULL) {
		printf("\n%d\t", tmp->a_rule.kind);
		printf("%d\t", tmp->a_rule.sym_no);
		printf("%d\n", tmp->lev);
		tmp = tmp->next;
	}
	printf("\n%d\t", tmp->a_rule.kind);
	printf("%d\t", tmp->a_rule.sym_no);
	printf("%d\n", tmp->lev);

	while (head != NULL) {
		tmp = head;
		head = head->next;
		free(tmp);
	}
	gotoxy(1, 2);
	printf("");
	free(ohead);

	return 0;
}
