BER-TLV parser and builder(C Standard)
==========================


How to parse
------------

```
	int   hexStrLen = 54;
	uint8 hexStr[hexStrLen] = "50045649534157131000023100000033D44122011003400000481F";
	uint8 bytes[hexStrLen/ 2];
    int bytesLength = hexStrLen / 2;
	tlvNode_t * tlvHeader; //Start Saving data from this point
	hex2bin(hexStr, bytes, hexStrLen);

	initTlvList();
	tlvListParse(bytes, bytesLength);
	tlvHeader = getTlvNodeHeader();

	printAllOfTlvTagValuePair();
	freeTlvList(); //Lost All Parsed Data
```

How to build
------------

```
	int tlvListBufferLength;
	uint8* tlvBufferList;
	uint8 tag1Str[1] = "88";
    uint8 value1Str[] = "02";
	uint8 tag2Str[] = "5F2D";
    uint8 value2Str[] = "656E";	
	uint8 tag1[1], value1[1], tag2[2], value2[2];
	hex2bin(tag1Str, tag1, 2);
    hex2bin(value1Str, value1, 2);
	hex2bin(tag2Str, tag2 4);
    hex2bin(value2Str, value2, 4);
	
    initTlvList();
    addNexTlvToList(tag1, sizeof(tag1), value1, sizeof(value1));
    addNexTlvToList(tag2, sizeof(tag2), value2, sizeof(value2));
    tlvListBufferLength = buildTlvListBuffer();
	tlvBufferList = getTlvListBuffer();
	
    printAllOfTlvBuffers();	
    freeTlvList();
	freeTlvListBuffer();

```