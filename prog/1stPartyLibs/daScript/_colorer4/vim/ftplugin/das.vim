" Vim filetype plugin file
" Language:         daScript
" Maintainer:       Alexander Polyakov <nolokor@gmail.com>
" Latest Revision:  2019-01-31

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

let b:undo_ftplugin = "setl com< cms< fo<"

setlocal comments=:// commentstring=//\ %s formatoptions-=t formatoptions+=croql

let &cpo = s:cpo_save
unlet s:cpo_save
