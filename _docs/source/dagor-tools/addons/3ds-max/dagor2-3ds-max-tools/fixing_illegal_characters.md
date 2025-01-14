# Dagor 2 Fixing Illegal Characters Tool

## Installation

[Install the script](installation.md) following the provided instructions.

```{important}
This script requires 3ds Max 2013 or newer version to run.
```

## Accessing the Fixing Illegal Characters Tool

1. Navigate to **Gaijin Tools** {bdg-dark-line}`1` **> Fix Illegal
   Characters...**. This will open the main window of the Dagor 2 Fix Illegal
   Characters script.

2. To verify the version of the script, go to **Gaijin Tools**
   {bdg-dark-line}`1` **> About** {bdg-dark-line}`2`. The **About** window will
   display the current version. It's important to check this regularly to ensure
   your script is up to date.

   <img src="_images/fixing_illegal_chars_01.png" alt="Fixing Illegal Characters Tool" align="center">

```{note}
Make sure that the plugin version is at least `1.4`.
```

## Using the Fixing Illegal Characters Tool

To begin, run the script from the **Gaijin Tools** {bdg-dark-line}`1` **> Fix
Illegal Characters...** menu. The following window will appear:

   <img src="_images/fixing_illegal_chars_02.png" alt="Fixing Illegal Characters Tool" align="center">

### Tool Options

- **Illegal Characters List** {bdg-dark-line}`2`: enter or delete illegal
  characters in this field.
- **Replace with Characters** {bdg-dark-line}`3`: specifies characters to
  replace the illegal ones. If left blank, illegal characters will be removed.
  This option supports UTF-8, including hieroglyphs, Slavic languages, and
  Arabic characters, etc.
- **Latinization of the Russian** {bdg-dark-line}`4`: converts all Russian
  characters to Latin according to standard Latinization rules. If unchecked,
  Russian characters will be replaced with the characters specified in the
  **Replace with Characters** {bdg-dark-line}`3` field.
- **Log Warning Bitmap Path Name** {bdg-dark-line}`5`: logs warnings for file
  paths containing illegal characters to the log window {bdg-dark-line}`17`.

  ```{note}
  No substitutions are made; this only outputs warnings.
  ```
- **Log Warning Texture Maps Names** {bdg-dark-line}`6`: logs warnings for
  texture names containing illegal characters to the log window
  {bdg-dark-line}`17`.

  ```{note}
  No substitutions are made; this only outputs warnings.
  ```
- **Fix Layers Names** {bdg-dark-line}`7`: replaces illegal characters in layer
  names with the specified replacement characters.

  ```{note}
  Layer 0 (default) cannot be renamed.
  ```

  There cannot be layers with the same name. If naming conflicts occur or
  renaming fails, a warning is displayed in the log window {bdg-dark-line}`17`:

   <img src="_images/fixing_illegal_chars_03.png" alt="Fixing Illegal Characters Tool" align="center">

- **Fix Objects Names** {bdg-dark-line}`8`: replaces illegal characters in
  object names with the specified replacement characters. For check naming it's
  necessary to select objects.

  ```{note}
  This may result in objects having identical names.
  ```

- **Fix Materials Names** {bdg-dark-line}`9`: replaces illegal characters in
  material names with the specified replacement characters.

  ```{note}
  This may result in materials having identical names.
  ```

- **All Changes to Lower Case** {bdg-dark-line}`10`: converts all names to lower
  case and performs checks in lower case.
- **DAG Export After Fixing Illegal Characters** {bdg-dark-line}`11`: opens the
  `.dag` format export window after all checks are complete.

  ```{note}
  Ensure you are familiar with how Dagor 2 plugins work before enabling this
  option.
  ```

- **Save Current Settings** {bdg-dark-line}`12`: saves the current settings.
- **Load Default Settings** {bdg-dark-line}`13`: loads the default settings.
- **FIX ILLEGAL CHARACTERS FROM SELECTION** {bdg-dark-line}`14`: executes the
  script to fix illegal characters in the selected objects.
- **Visit to Learning Website** {bdg-dark-line}`15`: links to this
  documentation.
- **Get in Touch with the Author** {bdg-dark-line}`16`: provides contact
  information for the developer if assistance is needed.
- **Errors Log** {bdg-dark-line}`17`: displays information about the results of
  the checks and any changes made.

If the scene contains no errors, you will see a window like this:

<img src="_images/fixing_illegal_chars_04.png" alt="Fixing Illegal Characters Tool" align="center">

Any errors will be highlighted in red and labeled as **ERROR!** or **WARNING!**:

<img src="_images/fixing_illegal_chars_05.png" alt="Fixing Illegal Characters Tool" align="center">

A scene containing multiple errors may produce results like this:

<img src="_images/fixing_illegal_chars_06.png" alt="Fixing Illegal Characters Tool" align="center">


