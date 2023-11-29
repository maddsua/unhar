//  2022 maddsua | https://gitlab.com/maddsua
//	No warranties are given, etc...
// This file is a component of the HAR Extractor


//	this bs can be found in sdkddkver.h
//	and yes, I use apis that require win vista and newer. goodby xp. rest in peace
#define NTDDI_VERSION	NTDDI_VISTA
#define _WIN32_WINNT	_WIN32_WINNT_VISTA

#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"uuid.lib")


#include <windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <time.h>
#include <shlobj.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>

#include <string>
#include <codecvt>
#include <locale>

using convert_t = std::codecvt_utf8<wchar_t>;
std::wstring_convert<convert_t, wchar_t> strconverter;

#include "inc/staticConfig.hpp"
#include "inc/staticData.hpp"

#include "inc/binbase64.hpp"

#include "harextract_private.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
HINSTANCE appInstance;

std::string openFileAtStartup;
std::mutex vectorlock;

bool openFileDialogWin(wchar_t* openPath, bool openDir);
void trimFilePathW(wchar_t* filename, const wchar_t* filepath);

bool loadFileW(const wchar_t* path, std::vector <std::string>* jsonLines);
unsigned int s2pidJSON(std::vector <std::string>* jsonLines, std::vector <response>* mediaFiles);

void makefile(struct response downFile, std::wstring dirName, bool* success);
std::wstring renameIfConflicts(const std::wstring filename);

void updateAcceptList(unsigned int index, std::vector <std::string>* list);
unsigned int removeUnaccepted(std::vector <response>* mediaFiles, std::vector <std::string> accept);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
	
	//	"parse" start arguments
	if(MAX_PATH > strlen(lpCmdLine) > 0 && strstr(lpCmdLine, ":\\") != NULL){
		
		openFileAtStartup = lpCmdLine;
	}
	
	WNDCLASSEX wc;
	HWND hwnd;
	MSG msg;

	memset(&wc,0,sizeof(wc));
	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.lpfnWndProc	 = WndProc;
	wc.hInstance	 = hInstance;
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszClassName = "WindowClass";
	wc.hIcon		 = LoadIcon(hInstance, "APPICON");
	wc.hIconSm		 = LoadIcon(hInstance, "APPICON");

	if(!RegisterClassEx(&wc)) {
		MessageBox(NULL, "Window Registration Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}
	
	//	export instance to global space to use in WM_CREATE
	appInstance = hInstance;

	//	calc window position (1/8 left; 1/10 top)
    int winPosx = (GetSystemMetrics(SM_CXSCREEN) / 2) - (windowSizeX);
    int winPosy = (GetSystemMetrics(SM_CYSCREEN) / 2) - (windowSizeY / 1.2);

	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "WindowClass",".har Extractor", WS_VISIBLE | WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
							winPosx, winPosy, windowSizeX, windowSizeY, NULL,NULL,hInstance,NULL);

	if(hwnd == NULL) {
		MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	
	while(GetMessage(&msg, NULL, 0, 0) > 0){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam){
	
	static std::vector <std::string> harContents;
	static std::vector <response> parsedFiles;
	static std::vector <std::string> accept;	
	static std::vector <std::string> partmedia;
	
	static wchar_t inputFile[MAX_PATH] = {0};
	static wchar_t inputFileName[stcTxtStrBuff] = {0};
	static wchar_t outputFolder[MAX_PATH] = {0};
	static wchar_t outputFolderName[stcTxtStrBuff] = {0};
	
	static HWND img_banner;
	static HWND stc_srcfile;
	static HWND stc_destfold;
	static HWND stc_proginfo;
	static HWND btn_selfile;
	static HWND btn_selfold;
	static HWND btn_start;
	static HWND btn_help;
	static HWND drop_mime;
	static HWND anim_progress;
	
	static short extractDataType = 0;
	
	switch(Message) {
		
		case WM_CREATE:{
			
		//	draw static
			//	banner
			img_banner = CreateWindow("STATIC", NULL, SS_BITMAP | WS_CHILD | WS_VISIBLE, 28, 15, 240, 50, hwnd, NULL, NULL, NULL);
				HBITMAP	bmp_banner = LoadBitmap(appInstance, MAKEINTRESOURCE(IMG_BANNER));
				if(bmp_banner != NULL){
					SendMessage(img_banner, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)bmp_banner);
				}
			
			//	selected file
			stc_srcfile = CreateWindowW(L"STATIC", L"No file Selected", WS_VISIBLE | WS_CHILD | WS_BORDER | SS_LEFT | SS_NOTIFY, 28, 98, 186, 18, hwnd, (HMENU)GUIID_SRCFILE, NULL, NULL);
			
			//	output folder
			stc_destfold = CreateWindowW(L"STATIC", L"No output directory", WS_VISIBLE | WS_CHILD | WS_BORDER | SS_LEFT | SS_NOTIFY, 28, 145, 186, 18, hwnd, (HMENU)GUIID_DESTDIR, NULL, NULL);
			
			stc_proginfo = CreateWindow("STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_LEFT | SS_SIMPLE, 28, 230, 186, 18, hwnd, (HMENU)GUIID_PROGINFO, NULL, NULL);
			
			//	labels
			CreateWindow("STATIC", "HAR file:", WS_VISIBLE | WS_CHILD | SS_SIMPLE, 18, 80, 130, 16, hwnd, (HMENU)GUIID_LABEL_0, NULL, NULL);
			CreateWindow("STATIC", "Save to:", WS_VISIBLE | WS_CHILD | SS_SIMPLE, 18, 125, 130, 16, hwnd, (HMENU)GUIID_LABEL_1, NULL, NULL);
			CreateWindow("STATIC", "Extract:", WS_VISIBLE | WS_CHILD | SS_SIMPLE, 18, 170, 130, 16, hwnd, (HMENU)GUIID_LABEL_2, NULL, NULL);
			
		//	draw buttons
			//	select file
			btn_selfile = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | BS_BITMAP, 224, 96, 24, 24, hwnd, (HMENU)GUIID_SELFILE, NULL, NULL);
				HBITMAP	bmp_file = LoadBitmap(appInstance, MAKEINTRESOURCE(IMG_FILE));
				if(bmp_file != NULL){
					SendMessage(btn_selfile, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)bmp_file);
				}
				
			//	select dir
			btn_selfold = CreateWindow("BUTTON", NULL, WS_VISIBLE | WS_CHILD | BS_BITMAP, 224, 140, 24, 24, hwnd, (HMENU)GUIID_SELDIR, NULL, NULL);
				HBITMAP	bmp_dir = LoadBitmap(appInstance, MAKEINTRESOURCE(IMG_FOLDER));
				if(bmp_dir != NULL){
					SendMessage(btn_selfold, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)bmp_dir);
				}
				
			//	start button
			btn_start = CreateWindow("BUTTON", "Extract", WS_VISIBLE | WS_CHILD, 168, 187, 80, 25, hwnd, (HMENU)GUIID_BTNRUN, NULL, NULL);
			
			//	help button
			btn_help = CreateWindow("STATIC", NULL, SS_BITMAP | WS_CHILD | WS_VISIBLE | SS_NOTIFY, 240, 16, 24, 24, hwnd, (HMENU)GUIID_BTNHELP, NULL, NULL);
				HBITMAP	bmp_help = LoadBitmap(appInstance, MAKEINTRESOURCE(IMG_HELP));
				if(bmp_help != NULL){
					SendMessage(btn_help, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)bmp_help);
				}
				
		//	draw dropboxes
			drop_mime = CreateWindow(WC_COMBOBOX, NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_SIMPLE | WS_VSCROLL, 28, 190, 130, 200, hwnd, (HMENU)GUIID_SELMIME, NULL, NULL);
				for(int i = 0; i < datatypes.size(); i++){
					
					SendMessage(drop_mime, CB_ADDSTRING, 0, (LPARAM) datatypes[i].c_str());
				}
				SendMessage(drop_mime, CB_SETCURSEL, extractDataType, (LPARAM)NULL);
			
			
		//	draw progress bar	
			//	progress bar
			anim_progress = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 28, 250, 220, 16, hwnd, NULL, NULL, NULL);   
			
			
			
		//	settings	
			//	set fonts
			for(int i = GUIID_SRCFILE; i <= GUIID_PROGINFO; i++){
				SendDlgItemMessage(hwnd, i, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(TRUE,0));
			}
			
			//	disable controls
			EnableWindow(GetDlgItem(hwnd, GUIID_BTNRUN), false);
			
			//	openfile at startup
			if(openFileAtStartup.size() > 0 && openFileAtStartup.size() < MAX_PATH){
				
				mbstowcs(inputFile, openFileAtStartup.c_str() + 1, openFileAtStartup.size() - 2);
			//	strncpy(inputFile, openFileAtStartup.c_str(), MAX_PATH - 1);
			
				trimFilePathW(inputFileName, inputFile);
				SetWindowTextW(stc_srcfile, inputFileName);
			}
			
			break;
		}
		
		case WM_COMMAND:{
			
			switch(HIWORD(wParam)){
				
				//	droplist
				case LBN_SELCHANGE:{
					
					switch(LOWORD(wParam)){
						
						//	mime types
						case GUIID_SELMIME:{
							
							extractDataType = (int) SendMessage(drop_mime, CB_GETCURSEL, 0, 0);
							
							updateAcceptList(extractDataType, &accept);
							break;
						}
					}
					
					break;
				}
				
				//	buttons
				case BN_CLICKED:{
					
					switch(LOWORD(wParam)){
						
						case GUIID_SELFILE:{
							
							if(openFileDialogWin(inputFile, false)){
								
								//	display file name
								trimFilePathW(inputFileName, inputFile);
								SetWindowTextW(stc_srcfile, inputFileName);
							}
														
							//	enable start button
							if(wcslen(inputFile) > 0 && wcslen(outputFolder) > 0){
								EnableWindow(GetDlgItem(hwnd, GUIID_BTNRUN), true);
							}
							
							break;
						}
						
						case GUIID_SELDIR:{
							
							if(openFileDialogWin(outputFolder, true)){
								
								//	display directory path
								trimFilePathW(outputFolderName, outputFolder);
								SetWindowTextW(stc_destfold, outputFolderName);
							}
							
							//	enable start button
							if(wcslen(inputFile) > 0 && wcslen(outputFolder) > 0){
								EnableWindow(GetDlgItem(hwnd, GUIID_BTNRUN), true);
							}
							
							break;
						}
						
						case GUIID_BTNRUN:{
							
							//	switch interface
							for(int i = GUIID_SRCFILE; i <= GUIID_BTNRUN; i++){
								EnableWindow(GetDlgItem(hwnd, i), false);
							}
							
							ShowWindow(anim_progress, true);
							ShowWindow(stc_proginfo, true);
							
							SetWindowText(stc_proginfo, "Reading JSON contents...");
							
							//	load file contents
							if(loadFileW(inputFile, &harContents)){
								
								//	parse archive
								unsigned int foundFiles = s2pidJSON(&harContents, &parsedFiles);
								
								//	remove file that we dont want to extract
								unsigned int filesToExtract = removeUnaccepted(&parsedFiles, accept);
								
								ShowWindow(stc_proginfo, false);
								
								//	set progress bar
								SendMessage(anim_progress, PBM_SETRANGE, 0, MAKELPARAM(0, filesToExtract));
								SendMessage(anim_progress, PBM_SETSTEP, 1, 0);
								
									// get number of proccessing units and use 3/4 of them
									short int thisPCthreads = std::thread::hardware_concurrency();
									short int useThreads = thisPCthreads * coreUtilPers;
									
										//	but if the cpu has less than 4 cores, use all of them 
										if(thisPCthreads < weakCPUcores){
											useThreads = thisPCthreads;
										}
	
									//	just initialize variables
									int stat_ok = 0;
									int stat_skip = 0;
									int stat_fail = 0;
									
									unsigned int processed = 0;	
									short int runThreads;
									
									char progressInfo[128];
									ShowWindow(stc_proginfo, true);
	
									//	threading loop. spawn threads in blocks
									while(processed < filesToExtract){
		
										//	adjust block size to number of files left
										unsigned int remaining = (filesToExtract - processed);
											if(remaining > useThreads){
												runThreads = useThreads;
											}
											else{
												runThreads = remaining;
											}
		
										//	create thread vector
										std::vector <ThreadItem> threadlist;
										threadlist.resize(runThreads);
		
										// spawn block of threads
										for(int i = 0; i < runThreads; i++) {
			
											threadlist[i].worker = std::thread(makefile, parsedFiles[processed], outputFolder, &threadlist[i].success);
											processed++;
										}
		
										// waits block to finish and get execution results
										for(int i = 0; i < runThreads; i++) {
			
											//	wait thread to join
											threadlist[i].worker.join();
											
											//	step progress bar
											SendMessage(anim_progress, PBM_STEPIT, 0, 0);
											
											//	update and redraw info string
											sprintf(progressInfo, "File %i of %i (%i)", processed, filesToExtract, foundFiles);
												SetWindowText(stc_proginfo, progressInfo);
											
												//	get results
												if(threadlist[i].success){
													stat_ok++;
												}
												else{
													stat_fail++;
												}
										}
									}

								//	show results
								char report[128];
									sprintf(report, "Extracted: %i files, skipped: %i, failed: %i. Of %i in archive", stat_ok, (foundFiles - filesToExtract), stat_fail, foundFiles);
									MessageBox(NULL, report,"Extraction results", MB_ICONASTERISK | MB_OK);
								
							}
							else{
								MessageBox(NULL, "har archive was not loaded", "Some problem occured", MB_ICONEXCLAMATION | MB_OK);
							}
							
							
							//	cleanup
							while(harContents.size() > 0){
								harContents.pop_back();
							}
							while(parsedFiles.size() > 0){
								parsedFiles.pop_back();
							}
								
							//	reset interface
							SendMessage(anim_progress, PBM_SETPOS, 0, 0);
								
							for(int i = GUIID_SRCFILE; i <= GUIID_BTNRUN; i++){
								EnableWindow(GetDlgItem(hwnd, i), true);
							}
							
							ShowWindow(anim_progress, false);
							ShowWindow(stc_proginfo, false);
														
							break;
						}
						
						case GUIID_BTNHELP:{
							
							char msgabout[512];
								sprintf(msgabout, "%s v%i.%i.%i\n\nA simple ulitily for unpacking\n media files from .har archives\n\n%s\n%s", FILE_DESCRIPTION, VER_MAJOR, VER_MINOR, VER_RELEASE, VER_AUTHSTAMP, LEGAL_COPYRIGHT);
							
							MessageBox(NULL, msgabout, PRODUCT_NAME, 0);
							break;
						}
					}
					
					break;
				}
			}
			
			break;
		}
		
		case WM_DESTROY: {
			PostQuitMessage(0);
			break;
		}
		
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}


void makefile(struct response downFile, std::wstring dirName, bool* success){

//	get file name
	std::wstring fileNameNoExt = strconverter.from_bytes(downFile.address);
	

//	trim some trash at the beginning of filetype string
	wchar_t trashToTrim[] = {L'?',L'#',L'.', L'\"',L','};
	unsigned int trashList = sizeof(trashToTrim)/sizeof(wchar_t);
	
	int res_trim_begin = fileNameNoExt.find_last_of(L'/');
	
		//	remove last slash if its last in string
		if(fileNameNoExt[res_trim_begin + 1] == L'\"'){
		
			fileNameNoExt.erase(res_trim_begin, fileNameNoExt.size() - 1);
			res_trim_begin = fileNameNoExt.find_last_of(L'/');
		}
		//	remove just last slash
		if(res_trim_begin != std::string::npos){
			fileNameNoExt.erase(0, res_trim_begin + 1);
		}
		//	remove first special character
		for(int i = 0; i < trashList; i++){
			
			if(fileNameNoExt[0] == trashToTrim[i]){
				fileNameNoExt.erase(0,1);
			}
		}

//	trim some trash at the end of filetype string
	unsigned int pass = 0;
	
	while(pass < trashList){
		int res_trim_end = fileNameNoExt.find_first_of(trashToTrim[pass]);
		
		if(res_trim_end == std::string::npos){
			pass++;	
		}
		else{
			fileNameNoExt.erase(res_trim_end, fileNameNoExt.size() - 1);
		}
	}
	//	check if name is too long or too short
	if(fileNameNoExt.size() > fsMaxName){	
		
		fileNameNoExt.erase(fsMaxName, fileNameNoExt.size() - 1);
	}
	else if(fileNameNoExt.size() < 1){
		fileNameNoExt = L"responce";
	}
	
	
//	get file extension

	// reset character case to lower
	for(int sym = 0; sym < downFile.mimetype.size(); sym++){
		if(downFile.mimetype[sym] > 0x40 && downFile.mimetype[sym] < 0x5B){
			downFile.mimetype[sym] += 0x20;
		}
	}
	
	//	set extension based on mime type
	std::string fileExt;
	for(int i = 0; i < fileExtens.size(); i++){
		if(downFile.mimetype.find(fileExtens[i].type) != std::string::npos){
			fileExt = fileExtens[i].extension;
			break;
		}
	}
	if(fileExt.size() == 0){
		fileExt = ".js";
	}

	
// clean up
	std::string fileContentsText = downFile.content;
	//	trim some trash at the beginning of contents string
	for(short i = 0; i < 3; i++){
		int cont_trim_begin = fileContentsText.find("\"");
			
		if(cont_trim_begin >=0 && cont_trim_begin < fileContentsText.size() -2){
			fileContentsText.erase(0, cont_trim_begin + 1);
		}
	}
	//	trim some trash at the end of contents string
	int cont_trim_end = fileContentsText.find_last_of('\"');
		if(cont_trim_end != std::string::npos){
			fileContentsText.erase(cont_trim_end, fileContentsText.size() - 1);
		}
	

//	check if a file with this name already excists
	std::wstring filePath = dirName + L"/" + fileNameNoExt + strconverter.from_bytes(fileExt);
	
	while(true){
		
		std::ifstream testIfExists;
		testIfExists.open(filePath.c_str(), std::ios::in);
			
    	if(testIfExists.is_open()){
    		
    		testIfExists.close();
    		filePath = dirName + L"/" + renameIfConflicts(fileNameNoExt + strconverter.from_bytes(fileExt));
		}
		else{
			break;
		}
	}
	
	
//	check for encoding and write down
	if(downFile.encoding.find("base64") != std::string::npos){
		
		std::vector <unsigned char> fileContsDecoded = base64_decode(fileContentsText);
		
		std::ofstream localFile(filePath.c_str(), std::ios::out | std::ios::binary | std::ios::app);
	
		if (localFile.is_open()){
			
			for(int i = 0; i < fileContsDecoded.size(); i++){
				localFile << fileContsDecoded[i];
			}

			localFile.close();
		}
		else{
    		*success = false;
			return;
		}
	}
	else{
		
		std::ofstream localFile(filePath.c_str());
	
		if(localFile.is_open()){
		
				localFile << fileContentsText;
				localFile.close();
		}
		else{
    		*success = false;
			return;
		}		
	}
	
   	*success = true;
	return;
}


std::wstring renameIfConflicts(const std::wstring filename){
	
	static std::vector <fileExists> renList;
	
	fileExists conficting;
	bool isItFirst = true;
	
	if(renList.size() > 0){
		
		for(int f = 0; f < renList.size(); f++){
			
			if(renList[f].name.find(filename) != std::string::npos){
				
				isItFirst = false;
				
				conficting.name = renList[f].name;
				
					std::lock_guard <std::mutex> guard(vectorlock);			
						renList[f].index++;
					
				conficting.index = renList[f].index;
				
				break;
			}
		}
	}
	
	if(renList.size() == 0 || isItFirst){
		
		conficting.index = 1;
		conficting.name = filename;
		
		std::lock_guard <std::mutex> guard(vectorlock);
			renList.push_back(conficting);
	}
	
	
	std::wstring newFileName = filename;
	
	int ren_dotpos = newFileName.find(L'.');
	
		if(ren_dotpos != std::string::npos){
			
			std::wstring tFileTempExt = newFileName;
				tFileTempExt.erase(0, ren_dotpos);
				
			newFileName.erase(ren_dotpos, newFileName.size() - 1);
			newFileName += L"_" + strconverter.from_bytes(std::to_string(conficting.index)) + tFileTempExt;
			
		}
		else{
			newFileName += L"_" + strconverter.from_bytes(std::to_string(conficting.index));
		}
		
	return newFileName;
}

unsigned int s2pidJSON(std::vector <std::string>* jsonLines, std::vector <response>* mediaFiles){
	
	bool parFlag[parFlagArray] = {false};	//	0 - started block, 1 - got address, 2 - got type, 3 - got encoding, 4 - got content
	unsigned int blockReadFail = 0;
	
	response fileCandidate;
	
	for(int line = 0; line < jsonLines->size(); line++){
		
//	start parsing request/response block
		if(jsonLines->at(line).find("\"request\":") != std::string::npos){
			
			//	skip this block as failed if json have some rough time with sequence
			if(parFlag[pf_runn]){
				blockReadFail++;
			}
			
			//	initialize read of block
			parFlag[pf_runn] = true;
			
			// clear file candidate structure
			fileCandidate = {};
		}
		
//	deal with this block contents
		if(parFlag[pf_runn]){
			
			//	get request url
			if(jsonLines->at(line).find("\"url\":") != std::string::npos){
				fileCandidate.address = jsonLines->at(line);
				parFlag[pf_addr] = true;
			}
			
			//	get response mime type
			if(jsonLines->at(line).find("\"mimeType\":") != std::string::npos){
				fileCandidate.mimetype = jsonLines->at(line);
				parFlag[pf_type] = true;
			}
			
			//	get response encoding
			if(jsonLines->at(line).find("\"encoding\":") != std::string::npos){
				fileCandidate.encoding = jsonLines->at(line);
				parFlag[pf_encd] = true;
			}
			
			//	get response contents
			if(jsonLines->at(line).find("\"text\":") != std::string::npos){
				fileCandidate.content = jsonLines->at(line);
				parFlag[pf_cont] = true;
			}
			
//	export this structure to file vector if url, mime type and contents are read. encoding does not interest us
			if(parFlag[pf_addr] && parFlag[pf_type] && parFlag[pf_cont]){
				
				mediaFiles->push_back(fileCandidate);
		
				//	clear parsing flags
				for(short i = 0; i < parFlagArray; i++){
					parFlag[i] = false;
				}
			}
		}	
	}
	
	return mediaFiles->size();
}

bool loadFileW(const wchar_t* path, std::vector <std::string>* jsonLines){
	
	std::ifstream openHAR;
    openHAR.open(path);

    if(openHAR.is_open()){
    			
    	while (!openHAR.eof()){
    		
    		std::string templine;
   				getline(openHAR, templine);
   				jsonLines->push_back(templine);
		}
		
		openHAR.close();
	}
	else{
		return false;
	}
	
	if(jsonLines->size() < jsonMinLen){
		return false;
	}
	
	return true;
}


void updateAcceptList(unsigned int index, std::vector <std::string>* list){

	//	clear accept list
	while(list->size() > 0){
		list->pop_back();
	}

	//	assign new
	switch(index){
		
		case datypeID_Text:{
				
			list->push_back("\"text/");	
			break;
		}
		case datypeID_Media:{
			
			list->push_back("\"image/");
			list->push_back("\"video/");
			list->push_back("\"audio/");
			list->push_back("vnd.apple.mpegurl\"");	//	m3u8 list files
			break;
		}
		case datypeID_TS:{
			
			list->push_back("\"video/mp2t");
			list->push_back("vnd.apple.mpegurl\"");	//	m3u8 list files
			break;
		}
		case datypeID_Video:{
			
			list->push_back("\"video/");
			list->push_back("vnd.apple.mpegurl\"");	//	m3u8 list files
			break;
			}
			
		case datypeID_Audio:{
			
			list->push_back("\"audio/");
			break;
		}
		case datypeID_Img:{
			
			list->push_back("\"image/");
			break;
		}
		case datypeID_Font:{
			
			list->push_back("\"font/");
			break;
		}
		case datypeID_App:{
				
			list->push_back("\"text/");
			list->push_back("\"application/");
			break;
		}
	}
	
	return;					
}

/*
bool openfolder(char* dirpath, char* dirname){
	
	char tempPath[MAX_PATH];
	
	BROWSEINFO bi = {0};
    bi.lpszTitle  = ("Browse for folder...");
    bi.ulFlags    = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    //bi.lpfn       = BrowseCallbackProc;
    //bi.lParam     = (LPARAM) path_param;
    
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    
	if(pidl != 0){
		SHGetPathFromIDList(pidl, tempPath);
	}
	else{
		return false;
	}
	
	if(strlen(tempPath) > 0){
		
		strcpy(dirpath, tempPath);
		trimFilePath(dirname, tempPath);
	
		return true;
	}
	
	return false;	
}
*/
/*
bool openfile(char* filepath, char* filename){
	
	char file[MAX_PATH] = {0};
	OPENFILENAME ofn = {0}; 

		ofn.lStructSize = sizeof(ofn); 
		ofn.hwndOwner = NULL; 
		ofn.lpstrFile = file; 
		ofn.nMaxFile = MAX_PATH; 
		ofn.lpstrFilter = ("HAR file\0*.har\0JSON file\0*.json\0All Files\0*.*\0\0"); 
		ofn.nFilterIndex = 1; 
		ofn.lpstrFileTitle = NULL; 
		ofn.nMaxFileTitle = 0; 
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		
	if(GetOpenFileName(&ofn)){

		strcpy(filepath, file);
		trimFilePath(filename, filepath);
		
		return true;
	}

	return false;
}
*/
/*
void trimFilePath(char* filename, const char* filepath){
	
	unsigned int pathLen = strlen(filepath);
	
	if(pathLen >= stcTxtStrBuff){
		
		strcpy(filename, " ...");
		
		unsigned int pathPosShift = pathLen - (stcTxtStrBuff - strlen(filename)) + 1;			
		strcat(filename, filepath + pathPosShift);
	}
	else{
		strcpy(filename, filepath);
	}
	
	return;
}
*/
void trimFilePathW(wchar_t* filename, const wchar_t* filepath){
	
	unsigned int pathLen = wcslen(filepath);
	
	if(pathLen >= stcTxtStrBuff){
		
		wcscpy(filename, L" ...");
		
		unsigned int pathPosShift = pathLen - (stcTxtStrBuff - wcslen(filename)) + 1;			
		wcscat(filename, filepath + pathPosShift);
	}
	else{
		wcscat(filename, filepath);
	}
	
	return;
}

unsigned int removeUnaccepted(std::vector <response>* mediaFiles, std::vector <std::string> accept){
	
	if(accept.size() > 0){
		
		unsigned int fl = 0;
		while(fl < mediaFiles->size()){
			
			bool remove = true;
			
			//	remove if file type is not wanted
			for(int ls = 0; ls < accept.size(); ls++){

				if(mediaFiles->at(fl).mimetype.find(accept[ls]) != std::string::npos){

					remove = false;
					break;
				}
			}
			
			//	remove if file contents is empty
			if(mediaFiles->at(fl).content.size() < netEmptyResp){
				remove = true;
			}
			
			if(remove){
				mediaFiles->erase(mediaFiles->begin() + fl, mediaFiles->begin() + fl + 1);
			}
			else{
				fl++;
			}
		}
	}
	
	return mediaFiles->size();
}

bool openFileDialogWin(wchar_t* openPath, bool openDir){
	
	wchar_t pathCandidate[MAX_PATH];
	
//	this part is copiend from internet and let's assume that it just works
	
	HRESULT opDialRslt = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    
	if(SUCCEEDED(opDialRslt)){
    	
		IFileOpenDialog* pFileOpen;
        
		// Create the FileOpenDialog object.
		opDialRslt = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if(SUCCEEDED(opDialRslt)){
			
			// do we look for dir?
			DWORD dwOptions;
			if(SUCCEEDED(pFileOpen->GetOptions(&dwOptions)) && openDir){
				
				pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);
			}
			else{
				
				const int fileFilters = 3;
				COMDLG_FILTERSPEC fFilter[fileFilters] = {{L"HAR file", L"*.har"},{L"JSON file", L"*.json"}, {L"All Files",L"*.*"}};
				pFileOpen->SetFileTypes(fileFilters, fFilter);
			}
			
			// Show the Open dialog box.
			opDialRslt = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
			if(SUCCEEDED(opDialRslt)){
            	
				IShellItem* pItem;
				opDialRslt = pFileOpen->GetResult(&pItem);
                
				if(SUCCEEDED(opDialRslt)){
                	
					wchar_t* pTemp;

					opDialRslt = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pTemp);

					wcscpy_s(pathCandidate, MAX_PATH, pTemp);

					if(SUCCEEDED(opDialRslt))
						CoTaskMemFree(pTemp);
					
					pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
    
//	my dumb part
    if(wcslen(pathCandidate) > 3 && (wcschr(pathCandidate, L'\\') != NULL || wcschr(pathCandidate, L'/') != NULL)){
    	
    	wcscpy_s(openPath, MAX_PATH, pathCandidate);
    	return true;
	}

    return false;
}
