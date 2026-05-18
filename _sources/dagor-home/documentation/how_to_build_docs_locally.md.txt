# How to Build Documentation Locally

## Prerequisites

1. **Clone the Repository**

   ```text
   git clone https://github.com/GaijinEntertainment/DagorEngine.git
   cd DagorEngine
   ```

2. **Install Python**

   Download and install from [python.org](https://www.python.org/downloads/).

3. **Install Sphinx and Dependencies**

   Download and install [Sphinx](https://www.sphinx-doc.org/en/master/) and
   required packages:

   ```text
   pip install -r requirements.txt
   ```

   ```{note}
   On Windows, Sphinx and its dependencies must be installed with administrator
   privileges.
   ```

4. **Install Doxygen**

   Download and install from [doxygen.nl](https://www.doxygen.nl/).

   ```{note}
   On Windows, add Doxygen to the environment PATH:
   `$env:PATH=$env:PATH+';<doxygen_installation_dir>\bin\'`,
   replacing `<doxygen_installation_dir>` with the path to your Doxygen
   installation directory (e.g., `c:\Program Files\doxygen`).
   ```

## Build HTML Documentation

1. **Generate Intermediate Files**

   From `_docs` run:

   ```text
   python3 build_all_docs.py
   ```

   This script will process documentation from `DagorEngine/prog` and output it
   to `_docs/source/`.

2. **Build HTML**

   From `_docs` run:

   ```text
   sphinx-build -c . -b html source _build/html
   ```

   Sphinx will generate the documentation in HTML format by default, placing the
   output in `_docs/_build/html`:

   - `sphinx-build`: Runs the Sphinx documentation builder.
   - `-c .`: Uses `conf.py` from the current directory.
   - `-b html`: Builds HTML output.
   - `source`: Directory with the documentation source files.
   - `_build/html`: Output directory for generated HTML.

   ```{note}
   Alternatively, you can run the `build_html.bat` script from the `_docs`
   folder to automate the build steps.
   ```

3. **View the Docs**

   Open `_docs/_build/html/index.html` in your browser to view the generated
   documentation.

## Optional: Speed Up Rebuilds (Skip Doxygen)

If source code hasn't changed, you can skip Doxygen:

```{eval-rst}
.. tab-set::

   .. tab-item:: PowerShell

      .. code-block:: text

         $env:SKIPDOXYGEN = "1"
         sphinx-build -c . -b html source _build/html

   .. tab-item:: Git Bash / MinGW

      .. code-block:: text

         SKIPDOXYGEN=1 sphinx-build -c . -b html source _build/html
```

This will reuse the previously generated XML from Doxygen instead of running it
again.

```{note}
Alternatively, you can run the `build_html_skipdoxygen.bat` script from the
`_docs` folder to automate this step.
```

## Optional: Build PDF Documentation

### Build PDF for Full Dagor Documentation

1. **Install TeXLive**

   Download and install TeXLive from
   [https://www.tug.org/texlive/](https://www.tug.org/texlive/).

2. **Generate LaTeX Files**

   From `_docs` run:

   ```text
   sphinx-build -c . -b latex source _build/pdf
   ```

   This will generate LaTeX source files in the `_build/pdf` directory.

3. **Build PDF**

   From `_build/pdf` run:

   ```text
   latexmk -xelatex dagor.tex
   ```

   The final PDF will be at `_docs/_build/pdf/dagor.pdf`.

   ```{note}
   Alternatively, you can run the `build_pdf_full.bat` script from the `_docs`
   folder to automate the build steps.
   ```

### Build PDF for Dagor Maxscript Toolbox

To build a standalone PDF for a specific documentation chapter, such as [Dagor
MaxScript
Toolbox](../../dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/index.rst),
use the `PDF_SUBDOC` environment variable in combination with the Sphinx build
command.

1. **Install TeXLive**

   Download and install TeXLive from
   [https://www.tug.org/texlive/](https://www.tug.org/texlive/).

2. **Generate LaTeX Files**

   From `_docs` run:

   ```{eval-rst}

   .. tab-set::

      .. tab-item:: PowerShell

         .. code-block:: text

            $env:PDF_SUBDOC = "maxscript_toolbox"
            sphinx-build -c . -b latex source/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox _build/pdf_maxscript_toolbox

      .. tab-item:: Git Bash / MinGW

         .. code-block:: text

            PDF_SUBDOC=maxscript_toolbox sphinx-build -c . -b latex source/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox _build/pdf_maxscript_toolbox
   ```

   This will generate LaTeX source files for the selected chapter in the
   `_build/pdf_maxscript_toolbox` directory.

3. **Build PDF**

   From `_build/pdf_maxscript_toolbox` run:

   ```text
   latexmk -xelatex dagor_maxscript_toolbox.tex
   ```

   The final PDF will be at
   `_docs/_build/pdf_maxscript_toolbox/dagor_maxscript_toolbox.pdf`.

   ```{note}
   Alternatively, you can run the `build_pdf_maxscript_toolbox.bat` script from
   the `_docs` folder to automate the build steps.
   ```
