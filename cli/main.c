
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>

#include "mimetypes.h"
#include "unhar_private.h"

unsigned char* dynReadBinFile(const char* path, uint64_t* binSize);
char* jsonStrVal(const char* jsonStr);
bool saveHttpsResponse(char* url, char* mime, char* encoded, char* text, char* path);
unsigned char* base64Decode(const char* baseString, size_t* decodedLen);
void cstrSlideBack(char* str, size_t pos, size_t count);
char* cstrSwapSubstr(char* str, const char* subStrAdd, size_t pos, size_t count);
char* jsonRecreateText(char* jsonVal);
char** listPush(char** list, size_t* itemsCount, const char* newItem);

bool flagOverwrite = false;
bool flagNonverbose = false;
bool flagRecreateText = true;

char** mimeFilterList;
size_t mimeFilterItems = 0;

char** extFilterList;
size_t extFilterItems = 0;

int main(int argc, char** argv) {
	
	printf("\nhttp archive unpacking tool v%i.%i\n\n", VER_MAJOR, VER_MINOR);

	char openFile[PATH_MAX + 1] = {0};
	char savePath[PATH_MAX + 1] = {0};

//	help message
	if(argc == 2 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) {
		
		printf("Usage: %s -i [har_file.har] -o [output_dir] -m [mime_filter] -f [format_filter]\n", ORIGINAL_FILENAME);
		printf(" -w - allow file overwrite\n -s - silent (non-verbose)\n -j - do not post-process text files\n");
		printf("\nSee readme.md for more details.\n\n%s\n", LEGAL_COPYRIGHT);
		return 0;
	}
	
	// go though start flags
	for(int i = 1; i < argc; i++) {
		
		//	overwrite flag
		if(!strcmp(argv[i], "-w")) {
			flagOverwrite = true;
			continue;
		} else if(!strcmp(argv[i], "-s")) {
			flagNonverbose = true;
			continue;
		} else if(!strcmp(argv[i], "-j")) {
			flagRecreateText = false;
			continue;
		}
		
		if((argc - i) > 1) {
			
			const size_t nextArgLen = strlen(argv[i + 1]);
			
			if(nextArgLen > 0) {
				
				//	input file
				if(!strcmp(argv[i], "-i")) {
			
					strncpy(openFile, argv[i + 1], PATH_MAX);
					i++;	
				}
				//	output file
				else if (!strcmp(argv[i], "-o")) {
			
					strncpy(savePath, argv[i + 1], PATH_MAX);
					i++;
				} else if (!strcmp(argv[i], "-m")) {
			
					mimeFilterList = listPush(mimeFilterList, &mimeFilterItems, argv[i + 1]);
					i++;
				} else if (!strcmp(argv[i], "-f")) {
			
					extFilterList = listPush(extFilterList, &extFilterItems, argv[i + 1]);
					i++;
				}
			}
		}
	}

	//	check pathes
	if (strlen(openFile) < 1) {
		printf("Please specify an input file\n\nUse --help for more\n");
		return 0;
	}

	if (strlen(savePath) < 1) {

		const char postfix[] = "_files";
		strncpy(savePath, openFile, (PATH_MAX - strlen(postfix)));

		//	remove extension
		for(int i = (strlen(savePath) - 1); i >= 0; i--) {
			if(savePath[i] == '.') savePath[i] = '\0';
		}

		//	add postfix
		strcat(savePath, postfix);

		printf("Output folder not specified. Defaulting to %s\n", savePath);
	}
	
	//	make dir
	DIR* dest = opendir(savePath);
		if(!dest) {
			if(mkdir(savePath)) {
				printf("Unable to create a directory \"%s\"\n", savePath);
				return 2;
			}
		} else {
			closedir(dest);
		}
	
	//	load har file
	uint64_t jsonLen = 0;
	unsigned char* json = dynReadBinFile(openFile, &jsonLen);
	
	if (json == NULL) {
		printf("Unable to open file %s\n", openFile);
		return 1;
	}
	if (jsonLen < 64) {
		printf("File is too short to be a http archive\n");
		return 2;
	}


/*		JSON parsing part		*/

	//	split file contents by lines, detect request-response block
	//	0x0D - CR;	0x0A - LF
	char* json_reqUrl = NULL;
	char* json_resMime = NULL;
	char* json_resEncode = NULL;
	char* json_resText = NULL;
	
	uint64_t lineCount = 0;

	size_t parsedBlocks = 0;
	size_t rr_blockCount = 0;
	size_t rr_blockInvalid = 0;
	size_t rr_blockError = 0;
	size_t savedFilesCount = 0;
	
		printf("Parsing JSON...\n");
			if(!flagNonverbose) printf("\n");

	for (uint64_t cpos = 0; cpos < jsonLen; cpos++) {
		
		static uint64_t nlpos = 0;
		
		//	detect a single line
		if (json[cpos] == 0x0A || json[cpos] == 0x0D || cpos == (jsonLen - 1)) {
			
			// get line length
			const uint64_t lineLen = (cpos - nlpos);
			const uint64_t lineSize = (lineLen + 1);
			
			//	extract line from file 'stream'
			char* line = (char*)malloc(lineSize);
				line[lineLen] = '\0';
				memcpy(line, json + nlpos, lineLen);
			
				//	detect req-res block start
				if (strstr(line, "\"request\":")) {
					
					rr_blockCount++;

					if (json_reqUrl != NULL && json_resMime != NULL && json_resText != NULL) {

						if(saveHttpsResponse(json_reqUrl, json_resMime, json_resEncode, json_resText, savePath)) savedFilesCount++;

						//	free block memory
						free(json_reqUrl);
							json_reqUrl = NULL;
						free(json_resMime);
							json_resMime = NULL;
						free(json_resText);
							json_resText = NULL;
						if(json_resEncode != NULL) {
							free(json_resEncode);
								json_resEncode = NULL;
						}

					} else {
						rr_blockInvalid++;
					}
				}

				//	deal with its contents
				if (strstr(line, "\"url\":") != NULL) json_reqUrl = jsonStrVal(line);	
				else if (strstr(line, "\"mimeType\":") != NULL) json_resMime = jsonStrVal(line);
				else if (strstr(line, "\"encoding\":") != NULL) json_resEncode = jsonStrVal(line);
				else if (strstr(line, "\"text\":") != NULL) json_resText = jsonStrVal(line);
				
				
				//	set the next line start position, add to line counter and free memory
				if(json[cpos + 1] == 0x0A || json[cpos + 1] == 0x0D) cpos++;	//	skip next character if line has windows line break (CR + LF)
				nlpos = (cpos + 1);
				lineCount++;

			//	destroy the line			
			free(line);
		}
	}
	
	if(rr_blockCount > 0 && rr_blockInvalid > 0) {
		rr_blockCount--;
		rr_blockInvalid--;
	}
	
	if(!flagNonverbose) printf("\n");
	printf("%i files saved to: \"%s/\", %i of %i blocks had no responce content\nJSON file: %i bytes in %i lines\n", savedFilesCount, savePath, rr_blockInvalid, rr_blockCount, jsonLen, lineCount);

	free(json);
	
	return 0;
}



unsigned char* dynReadBinFile(const char* path, uint64_t* binSize) {
		
	const short safeMargin = 8;
	const size_t chunkSize = 8388608;
	const size_t oneMbDataSize = 1048576;
	
			printf("Loading the file. Chunk size: %i MB.\n", (chunkSize / oneMbDataSize));
	
	uint64_t utilized = 0;
	uint64_t capacity = chunkSize;
	uint64_t binfileSize = (capacity + safeMargin);
	
	unsigned char* binfileContents = (unsigned char*)malloc(binfileSize);

	FILE* binaryFile = fopen(path, "rb");

	if(binaryFile == NULL) {
		return NULL;
	}
	
	while(!feof(binaryFile)) {
					
		utilized += fread(binfileContents + utilized, sizeof(unsigned char), chunkSize, binaryFile);
		
		if(utilized >= capacity) {

			printf("\rChunk %i...", ((unsigned int)(utilized / chunkSize) + 1));
						
			capacity += chunkSize;
			binfileSize = (capacity + safeMargin);
			char* tchunk = (char*)realloc(binfileContents, binfileSize);
			
			binfileContents = tchunk;
		}
	}

	fclose(binaryFile);
	
		if(utilized > chunkSize) printf(". Done.\n", utilized);
	
	*binSize = utilized;
	return binfileContents;
}


char* jsonStrVal(const char* jsonStr) {

	const size_t jstrLen = strlen(jsonStr);
	short qcount = 0;
	size_t vfrom = 0;
	size_t vuntil = (jstrLen);

	//	find start index of item value
	for(size_t i = 0; i < jstrLen; i++) {

		if(jsonStr[i] == '\"') qcount++;

		if(qcount == 3) {
			vfrom = (i + 1);
			break;
		}
	}

	if(qcount != 3 || vfrom >= jstrLen) return NULL;

	//	find end position of item value
	for(size_t i = jstrLen; i > 0; i--) {
		if(jsonStr[i] == '\"') {
			vuntil = i;
			break;
		}
	}

	//	save trimmed line to new string
	const size_t valLen = (vuntil - vfrom);
	unsigned char* valStr = (char*)malloc(valLen + 1);
		memcpy(valStr, jsonStr + vfrom, valLen);
		valStr[valLen] = '\0';

	return valStr;
}


bool saveHttpsResponse(char* url, char* mime, char* encoded, char* text, char* path) {
	
	const short filenameSafeMargin = 16;

	//	get file type and extension
	char fileExtension[8];
		strcpy(fileExtension, ".txt");			// set default

	// reset character case to lower (unnecessary step)
	for(int m = 0; m < strlen(mime); m++) {
		if(mime[m] > 0x40 && mime[m] < 0x5B) mime[m] += 0x20;
	}
		
	for(int i = 0; i < mimeTable; i++) {		// look for it in the table
		if(!strcmp(mime, mimeTypes[i].mime)) {
			strcpy(fileExtension, mimeTypes[i].ext);
			break;
		}
	}

	//	get filename: start
	char* slashPoint = strrchr(url, '/');
	size_t nameFrom = 0;
	size_t nameTo = strlen(url);
		if(slashPoint != NULL) nameFrom = ((int)(slashPoint - url) + 1);

	//	get filename end: cut params and other data
	for(size_t i = nameFrom; i < nameTo; i++) {
		if(url[i] == '?') {
			nameTo = i;
			break;
		}
	}

	//	get filename end: cut the extension
	for(size_t i = nameTo; i >= nameFrom; i--) {
		if(url[i] == '.') {
			nameTo = i;
			break;
		}
	}

	//	get filename
	size_t fileNameLen = (nameTo - nameFrom);
		if((fileNameLen + filenameSafeMargin + strlen(path)) > PATH_MAX) fileNameLen = (PATH_MAX - filenameSafeMargin - strlen(path));

	char* fileName = (char*)malloc(fileNameLen + 1);
	
			if(fileName == NULL) {
				printf("!! Memory allocation error\n");
				exit(5);
			}
			
		strncpy(fileName, url + nameFrom, fileNameLen);
		fileName[fileNameLen] = '\0';


	//	check mime filter
	bool mfilter_match = false;
	for(int mf = 0; mf < mimeFilterItems; mf++) {
		if(strstr(mime, mimeFilterList[mf]) != NULL) {
			mfilter_match = true;
			break;
		}
	}

		if(mimeFilterItems > 0 && !mfilter_match) {
			if(!flagNonverbose) printf(" --X \"%s\" dropped cause of mime filter\n", fileName);
			return false;
		}
	
	//	check extension filter
	bool efilter_match = false;
	for(int ff = 0; ff < extFilterItems; ff++) {
		if(!strcmp(fileExtension, extFilterList[ff])) {
			efilter_match = true;
			break;
		}
	}

		if(extFilterItems > 0 && !efilter_match) {
			if(!flagNonverbose) printf(" --X \"%s\" dropped cause of format filter\n", fileName);
			return false;
		}


	//	create path
	char fileFullPath[PATH_MAX + 1];
		fileFullPath[PATH_MAX] = '\0';
	unsigned int renameRetries = 0;
	short renameRetriesMax = 16;
	snprintf(fileFullPath, PATH_MAX, "%s\\%s.%s", path, fileName, fileExtension);

	// check if file already exsists
	if(!flagOverwrite) {

		while(renameRetries < renameRetriesMax) {
		
			FILE* tryFile = fopen(fileFullPath, "r");
		
			if(tryFile == NULL) {
				fclose(tryFile);
				break;
			}
			fclose(tryFile);

			renameRetries++;
			snprintf(fileFullPath, PATH_MAX, "%s\\%s_%i.%s", path, fileName, renameRetries, fileExtension);
		}
	}
	
	// check if the file is encoded
	bool fileEncodedBase64 = false;

	if(encoded != NULL) {
		if(!strcmp(encoded, "base64")) {
			fileEncodedBase64 = true;
		}
	}
	
	//	decode and write the file
	if(fileEncodedBase64) {
		
		FILE* writeBinFile = fopen(fileFullPath, "wb");
			if(writeBinFile == NULL) {
				printf("!! Unable to save a %s\n", fileFullPath);
				return false;
			}
			
		size_t declen;
		unsigned char* decoded = base64Decode(text, &declen);
		
			if(decoded == NULL) {
				printf("!! Memory allocation error (base64 block)\n");
				exit(5);
			}

			fwrite(decoded, sizeof(char), declen, writeBinFile);

		fclose(writeBinFile);
		free(decoded);

		if(!flagNonverbose) printf(" --> %s.%s,\t%i bytes\n", fileName, fileExtension, declen);
		
	} else {

		FILE* writeTextFile = fopen(fileFullPath, "w");
			if(writeTextFile == NULL) {
				printf("!! Unable to save a %s\n", fileFullPath);
				return false;
			} 


		if(flagRecreateText) {
			
			char* textPage = jsonRecreateText(text);
			
				if(textPage == NULL) {
					printf("!! Memory allocation error (json text restoration block)\n");
					exit(5);
				}
			
			fputs(textPage, writeTextFile);
			
			free(textPage);
		} else {
			fputs(text, writeTextFile);
		}
	
		fclose(writeTextFile);

		if(!flagNonverbose) printf(" --> %s.%s,\t%i bytes\n", fileName, fileExtension, strlen(text));
	}
	
	free(fileName);
	return true;
}


char* jsonRecreateText(char* jsonVal) {
	
	size_t jsonValLen = strlen(jsonVal);
	char* recreated = (char*)malloc(jsonValLen + 1);
		memset(recreated, 0, jsonValLen);
		
	size_t ti = 0;
	for(size_t i = 0; i < jsonValLen; ti++, i++) {
		
		
		if(jsonVal[i] == '\\') {
			
			const char nextChar = jsonVal[i + 1];

			if(nextChar == '\"' || nextChar == '\\') {
				
				i++;
				
			} else if (jsonVal[i + 1] == 'n') {

				i++;
				recreated[ti] = '\n';
				continue;
				
			} else if (jsonVal[i + 1] == 't') {
				
				i++;
				recreated[ti] = '\t';
				continue;
				
			} else if (jsonVal[i + 1] == 'r') {
				
				i++;
				recreated[ti] = '\r';
				continue;
				
			}
		}
		
		recreated[ti] = jsonVal[i];
	}
	
	return recreated;
}

char** listPush(char** list, size_t* itemsCount, const char* newItem) {
	
	const size_t itemLen = strlen(newItem);
	const size_t itemSize = ((itemLen + 1) * sizeof(char));
	
	if(*itemsCount > 0 && list != NULL) {
		
		const size_t oldItems = *itemsCount;
		*itemsCount += 1;

		char** newList = (char**)realloc(list, *itemsCount * sizeof(char*));
		
			for(int i = oldItems; i < *itemsCount; i++) {
				
				newList[i] = (char*)malloc(itemSize);
				newList[i][itemLen] = '\0';
			}
		
		strcpy(newList[*itemsCount - 1], newItem);

		return newList;
	}
	else {
		
		*itemsCount = 1;
		
		char** newList = (char**)malloc(*itemsCount * sizeof(char*));
			newList[0] = (char*)malloc(itemSize);
			newList[0][itemLen] = '\0';
		
		strcpy(newList[0], newItem);
		
		return newList;
	}
}
