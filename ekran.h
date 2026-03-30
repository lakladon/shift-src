#ifndef EKRAN_H
#define EKRAN_H

void pishi_na_ekran(const char* s, unsigned char color, int row, int col);
void pishi_bukvu(char c, unsigned char color, int row, int col);
void ochisti_ekranchik(unsigned char color);
void ubi_mig_stroku();
void prokruti_ekran_s(int start_row, unsigned char color);

#endif