QDox, writing your documentation
===================================

QDox allows you to generate docs (ReStrcturedText, sphinx rst) from .cpp files
(and later in future - perhaps from .nut quirrel modules).

written in python 3.10, no external dependecies, should work in Python 3.9 or later.
Inspired by LuaDox
but written much more dirty and probably needs refactor.
It was not possible to use LuaDox mostly not because of different comment style in cpp and lua,
but because of that Quirrel is much more complex language and I want to try to get more from cpp files

What is not done yet, but can be done in future:

  * outside the scope definitions and later bindings. Can produce unreferenced objects.
  * multiscope path definition ``@class foo/bar/Baz`` -- add class Baz in module foo and in table bar
  * support for ``@example`` ``@usage`` ``@image`` ``@icon`` and others handy things
  * multiline param and return definitions
  * generate stubs from parsed representation (one of the goals)

But it will likely require to significantly rewrite parser again. So maybe later.


A brief example of how to comment cpp code:

..  code-block:: cpp
    :caption: part of sqWebBrowser.cpp

    void bind_webbrowser(SqModules* sq_modules_mgr)
    {
      Sqrat::Table tbl(sq_modules_mgr->getVM());
      /**@module browser

    Opens embeded web browser. Requires native cef binary and its integration.

      */
      tbl
        .Func("can_use_embeded_browser", can_use_embedded_browser)
        ///@return b : Returns can browser be opened
        .Func("browser_go_back", go_back)
        .Func("browser_reload_page", reload)
        ///Will reload page
        .Func("browser_add_window_method", add_window_method)
        ///@param method c : add function 
      ;

      using namespace webbrowser;

      CONST(BROWSER_EVENT_INITIALIZED);
      CONST(BROWSER_EVENT_DOCUMENT_READY);

      sq_modules_mgr->addNativeModule("browser", tbl);
    }

The Basics
------------

All QDox comments should begin from three slashes ``///`` or doxygen style multiline comment ``/** */``.

Everything starts with ``@module`` or ``@page``
You should place ``///@module <your module name>`` or ``///@page <your page name>`` somewhere in the file,
cause without module all other things can't be added

There are several directives you need to know:

collections or containers can have some other members in them:

  * ``@module`` or ``@page`` can have tables, classes, functions, consts, values, enums. Usually it exports a table.
  * ``@table`` can have functions, classes, enums, tables, consts and values
  * ``@class`` can have functions, ctor, operators, Var, Prop, Const
  * ``@enum`` can have only consts and values

'Leaf' objects (can't have members):

  * ``@function`` - can be only in tables or module
  * ``@ctor`` - can be only in classes
  * ``@operator`` - can be only in classes
  * ``@property`` - can be only in classes
  * ``@var`` - can be only in classes
  * ``@value`` - can be in module, table, class, enum
  * ``@const`` - can be in module, table, class, enum

'Doc' objects:

  * multiline - ``@code``, ``@expand``, ``@tip``, ``@seealso``, ``@warning``, ``@tip``, ``@alert`` or ``@danger``, ``@note``.
    These objects close their scope when ``<tag_name>@`` appears (like ``code@``, ``tip@``, etc) or comment ended.
    These objects can be at any scope, as they just add simple way of create rst (see below)

Each object - container or leaf type - can be defined with scope of module. 

    ``@class myModule/MyClass`` will set that MyClass is in myModule.


Functions, tables and classes can be defined as a module:

    ``@class DataBlock = DataBlock`` will describe module that exports class DataBlock.


In function scope (as well as in operator and ctor) you can use following directive :
  * ``@paramsnum`` - integer for number of parameters (called paramscheck in cpp).
    If negative - means that function can be called with at least that amount of parameters, but also with more than that.
  * ``@kwarged`` - means that function paramaters should be provided via table. like: :sq:`function foo({argument=1, arg2=2})`
  * ``@typemask`` - string with typemask. See `documentation <https://quirrel.io/doc/reference/api/object_creation_and_handling.html#c.sq_setparamscheck>`_
  * ``@vargved`` - means that function can have variable arguments count
  * ``@param`` - should be ``@param <name> <type> : description``, for example ``@param title t : Title of messagebox``
  * ``@return`` - should be ``@return typemask : any description`` or ``@return typemask``

.. note::
  All directives, except doc-objects, ``@param`` and ``@return`` follows very simple declaration rule:

  ``@fieldname value brief description``
  
  ``@param`` and ``@return`` use ``:`` to separate description from other arguments, cause they can have variable number of arguments

.. note::
  ``@param`` can have following variants:

  ``@param name : description``

  ``@param name type : description``

  ``@param name type defvalue : description``

  and description is always optional

In class scope:
  * ``@extends`` - what class does this class extends

In property of class:
  * ``@writeable`` - can you modify property.

In value or const or Var scope:
  * ``@ftype`` - type of constant (typemask-like string or just any text)
  * ``@fvalue`` - value of constant

In scope of any object you can use following directives:
  * ``@brief`` - short description
  * ``@parent`` - scope reference (to show name like ``parent.Object``)


Special directives
^^^^^^^^^^^^^^^^^^^^

``@resetscope`` - to exit all scopes (modules, functions).
  Basically to stop autodocumentation till next module scope will be entered

``@skipline`` - skipnext line in parsing


Autodocumentation for .Func
-----------------------------
If function used in .Func() is defined in the same cpp file - it can be autodocumented.
No lambdas or namespaces are supported.
No multiline function definition is supported also

..  code-block:: cpp

  int addTen(int val)
  {
    return val+10;
  }
  ...
  .Func("bindFuncName", CppFuncName)  //will generate return type and arguments type and name in documentation


Doc-objects
------------

Mutltiline (blocks)
^^^^^^^^^^^^^^^^^^^^

``@alert``::

  @alert
    any alert here
  alert@

.. danger::
  any alert here

The same syntax goes for ``@tip``, ``@note``, ``@seealso``, ``@warning``

.. tip::
  Tip! Nice tip!

.. warning::
  Warning!

.. danger::
   Use this alert block to bring the reader's attention to a serious threat.

.. seealso::
  place here some links

.. note::
  And this is note

--------

``@expand`` (aliases ``@spoiler``, ``dropdown``)::

  @spoiler My spoiler
    toggle text that can be any rst (but not recursive code objects)
    
    preformat::

      some code or preformat text
  spoiler@

.. dropdown:: My spoiler

  toggle text that can be any rst (but not recursive code objects)
  
  preformat::
  
     some code or preformat text


--------

``@code``::
  
  @code some_name lang
    watch indent
      will generate block
  code@

.. code-block:: lang
  :caption: some_name

    watch indent
      will generate block


Configurate what to analyze and build
---------------------------------------

You need to set where your files to parse are placed and what is documentation about.
This is done via .qdox config
.qdox config is json (with cpp style single line comments supported)

..  code-block:: cpp
    :caption: .qdox

      [
      {
        "paths": ["prog/gameLibs/quirrel"], //folders or files list, required, relative to config
        "doc_chapter": "quirrel_gamelibs", //internal folder name, required
        "chapter_desc": "Docs for Quirrel modules extracted from source in gameLibs", //displayed title, optional
        "chapter_title": "Quirrel Gamelibs Native Modules", //displayed title, optional
        "extensions": [".cpp"], //optional, list of extensions of files
        "exclude_dirs_re": [], //optional, list of regexps that should full match to exclude dirs in recursive search
        "exclude_files_re": [], //optional, list of full match regexp to skip files'
        "recursive": true //optional, do recursive search for folders'
      }
      ]




Building and testing the qdox
-----------------------------

**You need python 3.10+ installed with python in paths**

Installation::

  pip install sphinx
  pip install myst
  pip install myst_parser
  pip install sphinx_rtd_theme
  pip install sphinx-autobuild

Than in your dagor folder you can just run following commandlines::

  python _docs/qdoc_main.py
  sphinx-build -b html -d _docs/build/doctrees _docs/source _docs/build/html

(in folder where .qdox config is build (see usage for details))

to watch changes in cpp files, configs and autorebuild::
  
  @start python _docs/watch.py
  @start sphinx-autobuild --port 8080 _docs/source _docs/build/html

(in .qdox folder to start autorebuild rst from source and config)

To have a look on a result - open with you browser ``_docs/build/html/index.html`` if you build manually (first way)
or ``localhost:8080`` if you launch autobuild

ReStructuredText
-----------------

Any docline that is not some directives is just restructured text

There is lots of RTFM of sphix rst in internet

**Strong emphasis:** `small emphasis` 


Here are some goodies you can do (see source in source/user-guid/qdox.rst to understand how that was made)



inline :sq:`let = function(){}` wow

inline :cpp:`void function(){}` here 


For example separate text by lines:

-------------


A coloured icon: :octicon:`report;1em;sd-text-info`, look at it.
