#include <assert.h>
#include "ecclib.h"

int main(void)
{
    ecc_t ECC;
    int i, j;
    unsigned int offset=0; int rc; unsigned char byteToRead;
    unsigned short bitToFlip, bitToFlip2;
    unsigned char *base_addr=enable_ecc_memory(&ECC);

    // NEGATIVE testing - flip a SINGLE bit, read to correct
    traceOn();


    // Loop through 13 SBE cases
    for(i=0; i<13; i++)
    {
        printf("test case %d\n", i);
        write_byte(&ECC, base_addr+0, (unsigned char)0xAB);
        bitToFlip=i;
        flip_bit(&ECC, base_addr+0, bitToFlip);
        rc=read_byte(&ECC, base_addr+offset, &byteToRead);
        flip_bit(&ECC, base_addr+0, bitToFlip);
        assert((rc=read_byte(&ECC, base_addr+offset, &byteToRead)) == NO_ERROR);
    }


    // Loop through 78 DBE cases
    for(i=0; i<13; i++)
    {
        write_byte(&ECC, base_addr+0, (unsigned char)0xAB);
        bitToFlip=i;
        flip_bit(&ECC, base_addr+0, bitToFlip);

        for(j=i+1; j<13; j++)
        {
            printf("test case %d, %d\n", i, j);
            bitToFlip2=j;
            flip_bit(&ECC, base_addr+0, bitToFlip2);
            rc=read_byte(&ECC, base_addr+offset, &byteToRead);
            flip_bit(&ECC, base_addr+0, bitToFlip2);
        }
    }


    // There are 8192 total bit patterns for 13 bits, so 8100 cases remain which are 3 or more bit MBEs

 
    //Demonstration of specific test cases

    // TEST CASE 1: bit 3 is d01 @ position 0
    printf("**** TEST CASE 1: bit 3 is d01 @ position 0 ******\n");
    write_byte(&ECC, base_addr+0, (unsigned char)0xAB);
    bitToFlip=3;
    flip_bit(&ECC, base_addr+0, bitToFlip);
    assert((rc=read_byte(&ECC, base_addr+offset, &byteToRead)) == bitToFlip);
    flip_bit(&ECC, base_addr+0, bitToFlip);
    assert((rc=read_byte(&ECC, base_addr+offset, &byteToRead)) == NO_ERROR);
    printf("**** END TEST CASE 1 *****************************\n\n");

    // TEST CASE 2: bit 10 is d05 @ position 10
    printf("**** TEST CASE 2: bit 10 is d05 @ position 10 ****\n");
    write_byte(&ECC, base_addr+0, (unsigned char)0x5A);
    bitToFlip=10;
    flip_bit(&ECC, base_addr+0, bitToFlip);
    assert((rc=read_byte(&ECC, base_addr+offset, &byteToRead)) == bitToFlip);
    flip_bit(&ECC, base_addr+0, bitToFlip);
    assert((rc=read_byte(&ECC, base_addr+offset, &byteToRead)) == NO_ERROR);
    printf("**** END TEST CASE 2 *****************************\n\n");

    // TEST CASE 3: bit 0 is pW @ position 0
    printf("**** TEST CASE 3: bit 0 is pW @ position 0 *******\n");
    write_byte(&ECC, base_addr+0, (unsigned char)0xCC);
    bitToFlip=0;
    flip_bit(&ECC, base_addr+0, bitToFlip);
    assert((rc=read_byte(&ECC, base_addr+offset, &byteToRead)) == PW_ERROR);
    printf("**** END TEST CASE 3 *****************************\n\n");

    // TEST CASE 4: bit 1 is p01 @ position 1
    printf("**** TEST CASE 4: bit 1 is p01 @ position 1 ******\n");
    write_byte(&ECC, base_addr+0, (unsigned char)0xAB);
    bitToFlip=1;
    flip_bit(&ECC, base_addr+0, bitToFlip);
    assert((rc=read_byte(&ECC, base_addr+offset, &byteToRead)) == bitToFlip);
    printf("**** END TEST CASE 4 *****************************\n\n");
    traceOff();


    // POSITIVE testing - do read after write on all locations
    
    // TEST CASE 5: Read after Write all
    printf("**** TEST CASE 5: Read after Write all ***********\n");
    for(offset=0; offset < MEM_SIZE; offset++)
        write_byte(&ECC, base_addr+offset, (unsigned char)offset);

    // read all of the locations back without injecting and error
    for(offset=0; offset < MEM_SIZE; offset++)
        assert((rc=read_byte(&ECC, base_addr+offset, &byteToRead)) == NO_ERROR);
    printf("**** END TEST CASE 5 *****************************\n\n");


    return NO_ERROR;
}
