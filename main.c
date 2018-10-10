#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

#ifndef uint8
#define uint8 unsigned char
#endif // uint8

#ifndef uint16
#define uint16 unsigned int
#endif // uint16

#ifndef uint32
#define uint32 unsigned long int
#endif // uint32

#ifndef __cplusplus
#define NULL ((void *)0)
#else   /* C++ */

#endif
#define LCD_STR_MEMMORY_LEAKAGE "Memmory Leakage"
#define MSG_ERROR               "Error:"
#define displayMessageBox(a,b)   printf("%s %s.\n\r", b, a)

typedef struct
{
uint8*  tag;
uint8*  length;
uint8*  value;
uint8*  tlvBuffer;
uint32  tagLen;
uint32  lenOfLength;
uint32  valueLen;
uint32  tlvBufferLen;
uint8   nestLevel;
}tlvInfoST;

typedef struct tlvNode {
    tlvInfoST * tlvInfo;
    struct tlvNode * next;
} tlvNode_t;

static tlvNode_t * tlvHeaderS;
static tlvNode_t * tlvCurrentS;
static uint8   * tlvListBufferS;

/*-------------  TLV Common Functions -------------*/

void byteArrayToHexStringWithZero(uint8* byteArray, uint16 length, uint8* outStr)
{
    uint16 i = 0;
    char hex[3] = { 0 };

    for (i = 0; i < length; i++)
    {
        sprintf(hex, "%02X", byteArray[i]);
        strcat(outStr, hex);
    }
}

uint8* getTlvListBuffer(void)
{
    return tlvListBufferS;
}

tlvNode_t* getTlvNodeHeader(void)
{
    return tlvHeaderS;
}

void freeTlvList(void)
{
    tlvNode_t* tlvPrevious;
    for(tlvCurrentS = tlvHeaderS; tlvCurrentS != NULL;)
    {
        free(tlvCurrentS->tlvInfo->tag);
        free(tlvCurrentS->tlvInfo->length);
        free(tlvCurrentS->tlvInfo->value);
        free(tlvCurrentS->tlvInfo->tlvBuffer);
        free(tlvCurrentS->tlvInfo);
        tlvPrevious = tlvCurrentS;
        tlvCurrentS = tlvCurrentS->next;
        free(tlvPrevious);
    }
}

void freeTlvListBuffer(void)
{
    free(tlvListBufferS);
    tlvListBufferS = NULL;
}

uint8 initTlvList(void)
{
    if((tlvHeaderS = (tlvNode_t*)calloc(1, sizeof(tlvNode_t))) == NULL)
    {
        displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
        return FALSE;
    }
    tlvHeaderS->tlvInfo = NULL;
    tlvHeaderS->next    = NULL;
    tlvCurrentS         = tlvHeaderS;
    return TRUE;
}

uint8 initTlvListBuffer(void)
{
    return TRUE;
}

/*-------------  TLV builder Functions -------------*/

uint32 caclulateLenOfLength(uint32 valueLen)
{
    uint32 numBytes = 1;
    if(valueLen > 0 && valueLen < 128)
    {
        numBytes = 1;
    }
    else
    {
        do{
            numBytes++;
        } while (valueLen >>= 8);
    }
    return numBytes;
}

void fillLength(tlvInfoST* tlvInfo)
{
    uint32 i;
    if(tlvInfo->lenOfLength < 2)
        tlvInfo->length[0] = tlvInfo->valueLen;
    else
    {
        tlvInfo->length[0] = (tlvInfo->lenOfLength - 1) | 0x80;
        for(i = 1; i < tlvInfo->lenOfLength; i++)
            tlvInfo->length[i] = (tlvInfo->valueLen >> (8 * (tlvInfo->lenOfLength - 1 - i))) & 0xFF;
    }
}

void updateTlvBuffer(tlvInfoST* tlvInfo)
{
    uint32 offset = 0;
    // TAG
    memcpy(tlvInfo->tlvBuffer + offset, tlvInfo->tag, tlvInfo->tagLen);
    offset += tlvInfo->tagLen;

    // LENGTH
    memcpy(tlvInfo->tlvBuffer + offset, tlvInfo->length, tlvInfo->lenOfLength);
    offset += tlvInfo->lenOfLength;

    // VALUE
    memcpy(tlvInfo->tlvBuffer + offset, tlvInfo->value, tlvInfo->valueLen);
//    offset += tlvInfo.valueLen;
}

uint8 addNexTlvToList(uint8 * tag, uint32 tagLen, uint8 * value, uint32 valueLen)
{
tlvNode_t* tlvPrevious;
tlvInfoST* tlvInfo;

/**< Memory Allocation of next node if tlvInfo of Current node is not empty*/
if(tlvCurrentS->tlvInfo != NULL)
{
    if((tlvCurrentS->next = (tlvNode_t*)calloc(1, sizeof(tlvNode_t))) == NULL)
    {
        displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
        return FALSE;
    }
    tlvPrevious = tlvCurrentS;
    tlvCurrentS = tlvCurrentS->next;
    tlvCurrentS->next = NULL;
}

/**< Allocate memory for tlvInfo */
if((tlvCurrentS->tlvInfo = (tlvInfoST*)calloc(1, sizeof(tlvInfoST))) == NULL)
{
    displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
    free(tlvCurrentS);
    tlvCurrentS = tlvPrevious;
    tlvCurrentS->next =NULL;
    return FALSE;
}

tlvInfo = tlvCurrentS->tlvInfo;
tlvInfo->tagLen = tagLen;
tlvInfo->valueLen = valueLen;
tlvInfo->lenOfLength = caclulateLenOfLength(valueLen);
tlvInfo->tlvBufferLen =  tlvInfo->tagLen + tlvInfo->lenOfLength + tlvInfo->valueLen;

if((tlvInfo->tag = (uint8*)calloc(tlvInfo->tagLen, sizeof(uint8))) == NULL)
{
    displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
    free(tlvInfo);
    free(tlvCurrentS);
    tlvCurrentS = tlvPrevious;
    tlvCurrentS->next =NULL;
    return FALSE;
}

if((tlvInfo->length = (uint8*)calloc(tlvInfo->lenOfLength, sizeof(uint8))) == NULL)
{
    displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
    free(tlvInfo->tag);
    free(tlvInfo);
    free(tlvCurrentS);
    tlvCurrentS = tlvPrevious;
    tlvCurrentS->next =NULL;
    return FALSE;
}

if((tlvInfo->value = (uint8*)calloc(tlvInfo->valueLen, sizeof(uint8))) == NULL)
{
    displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
    free(tlvInfo->tag);
    free(tlvInfo->length);
    free(tlvInfo);
    free(tlvCurrentS);
    tlvCurrentS = tlvPrevious;
    tlvCurrentS->next =NULL;
    return FALSE;
}

if((tlvInfo->tlvBuffer = (uint8*)calloc(tlvInfo->tlvBufferLen, sizeof(uint8))) == NULL)
{
    displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
    free(tlvInfo->tag);
    free(tlvInfo->length);
    free(tlvInfo->value);
    free(tlvInfo);
    free(tlvCurrentS);
    tlvCurrentS = tlvPrevious;
    tlvCurrentS->next =NULL;
    return FALSE;
}

/**< Fill buffers with data */
memcpy(tlvInfo->tag, tag, tlvInfo->tagLen);
fillLength(tlvInfo);
memcpy(tlvInfo->value, value, tlvInfo->valueLen);
updateTlvBuffer(tlvInfo);

return TRUE;
}

void printAllOfTlvBuffers(void)
{
    uint8 outstr[1000];
    uint32 i = 0;
    for(tlvCurrentS = tlvHeaderS; tlvCurrentS != NULL; tlvCurrentS = tlvCurrentS->next)
    {
        memset(outstr, 0x00, sizeof(outstr));
        byteArrayToHexStringWithZero(tlvCurrentS->tlvInfo->tlvBuffer, tlvCurrentS->tlvInfo->tlvBufferLen, outstr);
        printf("TLV Buffer %ld : %s\n\r", i++, outstr);
    }
}

uint32 caculateTlvListBufferLength(void)
{
    uint32 sum = 0;
    for(tlvCurrentS = tlvHeaderS; tlvCurrentS != NULL; tlvCurrentS = tlvCurrentS->next)
    {
        sum += tlvCurrentS->tlvInfo->tlvBufferLen;
    }
    return sum;
}

void fillTlvListBuffer(void)
{
    uint32 offset = 0;
    for(tlvCurrentS = tlvHeaderS; tlvCurrentS != NULL; tlvCurrentS = tlvCurrentS->next)
    {
        memcpy(tlvListBufferS + offset, tlvCurrentS->tlvInfo->tlvBuffer, tlvCurrentS->tlvInfo->tlvBufferLen);
        offset += tlvCurrentS->tlvInfo->tlvBufferLen;
    }
}

uint32 buildTlvListBuffer(void)
{
    uint32 tlvListBufferLength;
    tlvListBufferLength = caculateTlvListBufferLength();
    if(tlvListBufferS != NULL)
        freeTlvListBuffer();
    if((tlvListBufferS = (uint8*)calloc(tlvListBufferLength, sizeof(uint8))) == NULL)
    {
        displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
        return FALSE;
    }
    fillTlvListBuffer();
    return tlvListBufferLength;
}

void testLenOfLength(void)
{
    uint32 i;
    uint32 length[6] = {0x35, 0xA0, 0x100, 0xF2D3, 0xDC43FF, 0x34895F2E };
    uint8 testLenOfLength[6] = {1, 2, 3, 3, 4, 5};
    uint8 lenOfLength[6];
    memset(lenOfLength, 0x00, 6);
    for(i = 0; i < 6; i++)
        lenOfLength[i] = caclulateLenOfLength(length[i]);
    assert(memcmp(testLenOfLength, lenOfLength, 6) == 0);
    printf("Test function lenOfLength Successfully done.\n\r");
}

uint8 testFillLength(void)
{
    uint32 i, j;
    tlvInfoST tlvInfoTest;
    uint32 length[6] = {0x35, 0xA0, 0x100, 0xF2D3, 0xDC43FF, 0x34895F2E };
    uint8 testTlvLength[6][5] = {{0x35}, {0x81 ,0xA0}, {0x82 ,0x01 ,0x00}, {0x82 ,0xF2 ,0xD3}, {0x83 ,0xDC ,0x43 ,0xFF}, {0x84 ,0x34 ,0x89 ,0x5F ,0x2E}};

    for(i = 0; i < 6; i++) {
        tlvInfoTest.valueLen = length[i];
        tlvInfoTest.lenOfLength = caclulateLenOfLength(tlvInfoTest.valueLen);
        if((tlvInfoTest.length = (uint8*)calloc(tlvInfoTest.lenOfLength, sizeof(uint8))) == NULL)
        {
            displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
            for(j = 0; j < i; j++)
                free(tlvInfoTest.length);
            return FALSE;
        }
        fillLength(&tlvInfoTest);
        assert(memcmp(tlvInfoTest.length, testTlvLength[i], tlvInfoTest.lenOfLength) == 0);
        free(tlvInfoTest.length);
    }
    return TRUE;
    printf("Test function fillLength Successfully done.\n\r");
}

void testTlvListBuild(void)
{
    uint32 tlvListBufferLength;
    uint32 testTlvListBufferLength = 28;
    uint8 testTlvListBuffer[] = {
        0xDF, 0x92, 0x23, 0x05, 0x11, 0x19, 0x23, 0x44, 0x55,
        0x9F, 0x02, 0x06, 0x99, 0x88, 0x23, 0x44, 0x55, 0x56,
        0x53, 0x08, 0x99, 0x88, 0x23, 0x44, 0x55, 0x56, 0x78, 0x67
        };
    uint8 tag1[3] = {0xDF, 0x92, 0x23};
    uint8 value1[5] = {0x11, 0x19, 0x23, 0x44, 0x55};
    uint8 testTlvBuffer1[] = {0xDF, 0x92, 0x23, 0x05, 0x11, 0x19, 0x23, 0x44, 0x55};
    uint32     testTlvBufferLen1 = sizeof(testTlvBuffer1);
    uint32 tagLen1 = sizeof(tag1);
    uint32 valueLen1 = sizeof(value1);
    uint8 tag2[2] = {0x9F, 0x02};
    uint8 value2[6] = {0x99, 0x88, 0x23, 0x44, 0x55, 0x56};
    uint8 testTlvBuffer2[] = {0x9F, 0x02, 0x06, 0x99, 0x88, 0x23, 0x44, 0x55, 0x56};
    uint32     testTlvBufferLen2 = sizeof(testTlvBuffer2);
    uint32 tagLen2 = sizeof(tag2);
    uint32 valueLen2 = sizeof(value2);
    uint8 tag3[1] = {0x53};
    uint8 value3[8] = {0x99, 0x88, 0x23, 0x44, 0x55, 0x56, 0x78, 0x67};
    uint8 testTlvBuffer3[] = {0x53, 0x08, 0x99, 0x88, 0x23, 0x44, 0x55, 0x56, 0x78, 0x67};
    uint32     testTlvBufferLen3 = sizeof(testTlvBuffer3);
    uint32 tagLen3 = sizeof(tag3);
    uint32 valueLen3 = sizeof(value3);

    initTlvList();

    addNexTlvToList(tag1, tagLen1, value1, valueLen1);
    assert(tlvCurrentS->tlvInfo->tlvBufferLen == testTlvBufferLen1);
    assert(memcmp(tlvCurrentS->tlvInfo->tlvBuffer, testTlvBuffer1, testTlvBufferLen1) == 0);

    addNexTlvToList(tag2, tagLen2, value2, valueLen2);
    assert(tlvCurrentS->tlvInfo->tlvBufferLen == testTlvBufferLen2);
    assert(memcmp(tlvCurrentS->tlvInfo->tlvBuffer, testTlvBuffer2, testTlvBufferLen2) == 0);

    addNexTlvToList(tag3, tagLen3, value3, valueLen3);
    assert(tlvCurrentS->tlvInfo->tlvBufferLen == testTlvBufferLen3);
    assert(memcmp(tlvCurrentS->tlvInfo->tlvBuffer, testTlvBuffer3, testTlvBufferLen3) == 0);

    tlvListBufferLength = buildTlvListBuffer();

    printAllOfTlvBuffers();
    printf("TLV List Buffer length : %ld\n\r", tlvListBufferLength);
    assert(tlvListBufferLength == testTlvListBufferLength);
    assert(memcmp(tlvListBufferS, testTlvListBuffer, tlvListBufferLength) == 0);
    uint8 outstr[1000];
    memset(outstr, 0x00, sizeof(outstr));
    byteArrayToHexStringWithZero(tlvListBufferS, tlvListBufferLength, outstr);
    printf("TLV List Buffer : %s\n\r", outstr);
    freeTlvListBuffer();
    freeTlvList();
    printf("Test function tlvListBuild Successfully done.\n\r\n\r");
}

/**< Build a nested TLV */
void testBuildTlvExample1(void)
{
    uint32 tlvListBufferLength;
    uint8 testTlvListBuffer[] = {
        0x6F, 0x1A, 0x84, 0x0E, 0x31, 0x50, 0x41, 0x59, 0x2E, 0x53, 0x59, 0x53, 0x2E, 0x44,
        0x44, 0x46, 0x30, 0x31, 0xA5, 0x08, 0x88, 0x01, 0x02, 0x5F, 0x2D, 0x02, 0x65, 0x6E
        };
    uint32 testTlvListBufferLength = sizeof(testTlvListBuffer);
    initTlvList();
    uint8 tag1[] = {0x88};
    uint8 value1[] = {0x02};
    uint8 tag2[] = {0x5F, 0x2D};
    uint8 value2[] = {0x65, 0x6E};
    addNexTlvToList(tag1, sizeof(tag1), value1, sizeof(value1));
    addNexTlvToList(tag2, sizeof(tag2), value2, sizeof(value2));
    tlvListBufferLength = buildTlvListBuffer();
    printAllOfTlvBuffers();
    printf("TLV List Buffer length : %ld\n\r", tlvListBufferLength);
    freeTlvList();
    initTlvList();
    uint8 tag3[] = {0x84};
    uint8 value3[] = {0x31, 0x50, 0x41, 0x59, 0x2E, 0x53, 0x59, 0x53, 0x2E, 0x44, 0x44, 0x46, 0x30, 0x31};
    uint8 tag4[] = {0xA5};
    addNexTlvToList(tag3, sizeof(tag3), value3, sizeof(value3));
    addNexTlvToList(tag4, sizeof(tag4), tlvListBufferS, tlvListBufferLength);
//    freeTlvListBuffer();
    tlvListBufferLength = buildTlvListBuffer();
    printAllOfTlvBuffers();
    printf("TLV List Buffer length : %ld\n\r", tlvListBufferLength);
    freeTlvList();
    initTlvList();
    uint8 tag5[] = {0x6F};
    addNexTlvToList(tag5, sizeof(tag5), tlvListBufferS, tlvListBufferLength);
//    freeTlvListBuffer();
    tlvListBufferLength = buildTlvListBuffer();
    printAllOfTlvBuffers();
    printf("TLV List Buffer length : %ld\n\r", tlvListBufferLength);
    freeTlvList();
    assert(tlvListBufferLength == testTlvListBufferLength);
    assert(memcmp(tlvListBufferS, testTlvListBuffer, tlvListBufferLength) == 0);
    uint8 outstr[1000];
    memset(outstr, 0x00, sizeof(outstr));
    byteArrayToHexStringWithZero(tlvListBufferS, tlvListBufferLength, outstr);
    printf("TLV List Buffer : %s\n\r", outstr);
    freeTlvListBuffer();
}

/*-------------  TLV Parser Functions -------------*/

void printAllOfTlvTagValuePair(void)
{
    uint8 tagStr[5];
    uint8 valueStr[1000];
    uint32 i = 0;
    printf("\n\rTLV Parsed data:\n\r");
    for(tlvCurrentS = tlvHeaderS; tlvCurrentS != NULL; tlvCurrentS = tlvCurrentS->next)
    {
        memset(tagStr, 0x00, sizeof(tagStr));
        memset(valueStr, 0x00, sizeof(valueStr));
        byteArrayToHexStringWithZero(tlvCurrentS->tlvInfo->tag, tlvCurrentS->tlvInfo->tagLen, tagStr);
        byteArrayToHexStringWithZero(tlvCurrentS->tlvInfo->value, tlvCurrentS->tlvInfo->valueLen, valueStr);
        for(i = 0; i < tlvCurrentS->tlvInfo->nestLevel; i++)
            printf("\t");
        printf("%s: %s\n\r", tagStr, valueStr);
    }
}

uint32 getTagLenFromBufferList(uint8* tagStartBuffer)
{
    uint8* tag;
    uint8 maskFirstSubsequentByte = 0x1F;
    uint8 maskAnotherTagByte = 0x80;
    uint8 lastByte = 0;
    uint32 i = 0;
    if((tag = (uint8*)calloc(1, sizeof(uint8))) == NULL)
    {
        displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
        return FALSE;
    }
    memcpy(tag, tagStartBuffer, i + 1);
    if((tag[i] & maskFirstSubsequentByte) == maskFirstSubsequentByte)
    {
        i++;
        if((tag = (uint8*)realloc(tag, (i + 1) * sizeof(uint8))) == NULL)
        {
            displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
            return FALSE;
        }
        memcpy(tag, tagStartBuffer, i + 1);
        while(lastByte == 0)
        {
            if((tag[i] & maskAnotherTagByte) == maskAnotherTagByte)
            {
                i++;
                if((tag = (uint8*)realloc(tag, (i + 1) * sizeof(uint8))) == NULL)
                {
                    displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
                    return FALSE;
                }
                memcpy(tag, tagStartBuffer, i + 1);
            }
            else
                lastByte = 1;
        }
    }
    free(tag);
    return i + 1;
}

 uint32 getLenOfLengthFromList(uint8 firstByteOfLength)
 {
    uint32 lenOfLength;
    uint8 maskFirstMsbBit = 0x80;
    if ((firstByteOfLength & maskFirstMsbBit) == maskFirstMsbBit)
        lenOfLength = (firstByteOfLength & ~(maskFirstMsbBit)) + 1;
    else
        lenOfLength = 1;
    return lenOfLength;
 }

 uint32 getValueLenFromLength(uint8* length)
 {
     uint32 lenOfLength = getLenOfLengthFromList(length[0]);
     uint32 valueLen = 0;
     uint32 i;
     if(lenOfLength == 1)
     {
        valueLen = length[0];
     }
     else if(lenOfLength > 1)
     {
         for(i = 1; i < lenOfLength; i++)
         {
              valueLen |= length[i] << (8 * (lenOfLength - i - 1));
         }
     }
    return valueLen;
 }

uint32 getNextTlvFromList(uint8* tlvListBufferPtr, uint8 nestLevel)
{
tlvNode_t* tlvPrevious;
tlvInfoST* tlvInfo;
uint32 offset = 0;
uint32 tlvBufferLen;
uint8 maskCheckNested = 0x20;

/**< Memory Allocation of next node if tlvInfo of Current node is not empty*/
if(tlvCurrentS->tlvInfo != NULL)
{
    if((tlvCurrentS->next = (tlvNode_t*)calloc(1, sizeof(tlvNode_t))) == NULL)
    {
        displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
        return FALSE;
    }
    tlvPrevious = tlvCurrentS;
    tlvCurrentS = tlvCurrentS->next;
    tlvCurrentS->next = NULL;
}

/**< Allocate memory for tlvInfo */
if((tlvCurrentS->tlvInfo = (tlvInfoST*)calloc(1, sizeof(tlvInfoST))) == NULL)
{
    displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
    free(tlvCurrentS);
    tlvCurrentS = tlvPrevious;
    tlvCurrentS->next =NULL;
    return FALSE;
}

tlvInfo = tlvCurrentS->tlvInfo;

tlvInfo->nestLevel = nestLevel;

tlvInfo->tagLen = getTagLenFromBufferList(tlvListBufferPtr);
if((tlvInfo->tag = (uint8*)calloc(tlvInfo->tagLen, sizeof(uint8))) == NULL)
{
    displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
    free(tlvInfo);
    free(tlvCurrentS);
    tlvCurrentS = tlvPrevious;
    tlvCurrentS->next =NULL;
    return FALSE;
}
memcpy(tlvInfo->tag, tlvListBufferPtr + offset, tlvInfo->tagLen);

offset += tlvInfo->tagLen;
tlvInfo->lenOfLength = getLenOfLengthFromList(*(uint8*)(tlvListBufferPtr + offset));
if((tlvInfo->length = (uint8*)calloc(tlvInfo->lenOfLength, sizeof(uint8))) == NULL)
{
    displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
    free(tlvInfo->tag);
    free(tlvInfo);
    free(tlvCurrentS);
    tlvCurrentS = tlvPrevious;
    tlvCurrentS->next =NULL;
    return FALSE;
}
memcpy(tlvInfo->length, tlvListBufferPtr + offset, tlvInfo->lenOfLength);

tlvInfo->valueLen = getValueLenFromLength(tlvInfo->length);
if((tlvInfo->value = (uint8*)calloc(tlvInfo->valueLen, sizeof(uint8))) == NULL)
{
    displayMessageBox(LCD_STR_MEMMORY_LEAKAGE, MSG_ERROR);
    free(tlvInfo->tag);
    free(tlvInfo->length);
    free(tlvInfo);
    free(tlvCurrentS);
    tlvCurrentS = tlvPrevious;
    tlvCurrentS->next =NULL;
    return FALSE;
}
offset += tlvInfo->lenOfLength;
memcpy(tlvInfo->value, tlvListBufferPtr + offset, tlvInfo->valueLen);

tlvInfo->tlvBufferLen =  tlvInfo->tagLen + tlvInfo->lenOfLength + tlvInfo->valueLen;

tlvBufferLen =  tlvInfo->tlvBufferLen;

/**< Check if it's constructed, Parse constructed elements*/
if(tlvInfo->tag[0] & maskCheckNested)
{
    nestLevel++;
    offset = tlvInfo->tagLen + tlvInfo->lenOfLength;
    while(offset < tlvBufferLen)
    {
        offset +=  getNextTlvFromList(tlvListBufferPtr + offset, nestLevel);
    }
}
return tlvBufferLen;
}

uint8 tlvListParse(uint8* tlvListBuffer,uint32  tlvListBufferLength)
{
    uint8* tlvListBufferPtr;
    uint32 tlvBufferLen = 0;
    tlvListBufferPtr = tlvListBuffer;
    while(tlvListBufferPtr - tlvListBuffer <  tlvListBufferLength){
        tlvBufferLen = getNextTlvFromList(tlvListBufferPtr, 0);
        tlvListBufferPtr = (uint8*)(tlvListBufferPtr + tlvBufferLen);
    }
    return TRUE;
}

void testGetTagLenFromList(void)
{
    uint8 testTlvTag[3][4] = {{0x81, 0x23, 0x43, 0x12},{0x9F, 0x01, 0x11, 0x12}, {0x9F, 0x81, 0x23, 0x56}};
    uint32     testTlvTagLen[3] = {1, 2, 3};
    uint32 tlvTagLen[3];
    uint32 i;
    for(i = 0; i < 3; i++)
    {
        tlvTagLen[i] = getTagLenFromBufferList(testTlvTag[i]);
        assert(tlvTagLen[i] == testTlvTagLen[i]);
    }
    printf("Test function getTagLenFromBufferList Successfully done.\n\r");
}

void testGetLenOfLengthFromList(void)
{
    uint8 testFirstByteOfLength[3] = {0x55, 0x81, 0x85};
    uint8 testLenOfLength[3] = {1, 2, 6};
    uint32     lenOfLength[3];
    uint32 i;
    for (i = 0; i < 3; i++)
    {
        lenOfLength[i] = getLenOfLengthFromList(*(uint8 *)(testFirstByteOfLength + i));
        assert(testLenOfLength[i] == lenOfLength[i]);
    }
    printf("Test function getLenOfLengthFromList Successfully done.\n\r");
}

void testGetValueLenFromLength(void)
{
    uint8 testLength[5][6] = {{0x55, 0x20}, {0x81, 0x80, 0x22}, {0x82, 0x01, 0x22, 0x95}, {0x83, 0x03, 0x00, 0x11, 0x12}, {0x84, 0x12, 0x01, 0x16, 0x55, 0x22}};
    uint32 testValueLen[5] = {0x55, 0x80, 0x122, 0x30011, 0x12011655};
    uint32 valueLen[5];
    uint32 i;
    for (i = 0; i < 5; i++)
    {
        valueLen[i] = getValueLenFromLength(testLength[i]);
        assert(testValueLen[i] == valueLen[i]);
    }
    printf("Test function getValueLenFromLength Successfully done.\n\r");
}

void testTlvListParse(void)
{
    uint8 tlvListBuffer[] = {0xDF, 0x92, 0x23, 0x05, 0x11, 0x19, 0x23, 0x44, 0x55,
                               0x9F, 0x02, 0x06, 0x99, 0x88, 0x23, 0x44, 0x55, 0x56,
                               0x53, 0x08, 0x99, 0x88, 0x23, 0x44, 0x55, 0x56, 0x78, 0x67
                              };
    uint32 tlvListBufferLength =  sizeof(tlvListBuffer);
    uint8 testTag1[3] = {0xDF, 0x92, 0x23};
    uint8 testValue1[5] = {0x11, 0x19, 0x23, 0x44, 0x55};
    uint32 testTagLen1 = sizeof(testTag1);
    uint32 testValueLen1 = sizeof(testValue1);
    uint8 testTag2[2] = {0x9F, 0x02};
    uint8 testValue2[6] = {0x99, 0x88, 0x23, 0x44, 0x55, 0x56};
    uint32 testTagLen2 = sizeof(testTag2);
    uint32 testValueLen2 = sizeof(testValue2);
    uint8 testTag3[1] = {0x53};
    uint8 testValue3[8] = {0x99, 0x88, 0x23, 0x44, 0x55, 0x56, 0x78, 0x67};
    uint32 testTagLen3 = sizeof(testTag3);
    uint32 testValueLen3 = sizeof(testValue3);

    initTlvList();
    tlvListParse(tlvListBuffer, tlvListBufferLength);
    printAllOfTlvTagValuePair();

    tlvCurrentS = tlvHeaderS;
    assert(tlvCurrentS->tlvInfo->tagLen == testTagLen1);
    assert(memcmp(tlvCurrentS->tlvInfo->tag, testTag1, testTagLen1) == 0);
    assert(tlvCurrentS->tlvInfo->valueLen == testValueLen1);
    assert(memcmp(tlvCurrentS->tlvInfo->value, testValue1, testValueLen1) == 0);

    tlvCurrentS = tlvCurrentS->next;
    assert(tlvCurrentS->tlvInfo->tagLen == testTagLen2);
    assert(memcmp(tlvCurrentS->tlvInfo->tag, testTag2, testTagLen2) == 0);
    assert(tlvCurrentS->tlvInfo->valueLen == testValueLen2);
    assert(memcmp(tlvCurrentS->tlvInfo->value, testValue2, testValueLen2) == 0);

    tlvCurrentS = tlvCurrentS->next;
    assert(tlvCurrentS->tlvInfo->tagLen == testTagLen3);
    assert(memcmp(tlvCurrentS->tlvInfo->tag, testTag3, testTagLen3) == 0);
    assert(tlvCurrentS->tlvInfo->valueLen == testValueLen3);
    assert(memcmp(tlvCurrentS->tlvInfo->value, testValue3, testValueLen3) == 0);

    assert(tlvCurrentS->next == NULL);

    freeTlvList();

    printf("Test function tlvListParse Successfully done.\n\r");
}

void testParseExmaple1(void)
{
    uint8 tlvListBuffer[] = {
    0xDF, 0xEE, 0x25, 0x02, 0x00, 0x02, 0xDF, 0xEE, 0x26, 0x02, 0x20, 0x00, 0xDF, 0xEE, 0x12,
    0x0A, 0x62, 0x99, 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, 0xDF, 0xEF, 0x5D, 0x10,
    0x51, 0x28, 0xCC, 0xCC, 0xCC, 0xCC, 0x28, 0x77, 0xD1, 0x80, 0x16, 0x22, 0xCC, 0xCC, 0xCC,
    0xCC, 0x57, 0x18, 0x9F, 0x7E, 0x8B, 0x5A, 0x20, 0x6B, 0x4F, 0x2C, 0xEA, 0x93, 0x11, 0x48,
    0x70, 0x4E, 0xC5, 0x49, 0xED, 0xBA, 0xB7, 0x28, 0x64, 0x3E, 0x91, 0x97, 0xDF, 0xEF, 0x5B,
    0x08, 0x51, 0x28, 0xCC, 0xCC, 0xCC, 0xCC, 0x28, 0x77, 0x5A, 0x10, 0xB5, 0xDE, 0xCD, 0x79,
    0xE3, 0xD2, 0x00, 0xA6, 0xDE, 0x66, 0xA2, 0x0C, 0x18, 0xDE, 0x80, 0xAC, 0x5F, 0x20, 0x1A,
    0x2F, 0x43, 0x48, 0x49, 0x50, 0x20, 0x54, 0x45, 0x53, 0x54, 0x20, 0x43, 0x41, 0x52, 0x44,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x5F, 0x24, 0x03, 0x18,
    0x01, 0x31, 0x5F, 0x25, 0x03, 0x15, 0x01, 0x01, 0x5F, 0x28, 0x02, 0x08, 0x40, 0x5F, 0x2A,
    0x02, 0x08, 0x40, 0x5F, 0x2D, 0x02, 0x65, 0x6E, 0x5F, 0x34, 0x01, 0x00, 0x5F, 0x57, 0x01,
    0x00, 0x50, 0x10, 0x44, 0x65, 0x62, 0x69, 0x74, 0x20, 0x4D, 0x61, 0x73, 0x74, 0x65, 0x72,
    0x43, 0x61, 0x72, 0x64, 0x4F, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x04, 0x10, 0x10, 0x82, 0x02,
    0x39, 0x00, 0x84, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x04, 0x10, 0x10, 0x8C, 0x21, 0x9F, 0x02,
    0x06, 0x9F, 0x03, 0x06, 0x9F, 0x1A, 0x02, 0x95, 0x05, 0x5F, 0x2A, 0x02, 0x9A, 0x03, 0x9C,
    0x01, 0x9F, 0x37, 0x04, 0x9F, 0x35, 0x01, 0x9F, 0x45, 0x02, 0x9F, 0x4C, 0x08, 0x9F, 0x34,
    0x03, 0x8D, 0x0C, 0x91, 0x0A, 0x8A, 0x02, 0x95, 0x05, 0x9F, 0x37, 0x04, 0x9F, 0x4C, 0x08,
    0x8E, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x03, 0x44, 0x03, 0x41,
    0x03, 0x1E, 0x03, 0x1F, 0x03, 0x9C, 0x01, 0x00, 0x9F, 0x02, 0x06, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x9F, 0x03, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9F, 0x10, 0x12, 0x01,
    0x10, 0x20, 0x00, 0x05, 0x62, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0x9F, 0x13, 0x00, 0x9F, 0x20, 0x00, 0x9F, 0x26, 0x08, 0xC8, 0x37, 0xA8, 0x5C,
    0x5D, 0xFE, 0x75, 0x73, 0x9F, 0x27, 0x01, 0x00, 0x9F, 0x34, 0x03, 0x1E, 0x03, 0x00, 0x9F,
    0x36, 0x02, 0x02, 0x66, 0x9F, 0x37, 0x04, 0xBB, 0x80, 0x50, 0xC9, 0x9F, 0x38, 0x00, 0x9F,
    0x39, 0x01, 0x07, 0x9F, 0x4D, 0x00, 0x9F, 0x4F, 0x00, 0x95, 0x05, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x9B, 0x02, 0xE8, 0x00, 0x8A, 0x02, 0x5A, 0x33, 0x99, 0x00, 0x9F, 0x5B, 0x00, 0xDF,
    0xEF, 0x4C, 0x06, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0xDF, 0xEF, 0x4D, 0x28, 0xAA, 0x83,
    0x9B, 0x4B, 0x40, 0x20, 0x83, 0xDD, 0xEC, 0x00, 0x61, 0x4D, 0x17, 0x03, 0xB1, 0x39, 0xA0,
    0x75, 0x86, 0x45, 0x35, 0x83, 0xB4, 0xA0, 0x3A, 0xB3, 0x33, 0xFB, 0x21, 0x0F, 0xD1, 0xCD,
    0x4F, 0x8A, 0xC3, 0x60, 0x3D, 0x75, 0x68, 0x8E,
    };
    uint32 tlvListBufferLength =  sizeof(tlvListBuffer);
    initTlvList();
    tlvListParse(tlvListBuffer, tlvListBufferLength);
    printAllOfTlvTagValuePair();
    freeTlvList();
}

void testParseExmaple2(void)
{
    /**< Nested TLV*/
    uint8 tlvListBuffer[] = {
    0x6F, 0x1A, 0x84, 0x0E, 0x31, 0x50, 0x41, 0x59, 0x2E, 0x53, 0x59, 0x53, 0x2E, 0x44, 0x44,
    0x46, 0x30, 0x31, 0xA5, 0x08, 0x88, 0x01, 0x02, 0x5F, 0x2D, 0x02, 0x65, 0x6E
    };
    uint32 tlvListBufferLength =  sizeof(tlvListBuffer);
    initTlvList();
    tlvListParse(tlvListBuffer, tlvListBufferLength);
    printAllOfTlvTagValuePair();
    freeTlvList();
}

void tlvUnitTest(void)
{
    testLenOfLength();
    testFillLength();
    testTlvListBuild();
    testGetTagLenFromList();
    testGetLenOfLengthFromList();
    testGetValueLenFromLength();
    testTlvListParse();
    testBuildTlvExample1();
    testParseExmaple1();
    testParseExmaple2();
    printf("\n\rAll tests passed successfully.\n\r");
}

int main()
{
    tlvUnitTest();
    return 0;
}
