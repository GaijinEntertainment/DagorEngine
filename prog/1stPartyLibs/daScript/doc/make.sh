#!/bin/bash

# Command file for Sphinx documentation

if [ -z "$SPHINXBUILD" ]; then
    SPHINXBUILD=sphinx-build
fi

BUILDDIR=build
SITEDIR=../site
ALLSPHINXOPTS="-d $BUILDDIR/doctrees $SPHINXOPTS source"
I18NSPHINXOPTS="$SPHINXOPTS source"

if [ -n "$PAPER" ]; then
    ALLSPHINXOPTS="-D latex_paper_size=$PAPER $ALLSPHINXOPTS"
    I18NSPHINXOPTS="-D latex_paper_size=$PAPER $I18NSPHINXOPTS"
fi

# Function to display help
show_help() {
    echo "Please use \`make.sh <target>' where <target> is one of"
    echo "  html       to make standalone HTML files"
    echo "  dirhtml    to make HTML files named index.html in directories"
    echo "  singlehtml to make a single large HTML file"
    echo "  pickle     to make pickle files"
    echo "  json       to make JSON files"
    echo "  htmlhelp   to make HTML files and a HTML help project"
    echo "  qthelp     to make HTML files and a qthelp project"
    echo "  devhelp    to make HTML files and a Devhelp project"
    echo "  epub       to make an epub"
    echo "  latex      to make LaTeX files, you can set PAPER=a4 or PAPER=letter"
    echo "  latexpdf   to make LaTeX files and run them through pdflatex"
    echo "  text       to make text files"
    echo "  man        to make manual pages"
    echo "  texinfo    to make Texinfo files"
    echo "  gettext    to make PO message catalogs"
    echo "  changes    to make an overview over all changed/added/deprecated items"
    echo "  xml        to make Docutils-native XML files"
    echo "  pseudoxml  to make pseudoxml-XML files for display purposes"
    echo "  linkcheck  to check all external links for integrity"
    echo "  doctest    to run all doctests embedded in the documentation if enabled"
    echo "  coverage   to run coverage check of the documentation if enabled"
    echo "  site       to build HTML files directly to ../site/doc"
    echo "  clean      to remove all build artifacts"
}

# Check if sphinx-build is available
check_sphinx() {
    if ! command -v $SPHINXBUILD &> /dev/null; then
        echo
        echo "The '$SPHINXBUILD' command was not found. Make sure you have Sphinx"
        echo "installed, then set the SPHINXBUILD environment variable to point"
        echo "to the full path of the 'sphinx-build' executable. Alternatively you"
        echo "may add the Sphinx directory to PATH."
        echo
        echo "If you don't have Sphinx installed, grab it from"
        echo "http://sphinx-doc.org/"
        exit 1
    fi
}

# Main script logic
if [ $# -eq 0 ]; then
    show_help
    exit 0
fi

case "$1" in
    help)
        show_help
        ;;
    
    clean)
        if [ -d "$BUILDDIR" ]; then
            rm -rf $BUILDDIR/*
            echo "Build directory cleaned."
        else
            echo "Build directory does not exist."
        fi
        ;;
    
    site)
        check_sphinx
        $SPHINXBUILD -b html -d sphinx-build source ../site/doc
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished. The HTML pages are in ../site/doc"
        ;;
    
    html)
        check_sphinx
        $SPHINXBUILD -b html $ALLSPHINXOPTS $BUILDDIR/html
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished. The HTML pages are in $BUILDDIR/html"
        ;;
    
    dirhtml)
        check_sphinx
        $SPHINXBUILD -b dirhtml $ALLSPHINXOPTS $BUILDDIR/dirhtml
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished. The HTML pages are in $BUILDDIR/dirhtml"
        ;;
    
    singlehtml)
        check_sphinx
        $SPHINXBUILD -b singlehtml $ALLSPHINXOPTS $BUILDDIR/singlehtml
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished. The HTML page is in $BUILDDIR/singlehtml"
        ;;
    
    pickle)
        check_sphinx
        $SPHINXBUILD -b pickle $ALLSPHINXOPTS $BUILDDIR/pickle
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished; now you can process the pickle files."
        ;;
    
    json)
        check_sphinx
        $SPHINXBUILD -b json $ALLSPHINXOPTS $BUILDDIR/json
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished; now you can process the JSON files."
        ;;
    
    htmlhelp)
        check_sphinx
        $SPHINXBUILD -b htmlhelp $ALLSPHINXOPTS $BUILDDIR/htmlhelp
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished; now you can run HTML Help Workshop with the"
        echo ".hhp project file in $BUILDDIR/htmlhelp."
        ;;
    
    qthelp)
        check_sphinx
        $SPHINXBUILD -b qthelp $ALLSPHINXOPTS $BUILDDIR/qthelp
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished; now you can run \"qcollectiongenerator\" with the"
        echo ".qhcp project file in $BUILDDIR/qthelp, like this:"
        echo "> qcollectiongenerator $BUILDDIR/qthelp/testy_sphinxy.qhcp"
        echo "To view the help file:"
        echo "> assistant -collectionFile $BUILDDIR/qthelp/testy_sphinxy.ghc"
        ;;
    
    devhelp)
        check_sphinx
        $SPHINXBUILD -b devhelp $ALLSPHINXOPTS $BUILDDIR/devhelp
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished."
        ;;
    
    epub)
        check_sphinx
        $SPHINXBUILD -b epub $ALLSPHINXOPTS $BUILDDIR/epub
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished. The epub file is in $BUILDDIR/epub."
        ;;
    
    latex)
        check_sphinx
        $SPHINXBUILD -b latex $ALLSPHINXOPTS $BUILDDIR/latex
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished; the LaTeX files are in $BUILDDIR/latex."
        ;;
    
    latexpdf)
        check_sphinx
        $SPHINXBUILD -b latex $ALLSPHINXOPTS $BUILDDIR/latex
        if [ $? -ne 0 ]; then
            exit 1
        fi
        cd $BUILDDIR/latex
        pdflatex daslang.tex
        if [ $? -ne 0 ]; then
            cd - > /dev/null
            exit 1
        fi
        pdflatex daslangstdlib.tex
        if [ $? -ne 0 ]; then
            cd - > /dev/null
            exit 1
        fi
        cd - > /dev/null
        echo
        echo "Build finished; the PDF files are in $BUILDDIR/latex."
        ;;
    
    latexpdfja)
        check_sphinx
        $SPHINXBUILD -b latex $ALLSPHINXOPTS $BUILDDIR/latex
        if [ $? -ne 0 ]; then
            exit 1
        fi
        cd $BUILDDIR/latex
        make all-pdf-ja
        cd - > /dev/null
        echo
        echo "Build finished; the PDF files are in $BUILDDIR/latex."
        ;;
    
    text)
        check_sphinx
        $SPHINXBUILD -b text $ALLSPHINXOPTS $BUILDDIR/text
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished. The text files are in $BUILDDIR/text."
        ;;
    
    man)
        check_sphinx
        $SPHINXBUILD -b man $ALLSPHINXOPTS $BUILDDIR/man
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished. The manual pages are in $BUILDDIR/man."
        ;;
    
    texinfo)
        check_sphinx
        $SPHINXBUILD -b texinfo $ALLSPHINXOPTS $BUILDDIR/texinfo
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished. The Texinfo files are in $BUILDDIR/texinfo."
        ;;
    
    gettext)
        check_sphinx
        $SPHINXBUILD -b gettext $I18NSPHINXOPTS $BUILDDIR/locale
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished. The message catalogs are in $BUILDDIR/locale."
        ;;
    
    changes)
        check_sphinx
        $SPHINXBUILD -b changes $ALLSPHINXOPTS $BUILDDIR/changes
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "The overview file is in $BUILDDIR/changes."
        ;;
    
    linkcheck)
        check_sphinx
        $SPHINXBUILD -b linkcheck $ALLSPHINXOPTS $BUILDDIR/linkcheck
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Link check complete; look for any errors in the above output"
        echo "or in $BUILDDIR/linkcheck/output.txt."
        ;;
    
    doctest)
        check_sphinx
        $SPHINXBUILD -b doctest $ALLSPHINXOPTS $BUILDDIR/doctest
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Testing of doctests in the sources finished, look at the"
        echo "results in $BUILDDIR/doctest/output.txt."
        ;;
    
    coverage)
        check_sphinx
        $SPHINXBUILD -b coverage $ALLSPHINXOPTS $BUILDDIR/coverage
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Testing of coverage in the sources finished, look at the"
        echo "results in $BUILDDIR/coverage/python.txt."
        ;;
    
    xml)
        check_sphinx
        $SPHINXBUILD -b xml $ALLSPHINXOPTS $BUILDDIR/xml
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished. The XML files are in $BUILDDIR/xml."
        ;;
    
    pseudoxml)
        check_sphinx
        $SPHINXBUILD -b pseudoxml $ALLSPHINXOPTS $BUILDDIR/pseudoxml
        if [ $? -ne 0 ]; then
            exit 1
        fi
        echo
        echo "Build finished. The pseudo-XML files are in $BUILDDIR/pseudoxml."
        ;;
    
    *)
        echo "Unknown target: $1"
        echo
        show_help
        exit 1
        ;;
esac

exit 0

