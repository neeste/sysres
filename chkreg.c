/* chkreg.c - registry card information */

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "cardinfo.h"

#define round(x) ((int)((x)+0.5))

void     sio_set_cardinfo(CARDINFO, int);
CARDINFO sio_get_cardinfo(int);

/* chkreg - check registry for card information */

void
check_registry()
{
    char key_name[80], card_name[MAX_CT_NAME];
    int i, j;
    long n, r, t, temp;
    CARDINFO ci;
    HKEY hKey;

    sprintf(key_name,"SOFTWARE\\BTNRH\\SysRes\\CardTypes");
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name, 0, KEY_QUERY_VALUE, &hKey)
	!= ERROR_SUCCESS) {
	return;
    }
    for (i = 0; i < NCT; i++) {
	memset(&ci, 0, sizeof(ci));
        sprintf(key_name,"SOFTWARE\\BTNRH\\SysRes\\CardTypes\\CardType%d", i);
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name, 0, KEY_QUERY_VALUE, &hKey)
	    == ERROR_SUCCESS) {
	    n = MAX_CT_NAME;
	    t = REG_SZ;
	    r = RegQueryValueEx(hKey, "Name", NULL, &t, card_name, &n);
	    if (r == 0) {
		strncpy(ci.name, card_name, MAX_CT_NAME);
	    } else {
		continue;
	    }
	    n = sizeof(long);
	    t = REG_DWORD;
	    r = RegQueryValueEx(hKey, "bits", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		ci.bits = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "left", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		ci.left = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "nbps", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		ci.nbps = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "ncio", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		ci.ncad = ci.ncda = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "ncad", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		ci.ncad = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "ncda", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		ci.ncda = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "gdsr", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		ci.gdsr = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "ad_mv_fs", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		if (temp > 0)
		    for (j = 0; j < MAXNCH; j++)
			ci.ad_vfs[j] = temp * 0.001;
	    }
	    r = RegQueryValueEx(hKey, "da_mv_fs", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		if (temp > 0)
		    for (j = 0; j < MAXNCH; j++)
			ci.da_vfs[j] = temp * 0.001;
	    }
	    for (j = 0; j < MAXNCH; j++) {
		sprintf(key_name, "ad%d_mv_fs", j + 1);
		r = RegQueryValueEx(hKey, key_name, NULL, &t, (LPBYTE) &temp, &n);
		if (r == 0) {
		    ci.ad_vfs[j] = temp * 0.001;
		}
		sprintf(key_name, "da%d_mv_fs", j + 1);
		r = RegQueryValueEx(hKey, key_name, NULL, &t, (LPBYTE) &temp, &n);
		if (r == 0) {
		    ci.da_vfs[j] = temp * 0.001;
		}
	    }
            RegCloseKey(hKey);
	    sio_set_cardinfo(ci, i);
	}
    }
}
