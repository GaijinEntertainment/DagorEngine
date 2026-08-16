options gen2

expect 30297 : 2

typedef distinct Id = int
typedef distinct Id2 = Id           // 30297: distinct of distinct
typedef distinct Bad = array<int>   // 30297: non-workhorse underlying
