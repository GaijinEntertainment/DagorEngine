# Welcome to Quirrel!

## Why Quirrel?

### Features
Quirrel is a script language that is based on Squirrel language and is inspired by Python, Javascript and especially Lua (The API is very similar and the table code is based on the Lua one). Whole [syntax and documentation](http://quirrel.io/doc/reference/language.html) can be read in an approximately an hour, and it looks almost familiar if you know Javascript or C++.

Quirrel offers a wide range of features like:

* Lexical scoping
* Higher order functions, closures and lambda
* Classes and inheritance
* Tail recursion
* Delegation
* String interpolation
* Exception handling
* Cooperative threads (coroutines)
* Generators
* Performance - Quirrel is fast and small (comparable with Lua and sometimes even faster)
* Both compiler and virtual machine fit together in about 13k lines of C++ code and add only around 100kb-150kb the executable size.
* Automatic memory management (CPU bursts free - reference counting and GC)
* Optional 16bits characters strings
* Dynamic typing [type system](http://quirrel.io/doc/reference/language/datatypes.html) is close to Javascript but stricter and simpler (there is **integer** type and **no 'undefined'**)
* Powerful embedding api
* Modules
* Hot-reload (if you implement it in your code)
* Open Source MIT License

### Comparison with other scripting languages
If you need high-level multi-paradigm (imperative, functional, object-oriented) programming language for your game or another app there is a lot of options.
You usually want something to be light-weight in size, memory bandwidth and not unreasonably slower than your native code. Predictable memory usage and performance is important. Also you want minimum possible friction on using and embedding. And of course it would be great to have a language that you and your teammates already familiar with.
And you want to use something stable and proven - you do not want to debug rare bug in VM, you want language for it's agility not for computer science puzzles.
In video games JiT compiled code may not be an option, because on most platforms it is simply impossible.
So basically language (and it's VM) should be: no-JiT (at least have interpreter), high-level, easy-to-use, small embeddable language, some concurrency model is also usually required or good to have (usually it is coroutines or fibers), better to be language with familiar syntax and widely used or at least battle-proven. Here are some brief comparison of **Quirrel** with other popular embedded languages used in video-games fitting those criterias:

#### Lua
This is the most popular language for the video games.
It has different syntax (which can be good or bad for someone).

**Good**:
* Huge amount of libraries.
* Relatively big and active community.
* Battle-proven, lots of tools.

**Different**:
* Assigning nil to a slot removes it.
* Type system significantly differs from C\C++ (Numeric, Boolean, Table, String Function, Userdata, Thread, and Nil are only possible types). So it requires more type conversions and typechecks in some trivial cases than you may want to.
* No arrays - can be easier to learn for newbies, but brings a whole new class of bugs and difficulties. Sparse arrays if you are iterating only integer keys in table as array.
* Indexes from 1 (not definitely bad, but obviously bad for embedding in C\C++ and other languages that have different indexes). Also see https://www.cs.utexas.edu/users/EWD/transcriptions/EWD08xx/EWD831.html
* No continue statement, no switch statement.
* No classes. OOP can be implemented via tables and methatmethods but it can bring incompatibility and inconsistency in different class\objects systems and required for binding of C++ classes.
* `nil` and `false` are the only false values; 0, 0.0, "0" and all other values are interpreted as `true`.
* no ternary operator

**Bad**:
* Garbage collector - makes it harder to predict performance stalls and memory consumption.
* Global scoping by default
* Infamous "everything is nil".


#### Javascript
Javascript is very popular programming language. It is probably the most popular language in the world. It has thousands of libraries and books and all possible tools (like transpilers and analyzers).
Most programmers knows at least something about it.
However it is also well known for being let's say tricky and has reputation of being not the simplest or best language in the world (if not the worst).
Lots of its issues nowadays are somehow fixed with new versions of JS, with transpilers or analyzers. However that also means that there are hundreds of dialects of this language (or it is better to say - there are hundreds of Javascripts). So you have to start with some expensive tooling or with Vanilla JS that your VM will run.
There are several VMs you can choose. The most notable by our opinion are Duktape and QuickJS.
**Duktape**
Mature and actively developed JS VM made in C.
We do not know if any popular project in video-games that had used it, but we used it internally and it looks stable. It is also actively updated and maintained.
**Good**:
* Reference-counting garbage collector (as well as Quirrel), built-in debugger and other tools.
* builtin Unicode and modules support

**Bad**:
* E5\E5.1 ECMA(Javascript) (but with coroutines and some other Post-ES5 features)
* About 5 times more code than Quirrel.
* At average 1.5-3 times slower in all benchmarks than Quirrel or Lua.
The last part is probably critical for anyone in gamedev. Of course, high-level languages are not supposed to be used for performance-critical code, but there are always some reasonable limitation. And there is no extra performance in video game (or database).

**QuickJS** - this is completely new VM made in C (released in mid 2019).
* It supports ES 2019\2020 which is great.
* It has comparable with Quirrel or Lua performance (plus/minus the same in most cases).

QuickJS seems to be great, the main concern for it (if you do not concerned about Javascript itself) is that it is rather new and not tested "in the wild".
And of course it is still fully compatible ES2019 JavaScript so it has more LoC in implementation and in syntax documentation and features than any other language in the list.

#### Wren
Nice classy scripting language made in C.
Comparable performance and memory requirements, reference-counting garbage collector.
However it is built mostly around OOP paradigm and as far as we know was not used in big projects yet.

#### Scheme\Lisp (chibi VM).
It is probably great language if you like or familiar with Lisp\Scheme. There can be some difficulties with binding native C++ classes\methods, and Lisp is less popular than Lua\Javascript.
As cons - we do not know any popular games made with this VM (not the Lisp - world-famous Naughty Dog used their own Lisp in all their titles).
But if you like Lisp - go with Lisp.

#### DaScript
This is very fast scripting language, focused on performance and safety. It has uncomparable speed and would be the best option if you are focusing on things like stored procedures or script for data-oriented\data-driven frameworks (like ECS).
However it is also a bit harder to start program with it for non-programmers.
You also cannot write a functional-style code with higher-ordered functions (if it is important in your case).
If you want to never be concerned about performance - use daScript.

#### Language that you may ask for:
**C#** - no interpreted mode in any popular VM.
**LuaJIT** - without JiT it is not always faster than JiT but definitely much more complicated.
**Javascript V8** - JiT only, huge requirements in memory, hard to embed.
**Python** - significantly heavier in terms of memory, bindings and performance. However there are games made with Python, but it is definitely not on par with other languages seen as lightweight scripting language.

## Production ready and proven
We have **millions** of users running millions LoCs of Quirrel code *each day*.
That doesn't mean that language can not have bugs or other issues, but it obviously should be called *stable*. Use it on your own risk, but know that there are lots of developers and millions of users that use it everyday.

## Migration from Squirrel and useful tools

### Binding
Obviously if you are using embedded language your need some bindings with your native code. We took **sqrat** and [heavily changed it](https://github.com/GaijinEntertainment/sqrat), reduced code, improve performance and fixed some bugs. *Probably better to rename it too.*

### Static analyzer
We have [changed language a lot](http://quirrel.io/doc/reference/diff_from_original.html) in the past.
To be able to upgrade existing code to new language we have written a [static analyzer tool](https://github.com/GaijinEntertainment/quirrel_static_analyzer).
It is still under development and we would better have typehinting (that is important for dynamic typed languages),
but it already can be used and it finds potential bugs everyday, before they hit QA or real users.

### Strict mode
Like in Javascript, we have [strict-mode](http://quirrel.io/reference/language/compiler_directives.html). There may be more checks in a future, but you can use it for the greater good now.

### Debugger
We have our own debugger, however it currently heavily depends on our internal engine. May be in a future.

### Modules
It is hard and unproductive to have modern language without modules.
Modules allow to re-use and share code. It is also a way to isolate and encapsulate your pieces code which is also prove to be important to maintain a code.  

## History

Quirrel starts from Squirrel.
It was originally created as [Squirrel]([http://squirrel-lang.org/](http://squirrel-lang.org/)) by Alberto Demichelis based on his experience with Lua in CryTek. Squirrel was inspired by Lua, but targets to improve some infamous and uncomfortable features of Lua. It has JS\C++ like syntax (as most of game programmers are familiar with it), it has built-in classes (it helps with binding of C++ objects), floats and integers types, arrays and many other features that are really helpful when you are working from your script language with your native code.
Since that time it was used by game-developers in (for example Valve used it Left4Dead, Portal 2 and CS:GO games). It also used for IoT project called [Electric Imp]([https://www.electricimp.com/](https://www.electricimp.com/)) and some other projects.
Gaijin Entertainment used it since 2006 in all it's projects on all platforms (X360, PS3, PS4, XBoxOne, Switch, iOS, Android, Windows\Mac\Linux PC).
Using it a lot we have found that we need to improve it - make it faster (you never have extra performance), safer (dynamic languages can be tricky, and you do not want them to "trick" your users).
Initially we have submitted a few PRs for original language.

Our philosophy during development is inspired by [Zen of Python](https://www.python.org/dev/peps/pep-0020/) as we found that it leads to better and faster code.

Guided by this and by our practice we have removed all aliases in Squirrel (like push==append) and renamed some ambiguous methods (namely 'find').
Constants scope was made local by default, new keyword 'global' was introduced.
Duplicate names of variables/constants and table slots raise errors now.
This made language backward incompatible (see [documentation](http://quirrel.io/doc/reference/diff_from_original.html) for more details).
Noticeably it was very easy to support these changes to support - just simple search & replace.

But as we have made incompatibile changes we have to made a fork.

And as language nowadays is different enough and to avoid ambiguity we have renamed our fork to Quirrel.
We continued versioning from Squirrel version, so Quirrel started from version 4.0.0.

## Development state

Quirrel is in stable version 4.1.0.

This project has successfully been compiled and run on
  * Windows (x86 and amd64)
  * Linux (x86, amd64 and ARM)
  * Illumos (x86 and amd64)
  and several other platforms (including mobile and consoles)

The following compilers have been confirmed to be working:
   
    MS Visual C++  10.0 (all on x86 and amd64)
                   11.0   |
                   12.0   v
                   13.0
                   14.x   ---
    MinGW gcc 3.2 (mingw special 20020817-1)
    Cygnus gcc 3.2
    Linux gcc 3.2.3
              4.0.0 (x86 and amd64)
              5.3.1 (amd64)
    Illumos gcc 4.0.0 (x86 and amd64)
    ARM Linux gcc 4.6.3 (Raspberry Pi Model B)


## Things to add, to improve, to discuss and to change

### Planned

* Modules library documentation

### Likely to be implemented

* Fix: Null-propagation operators ?[] ?. silently consumes not only 'slot not found' result, but also any error raised by _get() metamethod

### Ideas in arbitrary order

* Pure functions (and a special keyword for this)
* const declaration of mutable values (forbid to assign new value for such consts)
* Better regexp library
* Better Unicode support (slice, replace, etc)
* reversed view for arrays

### Discussions and future ideas

* global varname = for global variable declaration (conflicts with hot reload)
* Typehinting in syntax and typechecking in analyzer
* AST-based compiler (to make language even more faster and safer)


## Welcome!

Feedback, PRs and suggestions are appreciated!
Documentation project page - http://quirrel.io
GitHub page - https://github.com/GaijinEntertainment/quirrel
