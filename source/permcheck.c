#include <gccore.h>
#include <wiiuse/wpad.h>

#include <fat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dirent.h>

#include <malloc.h>

bool CheckNANDAccess(void);

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define LOG "sd:/PermCheck.log"
#define VERSION "v0.1"


/* other needed functions */

// Check NAND access
inline bool CheckNANDAccess(void)
{
	int ret = IOS_Open("/ticket/00000001/00000002.tik", 1);
	if (ret >= 0) IOS_Close(ret);
	return (ret >= 0);
}

/* Clear console */
void Con_Clear(void)
{
	/* Clear console */
	printf("\x1b[2J");
	fflush(stdout);
}

/* Function for sorting titles */
/*int compareMyType (const void * a, const void * b)
{
	//u32 n1 = *(u32 *)p1;
	//u32 n2 = *(u32 *)p2;
	

	 Equal */
	/*if (n1 == n2)
		return 0;

	return (n1 > n2) ? 1 : -1;*/

	/*if (*(u32*)a == *(u32*)b) {
		return 0;
	}*/
	
	/*printf("a: %d, b: %d\n", *a >> 32, *b >> 32);
	
	if (*(u32*)a > *(u32*)b) {
		return 1;
	}
	
	if (*(u32*)a < *(u32*)b) {
		return -1;
	}
	
	return 0;*
	
}*/


/* main function */
int main(int argc, char **argv) {
	
	u32 numTitles, i;
	s32 ret;
	
	// Initialise the video system
	VIDEO_Init();
	
	// This function initialises the attached controllers
	WPAD_Init();
	
	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	
	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	
	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);
	
	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);
	
	// Make the display visible
	VIDEO_SetBlack(FALSE);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();


	// The console understands VT terminal escape codes
	// This positions the cursor on row 2, column 0
	// we can use variables for this with format codes too
	// e.g. printf ("\x1b[%d;%dH", row, column );
	printf("\x1b[2;0H");
	
	printf(" ___               ___ _           _   \n");
	printf("| _ \\___ _ _ _ __ / __| |_  ___ __| |__\n");
	printf("|  _/ -_) '_| '  \\ (__| ' \\/ -_) _| / /\n");
	printf("|_| \\___|_| |_|_|_\\___|_||_\\___\\__|_\\_\\ %s\n", VERSION);
	printf("by Crypto for http://www.wii-homebrew.com\n\n");    
		
	

	/* IOS Reload 36 or 236*/
	WPAD_Shutdown();
	if (!CheckNANDAccess()) {
		IOS_ReloadIOS(36);
		if (!CheckNANDAccess()) {
			IOS_ReloadIOS(236);
			if (!CheckNANDAccess()) {
				printf("ERROR reloading IOS\n");
				WPAD_Init();
				goto error;
			}
		}
	}
	WPAD_Init();
	printf("Selected IOS: %d\n\n" , IOS_GetVersion());
	
	/* Checking SD access */
	printf("Checking SD card access... ");
	//DIR* dir = opendir("/");
	//if (!dir) {
	if (!fatInitDefault()) {
		printf("ERROR\n\n");
		printf("Please insert SD card and restart\n\n");
		goto error;
	} else {
		printf("OK\n\n");
	}


	/* Checking NAND Aaccess */	
	printf("Checking NAND access... ");
	if (!CheckNANDAccess()) {
		printf("ERROR\n");
		goto error;
	} else {
		printf("OK\n");
	}
	
		
	/* Get number of titles */
	printf("Getting number of titles... ");
	if (ES_GetNumTitles(&numTitles) < 0) {
		printf("ERROR\n");
		goto error;
	} else {
		printf("OK (%d)\n", numTitles);
	}
	
	/* Get list of titles */
	u64 *titles = memalign(32, numTitles*sizeof(u64)); // Allocate the memory for titles
	printf("Getting title list... ");
	if (ES_GetTitles(titles, numTitles) < 0) {
		printf("ERROR\n");
		goto error;
	} else {
		printf("OK\n");
	}
	//qsort(titles, numTitles, sizeof(u64), compareMyType); // Sort title list *Maybe*
	
	/* Initializing filesystem */
	printf("Initializing filesystem... ");
	ret = ISFS_Initialize();
	if(ret < 0) {
		printf("ERROR %d\n", ret);
		goto error;
	}
	else {
		printf("OK\n");
	}

	printf("\nPress <A> to continue or <HOME> to exit.\n\n");
	
	for (;;) {
		WPAD_ScanPads();
		u32 pressed = WPAD_ButtonsDown(0);
		
		/* HOME button */
		if (pressed & WPAD_BUTTON_HOME)
			goto exit;

		/* A button */
		if (pressed & WPAD_BUTTON_A)
			break;
	}
	
	/* Clear console */
	Con_Clear();
	
	/* Creating logfile */
	printf("Creating logfile... ");
	FILE* file = fopen(LOG, "w");
	if(!file) {
		printf("ERROR\n");
		goto error;
	}
	else {
		printf("OK\n");
		fprintf(file, "PermCheck %s by Crypto\r\n\r\n", VERSION);
	}
	
	
	/* Checking titles */
	printf("\nChecking titles... \n");
	for (i = 0; i < numTitles; i++)
	{	
		/* Skipping non system titles */
		if (((titles[i] >> 32) == 1) && ((u32)(titles[i])) > 0) {
			
			static char filepath[256] ATTRIBUTE_ALIGN(32);
			sprintf(filepath, "/title/%08x/%08x/content/title.tmd", (u32)(titles[i] >> 32), (u32)(0x00000000ULL | titles[i]));
			
			/* Get permissions */
			u32 ownerID;
			u16 groupID;
			u8  attributes, ownerperm, groupperm, otherperm;
			
			printf("IOS%d (%s)... ", (u32)(titles[i]), filepath);
			ret = ISFS_GetAttr(filepath, &ownerID, &groupID, &attributes, &ownerperm, &groupperm, &otherperm);
			if(ret < 0) {
				printf("ERROR %d\n", ret);
				goto error;
			}
			else {
				printf("OK\n");
			}
			
			fprintf(file, "IOS%d (%s): OWNER_ID=%08x, GROUP_ID=%04x, ATTRIBUTES=%08x, OWNER_PERM=%08x, GROUP_PERM=%08x, OTHER_PERM=%08x\n", (u32)(titles[i]), filepath, ownerID, groupID, attributes, ownerperm, groupperm, otherperm);
		}
	}
	
	/* Closing logfile */
	fflush(file);
	fclose(file);
	

error:
	printf("\nPress <HOME> to exit.\n\n");
	for (;;) {
	
		// Call WPAD_ScanPads each loop, this reads the latest controller states
		WPAD_ScanPads();
		u32 pressed = WPAD_ButtonsDown(0);

		if (pressed & WPAD_BUTTON_HOME) {
			goto exit;
		}
		
	}

	

exit:
	exit(0);
	VIDEO_WaitVSync();

	return 0;
}




