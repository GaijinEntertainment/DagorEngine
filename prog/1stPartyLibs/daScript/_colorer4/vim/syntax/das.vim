" Vim syntax file
" Language:	daScript
" Maintainer:	Alexander Polyakov <nolokor@gmail.com>
" Last Change:	2019 Jan 31

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" daScript keywords
syn keyword dasType		auto bool void string
syn keyword dasType		int int64 int2 int3 int4 uint uint64 uint2 uint3 uint4
syn keyword dasType		float float2 float3 float4
syn keyword dasType		array table
syn keyword dasBoolean		true false
syn keyword dasStructure  struct typename type
syn keyword dasStatement  def let
syn keyword dasStatement  break return
syn keyword dasStatement  assert
syn keyword dasStatement  new
syn keyword dasPreCondit  require
syn keyword dasRepeat for while
syn keyword dasBuiltin range
syn keyword dasConditional	if elif else
syn keyword	dasStorageClass	const
syn keyword dasExceptions	throw try catch expect
syn keyword dasOperator	in sizeof
syn keyword dasConstant null

syn case ignore
syn match	  dasNumber		"\<0x\x\+\(u\=l\{0,2}\|ll\=u\)\>"
syn match	  dasNumber		"\<\d\+\(u\=l\{0,2}\|ll\=u\)\>"

syn match	  dasFloat		"\<\d\+f"
syn match	  dasFloat		"\<\d\+\.\d*\(e[-+]\=\d\+\)\=[fl]\="
syn match	  dasFloat		"\<\.\d\+\(e[-+]\=\d\+\)\=[fl]\=\>"
syn match	  dasFloat		"\<\d\+e[-+]\=\d\+[fl]\=\>"
syn match	  dasFloat		"\<0x\x*\.\x\+p[-+]\=\d\+[fl]\=\>"
syn match	  dasFloat		"\<0x\x\+\.\=p[-+]\=\d\+[fl]\=\>"
syn case match

syn match	  dasSpecial	display contained "\\\(x\x\+\|\o\{1,3}\|.\|$\)"
syn region	dasString		start=+L\="+ skip=+\\\\\|\\"+ end=+"+ contains=dasSpecial,@Spell

" Default highlighting
hi def link dasType		Type
hi def link dasBoolean		Boolean
hi def link dasStatement  Statement
hi def link dasStructure  Structure
hi def link dasPreCondit  PreCondit
hi def link dasStorageClass	StorageClass
hi def link dasExceptions	Exception
hi def link dasRepeat Repeat
hi def link dasOperator Operator
hi def link dasNumber Number
hi def link dasFloat Float
hi def link dasString		String
hi def link dasConditional		Conditional
hi def link dasConstant Constant
hi def link dasBuiltin  Function

let b:current_syntax = "das"

" vim: ts=8
