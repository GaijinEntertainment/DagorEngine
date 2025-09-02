# Dagor Fixing Illegal Characters Tool

## Installation

[Install the script](installation.md) following the provided instructions.

```{admonition} 3ds Max Version Requirement
:class: warning

This script requires **3ds Max 2013 or later**.
```

## Accessing Fixing Illegal Characters Tool

1. Navigate to **Gaijin Tools** {cnum}`1` **> Fix Illegal Characters...**. This
   will open the main window of the Dagor Fix Illegal Characters script.

2. To verify the version {cnum}`3` of the script, go to **Gaijin Tools**
   {cnum}`1` **> About** {cnum}`2`. The **About** window will display the
   current version. It's important to check this regularly to ensure your script
   is up to date.

   ```{eval-rst}
   .. image:: _images/fixing_illegal_chars_01.png
      :alt: Fixing Illegal Characters Tool
      :width: 55em
      :align: center
   ```

   ```{admonition} Plugin Version Requirement
   :class: warning

   Requires plugin version **1.4 or higher**.
   ```

## Using Fixing Illegal Characters Tool

To begin, run the script from the **Gaijin Tools** {cnum}`1` **> Fix Illegal
Characters...** menu. The following window will appear:

```{eval-rst}
.. image:: _images/fixing_illegal_chars_02.png
   :alt: Fixing Illegal Characters Tool
   :width: 55em
   :align: center
```

### Tool Options

- **Illegal Characters List** {cnum}`2`: enter or delete illegal characters in
  this field.
- **Replace with Characters** {cnum}`3`: specifies characters to replace the
  illegal ones. If left blank, illegal characters will be removed. This option
  supports UTF-8, including hieroglyphs, Slavic languages, and Arabic
  characters, etc.
- **Latinization of the Russian** {cnum}`4`: converts all Russian characters to
  Latin according to standard Latinization rules. If unchecked, Russian
  characters will be replaced with the characters specified in the **Replace
  with Characters** {cnum}`3` field.
- **Log Warning Bitmap Path Name** {cnum}`5`: logs warnings for file paths
  containing illegal characters to the log window {cnum}`17`.

  ```{note}
  No substitutions are made; this only outputs warnings.
  ```
- **Log Warning Texture Maps Names** {cnum}`6`: logs warnings for texture names
  containing illegal characters to the log window {cnum}`17`.

  ```{note}
  No substitutions are made; this only outputs warnings.
  ```

- **Fix Layers Names** {cnum}`7`: replaces illegal characters in layer names
  with the specified replacement characters.

  ```{note}
  Layer 0 (default) cannot be renamed.
  ```

  There cannot be layers with the same name. If naming conflicts occur or
  renaming fails, a warning is displayed in the log window {cnum}`17`:

  ```{eval-rst}
  .. image:: _images/fixing_illegal_chars_03.png
     :alt: Fixing Illegal Characters Tool
     :width: 40em
     :align: center
  ```

- **Fix Objects Names** {cnum}`8`: replaces illegal characters in object names
  with the specified replacement characters. For check naming it's necessary to
  select objects.

  ```{note}
  This may result in objects having identical names.
  ```

- **Fix Materials Names** {cnum}`9`: replaces illegal characters in material
  names with the specified replacement characters.

  ```{note}
  This may result in materials having identical names.
  ```

- **All Changes to Lower Case** {cnum}`10`: converts all names to lower case and
  performs checks in lower case.
- **DAG Export After Fixing Illegal Characters** {cnum}`11`: opens the DAG
  format export window after all checks are complete.

  ```{note}
  Ensure you are familiar with how Dagor plugins work before enabling this
  option.
  ```

- **Save Current Settings** {cnum}`12`: saves the current settings.
- **Load Default Settings** {cnum}`13`: loads the default settings.
- **FIX ILLEGAL CHARACTERS FROM SELECTION!** {cnum}`14`: executes the script to
  fix illegal characters in the selected objects.
- **Open Local Documentation** {cnum}`15`: links to this documentation.
- **Get in Touch with the Author** {cnum}`16`: provides contact information for
  the developer if assistance is needed.
- **Errors Log** {cnum}`17`: displays information about the results of the
  checks and any changes made.

If the scene contains no errors, you will see a window like this:

```{eval-rst}
.. image:: _images/fixing_illegal_chars_04.png
   :alt: Fixing Illegal Characters Tool
   :width: 17em
   :align: center
```

Any errors will be highlighted in red and labeled as **ERROR!** or **WARNING!**:

```{eval-rst}
.. image:: _images/fixing_illegal_chars_05.png
   :alt: Fixing Illegal Characters Tool
   :width: 30em
   :align: center
```

A scene containing multiple errors may produce results like this:

```{eval-rst}
.. image:: _images/fixing_illegal_chars_06.png
   :alt: Fixing Illegal Characters Tool
   :width: 70em
   :align: center
```

