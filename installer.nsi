
!include "MUI2.nsh"

Name "miRack"
OutFile "Rack-setup.exe"
SetCompressor "bzip2"
CRCCheck On

;Default installation folder
InstallDir "$PROGRAMFILES\miRack"

;Get installation folder from registry if available
InstallDirRegKey HKCU "Software\miRack" ""

;Request application privileges for Windows Vista
RequestExecutionLevel admin



!define MUI_ICON "icon.ico"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "installer-banner.bmp" ; 150x57
; !define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\win.bmp" ; 164x314
; !define MUI_UNWELCOMEFINISHPAGE_BITMAP  "${NSISDIR}\Contrib\Graphics\Wizard\win.bmp" ; 164x314
!define MUI_COMPONENTSPAGE_NODESC


; Pages

; !insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY

;second directory selection
; Var DbInstDir
; !define MUI_PAGE_HEADER_SUBTEXT "Choose the folder in which to install the database."
; !define MUI_DIRECTORYPAGE_TEXT_TOP "The installer will install the database(s) in the following folder. To install in a differenct folder, click Browse and select another folder. Click Next to continue."
; !define MUI_DIRECTORYPAGE_VARIABLE $DbInstDir ; <= the other directory will be stored into that variable
; !insertmacro MUI_PAGE_DIRECTORY

!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"



Section "!miRack" MIRACK
	SetOutPath "$INSTDIR"

	File /r "dist\Rack"

	;Store installation folder
	WriteRegStr HKCU "Software\miRack" "" $INSTDIR
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\miRack" "DisplayName" "miRack"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\miRack" "UninstallString" "$\"$INSTDIR\UninstallRack.exe$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\miRack" "QuietUninstallString" "$\"$INSTDIR\UninstallRack.exe$\" /S"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\miRack" "InstallLocation" "$\"$INSTDIR$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\miRack" "Publisher" "mifki"

	;Create uninstaller
	WriteUninstaller "$INSTDIR\UninstallRack.exe"

	;Create shortcuts
	CreateDirectory "$SMPROGRAMS"
	; Set working directory of shortcut
	SetOutPath "$INSTDIR\Rack"
	CreateShortcut "$SMPROGRAMS\miRack.lnk" "$INSTDIR\Rack\Rack.exe"
SectionEnd


; Section "VST Plugin" VST
; SectionEnd


Section "Uninstall"
	RMDir /r "$INSTDIR\Rack"
	Delete "$INSTDIR\UninstallRack.exe"
	RMDir "$INSTDIR"

	Delete "$SMPROGRAMS\miRack.lnk"

	DeleteRegKey /ifempty HKCU "Software\miRack"
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\miRack"
SectionEnd
