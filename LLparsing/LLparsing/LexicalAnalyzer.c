#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "lexan.h"
#define URT 55 // ���� �߻��� ����ڵ�

typedef struct tok { //�ϳ��� ��ū�� ������ �ִ� ������
	int kind; //��ū ������ȣ
	char attribute[5]; // ������ ��� ���� �Ǵ� �Ǽ��� ǥ��
					   // �񱳿����� ���� ǥ��
	int dnum; // ID ��ū�� �ɺ� , ������ ��� ���� ����
	double rnum; // �Ǽ��� ��� ���� ����
	char str[30]; // ��ū�� �����ϴ� ��Ʈ�� ���� ����
}tokentype;

typedef struct { char str[30]; int toknum; }key;  //Ű���̺��� �����ϴ� �ϳ��� ����
key key_tbl[53] = { // �̸� ����� Ű ���̺�
					// ����� �̷������ ����(ex. *,-,{,[ ��)�� ��ȣ�� ����
	{ "ID",0 },{ "NUM",1 },{ "ROP",2 },{ "+",3 },{ "-",4 },{ "*",5 },{ "/",6 },{ "%",7 },{ "=",8 },{ "->",9 },{ "!",10 },{ ".",11 },{ ",",12 },{ "&",13 },
	{ "++",14 },{ "--",15 },{ "(",16 },{ ")",17 },{ "{",18 },{ "}",19 },{ "[",20 },{ "]",21 },{ ",",22 },{ ":",23 },{ ";",24 },{ "\"",25 },{ "'",26 },{ "#",27 },
	{ "if",28 },{ "else",29 },{ "while",30 },{ "do",31 },{ "for",32 },{ "include",33 },{ "define",34 },{ "typedef",35 },{ "struct",36 },{ "int",37 },
	{ "char",38 },{ "float",39 },{ "double",40 },{ "void",41 },{ "return",42 },{ "EOF",43 },{ ">",44 },{ ">=",45 },{ "<",46 },{ "<=",47 },{ "<>",48 },{ "==",49 },{ "=!",50 },
	{ "&&",51 },{ "||",52 }
};

typedef struct { char symname[30]; int attribute; }sym_ent; // �ɺ����̺��� �� ��Ʈ���� ����
sym_ent symtbl[600];
int symt_idx = 0; // �ɺ����̺��� ������ ���� ��Ʈ�� ��ȣ


char myfgetc(FILE*); //���Ϸκ��� �� �ܾ ��������
tokentype lexan(FILE*, FILE*); // ������ ��ū�� �м��Ͽ� ��ȯ�ϴ� �Լ�
FILE *fp, *ofp; // ���� ������� ���� ������ ����
int line_cnt = 1; // ��� ���Ͽ� Line ���ڸ� ���
void print_token(tokentype, FILE*);
int match_tok(char c); // +,-�� ���� ��ȣ�� �޾��� �� key_tbl Ȯ�� �Լ�
int match_tok_str(tokentype token); // key_tbl�� ����� ���ڿ��� �̷���� key�� �ִ��� Ȯ���ϴ� �Լ�


void LexicalAnalyzer() {
	tokentype onetok; //�ϳ��� ��ū�� ����
	char source_file[50] = "sourcefile.txt"; // �Է� ���� ���ϸ�
	char out_file[50] = "outputfile.txt"; // ������� �� ���� ���ϸ�
	int state = 0;

	fp = fopen(source_file, "r");
	if (!fp) { printf("File open error of file=%s", source_file); return 0; }; //�����޽��� ���
	ofp = fopen(out_file, "w");
	if (!ofp) { printf("File open error of file=%s", out_file); return 0; }; //�����޽��� ���
	fprintf(ofp, "[��ȣ]\t������ȣ����\t��ū\tA1\tA2\n");
	while (!state) {
		onetok = lexan(fp, ofp);
		if (onetok.kind == 43) { // EOF�� �� ����
			state = 1;
		}
		else if (onetok.kind == URT) { // ERROR �߻� �� ����
			state = 1;
		}
	}
	fclose(fp);
	fclose(ofp);

	return 0;
}

char myfgetc(FILE* fp) {
	char c;
	c = fgetc(fp);
	return c;
}

tokentype lexan(FILE* fp, FILE* ofp) {
	char buf[30] = { 0 }; int bf = 0; // ���ڿ� ���ڸ� �о��� �� ������ ���� ����
	char c; // ���Ͽ��� ������ �� ���� Chracater�� ����
	int re = -1; // match_tok�Լ��� ������� ����
	int re_str = -1; //match_tok_str�Լ��� ������� ����
	int state = 0;
	int upper_n = 1; // ���ڸ� �޾��� ��� ���� �κ�
	double fraction = 0; // ���ڸ� �޾��� ��� �Ҽ� �κ�
	int FCNT = 0; // �Ҽ����ڸ� ���� ��� �ڸ��� ����� ���� ����
	char tmp[2]; //�ּ� ó���� ���� file���� �ѱ��� �ҷ��� �� ������ ����
	int flag = 0; //�ּ� ó���� �� //������ /*������ ���� �����÷���
	tokentype token;

	while (1) {

		switch (state) {
		case 0: //�ʱ����
			c = myfgetc(fp);
			if (c == ' ' || c == '\n' || c == '\t');
			else if (c == '/') { //�ּ� ó���� ���� ����
				tmp[0]=fgetc(fp);
				if (tmp[0] == '/' || tmp[0] == '*') {
					state = 6;
				}
				else { // �ּ��� �ƴ� ��� �ϳ��� ��ȣ�� �޾Ƶ��� 
					ungetc(tmp[0], fp);
					state = 5;
				}
			}
			else if (isdigit(c)) { upper_n = c - '0'; state = 2; } //���� ���ڰ� �����϶�
			else if (isalpha(c)) { state = 1; } //���� ���ڰ� �����϶�
			else if (!feof(fp)) { //���� ���ڰ� ���ڵ� ���ڵ� �ƴ� ��ȣ�� ��츦 ���
				re = match_tok(c);
				if (re != -1) { //���� ��ȣ�� key���̺� ���� ���
					state = 5;
				}
			}
			else if (feof(fp)) { //������ ���� ���� �� EOF ��ȣ �ο� �� ��ȯ
				token.kind = 43;
				return token;
			}
			else {
				printf("Something wrong!!");
				token.kind = URT; // ���� �߻��� 50���� �ĺ�
				return token;
			}
			break;


		case 1: // ���ʷ� ���ڸ� �޾��� ��
			buf[bf] = c;  // ���� ���ڸ� �ϴ� ���ۿ� ���� �� ���� ���ڸ� �޾Ƶ帲
			c = myfgetc(fp);
			if (isdigit(c) || isalpha(c) || c == '_') { //���� ���� ���� ���ڳ� ������ ��� ��� ���ڸ� �޾Ƶ���
				bf++;
			}
			else {
				state = 3; // ���� ���ڰ� ���ڳ� ���ڰ� �ƴ� �ٸ� ���϶� ��ū�ϼ��� ���� ���·� �̵�
			}
			break;


		case 2: // ���ʷ� ���ڸ� �޾��� ���
			c = myfgetc(fp);
			if (isdigit(c)) {
				upper_n = upper_n * 10 + c - '0';
			}
			else if (c == '.') { //�Ҽ��� ���
				state = 4;
			}
			else { // ���ڰ� ������ ��� 
				ungetc(c, fp);
				token.kind = 1;
				token.dnum = upper_n;
				strcpy(token.attribute, "in"); // �������� ��Ÿ��
				print_token(token, ofp);
				return token;
			}
			break;
		case 3: // ���ڷ� ���� ��ū�� �ϼ�
				// ��, �� ��� ROP�� NUM�� ���Ե��� �ʴ´�
			ungetc(c, fp); //���������� �� ĭ �ڷ� ������
			strcpy(token.str, buf);
			re_str = match_tok_str(token); //Key���̺��� �´� ���ڰ� �ִ��� Ȯ��
			if (strcmp(token.str, "NUM") == 0) { //������ ���� ��ū�� NUM�̶�� ��ū�� ��� ID token���� ����
				token.kind = 0;
				token.dnum = symt_idx; //�ɺ����̺��� ��Ʈ����ȣ�� ����
				strcpy(symtbl[symt_idx].symname, token.str);
				symt_idx++;
				print_token(token, ofp);
			}
			else if (re_str != -1) { //ID��ū�� ��ȣ�� ������ ����(��, '_'�� ���� ����)�� �̷��� ��ū���� Ȯ�� �� ��� 
				token.kind = re_str; //�Լ����� ��ȯ�� ���� �� ��ū�� ������ȣ
				print_token(token, ofp);
			}
			else {				//ID��ū�� ��� symbol���̺� ���� �� ����Ʈ 
				token.kind = 0;
				token.dnum = symt_idx; //�ɺ����̺��� ��Ʈ����ȣ�� ����
				strcpy(symtbl[symt_idx].symname, token.str);
				symt_idx++;
				print_token(token, ofp);
			}
			return token;


		case 4: //�Ҽ��� �޾��� ���
			c = myfgetc(fp);
			FCNT++;
			if (isdigit(c)) {
				fraction = fraction + (c - '0') / pow(10, FCNT);
			}
			else { // �Ҽ� ���
				ungetc(c, fp);
				token.kind = 1;
				token.rnum = (double)upper_n + fraction;
				strcpy(token.attribute, "do"); // �Ǽ����� ��Ÿ��
				print_token(token, ofp);
				return token;
			}
			break;


		case 5: // ��ȣ�� �޾��� ���
			if (re >= 44 && re <= 53) { // match_tok�Լ��� ������� -1�� �ƴ� ��� key���̺� �ִ� ��ȣ. �� �� ROP��ȣ ����
				token.kind = 2;
				strcpy(token.str, key_tbl[re].str);
				print_token(token, ofp);
				return token;
			}
			else { // ROP��ū ���� ��ȣ�� ���
				re = match_tok(c);
				token.kind = key_tbl[re].toknum;
				strcpy(token.str, key_tbl[re].str);
				print_token(token, ofp);
				return token;
			}
			break;

		case 6: //�ּ� ó����
			if (tmp[0] == '/' &&tmp[1] == '\n') { // �̹� ���� ���ڰ� /�� \n�� ��� �ּ��� // �� ���̹Ƿ� �ʱ���·� ������
				ungetc(c, fp);
				state = 0;
				break;
			}
			while (1) {
				if (tmp[0] == '/' || flag == 1) {// //�ּ����� ���
					flag = 1; // //�ּ����� ��� ���� �÷��� 1
					fgets(tmp, 2, fp);
					if (tmp[0] == '\n') { // //�ּ����� ���� �������� ��� 
						state = 0;
						break;
					}
					else if (tmp[1] == '\n') {  // //�ּ����� ���� �������� ��� 
						state = 0;
						break;
					}
				}

				else {
					fgets(tmp, 2, fp); // /* */ �ּ����� ���
					if (tmp[0] == '*'&&tmp[1] == '/') { // /* */ �ּ����� ���� ���� ���
						state = 0;
						break;
					}
					else if (tmp[1] == '*') { // /* */ �ּ����� ���� ���� ���
						c = myfgetc(fp);
						if (c == '/') {
							state = 0;
							break;
						}
						else {
							ungetc(c, fp);
						}
					}
				}
				if (feof(fp)) { //���� �� �˻�
					token.kind = 43;
					state = 0;
					break;
				}
			}
			break;
		default:
			printf("Something wrong~!!");
			token.kind = URT;
			return token;
		}
		if (feof(fp)) { // ���� ���� ��� token.kind�� ���� 43�� �ְ� ���� EOF�� �Ǻ��ϴ� ��ҿ��� ���� ��Ű���� ��
			token.kind = 43;
		}

	}

}
int match_tok(char c) { // ���� �ϳ��� �޾� key���̺��� �� ���ڰ� �ִ� �� Ȯ�� �� ������ key���̺��� �� ��° �ִ��� ��ȯ�ϴ� �Լ�
						// +,- ��� ���� ��ȣ�� �����ϴµ� ���
	int i = 0;;
	while (i<53) {
		if (c == key_tbl[i].str[0]) {
			return i;
		}
		else {
			i++;
		}
	}
	return -1;
}
int match_tok_str(tokentype token) { //�� ���� ���ڸ� �޾� key ���̺��� �� ���ڰ� �ִ��� Ȯ�� �� ������ �� ��° �ִ����� ��ȯ�ϴ� �Լ�
	int i = 0;
	int re;
	while (i<53) {
		re = strcmp(token.str, key_tbl[i].str);
		if (re == 0) {
			return i;
		}
		else {
			i++;
		}
	}
	return -1;
}
void print_token(tokentype token, FILE* ofp) { //output source file�� �� �� �� ��ū ���� ���

	fprintf(ofp, "[%d]\t", line_cnt); // �� ��ȣ ��� 
	switch (token.kind) { // ��ū������ȣ�� ���� ��¹�� ����
	case 0: // ID��ū�� ���
		fprintf(ofp, "%d\t", token.kind);
		fprintf(ofp, "ID\t%s\t", token.str);
		fprintf(ofp, "%d\n", token.dnum);
		break;
	case 1: // NUM��ū�� ���
		fprintf(ofp, "%d\t", token.kind);
		if (strcmp(token.attribute, "in") == 0) { // �������� ���
			fprintf(ofp, "NUM\t%d\tin\n", token.dnum);
		}
		else {
			fprintf(ofp, "NUM\t%.2f\tdo\n", token.rnum); // �Ǽ��� ���
		}
		break;
	case 2:
		switch (match_tok(token.str[0])) { //ROP ��ū�� ���
		case 44:
			fprintf(ofp, "%d\t", token.kind);
			fprintf(ofp, "ROP\t%s\tGP\n", token.str);
			break;
		case 45:
			fprintf(ofp, "%d\t", token.kind);
			fprintf(ofp, "ROP\t%s\tGE\n", token.str);
			break;
		case 46:
			fprintf(ofp, "%d\t", token.kind);
			fprintf(ofp, "ROP\t%s\tLT\n", token.str);
			break;
		case 47:
			fprintf(ofp, "%d\t", token.kind);
			fprintf(ofp, "ROP\t%s\tLE\n", token.str);
			break;
		case 49:
			fprintf(ofp, "%d\t", token.kind);
			fprintf(ofp, "ROP\t%s\tEQ\n", token.str);
			break;
		case 50:
			fprintf(ofp, "%d\t", token.kind);
			fprintf(ofp, "ROP\t%s\tNE\n", token.str);
			break;
		default:
			fprintf(ofp, "%d\t", token.kind);
			fprintf(ofp, "ROP\t%s\n", token.str);
			break;
		}
		break;
	default: // ID, ROP�� NUM�� ������ ������ ��ū���� ���
		fprintf(ofp, "%d\t", token.kind);
		fprintf(ofp, "%s\t", token.str);
		fprintf(ofp, "%s\n", token.str);
		break;
	}
	line_cnt++;
	return;
}