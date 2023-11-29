//	Base64 encoder/decoder
//	2022 maddsua | https://gitlab.com/maddsua
//
//	So, this could be one of the fastest base64 enc/decoder implementation. Or it's the dumbest one. Need your feedback to tell

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

bool base64IsValid(const char* basedString) {
	
	// just to not to recalc strlen() in every iteration
	const size_t inputLen = strlen(basedString);
	
	//	it just check if all of the character are in range to base64 drictionary... alphabet? whatever you call that in English
	for(size_t i = 0; i < inputLen; i++) {
		
		if((basedString[i] < 'A' || basedString[i] > 'Z') && (basedString[i] < 'a' || basedString[i] > 'z') && (basedString[i] < '0' || basedString[i] > '9') && 
			(basedString[i] != '+') && (basedString[i] != '/') && (basedString[i] != '=')) {
				
			return false;
		}
	}
	
	return true;
}

char* base64Encode(const unsigned char* source, size_t sourceLen) {
	
	//	yes. this table is here too
	static const char base64_EncodeTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	
	//	safe margins and other good stuff to not to shoot yourself in the foot
	const short safeMargin = 8;
	const size_t plainSize = ((sourceLen + safeMargin) * sizeof(unsigned char));
	const size_t encLen = ((sourceLen * 4) / 3);
	const size_t encSize = ((encLen + safeMargin) * sizeof(char));
	
	//	allocate safe memory so we wont go out of array. No SIGSEGV-zone thanks to safeMargin
	unsigned char* plain = (unsigned char*)malloc(plainSize);
		memset(plain, 0, plainSize);
		memcpy(plain, source, sourceLen);
	
	//	allocate memory for our output. Safe margin allows us to not keep a potential length differences in mind
	char* encoded = (char*)malloc(encSize);
		memset(encoded, 0, encSize);
	
	//	main encode loop that doe's not do any calculations but converting 8-bits to 6-bits. thats why it's fast
	size_t bpos = 0;
	for(size_t i = 0; i < sourceLen; i++) {
		
		// mindfck math here
		encoded[bpos] = base64_EncodeTable[(plain[i] >> 2)];
		encoded[bpos + 1] = base64_EncodeTable[((((plain[i] << 4) ^ (plain[i + 1] >> 4))) & 0x3F)];
		encoded[bpos + 2] = base64_EncodeTable[((((plain[i + 1] << 2) ^ (plain[i + 2] >> 6))) & 0x3F)];
		encoded[bpos + 3] = base64_EncodeTable[(plain[i + 2] & 0x3F)];
		
		i += 2;
		bpos += 4;
	}
	
	//	And now we deal with lenght and all of that.... no I'm just kidding, we put '=' and terminator at the end of asci string aka base64-encoded string and thats all.
	//	It works cuz we're returning a c string that is at least 5 bytes longer that its actual content. so when you call strlen() on it or copy it,
	//		it wold only "see  a valid base64 string and none of tail-garbage
	
	//	very "clever" calculations
	const size_t fullBlock = (sourceLen / 3) * 3;
	const short uncompBlock = (3 - (sourceLen - fullBlock));
	const size_t lastBlock = (fullBlock * 4) / 3;
		
		//	this fires when we have one or more "loose" bytes in a last block. it can't be three cuz of algorythm itself
		//	ok it's a padding-adding thing
		if(uncompBlock > 0) {
			
			//	and this will fire if we have two "loose" bytes (here it means a bytes with garbage)
			if(uncompBlock > 1) {
				encoded[lastBlock + 2] = '=';
			}
		
			encoded[lastBlock + 3] = '=';
			encoded[lastBlock + 4] = 0;
		}
		
	free(plain);
	return encoded;
}

unsigned char* base64Decode(const char* baseString, size_t* decodedLen) {
	
	//	full decode table does brrrrrr. high-speed tricks here
	static const unsigned char base64_DecodeTable[256] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0xF8,
		0,0,0,
		0xFC,0xD0,0xD4,0xD8,0xDC,0xE0,0xE4,0xE8,0xEC,0xF0,0xF4,
		0,0,0,0,0,0,0,
		0x00,0x04,0x08,0x0C,0x10,0x14,0x18,0x1C,0x20,0x24,0x28,0x2C,0x30,0x34,0x38,0x3C,0x40,0x44,0x48,0x4C,0x50,0x54,0x58,0x5C,0x60,0x64,
		0,0,0,0,0,0,
		0x68,0x6C,0x70,0x74,0x78,0x7C,0x80,0x84,0x88,0x8C,0x90,0x94,0x98,0x9C,0xA0,0xA4,0xA8,0xAC,0xB0,0xB4,0xB8,0xBC,0xC0,0xC4,0xC8,0xCC,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};
	
	//	another protection from foot-shooting
	const short safeMargin = 8;
	const size_t encLen = strlen(baseString);
	const size_t encodedSafeSize = ((encLen + safeMargin) * sizeof(char));
	
	//	allocate safe memory for encoded path. that's a text content here, ok?
	char* encoded = (char*)malloc(encodedSafeSize);
		memset(encoded, 0, encodedSafeSize);
		strncpy(encoded, baseString, encLen);
	
	//	determine an actual size of initial data cuz "=" will give a 0x00 and our buffer is already full of 0x00
	size_t decdLen = ((strlen(encoded) * 3) / 4);
		if(encoded[strlen(encoded) - 1] == '=') decdLen--;
		if(encoded[strlen(encoded) - 2] == '=') decdLen--;
	
	//	create memory for decoded result. with safe margin of 0x00 of course
	const size_t decdSize = ((decdLen + safeMargin) * sizeof(unsigned char));
	unsigned char* decoded = (unsigned char*)malloc(decdSize);
		memset(decoded, 0, decdSize);
		
		
	//	even more high-speed loop if comparing to encoding function. lightning fast. 50mb/s. well, it could be not very precise, but heyyyy
	size_t blpos = 0;
		for(size_t i = 0; i < encLen; i++){
		
			// mindfck math here again
			decoded[blpos] = base64_DecodeTable[encoded[i]] ^ (base64_DecodeTable[encoded[i+1]] >> 6);
			decoded[blpos + 1] = (base64_DecodeTable[encoded[i+1]] << 2) ^ (base64_DecodeTable[encoded[i+2]] >> 4);
			decoded[blpos + 2] = (base64_DecodeTable[encoded[i+2]] << 4) ^ (base64_DecodeTable[encoded[i+3]] >> 2);
		
			i += 3;
			blpos += 3;
		}
	
	//	return decoded size if provided variable-pointer is not a null pointer
	if(decodedLen != NULL) *decodedLen = decdLen;
	
	free(encoded);
	return decoded;
}
