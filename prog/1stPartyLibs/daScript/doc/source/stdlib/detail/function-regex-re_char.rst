<set-items>	::=	<set-item> | <set-item> <set-items>
<set-item>	::=	<range> | <char>
<range>	::=	<char> "-" <char>
<char>	::=	any non metacharacter | "\" metacharacter
