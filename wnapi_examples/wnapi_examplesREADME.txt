Notes about WinAPI cpp apps: -                      [251227]

In Windows OS with MSYS2 / MinGW  installed, allows me to develop in a
familiar Linux environment, where I use the msys2 terminals with vim and
compile g++ in the usual way. No need to use Visual Studio. I have the most
basic toolchain installed at present.

$ pacman -Qe
base 2022.06-1
base-devel 2024.11-1
filesystem 2025.05.08-2
git 2.52.0-2
mingw-w64-ucrt-x86_64-gcc 15.2.0-8
msys2-runtime 3.6.5-1
vim 9.1.1914-1

                     * * * * * # # # # || # # # # * * * * * 

To search the huge stack of windows header files: -
(1) Add the following function to your ~/.bashrc file : -

wnhblock() {
    grep -in "$1" _mingw.h _mingw_mac.h _mingw_secapi.h _cygwin.h stddef.h sdks/_mingw_ddk.h sdkddkver.h winresrc.h winuser.rh commctrl.rh dde.rh winnt.rh excpt.h stdarg.h _mingw_stdarg.h vadefs.h windef.h winapifamily.h minwindef.h specstrings.h winnt.h ctype.h apiset.h psdk_inc/intrin-impl.h basetsd.h pshpack4.h pshpack2.h string.h ktmtypes.h winbase.h _mingw_unicode.h apisetcconv.h winapifamily.h minwinbase.h bemapiset.h debugapi.h errhandlingapi.h fibersapi.h fileapi.h handleapi.h heapapi.h ioapiset.h interlockedapi.h jobapi.h libloaderapi.h memoryapi.h namedpipeapi.h namespaceapi.h processenv.h processthreadsapi.h processtopologyapi.h profileapi.h realtimeapiset.h securityappcontainer.h securitybaseapi.h synchapi.h sysinfoapi.h systemtopologyapi.h threadpoolapiset.h threadpoollegacyapiset.h utilapiset.h wow64apiset.h wingdi.h winuser.h guiddef.h winnls.h datetimeapi.h wincon.h wincontypes.h consoleapi.h consoleapi2.h consoleapi3.h winver.h winreg.h winnetwk.h wnnc.h virtdisk.h cderr.h dde.h ddeml.h dlgs.h lzexpand.h mmsystem.h mmsyscom.h pshpack1.h mciapi.h mmiscapi.h mmiscapi2.h playsoundapi.h mmeapi.h timeapi.h joystickapi.h poppack.h nb30.h rpc.h rpcdce.h rpcnsi.h rpcnterr.h winerror.h fltwinerror.h rpcasync.h shellapi.h winperf.h pshpack8.h winsock.h _timeval.h _bsd_types.h inaddr.h psdk_inc/_socket_types.h psdk_inc/_fd_types.h psdk_inc/_ip_types.h psdk_inc/_ip_mreq1.h psdk_inc/_wsadata.h psdk_inc/_xmitfile.h psdk_inc/_wsa_errnos.h wincrypt.h winefs.h winscard.h wtypes.h rpc.h rpcndr.h wtypesbase.h winioctl.h winsmcrd.h SCardErr.h winspool.h prsht.h ole.h ole2.h objbase.h combaseapi.h objidl.h urlmon.h propidl.h oleauto.h oleidl.h commdlg.h stralign.h sec_api/stralign_s.h ole2.h winsvc.h mcx.h imm.h
}

(2)  cd into the include folder: -   cd /ucrt64/include

(3) then just use: -  $ wnhblock "something"
       to search for 'something' in all those files.


                     * * * * * # # # # || # # # # * * * * * 

Example of a vim color file, where you can set the colors how you want them.
This example honors the background color you have set for your msys2 terminal
(in my case a very dark blue). In this directory: -  /usr/share/vim/vim91/colors
(you will also need to add   colo leon  near the end of your   .vimrc   file)

" leon.vim | 
" Vim Test - copied from default.vim and modified
" Author:	originally The Vim Project <https://github.com/vim/vim>
" Last Change:	250717

set background=dark

let g:colors_name = 'leon'

" Remove all existing highlighting and set the defaults.
hi clear

hi! link String Title

let s:t_Co = has('gui_running') ? -1 : (&t_Co ?? 0)

hi Normal guifg=#afaf5f guibg=NONE gui=NONE cterm=NONE
hi Comment guifg=#00afd7 guibg=NONE gui=NONE cterm=NONE
hi Title guifg=#b2b2b2 guibg=NONE gui=NONE cterm=NONE
hi Constant guifg=#ff5f5f guibg=NONE gui=NONE cterm=NONE
hi Statement guifg=#d7ff00 guibg=NONE gui=NONE cterm=NONE
hi Type guifg=#00ff00 guibg=NONE gui=NONE cterm=NONE
hi PreProc guifg=#ff5fd7 guibg=NONE gui=NONE cterm=NONE
hi Special guifg=#ffaf00 guibg=NONE gui=NONE cterm=NONE
hi Identifier guifg=#00ffaf guibg=NONE gui=NONE cterm=NONE

if s:t_Co >= 256
  hi Normal ctermfg=143 ctermbg=NONE cterm=NONE
  hi Comment ctermfg=38 ctermbg=NONE cterm=NONE
  hi Title ctermfg=249 ctermbg=NONE cterm=NONE
  hi Constant ctermfg=203 ctermbg=NONE cterm=NONE
  hi Statement ctermfg=190 ctermbg=NONE cterm=NONE
  hi Type ctermfg=46 ctermbg=NONE cterm=NONE
  hi PreProc ctermfg=206 ctermbg=NONE cterm=NONE
  hi Special ctermfg=214 ctermbg=NONE cterm=NONE
  hi Identifier ctermfg=49 ctermbg=NONE cterm=NONE
  unlet s:t_Co
  finish
endif

" Color: DarkKhaki          #afaf5f  143  foreground Normal
" Color: DeepSkyBlue2       #00afd7   38  Comment
" Color: Grey70             #b2b2b2  249  String
" Color: IndianRed1         #ff5f5f  203  Number Constant
" Color: Yellow2            #d7ff00  190  Keyword Statement Operator
" Color: Green1             #00ff00   46  Type
" Color: HotPink            #ff5fd7  206  PreProc
" Color: Orange1            #ffaf00  214  Special
" Color: MediumSpringGreen  #00ffaf   49  Identifier

" To show 256 colors, run this in a terminal: -
" for COLOR in {1..255}; do echo -en "\\e[38;5;${COLOR}m${COLOR} "; done; echo;
"
" Website which lists 256 colors with their #rrggbb values: -
"https://ss64.com/bash/syntax-colors.html

" vim: sw=2
"hi! link Terminal Normal
"hi! link CursorColumn CursorLine
"hi! link CursorLineNr CursorLine
"hi! link CursorIM Cursor
"hi! link LineNrAbove LineNr
"hi! link LineNrBelow LineNr
"hi! link Debug Special
"hi! link Added String
"hi! link Removed WarningMsg
"hi! link diffOnly WarningMsg
"hi! link diffNoEOL WarningMsg
"hi! link diffIsA WarningMsg
"hi! link diffIdentical WarningMsg
"hi! link diffDiffer WarningMsg
"hi! link diffCommon WarningMsg
"hi! link diffBDiffer WarningMsg
"hi! link lCursor Cursor
"hi! link CurSearch Search
"hi! link CursorLineFold CursorLine
"hi! link CursorLineSign CursorLine
"hi! link Boolean Constant
"hi! link Character Constant
"hi! link Conditional Statement
"hi! link Define PreProc
"hi! link Delimiter Special
"hi! link Exception Statement
"hi! link Float Constant
"hi! link Function Identifier
"hi! link Include PreProc
"hi! link Keyword Statement
"hi! link Label Statement
"hi! link Macro PreProc
"hi! link Number Constant
"hi! link Operator Statement
"hi! link PreCondit PreProc
"hi! link Repeat Statement
"hi! link SpecialChar Special
"hi! link SpecialComment Special
"hi! link StorageClass Type
"hi! link String Constant
"hi! link Structure Type
"hi! link Tag Special
"hi! link Typedef Type
"hi! link MessageWindow Pmenu
"hi! link PopupNotification Todo
"hi! link VertSplit StatusLineNC
"hi! link PopupSelected PmenuSel
"hi! link StatusLineTerm StatusLine
"hi! link StatusLineTermNC StatusLineNC
"hi! link TabLineFill TabLine
"
"ColorColumn',
"Comment',
"Conceal',
"Constant',
"CurSearch',
"Cursor',
"CursorColumn',
"CursorLine',
"CursorLineNr',
"CursorLineFold',
"CursorLineSign',
"DiffAdd',
"DiffChange',
"DiffDelete',
"DiffText',
"Directory',
"EndOfBuffer',
"Error',
"ErrorMsg',
"FoldColumn',
"Folded',
"Identifier',
"Ignore',
"IncSearch',
"LineNr',
"LineNrAbove',
"LineNrBelow',
"MatchParen',
"ModeMsg',
"MoreMsg',
"NonText',
"Normal',
"Pmenu',
"PmenuSbar',
"PmenuSel',
"PmenuThumb',
"PreProc',
"Question',
"QuickFixLine',
"Search',
"SignColumn',
"Special',
"SpecialKey',
"SpellBad',
"SpellCap',
"SpellLocal',
"SpellRare',
"Statement',
"StatusLine',
"StatusLineNC',
"StatusLineTerm',
"StatusLineTermNC',
"TabLine',
"TabLineFill',
"TabLineSel',
"Title',
"Todo',
"ToolbarButton',
"ToolbarLine'
"Type',
"Underlined',
"VertSplit',
"Visual',
"VisualNOS',
"WarningMsg',
"WildMenu',
"debugPC',
"debugBreakpoint',
