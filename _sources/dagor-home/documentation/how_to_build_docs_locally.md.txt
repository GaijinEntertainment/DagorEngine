## How to Build Documentation Locally

To generate local documentation for the *Dagor Engine*, follow these steps:

1. **Clone the Repository**

   Clone the [*Dagor Engine*
   repository](https://github.com/GaijinEntertainment/DagorEngine).

2. **Install Python**

   Download and install [*Python*](https://www.python.org/downloads/).

3. **Install Sphinx and Dependencies**

   Use the provided configuration file to install
   [*Sphinx*](https://www.sphinx-doc.org/en/master/) and required components:

   ```
   pip install -r requirements.txt
   ```

   ```{note}
   On Windows, *Sphinx* and its dependencies must be installed with
   administrator privileges.
   ```

4. **Install Doxygen**

   Download and install [*Doxygen*](https://www.doxygen.nl/).

   ```{note}
   On Windows, add *Doxygen* to the environment PATH:
   `$env:PATH=$env:PATH+';<doxygen_installation_dir>\bin\'`,
   replacing `<doxygen_installation_dir>` with the path to your *Doxygen*
   installation directory (e.g., `c:\Program Files\doxygen`).
   ```

5. **Build Documentation**

   In the `DagorEngine/_docs` directory, execute the following steps:

   - **Run the Build Script**

     Execute the script to parse source files:

     ```
     python3 build_all_docs.py
     ```

     This script will process documentation from `DagorEngine/prog` and output
     it to `_docs/source/`.

   - **Run Sphinx Build Command**

     Use *Sphinx* to build the documentation:

     - **On Windows**:

       ```
       sphinx-build . _build
       ```

     - **On Linux**:

       ```
       python3 -m sphinx . _build
       ```

     *Sphinx* will generate the documentation in HTML format by default, placing
     the output in `_docs/_build`.

6. **View the Documentation**

   Open the `index.html` file located in `_docs/_build` to view the generated
   documentation.


