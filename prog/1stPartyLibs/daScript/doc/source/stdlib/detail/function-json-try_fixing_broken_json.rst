fixes broken json. so far supported
1. "string" + "string" string concatination
2. "text "nested text" text" nested quotes
3. extra , at the end of object or array
4. /uXXXXXX sequences in the middle of white space
write until beginning of string or end
if eof we done
write the first quote
write until end of string or end of file
if eof we done, we fix close quote and we are done
skipping whitespace
if eof we done, we fix close quote and we are done
valid JSON things after string are :, }, ], or , if its one of those, we close the string and we are done
now, if its a + - it could be a string concatenation
if it is indeed new string, we go back to string writing, and add a separator
ok, its not a string concatination, or a valid character
if it was a whitespace after the quote, we assume its missing ','
. we assume its nested quotes and we replace with \"
if eof we done, we fix nested quote and and closing quote and we are done
write the second replaced quote
