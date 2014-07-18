#include "lib.h"

// Private Prototypes
void DisplayHelp(char *app_name);
void SetDefaults(user_settings *set);
int SetArgument(int argc, int i, char *argv[], user_settings *set);
int CheckArgumentCombination(user_settings *set);
void PrintNeedsArg(char *arg);
void PrintArgInvalid(char *arg);
void PrintArgReqParam(char *arg, u32 paramNum);
void PrintNoNeedParam(char *arg);

int ParseArgs(int argc, char *argv[], user_settings *usr_settings)
{
	if(argv == NULL || usr_settings == NULL)
		return USR_PTR_PASS_FAIL;
		
	if(argc < 2){
		DisplayHelp(argv[0]);
		return USR_HELP;
	}
		
	// Detecting Help Requried
	for(int i = 1; i < argc; i++){
		if(strcmp(argv[i],"-help") == 0){
			DisplayHelp(argv[0]);
			return USR_HELP;
		}
	}
	
	// Allocating Memory for Content Path Ptrs
	usr_settings->common.contentPath = calloc(CIA_MAX_CONTENT,sizeof(char*));
	if(usr_settings->common.contentPath == NULL){
		fprintf(stderr,"[SETTING ERROR] Not Enough Memory\n");
		return USR_MEM_ERROR;
	}
	
	// Initialise Keys
	InitKeys(&usr_settings->common.keys);

	// Setting Defaults
	SetDefaults(usr_settings);
	
	// Parsing Arguments
#ifdef DEBUG
	fprintf(stdout,"[DEBUG] Parsing Args\n");
#endif
	int set_result;
	int i = 1;
	while(i < argc){
		set_result = SetArgument(argc,i,argv,usr_settings);
		if(set_result < 1){
			fprintf(stderr,"[RESULT] Invalid arguments, see '%s -help'\n",argv[0]);
			return set_result;
		}
		i += set_result;
	}
	
	// Checking arguments
#ifdef DEBUG
	fprintf(stdout,"[DEBUG] Checking Args\n");
#endif
	set_result = CheckArgumentCombination(usr_settings);
	if(set_result) return set_result;
	
	// Setting Keys
#ifdef DEBUG
	fprintf(stdout,"[DEBUG] Setting Keys\n");
#endif
	set_result = SetKeys(&usr_settings->common.keys);
	if(set_result) return set_result;

#ifdef DEBUG
	fprintf(stdout,"[DEBUG] Generating output path name if required\n");
#endif

	if(!usr_settings->common.outFileName){
		char *source_path = NULL;
		if(usr_settings->ncch.buildNcch0) 
			source_path = usr_settings->common.rsfPath;
		else if(usr_settings->common.workingFileType == infile_ncsd || usr_settings->common.workingFileType == infile_srl) 
			source_path = usr_settings->common.workingFilePath;
		else 
			source_path = usr_settings->common.contentPath[0];
		u16 outfile_len = strlen(source_path) + 3;
		usr_settings->common.outFileName = calloc(outfile_len,sizeof(char));
		if(!usr_settings->common.outFileName){
			fprintf(stderr,"[SETTING ERROR] Not Enough Memory\n");
			return USR_MEM_ERROR;
		}
		usr_settings->common.outFileName_mallocd = true;
		append_filextention(usr_settings->common.outFileName,outfile_len,source_path,(char*)&output_extention[usr_settings->common.outFormat-1]);
	}
	return 0;
}

void SetDefaults(user_settings *set)
{
	// Target Info
	set->common.keys.keyset = pki_TEST;
	set->common.keys.accessDescSign.presetType = desc_preset_NONE;

	// Build NCCH Info
	set->ncch.buildNcch0 = false;
	set->ncch.includeExefsLogo = false;
	set->common.outFormat = NCCH;
	set->ncch.ncchType = format_not_set;

	// RSF Settings
	set->common.rsfSet.Option.EnableCompress = true;
	set->common.rsfSet.Option.EnableCrypt = true;
	set->common.rsfSet.Option.UseOnSD = false;
	set->common.rsfSet.Option.FreeProductCode = false;

	// Working File Info
	set->common.workingFileType = infile_ncch;

	// CCI Info
	set->cci.useSDKStockData = false;

	// CIA Info
	set->cia.useDataTitleVer = false;
	set->cia.titleVersion[VER_MAJOR] = MAX_U16; // invalid so changes can be detected
	set->cia.randomTitleKey = false;
	set->common.keys.aes.currentCommonKey = MAX_U8 + 1; // invalid so changes can be detected
	for(int i = 0; i < CIA_MAX_CONTENT; i++){
		set->cia.contentId[i] = MAX_U32 + 1; // invalid so changes can be detected
	}
}

int SetArgument(int argc, int i, char *argv[], user_settings *set)
{
	u16 ParamNum = 0;
	for(int j = i+1; j < argc && argv[j][0] != '-'; j++)
		ParamNum++;

	// Global Settings
	if(strcmp(argv[i],"-rsf") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-rsf",1);
			return USR_ARG_REQ_PARAM;
		}
		set->common.rsfPath = argv[i+1];
		return 2;
	}
	else if(strcmp(argv[i],"-f") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-f",1);
			return USR_ARG_REQ_PARAM;
		}
		if(strcasecmp(argv[i+1],"ncch") == 0) 
			set->common.outFormat = NCCH;
		else if(strcasecmp(argv[i+1],"cci") == 0)
			set->common.outFormat = CCI;
		else if(strcasecmp(argv[i+1],"cia") == 0)
			set->common.outFormat = CIA;
		else {
			fprintf(stderr,"[SETTING ERROR] Invalid output format '%s'\n",argv[i+1]);
			return USR_BAD_ARG;
		}		
		return 2;
	}
	else if(strcmp(argv[i],"-o") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-o",1);
			return USR_ARG_REQ_PARAM;
		}
		set->common.outFileName = argv[i+1];
		set->common.outFileName_mallocd = false;
		return 2;
	}
	// Key Options
	else if(strcmp(argv[i],"-target") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-target",1);
			return USR_ARG_REQ_PARAM;
		}
		if(strcasecmp(argv[i+1],"test") == 0 || strcasecmp(argv[i+1],"t") == 0)
			set->common.keys.keyset = pki_TEST;
		else if(strcasecmp(argv[i+1],"debug") == 0 || strcasecmp(argv[i+1],"development") == 0 || strcasecmp(argv[i+1],"d") == 0)
			set->common.keys.keyset = pki_DEVELOPMENT;
		else if(strcasecmp(argv[i+1],"retail") == 0 || strcasecmp(argv[i+1],"production") == 0 || strcasecmp(argv[i+1],"p") == 0)
			set->common.keys.keyset = pki_PRODUCTION;
		//else if(strcasecmp(argv[i+1],"custom") == 0 || strcasecmp(argv[i+1],"c") == 0)
		//	set->common.keys.keyset = pki_CUSTOM;
		else{
			fprintf(stderr,"[SETTING ERROR] Unrecognised target '%s'\n",argv[i+1]);
			return USR_BAD_ARG;
		}
		return 2;
	}
	else if(strcmp(argv[i],"-ckeyID") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-ckeyID",1);
			return USR_ARG_REQ_PARAM;
		}
		set->common.keys.aes.currentCommonKey = strtol(argv[i+1],NULL,0);
		if(set->common.keys.aes.currentCommonKey > 0xff)
		{
			fprintf(stderr,"[SETTING ERROR] Invalid Common Key ID: 0x%x\n",set->common.keys.aes.currentCommonKey);
			return USR_BAD_ARG;
		}
		return 2;
	}
	else if(strcmp(argv[i],"-showkeys") == 0){
		if(ParamNum){
			PrintNoNeedParam("-showkeys");
			return USR_BAD_ARG;
		}
		set->common.keys.dumpkeys = true;
		return 1;
	}
	else if(strcmp(argv[i],"-fsign") == 0){
		if(ParamNum){
			PrintNoNeedParam("-fsign");
			return USR_BAD_ARG;
		}
		set->common.keys.rsa.isFalseSign = true;
		return 1;
	}

	// Ncch Options
	else if(strcmp(argv[i],"-elf") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-elf",1);
			return USR_ARG_REQ_PARAM;
		}
		set->ncch.elfPath = argv[i+1];
		set->ncch.ncchType |= CXI;
		return 2;
	}
	
	else if(strcmp(argv[i],"-icon") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-icon",1);
			return USR_ARG_REQ_PARAM;
		}
		set->ncch.iconPath = argv[i+1];
		set->ncch.ncchType |= CFA;
		return 2;
	}
	else if(strcmp(argv[i],"-banner") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-banner",1);
			return USR_ARG_REQ_PARAM;
		}
		set->ncch.bannerPath = argv[i+1];
		set->ncch.ncchType |= CFA;
		return 2;
	}
	else if(strcmp(argv[i],"-logo") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-logo",1);
			return USR_ARG_REQ_PARAM;
		}
		set->ncch.logoPath = argv[i+1];
		set->ncch.ncchType |= CFA;
		return 2;
	}
	else if(strcmp(argv[i],"-desc") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-desc",1);
			return USR_ARG_REQ_PARAM;
		}
		char *tmp = argv[i+1];
		char *tmp2 = strstr(tmp,":");
		if(!tmp2){
			fprintf(stderr,"[SETTING ERROR] Bad argument '%s %s', correct format:\n",argv[i],argv[i+1]);
			fprintf(stderr,"	-desc <APP TYPE>:<TARGET FIRMWARE>\n");
		}
		if(strlen(tmp2) < 2){
			fprintf(stderr,"[SETTING ERROR] Bad argument '%s %s', correct format:\n",argv[i],argv[i+1]);
			fprintf(stderr,"	-desc <APP TYPE>:<TARGET FIRMWARE>\n");
		}

		u32 app_type_len = (u32)(tmp2-tmp);
		char *app_type = calloc(app_type_len+1,sizeof(char));
		memcpy(app_type,tmp,app_type_len);

		if(strcasecmp(app_type,"App") == 0 || strcasecmp(app_type,"SDApp") == 0) set->common.keys.accessDescSign.presetType = desc_preset_APP;
		else if(strcasecmp(app_type,"ECApp") == 0) set->common.keys.accessDescSign.presetType = desc_preset_EC_APP;
		else if(strcasecmp(app_type,"Demo") == 0) set->common.keys.accessDescSign.presetType = desc_preset_DEMO;
		else if(strcasecmp(app_type,"DlpChild") == 0 || strcasecmp(app_type,"Dlp") == 0) set->common.keys.accessDescSign.presetType = desc_preset_DLP;
		//else if(strcasecmp(app_type,"FIRM") == 0) set->common.keys.accessDescSign.presetType = desc_preset_FIRM;
		else{
			fprintf(stderr,"[SETTING ERROR] Accessdesc AppType preset '%s' not valid, please manually configure RSF\n",app_type);
			return USR_BAD_ARG;
		}


		char *target_firmware = (tmp2+1);
		set->common.keys.accessDescSign.targetFirmware = strtoul(target_firmware,NULL,0);
		switch(set->common.keys.accessDescSign.targetFirmware){
			case 1: 
				set->common.keys.accessDescSign.targetFirmware = 0x1B; // or 0x1C
				break;
			case 2: 
				set->common.keys.accessDescSign.targetFirmware = 0x1D; // or 0x1E/0x1F
				break;
			case 3: 
				set->common.keys.accessDescSign.targetFirmware = 0x20;
				break;
			case 4: 
				set->common.keys.accessDescSign.targetFirmware = 0x21; // or 0x22
				break;
			case 5: 
				set->common.keys.accessDescSign.targetFirmware = 0x23; // or 0x24
				break;
			case 6: 
				set->common.keys.accessDescSign.targetFirmware = 0x25; // or 0x26
				break;
			case 7: 
				set->common.keys.accessDescSign.targetFirmware = 0x27; // or 0x28
				break;
			case 8: 
				set->common.keys.accessDescSign.targetFirmware = 0x2C;
				break;
			default:
				break;
		}
		
		set->ncch.ncchType |= CXI;
		return 2;
	}
	else if(strcmp(argv[i],"-exefslogo") == 0){
		if(ParamNum){
			PrintNoNeedParam("-exefslogo");
			return USR_BAD_ARG;
		}
		set->ncch.includeExefsLogo = true;
		set->ncch.ncchType |= CFA;
		return 1;
	}

	// Ncch Rebuild Options
	else if(strcmp(argv[i],"-code") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-code",1);
			return USR_ARG_REQ_PARAM;
		}
		set->ncch.codePath = argv[i+1];
		set->ncch.ncchType |= CXI;
		return 2;
	}
	else if(strcmp(argv[i],"-exheader") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-exheader",1);
			return USR_ARG_REQ_PARAM;
		}
		set->ncch.exheaderPath = argv[i+1];
		set->ncch.ncchType |= CXI;
		return 2;
	}
	else if(strcmp(argv[i],"-plain-region") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-plain-region",1);
			return USR_ARG_REQ_PARAM;
		}
		set->ncch.plainRegionPath = argv[i+1];
		set->ncch.ncchType |= CXI;
		return 2;
	}
	else if(strcmp(argv[i],"-romfs") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-romfs",1);
			return USR_ARG_REQ_PARAM;
		}
		set->ncch.romfsPath = argv[i+1];
		set->ncch.ncchType |= CFA;
		return 2;
	}
	// Cci Options
	else if(strcmp(argv[i],"-devcardcci") == 0){
		if(ParamNum){
			PrintNoNeedParam("-devcardcci");
			return USR_BAD_ARG;
		}
		set->cci.useSDKStockData = true;
		return 1;
	}
	else if(strcmp(argv[i],"-nomodtid") == 0){
		if(ParamNum){
			PrintNoNeedParam("-nomodtid");
			return USR_BAD_ARG;
		}
		set->cci.dontModifyNcchTitleID = true;
		return 1;
	}
	else if(strcmp(argv[i],"-alignwr") == 0){
		if(ParamNum){
			PrintNoNeedParam("-alignwr");
			return USR_BAD_ARG;
		}
		set->cci.closeAlignWritableRegion = true;
		return 1;
	}
	else if(strcmp(argv[i],"-genupdatenote") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-genupdatenote",1);
			return USR_BAD_ARG;
		}
		set->cci.cverCiaPath = argv[i+1];
		return 2;
	}

	// Cia Options
	else if(strcmp(argv[i],"-major") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-major",1);
			return USR_ARG_REQ_PARAM;
		}
		set->cia.useNormTitleVer = true;
		u32 ver = strtoul(argv[i+1],NULL,10);
		if(ver > VER_MAJOR_MAX){
			fprintf(stderr,"[SETTING ERROR] Major version: '%d' is too large, max: '%d'\n",ver,VER_MAJOR_MAX);
			return USR_BAD_ARG;
		}
		set->cia.titleVersion[VER_MAJOR] = ver;
		return 2;
	}
	else if(strcmp(argv[i],"-minor") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-minor",1);
			return USR_ARG_REQ_PARAM;
		}
		set->cia.useNormTitleVer = true;
		u32 ver = strtoul(argv[i+1],NULL,10);
		if(ver > VER_MINOR_MAX){
			fprintf(stderr,"[SETTING ERROR] Minor version: '%d' is too large, max: '%d'\n",ver,VER_MINOR_MAX);
			return USR_BAD_ARG;
		}
		set->cia.titleVersion[VER_MINOR] = ver;
		return 2;
	}
	else if(strcmp(argv[i],"-micro") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-micro",1);
			return USR_ARG_REQ_PARAM;
		}
		u32 ver = strtoul(argv[i+1],NULL,10);
		if(ver > VER_MICRO_MAX){
			fprintf(stderr,"[SETTING ERROR] Micro version: '%d' is too large, max: '%d'\n",ver,VER_MICRO_MAX);
			return USR_BAD_ARG;
		}
		set->cia.titleVersion[VER_MICRO] = ver;
		return 2;
	}
	else if(strcmp(argv[i],"-dver") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-dver",1);
			return USR_ARG_REQ_PARAM;
		}
		set->cia.useDataTitleVer = true;
		u32 ver = strtoul(argv[i+1],NULL,10);
		if(ver > VER_DVER_MAX){
			fprintf(stderr,"[SETTING ERROR] Data version: '%d' is too large, max: '%d'\n",ver,VER_DVER_MAX);
			return USR_BAD_ARG;
		}
		set->cia.titleVersion[VER_MAJOR] = (ver >> 6) & VER_MAJOR_MAX;
		set->cia.titleVersion[VER_MINOR] = ver & VER_MAJOR_MAX;
		return 2;
	}
	else if(strcmp(argv[i],"-rand") == 0){
		if(ParamNum){
			PrintNoNeedParam("-rand");
			return USR_BAD_ARG;
		}
		set->cia.randomTitleKey = true;
		return 1;
	}
	else if(strcmp(argv[i],"-cci") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-cci",1);
			return USR_ARG_REQ_PARAM;
		}
		set->ncch.buildNcch0 = false;
		set->common.workingFileType = infile_ncsd;
		set->common.workingFilePath = argv[i+1];
		set->common.outFormat = CIA;
		return 2;
	}
	else if(strcmp(argv[i],"-srl") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-srl",1);
			return USR_ARG_REQ_PARAM;
		}
		set->ncch.buildNcch0 = false;
		set->common.workingFileType = infile_srl;
		set->common.workingFilePath = argv[i+1];
		set->common.outFormat = CIA;
		return 2;

	}
	else if(strcmp(argv[i],"-dlc") == 0){
		if(ParamNum){
			PrintNoNeedParam("-dlc");
			return USR_BAD_ARG;
		}
		set->cia.DlcContent = true;
		return 1;
	}

	// Other Setting
	else if(strcmp(argv[i],"-content") == 0){
		if(ParamNum != 1){
			PrintArgReqParam("-content",1);
			return USR_ARG_REQ_PARAM;
		}
		char *pos = strstr(argv[i+1],":");
		if(!pos){
			fprintf(stderr,"[SETTING ERROR] Bad argument '%s %s', correct format:\n",argv[i],argv[i+1]);
			fprintf(stderr,"	-content <CONTENT PATH>:<INDEX>\n");
			fprintf(stderr,"  If generating a CIA, then use the format:\n");
			fprintf(stderr,"	-content <CONTENT PATH>:<INDEX>:<ID>\n");
			return USR_BAD_ARG;
		}		
		if(strlen(pos) < 2){
			fprintf(stderr,"[SETTING ERROR] Bad argument '%s %s', correct format:\n",argv[i],argv[i+1]);
			fprintf(stderr,"	-content <CONTENT PATH>:<INDEX>\n");
			fprintf(stderr,"  If generating a CIA, then use the format:\n");
			fprintf(stderr,"	-content <CONTENT PATH>:<INDEX>:<ID>\n");
			return USR_BAD_ARG;
		}

		/* Getting Content Index */
		u16 content_index = strtol((char*)(pos+1),NULL,0);

		/* Storing Content Filepath */
		u32 path_len = (u32)(pos-argv[i+1])+1;
		
		if(content_index == 0) set->ncch.buildNcch0 = false;
		if(set->common.contentPath[content_index] != NULL){
			fprintf(stderr,"[SETTING ERROR] Content %d is already specified\n",content_index);
			return USR_BAD_ARG;
		}
		set->common.contentPath[content_index] = calloc(path_len,sizeof(char));
		if(set->common.contentPath[content_index] == NULL){
			fprintf(stderr,"[SETTING ERROR] Not enough memory\n");
			return USR_MEM_ERROR;
		}
		strncpy(set->common.contentPath[content_index],argv[i+1],path_len-1);	

		/* Get ContentID for CIA gen */
		char *pos2 = strstr(pos+1,":"); 
		if(pos2) {
			set->cia.contentId[content_index] = strtoul((pos2+1),NULL,0);
		}
		
		/* Return Next Arg Pos*/
		return 2;
	}
	// RSF Value Substitution
	else if(strncmp(argv[i],"-D",2) == 0){
		if(ParamNum){
			PrintNoNeedParam("-DNAME=VALUE");
			return USR_BAD_ARG;
		}
		if(set->dname.m_items == 0){
			set->dname.m_items = 10;
			set->dname.u_items = 0;
			set->dname.items = calloc(set->dname.m_items,sizeof(dname_item));
			if(!set->dname.items){
				fprintf(stderr,"[SETTING ERROR] Not enough memory\n");
				return MEM_ERROR;
			}
			//memset(set->dname.items,0,sizeof(dname_item)*set->dname.m_items);
		}
		else if(set->dname.m_items == set->dname.u_items){
			set->dname.m_items *= 2;
			/*
			dname_item *tmp = malloc(sizeof(dname_item)*set->dname.m_items);
			if(!tmp){
				fprintf(stderr,"[SETTING ERROR] Not enough memory\n");
				return MEM_ERROR;
			}
			memset(tmp,0,sizeof(dname_item)*set->dname.m_items);
			memcpy(tmp,set->dname.items,sizeof(dname_item)*set->dname.u_items);
			free(set->dname.items);
			set->dname.items = tmp;
			*/
			set->dname.items = realloc(set->dname.items,sizeof(dname_item)*set->dname.m_items);
			if(!set->dname.items){
				fprintf(stderr,"[SETTING ERROR] Not enough memory\n");
				return MEM_ERROR;
			}
		}

		char *name_pos = (char*)(argv[i]+2);
		u32 name_len = 0;
		char *val_pos = strstr(name_pos,"=");
		u32 val_len = 0;
		if(!val_pos){
			fprintf(stderr,"[SETTING ERROR] Format: '%s' is invalid\n",argv[i]);
			return USR_BAD_ARG;
		}
		if(strlen(val_pos) < 2){
			fprintf(stderr,"[SETTING ERROR] Format: '%s' is invalid\n",argv[i]);
			return USR_BAD_ARG;
		}
		val_pos = (val_pos + 1);
		name_len = (val_pos - 1 - name_pos);
		set->dname.items[set->dname.u_items].name = malloc(name_len+1);
		memset(set->dname.items[set->dname.u_items].name,0,name_len+1);
		memcpy(set->dname.items[set->dname.u_items].name,name_pos,name_len);

		val_len = strlen(val_pos);
		set->dname.items[set->dname.u_items].value = malloc(val_len+1);
		memset(set->dname.items[set->dname.u_items].value,0,val_len+1);
		memcpy(set->dname.items[set->dname.u_items].value,val_pos,val_len);

		set->dname.u_items++;

		return 1;
	}
	

	// If not a valid argument
	fprintf(stderr,"[SETTING ERROR] Unrecognised argument '%s'\n",argv[i]);
	return USR_UNK_ARG;
}

int CheckArgumentCombination(user_settings *set)
{
	if(set->ncch.ncchType & (CXI|CFA)){
		set->ncch.buildNcch0 = true;
		if(set->ncch.ncchType & CXI)
			set->ncch.ncchType = CXI;
		else
			set->ncch.ncchType = CFA;			
	}		
	
	if(set->common.outFormat == NCCH){
		set->ncch.buildNcch0 = true;
		if(set->ncch.ncchType)
			set->common.outFormat = set->ncch.ncchType;
		else{
			set->ncch.ncchType = CFA;
			set->common.outFormat = CFA;
		}
	}

	for(int i = 0; i < CIA_MAX_CONTENT; i++){
		if( i > CCI_MAX_CONTENT-1 && set->common.contentPath[i] && set->common.outFormat == CCI){
			fprintf(stderr,"[SETTING ERROR] Content indexes > %d are invalid for CCI\n",CCI_MAX_CONTENT-1);
			return USR_BAD_ARG;
		}
		if(set->common.contentPath[i] && (set->common.outFormat == CXI || set->common.outFormat == CFA)){
			fprintf(stderr,"[SETTING ERROR] You cannot specify content while outputting CXI/CFA files\n");
			return USR_BAD_ARG;
		}
	}
	
	if(set->common.contentPath[0] && set->ncch.buildNcch0){
		fprintf(stderr,"[SETTING ERROR] You cannot both import and build content 0\n");
		return USR_BAD_ARG;
	}

	if(set->common.outFormat == CIA && set->cci.cverCiaPath){
		fprintf(stderr,"[SETTING ERROR] You cannot use argument \"-genupdatenote\" when generating a CIA\n");
		return USR_BAD_ARG;
	}

	if(set->cia.useDataTitleVer && set->cia.useNormTitleVer){
		fprintf(stderr,"[SETTING ERROR] Arguments \"-dver\" and \"-major\"/\"-minor\" cannot be used together\n");
		return USR_BAD_ARG;
	}

	if(set->ncch.elfPath && set->ncch.codePath){
		fprintf(stderr,"[SETTING ERROR] Arguments \"-elf\" and \"-code\" cannot be used together\n");
		return USR_BAD_ARG;
	}

	bool buildCXI = set->ncch.ncchType == CXI;
	bool buildCFA = set->ncch.ncchType == CFA;
	// Detecting Required Arguments
	if(buildCXI && !set->ncch.elfPath && !set->ncch.codePath){
		PrintNeedsArg("-elf");
		return USR_BAD_ARG;
	}
	if((buildCXI || buildCFA) && !set->common.rsfPath){
		PrintNeedsArg("-rsf");
		return USR_BAD_ARG;
	}
	if(buildCXI && set->ncch.codePath && !set->ncch.exheaderPath){
		PrintNeedsArg("-exheader");
		return USR_BAD_ARG;
	}

	// Reporting bad arguments
	if(!buildCXI && set->ncch.elfPath){
		PrintArgInvalid("-elf");
		return USR_BAD_ARG;
	}
	if(!buildCXI && set->ncch.codePath){
		PrintArgInvalid("-code");
		return USR_BAD_ARG;
	}
	if(!buildCXI && set->ncch.exheaderPath){
		PrintArgInvalid("-exheader");
		return USR_BAD_ARG;
	}
	if(!buildCXI && set->ncch.plainRegionPath){
		PrintArgInvalid("-plain-region");
		return USR_BAD_ARG;
	}
	if(!set->ncch.buildNcch0 && set->ncch.includeExefsLogo){
		PrintArgInvalid("-exefslogo");
		return USR_BAD_ARG;
	}
	if(!set->ncch.buildNcch0 && set->ncch.romfsPath){
		PrintArgInvalid("-romfs");
		return USR_BAD_ARG;
	}

	return 0;
}

void init_UserSettings(user_settings *usr_settings)
{
	memset(usr_settings,0,sizeof(user_settings));
}

void free_UserSettings(user_settings *usr_settings)
{
	// Free Content Paths
	if(usr_settings->common.contentPath){
		for(int i = 0; i < CIA_MAX_CONTENT; i++)
			free(usr_settings->common.contentPath[i]);
		free(usr_settings->common.contentPath);
	}

	// free -DNAME=VALUE
	for(u32 i = 0; i < usr_settings->dname.u_items; i++){
		free(usr_settings->dname.items[i].name);
		free(usr_settings->dname.items[i].value);
	}
	free(usr_settings->dname.items);

	// Free Spec File Setting
	free_RsfSettings(&usr_settings->common.rsfSet);

	// Free Key Data
	FreeKeys(&usr_settings->common.keys);
	
	// Free Working File
	free(usr_settings->common.workingFile.buffer);
		
	// Free outfile path, if malloc'd
	if(usr_settings->common.outFileName_mallocd) 
		free(usr_settings->common.outFileName);
	
	// Clear settings
	init_UserSettings(usr_settings);
	
	// Free
	free(usr_settings);
}

void PrintNeedsArg(char *arg)
{
	fprintf(stderr,"[SETTING ERROR] Argument \"%s\" is required\n",arg);
}

void PrintArgInvalid(char *arg)
{
	fprintf(stderr,"[SETTING ERROR] Argument \"%s\" is invalid\n",arg);
}

void PrintArgReqParam(char *arg, u32 paramNum)
{
	if(paramNum == 1)
		fprintf(stderr,"[SETTING ERROR] \"%s\" takes one parameter\n",arg);
	else
		fprintf(stderr,"[SETTING ERROR] \"%s\" requires %d parameters\n",arg,paramNum);
}

void PrintNoNeedParam(char *arg)
{
	fprintf(stderr,"[SETTING ERROR] \"%s\" does not take a parameter\n",arg);
}

void DisplayHelp(char *app_name)
{
	printf("CTR MAKEROM %d.%d\n",MAKEROM_VER_MAJOR,MAKEROM_VER_MINOR);
	printf("(C) 3DSGuy 2014\n");
	printf("Usage: %s [options... ]\n",app_name);
	printf("Option          Parameter           Explanation\n");
	printf("GLOBAL OPTIONS:\n");
	printf(" -help                              Display this text\n");
	printf(" -rsf           <file>              Rom Specification File (*.rsf)\n");
	printf(" -f             <ncch|cci|cia>      Output Format, defaults to 'ncch'\n");
	printf(" -o             <file>              Output File\n");
	//printf(" -v                                 Verbose\n");
	printf(" -DNAME=VALUE                       Substitute values in Spec file\n");
	printf("KEY OPTIONS:\n");
	//printf(" -target        <t|d|p|c>           Target for crypto, defaults to 't'\n");
	printf(" -target        <t|d|p>             Target for crypto, defaults to 't'\n");
	printf("                                    't' Test(false) Keys & prod Certs\n");
	printf("                                    'd' Development Keys & Certs\n");
	printf("                                    'p' Production Keys & Certs\n");
	//printf("                                    'c' Custom Keys & Certs\n");
	printf(" -ckeyID        <u8 value>          Override the automatic commonKey selection\n");
	printf(" -showkeys                          Display the loaded keychain\n");
	printf(" -fsign                             Ignore invalid signatures\n");
	printf("NCCH OPTIONS:\n");
	printf(" -elf           <file>              ELF File\n");
	printf(" -icon          <file>              Icon File\n");
	printf(" -banner        <file>              Banner File\n");
	printf(" -logo          <file>              Logo File (Overrides \"BasicInfo/Logo\" in RSF)\n");
	printf(" -desc          <apptype>:<fw>      Specify Access Descriptor Preset\n");
	//printf("                                    AppTypes:\n");
	//printf("                                    'SDApp' Normal SD Application\n");
	//printf("                                    'ECApp' SD Application with DLC Capability\n");
	//printf("                                    'Demo'  SD Demo Application\n");
	//printf("                                    'Dlp'   NAND DLP Child Application\n");
	//printf("                                    'FIRM'  FIRM CXI\n");
	printf(" -exefslogo                         Include Logo in ExeFs (Required for usage on <5.X Systems)\n");
	printf("NCCH REBUILD OPTIONS:\n");
	printf(" -code          <code path>         Specify ExeFs code File\n");
	printf(" -exheader      <exhdr path>        ExHeader Template File\n");
	printf(" -plain-region  <pln region path>   PlainRegion File\n");
	printf(" -romfs         <romfs path>        RomFS File\n");	
	printf("CCI OPTIONS:\n");
	printf(" -content       <filepath>:<index>  Specify content files\n");
	printf(" -devcardcci                        Use SDK CardInfo Method\n");
	printf(" -nomodtid                          Don't Modify Content TitleIDs\n");
	printf(" -alignwr                           Align Writeable Region to the end of last NCCH\n");
	printf(" -genupdatenote <cver cia path>     Create Update Partition Notes\n");
	printf("CIA OPTIONS:\n");
	printf(" -content <filepath>:<index>:<id>   Specify content files\n");
	printf(" -major         <major version>     Specify Major Version\n");
	printf(" -minor         <minor version>     Specify Minor Version\n");
	printf(" -micro         <micro version>     Specify Micro Version\n");
	printf(" -dver          <datatitle ver>     Specify Data Title Version\n");
	printf(" -rand                              Use a random title key\n");
	printf(" -cci           <cci path>          Convert CCI to CIA\n");
	printf(" -srl           <srl path>          Use TWL SRL as Content0\n");
	printf(" -dlc                               Create DLC CIA\n");
}