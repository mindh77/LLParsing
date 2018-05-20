#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "lexan.h"
#define URT 55 // 에러 발생시 사용코드

typedef struct tok { //하나의 토큰이 가지고 있는 정보들
	int kind; //토큰 고유번호
	char attribute[5]; // 숫자일 경우 정수 또는 실수를 표현
					   // 비교연산자 종류 표현
	int dnum; // ID 토큰의 심볼 , 정수일 경우 값을 저장
	double rnum; // 실수일 경우 값을 저장
	char str[30]; // 토큰을 구성하는 스트링 값을 저장
}tokentype;

typedef struct { char str[30]; int toknum; }key;  //키테이블을 구성하는 하나의 원소
key key_tbl[53] = { // 미리 선언된 키 테이블
					// 영어로 이루어지지 않은(ex. *,-,{,[ 등)은 기호로 구분
	{ "ID",0 },{ "NUM",1 },{ "ROP",2 },{ "+",3 },{ "-",4 },{ "*",5 },{ "/",6 },{ "%",7 },{ "=",8 },{ "->",9 },{ "!",10 },{ ".",11 },{ ",",12 },{ "&",13 },
	{ "++",14 },{ "--",15 },{ "(",16 },{ ")",17 },{ "{",18 },{ "}",19 },{ "[",20 },{ "]",21 },{ ",",22 },{ ":",23 },{ ";",24 },{ "\"",25 },{ "'",26 },{ "#",27 },
	{ "if",28 },{ "else",29 },{ "while",30 },{ "do",31 },{ "for",32 },{ "include",33 },{ "define",34 },{ "typedef",35 },{ "struct",36 },{ "int",37 },
	{ "char",38 },{ "float",39 },{ "double",40 },{ "void",41 },{ "return",42 },{ "EOF",43 },{ ">",44 },{ ">=",45 },{ "<",46 },{ "<=",47 },{ "<>",48 },{ "==",49 },{ "=!",50 },
	{ "&&",51 },{ "||",52 }
};

typedef struct { char symname[30]; int attribute; }sym_ent; // 심볼테이블의 한 엔트리의 구조
sym_ent symtbl[600];
int symt_idx = 0; // 심볼테이블의 다음에 넣을 엔트리 번호


char myfgetc(FILE*); //파일로부터 한 단어씩 가져오기
tokentype lexan(FILE*, FILE*); // 실제로 토큰을 분석하여 반환하는 함수
FILE *fp, *ofp; // 파일 입출력을 위한 포인터 선언
int line_cnt = 1; // 출력 파일에 Line 숫자를 출력
void print_token(tokentype, FILE*);
int match_tok(char c); // +,-과 같은 기호를 받았을 시 key_tbl 확인 함수
int match_tok_str(tokentype token); // key_tbl에 저장된 문자열로 이루어진 key가 있는지 확인하는 함수


void LexicalAnalyzer() {
	tokentype onetok; //하나의 토큰을 만듬
	char source_file[50] = "sourcefile.txt"; // 입력 받을 파일명
	char out_file[50] = "outputfile.txt"; // 결과물로 내 놓을 파일명
	int state = 0;

	fp = fopen(source_file, "r");
	if (!fp) { printf("File open error of file=%s", source_file); return 0; }; //오류메시지 출력
	ofp = fopen(out_file, "w");
	if (!ofp) { printf("File open error of file=%s", out_file); return 0; }; //오류메시지 출력
	fprintf(ofp, "[번호]\t고유번호종류\t토큰\tA1\tA2\n");
	while (!state) {
		onetok = lexan(fp, ofp);
		if (onetok.kind == 43) { // EOF일 때 종료
			state = 1;
		}
		else if (onetok.kind == URT) { // ERROR 발생 시 종료
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
	char buf[30] = { 0 }; int bf = 0; // 문자열 글자를 읽었을 때 저장해 놓을 공간
	char c; // 파일에서 가져온 한 개의 Chracater를 저장
	int re = -1; // match_tok함수의 결과값을 저장
	int re_str = -1; //match_tok_str함수의 결과값을 저장
	int state = 0;
	int upper_n = 1; // 숫자를 받았을 경우 정수 부분
	double fraction = 0; // 숫자를 받았을 경우 소수 부분
	int FCNT = 0; // 소수숫자를 받을 경우 자릿수 계산을 위한 변수
	char tmp[2]; //주석 처리를 위해 file에서 한글을 불러올 때 저장할 공간
	int flag = 0; //주석 처리할 때 //문인지 /*인지에 따른 상태플래그
	tokentype token;

	while (1) {

		switch (state) {
		case 0: //초기상태
			c = myfgetc(fp);
			if (c == ' ' || c == '\n' || c == '\t');
			else if (c == '/') { //주석 처리를 위한 구문
				tmp[0]=fgetc(fp);
				if (tmp[0] == '/' || tmp[0] == '*') {
					state = 6;
				}
				else { // 주석이 아닐 경우 하나의 기호로 받아들임 
					ungetc(tmp[0], fp);
					state = 5;
				}
			}
			else if (isdigit(c)) { upper_n = c - '0'; state = 2; } //받은 글자가 숫자일때
			else if (isalpha(c)) { state = 1; } //받은 글자가 문자일때
			else if (!feof(fp)) { //받은 글자가 숫자도 문자도 아닌 기호일 경우를 고려
				re = match_tok(c);
				if (re != -1) { //받은 기호가 key테이블에 있을 경우
					state = 5;
				}
			}
			else if (feof(fp)) { //파일의 끝에 도달 시 EOF 번호 부여 후 반환
				token.kind = 43;
				return token;
			}
			else {
				printf("Something wrong!!");
				token.kind = URT; // 에러 발생시 50으로 식별
				return token;
			}
			break;


		case 1: // 최초로 문자를 받았을 때
			buf[bf] = c;  // 받은 문자를 일단 버퍼에 저장 후 다음 글자를 받아드림
			c = myfgetc(fp);
			if (isdigit(c) || isalpha(c) || c == '_') { //다음 글자 또한 문자나 숫자일 경우 계속 글자를 받아들임
				bf++;
			}
			else {
				state = 3; // 받은 글자가 문자나 숫자가 아닌 다른 것일때 토큰완성을 위한 상태로 이동
			}
			break;


		case 2: // 최초로 숫자를 받았을 경우
			c = myfgetc(fp);
			if (isdigit(c)) {
				upper_n = upper_n * 10 + c - '0';
			}
			else if (c == '.') { //소수일 경우
				state = 4;
			}
			else { // 숫자가 정수일 경우 
				ungetc(c, fp);
				token.kind = 1;
				token.dnum = upper_n;
				strcpy(token.attribute, "in"); // 정수임을 나타냄
				print_token(token, ofp);
				return token;
			}
			break;
		case 3: // 문자로 받은 토큰을 완성
				// 단, 이 경우 ROP와 NUM은 포함되지 않는다
			ungetc(c, fp); //파일포인터 한 칸 뒤로 돌려줌
			strcpy(token.str, buf);
			re_str = match_tok_str(token); //Key테이블에서 맞는 문자가 있는지 확인
			if (strcmp(token.str, "NUM") == 0) { //실제로 받은 토큰이 NUM이라는 토큰일 경우 ID token으로 생각
				token.kind = 0;
				token.dnum = symt_idx; //심볼테이블의 엔트리번호를 저장
				strcpy(symtbl[symt_idx].symname, token.str);
				symt_idx++;
				print_token(token, ofp);
			}
			else if (re_str != -1) { //ID토큰과 기호를 제외한 문자(단, '_'는 포함 가능)로 이뤄진 토큰으로 확인 된 경우 
				token.kind = re_str; //함수에서 반환된 값은 곧 토큰의 고유번호
				print_token(token, ofp);
			}
			else {				//ID토큰일 경우 symbol테이블에 저장 후 프린트 
				token.kind = 0;
				token.dnum = symt_idx; //심볼테이블의 엔트리번호를 저장
				strcpy(symtbl[symt_idx].symname, token.str);
				symt_idx++;
				print_token(token, ofp);
			}
			return token;


		case 4: //소수를 받았을 경우
			c = myfgetc(fp);
			FCNT++;
			if (isdigit(c)) {
				fraction = fraction + (c - '0') / pow(10, FCNT);
			}
			else { // 소수 출력
				ungetc(c, fp);
				token.kind = 1;
				token.rnum = (double)upper_n + fraction;
				strcpy(token.attribute, "do"); // 실수임을 나타냄
				print_token(token, ofp);
				return token;
			}
			break;


		case 5: // 기호를 받았을 경우
			if (re >= 44 && re <= 53) { // match_tok함수의 결과값이 -1이 아닐 경우 key테이블에 있는 기호. 그 중 ROP기호 필터
				token.kind = 2;
				strcpy(token.str, key_tbl[re].str);
				print_token(token, ofp);
				return token;
			}
			else { // ROP토큰 외의 기호들 출력
				re = match_tok(c);
				token.kind = key_tbl[re].toknum;
				strcpy(token.str, key_tbl[re].str);
				print_token(token, ofp);
				return token;
			}
			break;

		case 6: //주석 처리문
			if (tmp[0] == '/' &&tmp[1] == '\n') { // 이미 받은 문자가 /와 \n일 경우 주석문 // 의 끝이므로 초기상태로 돌려줌
				ungetc(c, fp);
				state = 0;
				break;
			}
			while (1) {
				if (tmp[0] == '/' || flag == 1) {// //주석문일 경우
					flag = 1; // //주석문일 경우 상태 플래그 1
					fgets(tmp, 2, fp);
					if (tmp[0] == '\n') { // //주석문의 끝에 도달했을 경우 
						state = 0;
						break;
					}
					else if (tmp[1] == '\n') {  // //주석문의 끝에 도달했을 경우 
						state = 0;
						break;
					}
				}

				else {
					fgets(tmp, 2, fp); // /* */ 주석문일 경우
					if (tmp[0] == '*'&&tmp[1] == '/') { // /* */ 주석문이 종료 됬을 경우
						state = 0;
						break;
					}
					else if (tmp[1] == '*') { // /* */ 주석문이 종료 됬을 경우
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
				if (feof(fp)) { //파일 끝 검사
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
		if (feof(fp)) { // 파일 끝일 경우 token.kind의 값에 43을 넣고 위의 EOF를 판별하는 장소에서 종료 시키도록 함
			token.kind = 43;
		}

	}

}
int match_tok(char c) { // 글자 하나를 받아 key테이블에서 그 글자가 있는 지 확인 후 있으면 key테이블의 몇 번째 있는지 반환하는 함수
						// +,- 등과 같은 기호를 구분하는데 사용
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
int match_tok_str(tokentype token) { //한 개의 문자를 받아 key 테이블에서 그 문자가 있는지 확인 후 있으면 몇 번째 있는지를 반환하는 함수
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
void print_token(tokentype token, FILE* ofp) { //output source file에 한 줄 씩 토큰 내용 출력

	fprintf(ofp, "[%d]\t", line_cnt); // 줄 번호 출력 
	switch (token.kind) { // 토큰고유번호에 따라 출력방법 상이
	case 0: // ID토큰일 경우
		fprintf(ofp, "%d\t", token.kind);
		fprintf(ofp, "ID\t%s\t", token.str);
		fprintf(ofp, "%d\n", token.dnum);
		break;
	case 1: // NUM토큰일 경우
		fprintf(ofp, "%d\t", token.kind);
		if (strcmp(token.attribute, "in") == 0) { // 정수형일 경우
			fprintf(ofp, "NUM\t%d\tin\n", token.dnum);
		}
		else {
			fprintf(ofp, "NUM\t%.2f\tdo\n", token.rnum); // 실수일 경우
		}
		break;
	case 2:
		switch (match_tok(token.str[0])) { //ROP 토큰일 경우
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
	default: // ID, ROP와 NUM을 제외한 나머지 토큰들일 경우
		fprintf(ofp, "%d\t", token.kind);
		fprintf(ofp, "%s\t", token.str);
		fprintf(ofp, "%s\n", token.str);
		break;
	}
	line_cnt++;
	return;
}