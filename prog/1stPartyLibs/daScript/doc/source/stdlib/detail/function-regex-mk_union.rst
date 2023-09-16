union(union(x),union(y)) = union(xy)
union(union(x),y) = union(xy)
union(x,union(y)) = union(xy)
<RE>	::=	<union> | <simple-RE>
<union>	::=	<RE> "|" <simple-RE>
<simple-RE>	::=	<concatenation> | <basic-RE>
<concatenation>	::=	<simple-RE> <basic-RE>
