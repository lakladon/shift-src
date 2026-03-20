#ifndef IOZH_H
#define IOZH_H

void zhmyak_out(unsigned short port, unsigned char value);
unsigned char zhmyak_in(unsigned short port);
void zhmyak_out16(unsigned short port, unsigned short value);
unsigned short zhmyak_in16(unsigned short port);

#endif