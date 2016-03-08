/*
This file is part of Compare plugin for Notepad++
Copyright (C)2011 Jean-Sébastien Leroy (jean.sebastien.leroy@gmail.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Compare.h"
#include "NPPHelpers.h"
#include "ScmHelper.h"

TCHAR emptyLinesDoc[MAX_PATH];
#define MAXCOMPARE 50
int compareDocs[MAXCOMPARE];

int  tempWindow = -1;
bool notepadVersionOk = false;
bool active = false;
bool skipAutoReset = false;
int closingWin = -1;
HWND closingView = NULL;
blankLineList *lastEmptyLines=NULL;
int  topLine = 0;
long start_old = -1;
long visible_line_count_old = -1;
bool panelsOpened = false;
bool syncScrollVwasChecked = false;
bool syncScrollHwasChecked = false;

CProgress* progDlg = NULL;
CProgress_IsCanceled_fn CProgress_IsCanceled = NULL;
CProgress_Increment_fn CProgress_Increment = NULL;
int progMax = 0;
int progCounter = 0;

const TCHAR PLUGIN_NAME[] = TEXT("Compare");
TCHAR iniFilePath[MAX_PATH];
const TCHAR sectionName[] = TEXT("Compare Settings");
const TCHAR addLinesOption[] = TEXT("Align Matches");
const TCHAR ignoreSpacesOption[] = TEXT("Include Spaces");
const TCHAR detectMovesOption[] = TEXT("Detect Move Blocks");

const TCHAR colorsSection[]        = TEXT("Colors");
const TCHAR addedColorOption[]     = TEXT("Added");
const TCHAR removedColorOption[]   = TEXT("Removed");
const TCHAR changedColorOption[]   = TEXT("Changed");
const TCHAR movedColorOption[]     = TEXT("Moved");
const TCHAR highlightColorOption[] = TEXT("Highlight");
const TCHAR highlightAlphaOption[] = TEXT("Alpha");
const TCHAR symbolsOption[]        = TEXT("Symbols");
const TCHAR NavBarOption[]         = TEXT("Navigation bar");

const TCHAR localConfFile[] = TEXT("doLocalConf.xml");

bool different = TRUE;

TCHAR compareFilePath[MAX_PATH];
TCHAR compareFile[] = TEXT("Compare File");
NppData nppData;

FuncItem funcItem[NB_MENU_COMMANDS];

HINSTANCE hInstance;

sUserSettings Settings;
AboutDialog   AboutDlg;
OptionDialog  OptionDlg;
NavDialog     NavDlg;

toolbarIcons  tbPrev;
toolbarIcons  tbNext;
toolbarIcons  tbFirst;
toolbarIcons  tbLast;

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD  reasonForCall, LPVOID /*lpReserved*/)
 {
    hInstance = hinstDLL;

	switch (reasonForCall)
	{
	case DLL_PROCESS_ATTACH:
		{
			funcItem[CMD_COMPARE]._pFunc = compare;
			lstrcpy(funcItem[CMD_COMPARE]._itemName, TEXT("Compare"));
			funcItem[CMD_COMPARE]._pShKey = new ShortcutKey;
			funcItem[CMD_COMPARE]._pShKey->_isAlt = true;
			funcItem[CMD_COMPARE]._pShKey->_isCtrl = false;
			funcItem[CMD_COMPARE]._pShKey->_isShift = false;
			funcItem[CMD_COMPARE]._pShKey->_key = 'D';
			funcItem[CMD_COMPARE]._init2Check = false;

			funcItem[CMD_CLEAR_RESULTS]._pFunc = reset;
			lstrcpy(funcItem[CMD_CLEAR_RESULTS]._itemName, TEXT("Clear Results"));
			funcItem[CMD_CLEAR_RESULTS]._pShKey = new ShortcutKey;
			funcItem[CMD_CLEAR_RESULTS]._pShKey->_isAlt = true;
			funcItem[CMD_CLEAR_RESULTS]._pShKey->_isCtrl = true;
			funcItem[CMD_CLEAR_RESULTS]._pShKey->_isShift = false;
			funcItem[CMD_CLEAR_RESULTS]._pShKey->_key = 'D';
			funcItem[CMD_CLEAR_RESULTS]._init2Check = false;

			funcItem[CMD_SEPARATOR_1]._pFunc = NULL;
			lstrcpy(funcItem[CMD_SEPARATOR_1]._itemName, TEXT("-----------"));
			funcItem[CMD_SEPARATOR_1]._pShKey = NULL;

			funcItem[CMD_COMPARE_LAST_SAVE]._pFunc = compareLocal;
			lstrcpy(funcItem[CMD_COMPARE_LAST_SAVE]._itemName, TEXT("Compare to last save"));
			funcItem[CMD_COMPARE_LAST_SAVE]._pShKey = new ShortcutKey;
			funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_isAlt = true;
			funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_isCtrl = false;
			funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_isShift = false;
			funcItem[CMD_COMPARE_LAST_SAVE]._pShKey->_key = 'S';
			funcItem[CMD_COMPARE_LAST_SAVE]._init2Check = false;

			funcItem[CMD_COMPARE_SVN_BASE]._pFunc = compareSvnBase;
			lstrcpy(funcItem[CMD_COMPARE_SVN_BASE]._itemName, TEXT("Compare against SVN base"));
			funcItem[CMD_COMPARE_SVN_BASE]._pShKey = new ShortcutKey;
			funcItem[CMD_COMPARE_SVN_BASE]._pShKey->_isAlt = true;
			funcItem[CMD_COMPARE_SVN_BASE]._pShKey->_isCtrl = false;
			funcItem[CMD_COMPARE_SVN_BASE]._pShKey->_isShift = false;
			funcItem[CMD_COMPARE_SVN_BASE]._pShKey->_key = 'B';
			funcItem[CMD_COMPARE_SVN_BASE]._init2Check = false;

			funcItem[CMD_COMPARE_GIT_BASE]._pFunc = compareGitBase;
			lstrcpy(funcItem[CMD_COMPARE_GIT_BASE]._itemName, TEXT("Compare against GIT base"));
			funcItem[CMD_COMPARE_GIT_BASE]._pShKey = new ShortcutKey;
			funcItem[CMD_COMPARE_GIT_BASE]._pShKey->_isAlt = true;
			funcItem[CMD_COMPARE_GIT_BASE]._pShKey->_isCtrl = true;
			funcItem[CMD_COMPARE_GIT_BASE]._pShKey->_isShift = false;
			funcItem[CMD_COMPARE_GIT_BASE]._pShKey->_key = 'B';
			funcItem[CMD_COMPARE_GIT_BASE]._init2Check = false;

			funcItem[CMD_SEPARATOR_2]._pFunc = NULL;
			lstrcpy(funcItem[CMD_SEPARATOR_2]._itemName, TEXT("------------"));
			funcItem[CMD_SEPARATOR_2]._pShKey = NULL;

			funcItem[CMD_ALIGN_MATCHES]._pFunc = alignMatches;
			lstrcpy(funcItem[CMD_ALIGN_MATCHES]._itemName, TEXT("Align Matches"));
			funcItem[CMD_ALIGN_MATCHES]._pShKey = NULL;
			funcItem[CMD_ALIGN_MATCHES]._init2Check = false;

			funcItem[CMD_IGNORE_SPACING]._pFunc = includeSpacing;
			lstrcpy(funcItem[CMD_IGNORE_SPACING]._itemName, TEXT("Ignore Spacing"));
			funcItem[CMD_IGNORE_SPACING]._pShKey = NULL;
			funcItem[CMD_IGNORE_SPACING]._init2Check = false;

			funcItem[CMD_DETECT_MOVES]._pFunc = detectMoves;
			lstrcpy(funcItem[CMD_DETECT_MOVES]._itemName, TEXT("Detect Moves"));
			funcItem[CMD_DETECT_MOVES]._pShKey = NULL;
			funcItem[CMD_DETECT_MOVES]._init2Check = false;

			funcItem[CMD_USE_NAV_BAR]._pFunc = ViewNavigationBar;
			lstrcpy(funcItem[CMD_USE_NAV_BAR]._itemName, TEXT("Navigation bar"));
			funcItem[CMD_USE_NAV_BAR]._pShKey = NULL;
			funcItem[CMD_USE_NAV_BAR]._init2Check = false;

			funcItem[CMD_SEPARATOR_3]._pFunc = NULL;
			lstrcpy(funcItem[CMD_SEPARATOR_3]._itemName, TEXT("-----------"));
			funcItem[CMD_SEPARATOR_3]._pShKey = NULL;

			funcItem[CMD_PREV]._pFunc = Prev;
			lstrcpy(funcItem[CMD_PREV]._itemName, TEXT("Previous"));
			funcItem[CMD_PREV]._pShKey = new ShortcutKey;
			funcItem[CMD_PREV]._pShKey->_isAlt = false;
			funcItem[CMD_PREV]._pShKey->_isCtrl = true;
			funcItem[CMD_PREV]._pShKey->_isShift = false;
			funcItem[CMD_PREV]._pShKey->_key = VK_PRIOR;
			funcItem[CMD_PREV]._init2Check = false;

			funcItem[CMD_NEXT]._pFunc = Next;
			lstrcpy(funcItem[CMD_NEXT]._itemName, TEXT("Next"));
			funcItem[CMD_NEXT]._pShKey = new ShortcutKey;
			funcItem[CMD_NEXT]._pShKey->_isAlt = false;
			funcItem[CMD_NEXT]._pShKey->_isCtrl = true;
			funcItem[CMD_NEXT]._pShKey->_isShift = false;
			funcItem[CMD_NEXT]._pShKey->_key = VK_NEXT;
			funcItem[CMD_NEXT]._init2Check = false;

			funcItem[CMD_FIRST]._pFunc = First;
			lstrcpy(funcItem[CMD_FIRST]._itemName, TEXT("First"));
			funcItem[CMD_FIRST]._pShKey = new ShortcutKey;
			funcItem[CMD_FIRST]._pShKey->_isAlt = false;
			funcItem[CMD_FIRST]._pShKey->_isCtrl = true;
			funcItem[CMD_FIRST]._pShKey->_isShift = true;
			funcItem[CMD_FIRST]._pShKey->_key = VK_PRIOR;
			funcItem[CMD_FIRST]._init2Check = false;

			funcItem[CMD_LAST]._pFunc = Last;
			lstrcpy(funcItem[CMD_LAST]._itemName, TEXT("Last"));
			funcItem[CMD_LAST]._pShKey = new ShortcutKey;
			funcItem[CMD_LAST]._pShKey->_isAlt = false;
			funcItem[CMD_LAST]._pShKey->_isCtrl = true;
			funcItem[CMD_LAST]._pShKey->_isShift = true;
			funcItem[CMD_LAST]._pShKey->_key = VK_NEXT;
			funcItem[CMD_LAST]._init2Check = false;

			funcItem[CMD_SEPARATOR_4]._pFunc = NULL;
			lstrcpy(funcItem[CMD_SEPARATOR_4]._itemName, TEXT("-----------"));
			funcItem[CMD_SEPARATOR_4]._pShKey = NULL;

			funcItem[CMD_OPTION]._pFunc = openOptionDlg;
			lstrcpy(funcItem[CMD_OPTION]._itemName, TEXT("Option"));
			funcItem[CMD_OPTION]._pShKey = NULL;
			funcItem[CMD_OPTION]._init2Check = false;

			funcItem[CMD_ABOUT]._pFunc = openAboutDlg;
			lstrcpy(funcItem[CMD_ABOUT]._itemName, TEXT("About"));
			funcItem[CMD_ABOUT]._pShKey = NULL;
			funcItem[CMD_ABOUT]._init2Check = false;

			for(int i = 0; i < MAXCOMPARE; i++)
				compareDocs[i]=-1;

			TCHAR nppPath[MAX_PATH];
            GetModuleFileName(hInstance, nppPath, sizeof(nppPath));

			// remove the module name : get plugins directory path
			PathRemoveFileSpec(nppPath);

			// cd .. : get npp executable path
			PathRemoveFileSpec(nppPath);

			// Make localConf.xml path
			TCHAR localConfPath[MAX_PATH];
			lstrcpy(localConfPath, nppPath);
			PathAppend(localConfPath, localConfFile);

			// Test if localConf.xml exist
			const bool isLocal = (PathFileExists(localConfPath) == TRUE);

			if (isLocal) 
			{
				lstrcpy(iniFilePath, nppPath);
				lstrcpy(compareFilePath, nppPath);

				PathAppend(iniFilePath, TEXT("plugins\\config\\Compare.ini"));
			}
			else 
			{
				ITEMIDLIST *pidl;
				SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
				SHGetPathFromIDList(pidl, iniFilePath);
				SHGetPathFromIDList(pidl, compareFilePath);

				PathAppend(iniFilePath, TEXT("Notepad++\\Compare.ini"));
			}

			loadSettings();
			break;
		}

	case DLL_PROCESS_DETACH:
		{
			if (tbNext.hToolbarBmp)
				::DeleteObject(tbNext.hToolbarBmp);

			if (tbPrev.hToolbarBmp)
				::DeleteObject(tbPrev.hToolbarBmp);

			if (tbFirst.hToolbarBmp)
				::DeleteObject(tbFirst.hToolbarBmp);

			if (tbLast.hToolbarBmp)
				::DeleteObject(tbLast.hToolbarBmp);

			saveSettings();
			OptionDlg.destroy();
			AboutDlg.destroy();
			NavDlg.destroy();

			// Don't forget to deallocate your shortcut here
			for (int i = 0; i < NB_MENU_COMMANDS; i++)
				if (funcItem[i]._pShKey != NULL)
					delete funcItem[i]._pShKey;

			break;
		}

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
#endif

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;

    AboutDlg.init(hInstance, nppData);
    OptionDlg.init(hInstance, nppData);
    NavDlg.init(hInstance, nppData);
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = NB_MENU_COMMANDS;
	return funcItem;
}

void loadSettings(void)
{
	// Try loading previous color settings
	int colors = GetPrivateProfileInt(colorsSection, addedColorOption, -1, iniFilePath);

	// If there is no previous color settings, load default value
	if(colors == -1)
	{
		Settings.ColorSettings.added     = ::GetPrivateProfileInt(colorsSection, addedColorOption, DEFAULT_ADDED_COLOR, iniFilePath);
		Settings.ColorSettings.deleted   = ::GetPrivateProfileInt(colorsSection, removedColorOption, DEFAULT_DELETED_COLOR, iniFilePath);
		Settings.ColorSettings.changed   = ::GetPrivateProfileInt(colorsSection, changedColorOption, DEFAULT_CHANGED_COLOR, iniFilePath);
		Settings.ColorSettings.moved     = ::GetPrivateProfileInt(colorsSection, movedColorOption, DEFAULT_MOVED_COLOR, iniFilePath);
		Settings.ColorSettings.highlight = ::GetPrivateProfileInt(colorsSection, highlightColorOption, DEFAULT_HIGHLIGHT_COLOR, iniFilePath);
		Settings.ColorSettings.alpha     = ::GetPrivateProfileInt(colorsSection, highlightAlphaOption, DEFAULT_HIGHLIGHT_ALPHA, iniFilePath);
	}
	// Else load stored color settings
	else
	{
		Settings.ColorSettings.added     = ::GetPrivateProfileInt(colorsSection, addedColorOption, DEFAULT_ADDED_COLOR, iniFilePath);
		Settings.ColorSettings.deleted   = ::GetPrivateProfileInt(colorsSection, removedColorOption, DEFAULT_DELETED_COLOR, iniFilePath);
		Settings.ColorSettings.changed   = ::GetPrivateProfileInt(colorsSection, changedColorOption, DEFAULT_CHANGED_COLOR, iniFilePath);
		Settings.ColorSettings.moved     = ::GetPrivateProfileInt(colorsSection, movedColorOption, DEFAULT_MOVED_COLOR, iniFilePath);
		Settings.ColorSettings.highlight = ::GetPrivateProfileInt(colorsSection, highlightColorOption, DEFAULT_HIGHLIGHT_COLOR, iniFilePath);
		Settings.ColorSettings.alpha     = ::GetPrivateProfileInt(colorsSection, highlightAlphaOption, DEFAULT_HIGHLIGHT_ALPHA, iniFilePath);
	}

	// Try loading behavior settings, else load default value
	Settings.AddLine      = ::GetPrivateProfileInt(sectionName, addLinesOption, 1, iniFilePath) == 1;
	Settings.IncludeSpace = ::GetPrivateProfileInt(sectionName, ignoreSpacesOption, 0, iniFilePath) == 1;
	Settings.DetectMove   = ::GetPrivateProfileInt(sectionName, detectMovesOption, 1, iniFilePath) == 1;
	Settings.OldSymbols   = ::GetPrivateProfileInt(sectionName, symbolsOption, 0, iniFilePath) == 1;
	Settings.UseNavBar    = ::GetPrivateProfileInt(sectionName, NavBarOption, 1, iniFilePath) == 1;
}

void saveSettings(void)
{
	TCHAR buffer[64];

	_itot_s(Settings.ColorSettings.added, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, addedColorOption, buffer, iniFilePath);

	_itot_s(Settings.ColorSettings.deleted, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, removedColorOption, buffer, iniFilePath);

	_itot_s(Settings.ColorSettings.changed, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, changedColorOption, buffer, iniFilePath);

	_itot_s(Settings.ColorSettings.moved, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, movedColorOption, buffer, iniFilePath);

	_itot_s(Settings.ColorSettings.highlight, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, highlightColorOption, buffer, iniFilePath);

	_itot_s(Settings.ColorSettings.alpha, buffer, 64, 10);
	::WritePrivateProfileString(colorsSection, highlightAlphaOption, buffer, iniFilePath);

	::WritePrivateProfileString(sectionName, addLinesOption, Settings.AddLine ? TEXT("1") : TEXT("0"), iniFilePath);
	::WritePrivateProfileString(sectionName, ignoreSpacesOption, Settings.IncludeSpace ? TEXT("1") : TEXT("0"), iniFilePath);
	::WritePrivateProfileString(sectionName, detectMovesOption, Settings.DetectMove ? TEXT("1") : TEXT("0"), iniFilePath);
	::WritePrivateProfileString(sectionName, symbolsOption, Settings.OldSymbols ? TEXT("1") : TEXT("0"), iniFilePath);
	::WritePrivateProfileString(sectionName, NavBarOption, Settings.UseNavBar ? TEXT("1") : TEXT("0"), iniFilePath);
}

int getCompare(int window)
{
	for(int i = 0; i < MAXCOMPARE; i++)
		if(compareDocs[i] == window)
			return i;

	return -1;
}

void removeCompare(int window)
{
	const int val = getCompare(window);
	if(val != -1)
		compareDocs[val] = -1;
}

int setCompare(int window)
{
	const int val = getCompare(window);

	if(val != -1)
		return val;

	for(int i = 0; i < MAXCOMPARE; i++)
	{
		if(compareDocs[i] == -1)
		{
			compareDocs[i] = window;
			return i;
		}
	}

	return -1;
}

void alignMatches()
{
	HMENU hMenu = GetMenu(nppData._nppHandle);
	Settings.AddLine = !Settings.AddLine;
	if (hMenu)
		CheckMenuItem(hMenu,
					  funcItem[CMD_ALIGN_MATCHES]._cmdID,
					  MF_BYCOMMAND | (Settings.AddLine ? MF_CHECKED : MF_UNCHECKED));
}

void includeSpacing()
{
	HMENU hMenu = GetMenu(nppData._nppHandle);
	Settings.IncludeSpace = !Settings.IncludeSpace;
	if (hMenu)
		CheckMenuItem(hMenu,
					  funcItem[CMD_IGNORE_SPACING]._cmdID,
					  MF_BYCOMMAND | (Settings.IncludeSpace ? MF_CHECKED : MF_UNCHECKED));
}

void detectMoves()
{
	HMENU hMenu = GetMenu(nppData._nppHandle);
	Settings.DetectMove = !Settings.DetectMove;
	if (hMenu)
		CheckMenuItem(hMenu,
					  funcItem[CMD_DETECT_MOVES]._cmdID,
					  MF_BYCOMMAND | (Settings.DetectMove ? MF_CHECKED : MF_UNCHECKED));
}

void openOptionDlg(void)
{
	if (OptionDlg.doDialog(&Settings) == IDOK)
	{
		saveSettings();
		if (active)
		{
			setStyles(&Settings);
			
			if (NavDlg.isVisible())
			{
				NavDlg.SetColor(
					Settings.ColorSettings.added,
					Settings.ColorSettings.deleted,
					Settings.ColorSettings.changed,
					Settings.ColorSettings.moved,
                    Settings.ColorSettings.blank,
                    Settings.ColorSettings._default);

				NavDlg.CreateBitmap();
			}
		}
	}
}

void jumpChangedLines(bool direction)
{
	HWND CurView = getCurrentWindow();

	const int sci_search_mask = (1 << MARKER_MOVED_LINE)
							  | (1 << MARKER_CHANGED_LINE)
							  | (1 << MARKER_ADDED_LINE)
							  | (1 << MARKER_REMOVED_LINE)
							  | (1 << MARKER_BLANK_LINE);

	const int posStart = ::SendMessage(CurView, SCI_GETCURRENTPOS, 0, 0);
	const int lineMax = ::SendMessage(CurView, SCI_GETLINECOUNT, 0, 0);
	int lineStart = ::SendMessage(CurView, SCI_LINEFROMPOSITION, posStart, 0);
	int prevLine = lineStart;

	int currLine;
	int nextLine;
	int sci_marker_direction;

	while (true)
	{
		if (direction) 
		{
			currLine = (lineStart < lineMax) ? (lineStart + 1) : (0);
			sci_marker_direction = SCI_MARKERNEXT;
		}
		else 
		{
			currLine = (lineStart > 0) ? (lineStart - 1) : (lineMax);
			sci_marker_direction = SCI_MARKERPREVIOUS;
		}

		nextLine = ::SendMessage(CurView, sci_marker_direction, currLine, sci_search_mask);

		if (nextLine < 0) 
		{
			currLine = (direction) ? (0) : (lineMax);
			nextLine = ::SendMessage(CurView, sci_marker_direction, currLine, sci_search_mask);
			break;
		}

		if (nextLine != currLine)
			break;
		else if (direction)
			lineStart++;
		else
			lineStart--;
	}

	if ((direction && (nextLine < prevLine)) ||
		(!direction && (nextLine > prevLine)))
	{
		FLASHWINFO flashInfo;
		flashInfo.cbSize = sizeof(FLASHWINFO);
		flashInfo.hwnd = nppData._nppHandle;
		flashInfo.uCount = 2;
		flashInfo.dwTimeout = 100;
		flashInfo.dwFlags = FLASHW_ALL;
		FlashWindowEx(&flashInfo);
	}

	::SendMessage(CurView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0);
	::SendMessage(CurView, SCI_GOTOLINE, nextLine, 0);
}

void Prev(void)
{
	if (active)
		jumpChangedLines(false);
}

void Next(void)
{
	if (active)
		jumpChangedLines(true);
}

void First(void)
{
	if (active)
	{
		HWND CurView = getCurrentWindow();
		HWND OtherView = getOtherWindow();

		const int sci_search_mask = (1 << MARKER_MOVED_LINE)
								  | (1 << MARKER_CHANGED_LINE)
								  | (1 << MARKER_ADDED_LINE)
								  | (1 << MARKER_REMOVED_LINE)
								  | (1 << MARKER_BLANK_LINE);

		const int nextLine = ::SendMessage(CurView, SCI_MARKERNEXT, 0, sci_search_mask );
		::SendMessage(CurView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0);
		::SendMessage(OtherView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0);
		::SendMessage(CurView, SCI_GOTOLINE, nextLine, 0);
		::SendMessage(OtherView, SCI_GOTOLINE, nextLine, 0);
	}
}

void Last(void)
{
	if (active)
	{
		HWND CurView = getCurrentWindow();

		const int sci_search_mask = (1 << MARKER_MOVED_LINE)
								  | (1 << MARKER_CHANGED_LINE)
								  | (1 << MARKER_ADDED_LINE)
								  | (1 << MARKER_REMOVED_LINE)
								  | (1 << MARKER_BLANK_LINE);

		const int lineMax = ::SendMessage(CurView, SCI_GETLINECOUNT, 0, 0);
		const int nextLine = ::SendMessage(CurView, SCI_MARKERPREVIOUS, lineMax, sci_search_mask);
		::SendMessage(CurView, SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine, 0);
		::SendMessage(CurView, SCI_GOTOLINE, nextLine, 0);
	}
}

void openAboutDlg(void)
{
	AboutDlg.doDialog();
}

void ViewNavigationBar(void)
{
	HMENU hMenu = GetMenu(nppData._nppHandle);
	Settings.UseNavBar = !Settings.UseNavBar;

	if (hMenu)
		CheckMenuItem(hMenu,
					  funcItem[CMD_USE_NAV_BAR]._cmdID,
					  MF_BYCOMMAND | (Settings.UseNavBar ? MF_CHECKED : MF_UNCHECKED));

	if (active)
	{
		if (Settings.UseNavBar) 
		{
			// Save current N++ focus
			HWND hwnd = GetFocus();

			// Configure NavBar
			NavDlg.SetColor(
				Settings.ColorSettings.added, 
				Settings.ColorSettings.deleted, 
				Settings.ColorSettings.changed, 
				Settings.ColorSettings.moved,
                Settings.ColorSettings.blank,
                Settings.ColorSettings._default);

			// Display Navbar
			NavDlg.doDialog(true);
			start_old = -1;
            visible_line_count_old = -1;

			// Restore N++ focus
			SetFocus(hwnd);
		}
		else
		{
			NavDlg.doDialog(false);            
		}
	}
}

HWND getCurrentHScintilla(int which)
{
	return (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
};

// NP++ messages processing
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (Message == WM_CREATE)
	{
		HMENU hMenu = ::GetMenu(nppData._nppHandle);
		::ModifyMenu(hMenu, funcItem[CMD_SEPARATOR_1]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
		::ModifyMenu(hMenu, funcItem[CMD_SEPARATOR_2]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
		::ModifyMenu(hMenu, funcItem[CMD_SEPARATOR_3]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
		::ModifyMenu(hMenu, funcItem[CMD_SEPARATOR_4]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
	}

	return TRUE;
}

HWND openTempFile(void)
	{
	char original[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)original);
	HWND originalwindow = getCurrentWindow();	

	LRESULT curBuffer = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
	LangType curLang = (LangType)::SendMessage(nppData._nppHandle, NPPM_GETBUFFERLANGTYPE, curBuffer, 0);

	int result = ::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)compareFilePath);
	HWND window = getCurrentWindow();		
	int win = SendMessageA(window, SCI_GETDOCPOINTER, 0, 0);
	
	if(result == 0 || win != tempWindow)
	{
		::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_FILE_NEW, (LPARAM)0);
		::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM)compareFilePath);
		tempWindow = SendMessageA(window, SCI_GETDOCPOINTER, 0, 0);

		curBuffer = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
		::SendMessage(nppData._nppHandle, NPPM_SETBUFFERLANGTYPE, curBuffer, curLang);
	}	

	if(originalwindow == window)
	{
		::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)original);
		skipAutoReset = true;
		SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_GOTO_ANOTHER_VIEW, 0);
		skipAutoReset = false;
		::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)compareFilePath);
		panelsOpened = true;
	}

	result=::SendMessage(nppData._nppHandle,NPPM_SWITCHTOFILE, 0, (LPARAM)original);

	window = getOtherWindow();

	int pointer = SendMessageA(window, SCI_GETDOCPOINTER, 0, 0);
	if(tempWindow != pointer)
	{
		window = getCurrentWindow();
		pointer = SendMessageA(window, SCI_GETDOCPOINTER, 0, 0);
	}

	//assert(tempWindow == pointer);

	//move focus to new document, or the other document will be marked as dirty
	::SendMessageA(window, SCI_GRABFOCUS, 0, 0);
	::SendMessageA(window, SCI_SETREADONLY, 0, 0);
	::SendMessageA(window, SCI_CLEARALL, 0, 0);

	return window;
}

void openFile(TCHAR *file)
{
	if(file == NULL || PathFileExists(file) == FALSE)
	{
		::MessageBox(nppData._nppHandle, TEXT("No file to open"), TEXT("error"), MB_OK);
		return;
	}

	ifstream myfile(file,ios::in|ios::ate| ios::binary);

	if(myfile.is_open())
	{		
		long size = (long)myfile.tellg();
		char *memblock = new char [size+1];
		myfile.seekg(0);
		myfile.read(memblock, size);
		myfile.close();	

		memblock[size] = 0;

		openMemBlock(memblock, size);

		delete[] memblock;
	}
}

void openMemBlock(void *memblock, long size)
{
	HWND window = openTempFile();

	::SendMessageA(window, SCI_GRABFOCUS, 0, 0);
	::SendMessageA(window, SCI_APPENDTEXT, size, (LPARAM)memblock);	

	if(startCompare())
	{
		::SendMessageA(window, SCI_GRABFOCUS, 0, 0);
		::SendMessageA(window, SCI_SETSAVEPOINT, 1, 0);
		::SendMessageA(window, SCI_EMPTYUNDOBUFFER, 0, 0);
		::SendMessageA(window, SCI_SETREADONLY, 1, 0);
		reset();
	}
	else
	{
		::SendMessageA(window, SCI_GRABFOCUS, 0, 0);
		::SendMessageA(window, SCI_SETSAVEPOINT, 1, 0);
		::SendMessageA(window, SCI_EMPTYUNDOBUFFER, 0, 0);
		::SendMessageA(window, SCI_SETREADONLY, 1, 0);            
		::SendMessageA(nppData._scintillaSecondHandle, SCI_GRABFOCUS, 0, 1);
	}
}

// Exit compare session
// - Clear results
// - Delete objects
// - Restore previous NP++ appearance (markers, highlight, ...)
void reset()
{
	if (active == true)
	{
		active = false;

		LRESULT RODoc1;
		LRESULT RODoc2;
		int doc1 = 0;
		int doc2 = 0;
		int doc1Index = -1;
		int doc2Index = -1;

		if (closingView != nppData._scintillaMainHandle)
		{
			// Remove read-only attribute
			if ((RODoc1 = SendMessage(nppData._scintillaMainHandle, SCI_GETREADONLY, 0, 0)) == 1)
				SendMessage(nppData._scintillaMainHandle, SCI_SETREADONLY, false, 0);
			doc1 = SendMessageA(nppData._scintillaMainHandle, SCI_GETDOCPOINTER, 0, 0);
			doc1Index = getCompare(doc1);
			if (doc1Index != -1)
				clearWindow(nppData._scintillaMainHandle, true);
			// Remove margin mask
			::SendMessage(nppData._scintillaMainHandle, SCI_SETMARGINMASKN, (WPARAM)4, (LPARAM)0);
			// Remove margin
			::SendMessage(nppData._scintillaMainHandle, SCI_SETMARGINWIDTHN, (WPARAM)4, (LPARAM)0);
			// Restore previous read-only attribute
			if (RODoc1 == 1)
				SendMessage(nppData._scintillaMainHandle, SCI_SETREADONLY, true, 0);
		}
		removeCompare(doc1);

		if (closingView != nppData._scintillaSecondHandle)
		{
			if ((RODoc2 = SendMessage(nppData._scintillaSecondHandle, SCI_GETREADONLY, 0, 0)) == 1)
				SendMessage(nppData._scintillaSecondHandle, SCI_SETREADONLY, false, 0);
			doc2 = SendMessageA(nppData._scintillaSecondHandle, SCI_GETDOCPOINTER, 0, 0);
			doc2Index = getCompare(doc2);
			if (doc2Index != -1)
				clearWindow(nppData._scintillaSecondHandle, true);
			::SendMessage(nppData._scintillaSecondHandle, SCI_SETMARGINMASKN, (WPARAM)4, (LPARAM)0);
			::SendMessage(nppData._scintillaSecondHandle, SCI_SETMARGINWIDTHN, (WPARAM)4, (LPARAM)0);
			if (RODoc2 == 1)
				SendMessage(nppData._scintillaSecondHandle, SCI_SETREADONLY, true, 0);
		}
		removeCompare(doc2);

		if (panelsOpened && (closingWin == -1))
		{
			::SendMessageA(nppData._scintillaSecondHandle, SCI_GRABFOCUS, 0, (LPARAM)0);
			skipAutoReset = true;
			SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_GOTO_ANOTHER_VIEW, 0);
			skipAutoReset = false;
		}
		panelsOpened = false;

		if (tempWindow != -1)
		{
			if (doc1 != closingWin)
			{
				::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)compareFilePath);
				HWND window = getCurrentWindow();
				int tempPointer = SendMessageA(window, SCI_GETDOCPOINTER, 0, 0);
				if (tempPointer == tempWindow)
				{
					SendMessageA(window, SCI_EMPTYUNDOBUFFER, 0, 0);
					skipAutoReset = true;
					SendMessage(nppData._nppHandle, WM_COMMAND, IDM_FILE_CLOSE, 0);
					skipAutoReset = false;
				}
			}
			tempWindow = -1;
			LRESULT ROTemp = RODoc1; RODoc1 = RODoc2; RODoc2 = ROTemp;
		}

		// Restore sync scroll buttons
		if (syncScrollVwasChecked)
			::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLV, 0), 0);
		if (!syncScrollHwasChecked)
			::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLH, 0), 0);

		// Close NavBar
		NavDlg.doDialog(false);

		// Disable Prev/Next menu entry
		HMENU hMenu = ::GetMenu(nppData._nppHandle);
		::EnableMenuItem(hMenu, funcItem[CMD_PREV]._cmdID,  MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		::EnableMenuItem(hMenu, funcItem[CMD_NEXT]._cmdID,  MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		::EnableMenuItem(hMenu, funcItem[CMD_FIRST]._cmdID, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		::EnableMenuItem(hMenu, funcItem[CMD_LAST]._cmdID,  MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

		// Restore side bar item entry state (because tick has been removed by the docked window)
		CheckMenuItem(hMenu, funcItem[CMD_USE_NAV_BAR]._cmdID, MF_BYCOMMAND | (Settings.UseNavBar ? MF_CHECKED : MF_UNCHECKED));

		::SendMessageA(getCurrentWindow(), SCI_GRABFOCUS, 0, 0);

		closingWin = -1;
		closingView = NULL;
	}
}

void compareLocal()
{
	TCHAR file[MAX_PATH];
	::SendMessage(nppData._nppHandle,NPPM_GETCURRENTDIRECTORY,0,(LPARAM)file);
	if(file[0] != 0)
		::SendMessage(nppData._nppHandle,NPPM_GETFULLCURRENTPATH,0,(LPARAM)file);
	openFile(file);
}

void compareSvnBase()
{
	TCHAR curDir[MAX_PATH];

	SendMessage(nppData._nppHandle, NPPM_GETCURRENTDIRECTORY, 0, (LPARAM)curDir);
	
	if (curDir[0] != 0)
	{
		TCHAR curDirCanon[MAX_PATH];
		TCHAR svnDir[MAX_PATH];

		PathCanonicalize(curDirCanon, curDir);

		if (GetScmBaseFolder(L".svn", curDirCanon, svnDir))
		{
			TCHAR curFile[MAX_PATH];
			TCHAR svnBaseFile[MAX_PATH];

			SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM)curFile);

			if (curFile[0] != 0)
			{
				if (GetSvnBaseFile(curDirCanon, svnDir, curFile, svnBaseFile))
				{
					openFile(svnBaseFile);
					return;
				}
			}
		}
	}
	MessageBox(nppData._nppHandle, L"Can't locate SVN information", L"ComparePlugin", MB_OK);
}

void compareGitBase()
{
	TCHAR curDir[MAX_PATH];

	SendMessage(nppData._nppHandle, NPPM_GETCURRENTDIRECTORY, 0, (LPARAM)curDir);

	if (curDir[0] != 0)
	{
		TCHAR curDirCanon[MAX_PATH];
		TCHAR gitDir[MAX_PATH];

		PathCanonicalize(curDirCanon, curDir);

		if (GetScmBaseFolder(L".git", curDirCanon, gitDir))
		{
			TCHAR curFile[MAX_PATH];

			SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM)curFile);

			if (curFile[0] != 0)
			{
				TCHAR gitBaseFile[MAX_PATH];

				GetLocalScmPath(curDir, gitDir, curFile, gitBaseFile);
	
				long size = 0;
				HGLOBAL hMem = GetContentFromGitRepo(gitDir, gitBaseFile, &size);
				if (size)
				{
					openMemBlock(hMem, size);
					GlobalFree(hMem);
					return;
				}
			}
		}
	}
	MessageBox(nppData._nppHandle, L"Can't locate GIT information", L"ComparePlugin", MB_OK);
}

int CProgress_IsCanceled_Callback()
{
	return (int)progDlg->IsCancelled();
}

void CProgress_Increment_Callback(int mid)
{
	if (mid > progMax)
	{
		progMax = mid;
	}
	if (progMax)
	{
		int perc = (++progCounter * 100) / (progMax * 4);
		progDlg->SetPercent(perc);
	}
}

bool compareNew()
{
	TCHAR filenameMain[MAX_PATH];
	TCHAR filenameSecond[MAX_PATH];
	TCHAR buffer[1024];

	clearWindow(nppData._scintillaMainHandle, true);
	clearWindow(nppData._scintillaSecondHandle, true);
	
	active = true;

	int doc1Length;
	int *lineNum1;

	char **doc1 = getAllLines(nppData._scintillaMainHandle, &doc1Length, &lineNum1);

    if (doc1Length < 1)
    {
        return true;
    }
    else if (doc1Length == 1)
    {
        if (SendMessage(nppData._scintillaMainHandle, SCI_LINELENGTH, 0, (LPARAM)0) == 0)
        {
            char *memblock[1] = { 0 };
            SendMessage(nppData._scintillaMainHandle, SCI_APPENDTEXT, 1, (LPARAM)memblock);
            SendMessage(nppData._scintillaMainHandle, SCI_SETSAVEPOINT, 0, (LPARAM)0);
        }
    }

	int doc2Length;
	int *lineNum2;

	char **doc2 = getAllLines(nppData._scintillaSecondHandle, &doc2Length, &lineNum2);
	
	if(doc2Length < 1)
		return true;

	int	doc1Changed = 0;
	int	doc2Changed = 0;
	diff_edit *doc1Changes = NULL;
	diff_edit *doc2Changes = NULL;

	unsigned int *doc1Hashes = computeHashes(doc1, doc1Length, Settings.IncludeSpace);
	unsigned int *doc2Hashes = computeHashes(doc2, doc2Length, Settings.IncludeSpace);

	/* show progress dialog */
	HWND hwnd = GetFocus();
	SetFocus(nppData._scintillaMainHandle);
	SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM)filenameMain);
	SetFocus(nppData._scintillaSecondHandle);
	SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM)filenameSecond);
	if (lstrlen(filenameMain) > 20)
	{
		lstrcpy(filenameMain + 17, L"...");
	}
	if (lstrlen(filenameSecond) > 20)
	{
		lstrcpy(filenameSecond + 17, L"...");
	}
	SetFocus(hwnd);
	wsprintf(buffer, L"Compare: '%s' vs. '%s'", filenameMain, filenameSecond);
	CProgress_IsCanceled = CProgress_IsCanceled_Callback;
	CProgress_Increment = CProgress_Increment_Callback;
	progMax = 0;
	progCounter = 0;
    progDlg = new CProgress();
	EnableWindow(nppData._nppHandle, FALSE);
    progDlg->Open(NULL, buffer);

	/* make diff */
	int sn;
	struct varray *ses = varray_new(sizeof(struct diff_edit), NULL);
	int result = (diff(doc1Hashes, 0, doc1Length, doc2Hashes, 0, doc2Length,
		(idx_fn)(getLineFromIndex), (cmp_fn)(compareLines), NULL, 0, ses, &sn, NULL));

	if (result != -1)
	{
		shift_boundries(ses, sn, doc1Hashes, doc2Hashes, doc1Length, doc2Length);
		find_moves(ses, sn, doc1Hashes, doc2Hashes, Settings.DetectMove);
		/*
		 * - insert empty lines
		 * - count changed lines
		 */
		doc1Changed = 0;
		doc2Changed = 0;

		for (int i = 0; i < sn; i++)
		{
			struct diff_edit *e = (diff_edit*)varray_get(ses, i);
			if (e->op == DIFF_DELETE)
			{
				e->changeCount = 0;
				doc1Changed += e->len;
				struct diff_edit *e2 = (diff_edit*)varray_get(ses, i + 1);
				e2->changeCount = 0;

				if (e2->op == DIFF_INSERT)
				{
					//see if the DELETE/INSERT COMBO includes changed lines or if its a completely new block
					if (compareWords(e, e2, doc1, doc2, Settings.IncludeSpace))
					{
						e->op = DIFF_CHANGE1;
						e2->op = DIFF_CHANGE2;
						doc2Changed += e2->len;
					}
				}
			}
			else if (e->op == DIFF_INSERT)
			{
				e->changeCount = 0;
				doc2Changed += e->len;
			}
		}
	}

	int doc1CurrentChange = 0;
	int doc2CurrentChange = 0;
	doc1Changes = new diff_edit[doc1Changed];
	for (int i = 0; i < doc1Changed; i++)
	{
		doc1Changes[i] = {};
	}
	doc2Changes = new diff_edit[doc2Changed];
	for (int i = 0; i < doc2Changed; i++)
	{
		doc2Changes[i] = {};
	}
	int doc1Offset = 0;
	int doc2Offset = 0;

	// Switch from blocks of lines to one change per line.
	// Change CHANGE to DELETE or INSERT if there are no changes on that line
	int added;

	if (!progDlg->IsCancelled())
	{
		for (int i = 0; i < sn; i++)
		{
			struct diff_edit *e = (diff_edit*)varray_get(ses, i);
			e->set = i;

			switch (e->op)
			{
			case DIFF_CHANGE1:
			case DIFF_DELETE:
				added = setDiffLines(e, doc1Changes, &doc1CurrentChange, DIFF_DELETE, e->off + doc2Offset);
				doc2Offset -= added;
				doc1Offset += added;
				break;
			case DIFF_INSERT:
			case DIFF_CHANGE2:
				added = setDiffLines(e, doc2Changes, &doc2CurrentChange, DIFF_INSERT, e->off + doc1Offset);
				doc1Offset -= added;
				doc2Offset += added;
				break;
			}
		}
	}

	if ((result != -1) && !progDlg->IsCancelled())
	{
		int textIndex;
		different = (doc1Changed > 0) || (doc2Changed > 0);

		for (int i = 0; i < doc1Changed; i++)
		{
			switch (doc1Changes[i].op)
			{
			case DIFF_DELETE:
				markAsRemoved(nppData._scintillaMainHandle, doc1Changes[i].off);
				break;

			case DIFF_CHANGE1:
				markAsChanged(nppData._scintillaMainHandle, doc1Changes[i].off);
				textIndex = lineNum1[doc1Changes[i].off];

				for (int k = 0; k < doc1Changes[i].changeCount; k++)
				{
					struct diff_change *change = (diff_change*)varray_get(doc1Changes[i].changes, k);
					markTextAsChanged(nppData._scintillaMainHandle, textIndex + change->off, change->len);
				}
				break;

			case DIFF_MOVE:
				markAsMoved(nppData._scintillaMainHandle, doc1Changes[i].off);
				break;

			}
		}

		for (int i = 0; i < doc2Changed; i++)
		{
			switch (doc2Changes[i].op)
			{
			case DIFF_INSERT:
				markAsAdded(nppData._scintillaSecondHandle, doc2Changes[i].off);
				break;

			case DIFF_CHANGE2:
				markAsChanged(nppData._scintillaSecondHandle, doc2Changes[i].off);
				textIndex = lineNum2[doc2Changes[i].off];
				for (int k = 0; k < doc2Changes[i].changeCount; k++)
				{
					struct diff_change *change = (diff_change*)varray_get(doc2Changes[i].changes, k);
					markTextAsChanged(nppData._scintillaSecondHandle, textIndex + change->off, change->len);
				}
				break;

			case DIFF_MOVE:
				markAsMoved(nppData._scintillaSecondHandle, doc2Changes[i].off);
				break;
			}
		}

		doc1Offset = 0;
		doc2Offset = 0;

		if (Settings.AddLine)
		{
			int length = 0;
			int off = -1;
			for (int i = 0; i < doc1Changed; i++)
			{
				switch (doc1Changes[i].op)
				{
				case DIFF_DELETE:
				case DIFF_MOVE:
					if (doc1Changes[i].altLocation == off)
					{
						length++;
					}
					else
					{
						addEmptyLines(nppData._scintillaSecondHandle, off + doc2Offset, length);
						doc2Offset += length;
						off = doc1Changes[i].altLocation;
						length = 1;
					}
					break;
				}
			}

			addEmptyLines(nppData._scintillaSecondHandle, off + doc2Offset, length);

			if (doc2Offset > 0)
			{
				clearUndoBuffer(nppData._scintillaSecondHandle);
			}

			length = 0;
			off = 0;
			doc1Offset = 0;

			for (int i = 0; i < doc2Changed; i++)
			{
				switch (doc2Changes[i].op)
				{
				case DIFF_INSERT:
				case DIFF_MOVE:
					if (doc2Changes[i].altLocation == off)
					{
						length++;
					}
					else
					{
						addEmptyLines(nppData._scintillaMainHandle, off + doc1Offset, length);
						doc1Offset += length;
						off = doc2Changes[i].altLocation;
						length = 1;
					}
					break;
				}
			}

    		addEmptyLines(nppData._scintillaMainHandle, off + doc1Offset, length);

			if (doc1Offset > 0)
			{
				clearUndoBuffer(nppData._scintillaMainHandle);
			}
		}
	}

//clean up resources
#if CLEANUP

	bool compareCanceled = progDlg->IsCancelled();
	progDlg->Close();
	delete progDlg;
	EnableWindow(nppData._nppHandle, TRUE);
	SetForegroundWindow(nppData._nppHandle);
	SetFocus(nppData._nppHandle);

	if (result != -1)
	{
		for (int i = 0; i < doc1Length; i++)
		{
			if (*doc1[i] != 0)
			{
				delete[] doc1[i];
			}
		}

		delete[] doc1;
		delete[] lineNum1;

		for (int i = 0; i < doc2Length; i++)
		{
			if (*doc2[i] != 0)
			{
				delete[] doc2[i];
			}
		}

		delete[] doc2;
		delete lineNum2;

		delete[] doc1Hashes;
		delete[] doc2Hashes;

		clearEdits(ses, sn);

		for (int i = 0; i < doc1Changed; i++)
		{
			clearEdit(doc1Changes + (i));
		}

		delete[] doc1Changes;

		for (int i = 0; i < doc2Changed; i++)
		{
			clearEdit(doc2Changes + (i));
		}

		delete[] doc2Changes;
	}
#endif // CLEANUP

	if (compareCanceled)
	{
		return true;
	}

	if (result != -1)
	{
		if(!different)
		{
			::MessageBox(nppData._nppHandle, TEXT("Files Match"), TEXT("Results :"), MB_OK);
			return true;
		}

        ::SendMessageA(nppData._scintillaMainHandle, SCI_SHOWLINES, 0, (LPARAM)1);
        ::SendMessageA(nppData._scintillaSecondHandle, SCI_SHOWLINES, 0, (LPARAM)1);
        First();

		return false;
	}
	return false;
}

void activateBufferID(LRESULT bufferID, int view)
{
	LRESULT index = SendMessage(nppData._nppHandle, NPPM_GETPOSFROMBUFFERID, bufferID, view);
	index = index & 0x3FFFFFFF;
	
	SendMessage(nppData._nppHandle, NPPM_ACTIVATEDOC, view, index);
}

bool startCompare()
{
	LRESULT RODoc1;
	LRESULT RODoc2;

	if(!IsWindowVisible(nppData._scintillaMainHandle) || !IsWindowVisible(nppData._scintillaSecondHandle))	// Yaron - One-View mode.
	{
		skipAutoReset = true;

		// Yaron - In One-View mode, the current view can be 0 or 1.
		// In Two-Views mode, the top (or left) view is ALWAYS 0, and the bottom view is ALWAYS 1.
		int currentView = SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0);
		LRESULT bufferID = SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);

		// Yaron - If the current view is 0, we want the prev file to be moved to the bottom; if the current view is 1, we still need to activate it and make sure it's compared to the current file.
		SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_TAB_PREV, 0);

		if(currentView == 1)
		{
			bufferID = SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
			SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_TAB_NEXT, 0);		// Yaron - Switch back to current file.
		}

		SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_GOTO_ANOTHER_VIEW, 0);		// Yaron - Current file is ALWAYS at top, and prev file is ALWAYS at bottom.
		
		activateBufferID(bufferID, currentView);		// Yaron - If the current view is 0, activate current file at top; if the current view is 1, activate prev file at bottom (possibly multiple files there).

		if (currentView == 1)
			SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_SWITCHTO_OTHER_VIEW, 0);       // Yaron - Activate current file at top.

		skipAutoReset = false;
		panelsOpened = true;
	}

	if(!IsWindowVisible(nppData._scintillaMainHandle) || !IsWindowVisible(nppData._scintillaSecondHandle))
	{	
		panelsOpened = false;
		::MessageBox(nppData._nppHandle, TEXT("Nothing to compare!"), TEXT("Error"), MB_OK);
		return true;
	}

	if(!notepadVersionOk)
	{
		int version = ::SendMessage(nppData._nppHandle,NPPM_GETNPPVERSION, 0, 0);
		if(version > 0)
		{
			notepadVersionOk = true;
		}
	}

	if(Settings.AddLine && !notepadVersionOk)
	{
		::MessageBox(nppData._nppHandle, TEXT("Notepad v4.5 or higher is required to line up matches. This feature will be turned off"), TEXT("Incorrect Version"), MB_OK);
		Settings.AddLine = false;
		::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_ALIGN_MATCHES]._cmdID, (LPARAM)Settings.AddLine);
	}

	// Remove read-only attribute
	if ((RODoc1 = SendMessage(nppData._scintillaMainHandle, SCI_GETREADONLY, 0, 0)) == 1)
		SendMessage(nppData._scintillaMainHandle, SCI_SETREADONLY, false, 0);

	if ((RODoc2 = SendMessage(nppData._scintillaSecondHandle, SCI_GETREADONLY, 0, 0)) == 1)
		SendMessage(nppData._scintillaSecondHandle, SCI_SETREADONLY, false, 0);

	setStyles(&Settings);

	int doc1 = SendMessageA(nppData._scintillaMainHandle, SCI_GETDOCPOINTER, 0, 0);
	int doc2 = SendMessageA(nppData._scintillaSecondHandle, SCI_GETDOCPOINTER, 0, 0);

	setCompare(doc1);
	setCompare(doc2);

	/* sync pannels */
	HMENU hMenu = ::GetMenu(nppData._nppHandle);

    syncScrollVwasChecked = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLV, MF_BYCOMMAND) & MF_CHECKED) != 0;
    syncScrollHwasChecked = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLH, MF_BYCOMMAND) & MF_CHECKED) != 0;
    if (syncScrollVwasChecked)
        ::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLV, 0), 0);
    if (syncScrollHwasChecked)
        ::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLH, 0), 0);

	// let the second view inherit the zoom level of the main view
	int mainZoomLevel = SendMessage(nppData._scintillaMainHandle, SCI_GETZOOM, 0, 0);
	SendMessage(nppData._scintillaSecondHandle, SCI_SETZOOM, mainZoomLevel, 0);

	::SendMessageA(nppData._scintillaMainHandle, SCI_GOTOPOS, 1, 0);
	::SendMessageA(nppData._scintillaSecondHandle, SCI_GOTOPOS, 1, 0);
	::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLH, 0), 0);
	::SendMessageA(nppData._scintillaMainHandle, SCI_SETUNDOCOLLECTION, FALSE, 0);
	::SendMessageA(nppData._scintillaSecondHandle, SCI_SETUNDOCOLLECTION, FALSE, 0);

	// Compare files (return False if files differs)
	bool result = compareNew();

	::SendMessageA(nppData._scintillaMainHandle, SCI_SETUNDOCOLLECTION, TRUE, 0);
	::SendMessageA(nppData._scintillaSecondHandle, SCI_SETUNDOCOLLECTION, TRUE, 0);

	// Restore previous read-only attribute
	if (RODoc1 == 1)
		SendMessage(nppData._scintillaMainHandle, SCI_SETREADONLY, true, 0);

	if (RODoc2 == 1)
		SendMessage(nppData._scintillaSecondHandle, SCI_SETREADONLY, true, 0);

	if (!result)
	{
		if(Settings.UseNavBar)
		{
			// Save current N++ focus
			HWND hwnd = GetFocus();

			// Configure NavBar
			NavDlg.SetColor(
				Settings.ColorSettings.added, 
				Settings.ColorSettings.deleted, 
				Settings.ColorSettings.changed, 
				Settings.ColorSettings.moved,
                Settings.ColorSettings.blank,
                Settings.ColorSettings._default);

			// Display Navbar
			NavDlg.doDialog(true);
			start_old = -1;
            visible_line_count_old = -1;

			// Restore N++ focus
			SetFocus(hwnd);

		}
		// Enable Prev/Next menu entry
		::EnableMenuItem(hMenu, funcItem[CMD_PREV]._cmdID,  MF_BYCOMMAND | MF_ENABLED);
		::EnableMenuItem(hMenu, funcItem[CMD_NEXT]._cmdID, MF_BYCOMMAND | MF_ENABLED);
		::EnableMenuItem(hMenu, funcItem[CMD_FIRST]._cmdID, MF_BYCOMMAND | MF_ENABLED);
		::EnableMenuItem(hMenu, funcItem[CMD_LAST]._cmdID, MF_BYCOMMAND | MF_ENABLED);
	}

	return result;
}

void compare()
{
	bool ret = startCompare();

	// Files match - exit comparison
	if (ret)
		reset();
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	switch (notifyCode->nmhdr.code) 
	{
	case SCN_PAINTED:
		{
			if(active) 
			{   
                // update nav bar if npp views got scrolled, resized, etc..
                long start, visible_line_count;

				start = SendMessage(nppData._scintillaMainHandle, SCI_GETFIRSTVISIBLELINE, 0, 0);
                visible_line_count = max(
                    SendMessage(nppData._scintillaMainHandle, SCI_GETLINECOUNT, 0, 0),
                    SendMessage(nppData._scintillaSecondHandle, SCI_GETLINECOUNT, 0, 0));
                visible_line_count = SendMessage(nppData._scintillaMainHandle, SCI_VISIBLEFROMDOCLINE, visible_line_count, 0);

                if ((NavDlg.ReadyToDraw == TRUE) && ((start != start_old) || (visible_line_count != visible_line_count_old)))
				{
                    NavDlg.DrawView();
					start_old = start;
                    visible_line_count_old = visible_line_count;
				}
			}
			break;
		}

	case NPPN_TBMODIFICATION:
		{         
            tbNext.hToolbarBmp = (HBITMAP)::LoadImage(hInstance,
													  MAKEINTRESOURCE(IDB_NEXT),
													  IMAGE_BITMAP,
													  0, 0,
													  (LR_LOADTRANSPARENT | LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));

            tbPrev.hToolbarBmp = (HBITMAP)::LoadImage(hInstance,
													  MAKEINTRESOURCE(IDB_PREV),
													  IMAGE_BITMAP,
													  0, 0,
													  (LR_LOADTRANSPARENT | LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));

            tbFirst.hToolbarBmp = (HBITMAP)::LoadImage(hInstance,
													   MAKEINTRESOURCE(IDB_FIRST),
													   IMAGE_BITMAP,
													   0, 0,
													   (LR_LOADTRANSPARENT | LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));

            tbLast.hToolbarBmp = (HBITMAP)::LoadImage(hInstance,
													  MAKEINTRESOURCE(IDB_LAST),
													  IMAGE_BITMAP,
													  0, 0,
													  (LR_LOADTRANSPARENT | LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));

			::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_FIRST]._cmdID, (LPARAM)&tbFirst);
			::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_PREV]._cmdID, (LPARAM)&tbPrev);
			::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_NEXT]._cmdID, (LPARAM)&tbNext);
			::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[CMD_LAST]._cmdID, (LPARAM)&tbLast);

			HMENU hMenu = ::GetMenu(nppData._nppHandle);
			::EnableMenuItem(hMenu, funcItem[CMD_PREV]._cmdID,  MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			::EnableMenuItem(hMenu, funcItem[CMD_NEXT]._cmdID,  MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			::EnableMenuItem(hMenu, funcItem[CMD_FIRST]._cmdID, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			::EnableMenuItem(hMenu, funcItem[CMD_LAST]._cmdID,  MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

			::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_ALIGN_MATCHES]._cmdID, (LPARAM)Settings.AddLine);
			::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_IGNORE_SPACING]._cmdID, (LPARAM)Settings.IncludeSpace);
			::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_DETECT_MOVES]._cmdID, (LPARAM)Settings.DetectMove);
			::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_USE_NAV_BAR]._cmdID, (LPARAM)Settings.UseNavBar);

			break;
		}

	case SCN_UPDATEUI:
		{
			if (active)
			{
				HWND activeView = NULL;
				HWND otherView = NULL;
				if (notifyCode->updated & (SC_UPDATE_SELECTION | SC_UPDATE_V_SCROLL))
				{
					int currentEdit = -1;
					if (notifyCode->updated & SC_UPDATE_SELECTION)
					{
						::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
					}
					else if (notifyCode->updated & SC_UPDATE_V_SCROLL)
					{
						if (notifyCode->nmhdr.hwndFrom == nppData._scintillaMainHandle)
						{
							currentEdit = 0;
						}
						else if (notifyCode->nmhdr.hwndFrom == nppData._scintillaSecondHandle)
						{
							currentEdit = 1;
						}
					}
					if (currentEdit != -1)
					{
						activeView = (currentEdit == 0) ? (nppData._scintillaMainHandle) : (nppData._scintillaSecondHandle);
						otherView = (currentEdit == 0) ? (nppData._scintillaSecondHandle) : (nppData._scintillaMainHandle);
					}
				}
				if ((activeView != NULL) && (otherView != NULL))
				{
					int activeViewTopLine = ::SendMessage(activeView, SCI_GETFIRSTVISIBLELINE, 0, 0);
					activeViewTopLine = ::SendMessage(activeView, SCI_DOCLINEFROMVISIBLE, activeViewTopLine, 0);
					int otherViewTopLine = ::SendMessage(otherView, SCI_VISIBLEFROMDOCLINE, activeViewTopLine, 0);
					::SendMessage(otherView, SCI_SETFIRSTVISIBLELINE, otherViewTopLine, 0);
				}
			}
			break;
		}

	case NPPN_FILEBEFORECLOSE:
		{
			if (!skipAutoReset)
			{
				notepadVersionOk = true;
				HWND window = getCurrentWindow();
				int win = SendMessageA(window, SCI_GETDOCPOINTER, 0, 0);
				if (getCompare(win) != -1)
				{
					closingWin = win;
					closingView = (HWND)notifyCode->nmhdr.hwndFrom;
					reset();
				}
			}
			break;
		}

	case NPPN_FILECLOSED:
	case NPPN_FILEBEFOREOPEN:
	case NPPN_FILEOPENED:
		{
			notepadVersionOk = true;
			break;
		}

	case NPPN_FILEBEFORESAVE:
		{
			notepadVersionOk = true;

			SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)emptyLinesDoc);

			HWND window = getCurrentWindow();						
			int win = SendMessageA(window, SCI_GETDOCPOINTER, 0,0);

			if(getCompare(win)!=-1)
			{				
				topLine = SendMessageA(window,SCI_GETFIRSTVISIBLELINE,0,0);
				lastEmptyLines = removeEmptyLines(window,true);
			}
			else
			{
				lastEmptyLines=NULL;
			}
			break;
		}

	case NPPN_FILESAVED:
		{
			notepadVersionOk = true;
			TCHAR name[MAX_PATH];
			SendMessage(nppData._nppHandle,NPPM_GETFULLCURRENTPATH,0,(LPARAM)name);

			if(lastEmptyLines != NULL && lstrcmp(name, emptyLinesDoc) == 0)
			{
				HWND window = getCurrentWindow();
				::addBlankLines(window,lastEmptyLines);
				int linesOnScreen = SendMessageA(window,SCI_LINESONSCREEN,0,0);
				int curPosBeg = ::SendMessageA(window, SCI_GETSELECTIONSTART, 0, 0);
				int curPosEnd = ::SendMessageA(window, SCI_GETSELECTIONEND, 0, 0);
				SendMessageA(window,SCI_GOTOLINE,topLine,0);
				SendMessageA(window,SCI_GOTOLINE,topLine+linesOnScreen-1,0);
				SendMessageA(window, SCI_SETSEL, curPosBeg, curPosEnd);
				cleanEmptyLines(lastEmptyLines);
				delete lastEmptyLines;
				lastEmptyLines = NULL;
				emptyLinesDoc[0] = 0;
			}
			break;
		}

	case NPPN_SHUTDOWN:
		{
			// Always close it, else N++'s plugin manager would call 'ViewNavigationBar'
			// on startup, when N++ has been shut down before with opened navigation bar
			if (NavDlg.isVisible())
				NavDlg.doDialog(false);
			break;
		}
	}
}
