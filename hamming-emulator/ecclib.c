#include "ecclib.h"

static int printTrace=1;

void traceOn(void)
{
    printTrace=1;
}

void traceOff(void)
{
    printTrace=0;
}

int write_byte(ecc_t *ecc, unsigned char *address, unsigned char byteToWrite) {
    unsigned int offset = address - ecc->data_memory;
    unsigned char codeword=0;

    ecc->data_memory[offset]= byteToWrite;
    codeword = get_codeword(ecc, offset);
    ecc->code_memory[offset] = codeword;

    if(printTrace) { printf("WRITE : COMPUTED PARITY = ");print_code(codeword); }
    if(printTrace) { printf("WRITE : PARITY          = ");print_code_word(ecc, address); }
    if(printTrace) { printf("WRITE : DATA            = ");print_data_word(ecc, address); }
    if(printTrace) { printf("WRITE : ECODED          = ");print_encoded(ecc, address); }
    if(printTrace) { printf("\n"); }
    

    return NO_ERROR;
}


int read_byte(ecc_t *ecc, unsigned char *address, unsigned char *byteRead) {
    unsigned int offset = address - ecc->data_memory;
    unsigned char SYNDROME=0, pW2=0, pW=0, codeword=0;

    codeword = get_codeword(ecc, offset);

    SYNDROME = (codeword & SYNBITS) ^ (ecc->code_memory[offset] & SYNBITS);

    pW = (ecc->code_memory[offset]) & PW_BIT;
    // BUG -- pW2 = (codeword) & PW_BIT; 
    pW2 |= (PW_BIT & (
                      (((
                          (ecc->data_memory[offset] & DATA_BIT_1) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_2)>>1) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_3)>>2) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_4)>>3) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_5)>>4) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_6)>>5) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_7)>>6) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_8)>>7)) ^ 
                         ((ecc->code_memory[offset] & P01_BIT) ^ 
                         ((ecc->code_memory[offset] & P02_BIT)>>1) ^ 
                         ((ecc->code_memory[offset] & P03_BIT)>>2) ^ 
                         ((ecc->code_memory[offset] & P04_BIT)>>3)))<<4) 
                        ) );


    if(printTrace) { printf("READ  : COMPUTED PARITY = ");print_code(codeword); }
    if(printTrace) { printf("READ  : PARITY          = ");print_code_word(ecc, address); }
    if(printTrace) { printf("READ  : DATA            = ");print_data_word(ecc, address); }
    if(printTrace) { printf("READ  : ECODED          = ");print_encoded(ecc, address); }
    if(printTrace) { printf("READ  : SYNDROME        = ");print_code(SYNDROME); }
    if(printTrace) { printf("READ  : PW              = ");print_code(pW); }
    if(printTrace) { printf("READ  : PW2             = ");print_code(pW2); }
    if(printTrace) { printf("\n"); }
    
    // 1) if SYNDROME ==0 and pW == pW2, return NO_ERROR
    if((SYNDROME == 0) && (pW == pW2))
    {
        return NO_ERROR;
    }

    // 2) if SYNDROME ==0 and pW != pW2, return PW_ERROR
        if((SYNDROME == 0) && (pW != pW2))
    {
        // restore pW to PW2
        printf("PW ERROR\n\n");
        ecc->code_memory[offset] |= pW2 & PW_BIT;
        return PW_ERROR;
    }

    // 3) if SYNDROME !=0 and pW == pW2, return DOUBLE_BIT_ERROR
    if((SYNDROME != 0) && (pW == pW2))
    {
        printf("DOUBLE BIT ERROR\n\n");
        return DOUBLE_BIT_ERROR;
    }

    // 4) if SYNDROME !=0 and pW != pW2, SBE, return SYNDROME
    if((SYNDROME != 0) && (pW != pW2))
    {
        printf("SBE @ %d\n\n", SYNDROME);
        return SYNDROME;
    }

    // if we get here, something is seriously wrong like triple bit
    // or worse error, so return UNKNOWN_ERROR    
    return UNKNOWN_ERROR;
}


unsigned char *enable_ecc_memory(ecc_t *ecc){
    int idx;
    for(idx=0; idx < MEM_SIZE; idx++) ecc->code_memory[idx]=0;
    return ecc->data_memory;
}

unsigned char get_codeword(ecc_t *ecc, unsigned int offset)
{

    unsigned char codeword=0;

    //printf("CODEWORD offset=%d\n", offset);

    // p01 - per spreadsheet model, compute even parity=0 over 7,5,4,3,2,1 bits
    codeword |= (P01_BIT & (
                        ((ecc->data_memory[offset] & DATA_BIT_1) ^ 
                        ((ecc->data_memory[offset] & DATA_BIT_2)>>1) ^ 
                        ((ecc->data_memory[offset] & DATA_BIT_4)>>3) ^ 
                        ((ecc->data_memory[offset] & DATA_BIT_5)>>4) ^ 
                        ((ecc->data_memory[offset] & DATA_BIT_7)>>6)) 
                       ) );

    // p02 - per spreadsheet modell, compute even parity=0 over 7,6,4,3,1 bits
    codeword |= (P02_BIT & (
                        (
                         ((ecc->data_memory[offset] & DATA_BIT_1) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_3)>>2) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_4)>>3) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_6)>>5) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_7)>>6))<<1) 
                        ) );

    // p03 - per spreadsheet model, compute even parity=0 over 8,4,3,2 bits
    codeword |= (P03_BIT & (
                        ((
                          ((ecc->data_memory[offset] & DATA_BIT_2)>>1) ^ 
                          ((ecc->data_memory[offset] & DATA_BIT_3)>>2) ^ 
                          ((ecc->data_memory[offset] & DATA_BIT_4)>>3) ^ 
                          ((ecc->data_memory[offset] & DATA_BIT_8)>>7))<<2) 
                         ) );

    // p04 - per spreadsheet model, compute even parity=0 over 8,7,6,5 bits
    codeword |= (P04_BIT & (
                        ((
                          ((ecc->data_memory[offset] & DATA_BIT_5)>>4) ^ 
                          ((ecc->data_memory[offset] & DATA_BIT_6)>>5) ^ 
                          ((ecc->data_memory[offset] & DATA_BIT_7)>>6) ^ 
                          ((ecc->data_memory[offset] & DATA_BIT_8)>>7))<<3) 
                         ) );

    // pW - per spreadsheet model compute even parity=0 over all bits
    codeword |= (PW_BIT & (
                      (((
                          (ecc->data_memory[offset] & DATA_BIT_1) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_2)>>1) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_3)>>2) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_4)>>3) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_5)>>4) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_6)>>5) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_7)>>6) ^ 
                         ((ecc->data_memory[offset] & DATA_BIT_8)>>7)) ^ 
                         ((codeword & P01_BIT) ^ 
                         ((codeword & P02_BIT)>>1) ^ 
                         ((codeword & P03_BIT)>>2) ^ 
                         ((codeword & P04_BIT)>>3)))<<4) 
                        ) );

    // set the encoded bit
    codeword |= ENCODED_BIT;

    return codeword;

}


void print_code_word(ecc_t *ecc, unsigned char *address) {
    unsigned int offset = address - ecc->data_memory;
    unsigned char codeword = ecc->code_memory[offset];

    printf("addr=%p (offset=%d) ", address, offset);
    if(codeword & PW_BIT) printf("1"); else printf("0");
    if(codeword & P01_BIT) printf("1"); else printf("0");
    if(codeword & P02_BIT) printf("1"); else printf("0");
    printf("_");
    if(codeword & P03_BIT) printf("1"); else printf("0");
    printf("___");
    if(codeword & P04_BIT) printf("1"); else printf("0");
    printf("____");
    printf("\n");
}


void print_data_word(ecc_t *ecc, unsigned char *address)
{
    unsigned int offset = address - ecc->data_memory;
    unsigned char dataword = ecc->data_memory[offset];

    printf("addr=%p (offset=%d) ", address, offset);

    printf("___");
    if(dataword & DATA_BIT_1) printf("1"); else printf("0");
    printf("_");
    if(dataword & DATA_BIT_2) printf("1"); else printf("0");
    if(dataword & DATA_BIT_3) printf("1"); else printf("0");
    if(dataword & DATA_BIT_4) printf("1"); else printf("0");
    printf("_");
    if(dataword & DATA_BIT_5) printf("1"); else printf("0");
    if(dataword & DATA_BIT_6) printf("1"); else printf("0");
    if(dataword & DATA_BIT_7) printf("1"); else printf("0");
    if(dataword & DATA_BIT_8) printf("1"); else printf("0");

    printf("    => ");
    if(dataword & DATA_BIT_8) printf("1"); else printf("0");
    if(dataword & DATA_BIT_7) printf("1"); else printf("0");
    if(dataword & DATA_BIT_6) printf("1"); else printf("0");
    if(dataword & DATA_BIT_5) printf("1"); else printf("0");
    printf(" ");
    if(dataword & DATA_BIT_4) printf("1"); else printf("0");
    if(dataword & DATA_BIT_3) printf("1"); else printf("0");
    if(dataword & DATA_BIT_2) printf("1"); else printf("0");
    if(dataword & DATA_BIT_1) printf("1"); else printf("0");
    printf(" [0x%02X]", dataword);

    printf("\n");
}


void print_encoded(ecc_t *ecc, unsigned char *address)
{
    unsigned int offset = address - ecc->data_memory;
    unsigned char codeword = ecc->code_memory[offset];
    unsigned char dataword = ecc->data_memory[offset];

    printf("addr=%p (offset=%d) ", address, offset);

    if(codeword & PW_BIT) printf("1"); else printf("0");
    if(codeword & P01_BIT) printf("1"); else printf("0");
    if(codeword & P02_BIT) printf("1"); else printf("0");
    if(dataword & DATA_BIT_1) printf("1"); else printf("0");
    if(codeword & P03_BIT) printf("1"); else printf("0");
    if(dataword & DATA_BIT_2) printf("1"); else printf("0");
    if(dataword & DATA_BIT_3) printf("1"); else printf("0");
    if(dataword & DATA_BIT_4) printf("1"); else printf("0");
    if(codeword & P04_BIT) printf("1"); else printf("0");
    if(dataword & DATA_BIT_5) printf("1"); else printf("0");
    if(dataword & DATA_BIT_6) printf("1"); else printf("0");
    if(dataword & DATA_BIT_7) printf("1"); else printf("0");
    if(dataword & DATA_BIT_8) printf("1"); else printf("0");

    printf("\n");
}

void print_code(unsigned char codeword)
{
    printf("codeword=");
    if(codeword & PW_BIT) printf("1"); else printf("0");
    if(codeword & P01_BIT) printf("1"); else printf("0");
    if(codeword & P02_BIT) printf("1"); else printf("0");
    if(codeword & P03_BIT) printf("1"); else printf("0");
    if(codeword & P04_BIT) printf("1"); else printf("0");

    printf("\n");
}

// flip bit in encoded word: pW p1 p2 d1 p3 d2 d3 d4 p4 d5 d6 d7 d8
// bit position:             00 01 02 03 04 05 06 07 08 09 10 11 12
void flip_bit(ecc_t *ecc, unsigned char *address, unsigned short bit_to_flip) {
    unsigned int offset = address - ecc->data_memory;
    unsigned char byte=0;
    unsigned short data_bit_to_flip=0, parity_bit_to_flip=0;
    int data_flip=1;

    switch(bit_to_flip)
    {
        // parity bit pW, p01 ... p04
        case 0: 
            parity_bit_to_flip = 4;
            data_flip=0;
            break;
        case 1: 
            parity_bit_to_flip = 0;
            data_flip=0;
            break;
        case 2:
            parity_bit_to_flip = 1;
            data_flip=0;
            break;
        case 4:
            data_flip=0;
            parity_bit_to_flip = 2;
            break;
        case 8:
            data_flip=0;
            parity_bit_to_flip = 3;
            break;

        // data bit d01 ... d08
        case 3: 
            data_bit_to_flip = 0;
            break;
        case 5: 
            data_bit_to_flip = 1;
            break;
        case 6: 
            data_bit_to_flip = 2;
            break;
        case 7: 
            data_bit_to_flip = 3;
            break;
        case 9: 
            data_bit_to_flip = 4;
            break;
        case 10: 
            data_bit_to_flip = 5;
            break;
        case 11: 
            data_bit_to_flip = 6;
            break;
        case 12: 
            data_bit_to_flip = 7; 
            break;


        default:
            printf("flipped bit OUT OF RANGE\n");
            return;
    }

    if(data_flip)
    {
        printf("DATA  : request=%hu\n", bit_to_flip);
        printf("DATA  : bit to flip=%hu\n", data_bit_to_flip);

        byte = ecc->data_memory[offset];
        printf("DATA  : original byte    = 0x%02X\n", byte);
        byte ^= (1 << (data_bit_to_flip));
        printf("DATA  : flipped bit byte = 0x%02X\n\n", byte);
        ecc->data_memory[offset] = byte;

    }
    else
    {
        printf("PARITY: request=%hu\n", bit_to_flip);
        printf("PARITY: bit to flip=%hu\n", parity_bit_to_flip);

        byte = ecc->code_memory[offset];
        printf("PARITY: original byte    = 0x%02X\n", byte);
        byte ^= (1 << (parity_bit_to_flip));
        printf("PARITY: flipped bit byte = 0x%02X\n\n", byte);
        ecc->code_memory[offset] = byte;
    }

}