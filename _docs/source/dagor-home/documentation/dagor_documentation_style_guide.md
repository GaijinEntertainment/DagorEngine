# Dagor Documentation Style Guide

## About This Guide

This style guide outlines editorial guidelines for creating clear, consistent,
and accurate technical documentation intended for software developers and other
technical professionals. It is designed to ensure consistency across all
documentation related to Dagor Engine.

This guide is the primary reference for creating Dagor Engine documentation. If
the guidelines provided here do not cover specific scenarios, refer to the
following editorial resources:

- **Spelling:**	[Merriam-Webster](https://www.merriam-webster.com/).
- **Non-technical style:** [*The Chicago Manual of Style Online*](https://www.chicagomanualofstyle.org/home.html).
- **Technical style:** [Google developer documentation style guide](https://developers.google.com/style).

## Voice and Tone

Adopt a voice and tone that is professional yet approachable – clear,
respectful, and free from slang or excessive informality. The goal is to sound
like a knowledgeable guide who understands the developer's needs, providing
helpful insights without being overly formal or condescending.

While a conversational tone is encouraged, avoid writing exactly as you speak.
Strike a balance between clarity and relatability to keep the content engaging
but focused.

Avoid being overly entertaining or excessively dry. The primary purpose of the
documentation is to convey information efficiently to readers who may be seeking
answers under time constraints.

Keep in mind that the audience may include readers from diverse cultural
backgrounds and varying levels of proficiency in English. Writing in simple,
consistent language enhances readability and facilitates easier translation into
other languages.

## Word List

**3ds Max**
: Always capitalize *Max*.

**add-on (noun or adjective), add on (verb)**
: Not *addon*.

**Asset Viewer**
: Capitalize each word.

**assetViewer{}**

**Blender**
: Always capitalize.

**BLK format**
: When referring to a format in general.

**`.blk` files**
: When referring to specific files with the BLK format. File extensions are
  formatted with a [code font](#code-font).

**Building Resources**
: Use *Building Resources* when referring to the title or heading, and *resource
  building process* (or similar) when describing the act of compiling resources.

**button**
: You *click* on-screen buttons and mouse buttons, and *press* keys on the keyboard.

**checkbox**
: Use *check*/*uncheck* for clear and specific checkbox actions.

  {octicon}`check;1.4em` Check the **Enable Ray Tracing** box to activate
advanced lighting effects in your scene.

  {octicon}`check;1.4em` Uncheck the **Enable Shadows** box to disable shadow
rendering and improve performance.

  Avoid ambiguity by clarifying the checkbox's effect.

  {octicon}`check;1.4em` Check the **Dynamic Lighting** box to apply real-time
lighting changes during gameplay.

  {octicon}`x;1.4em` Enable the box for dynamic lighting. (Unclear: What does
"enable the box" mean? Does it turn on the feature or just make the checkbox
switchable?)

**daBuild**

**daEditor**

**Dagor Engine**
: Capitalize each word.

**daNetGame**

**daNetGame-based, daNetGame-like**

**dynmodel**
: Lowercase except at the beginning of a sentence, heading, or list item.

**ECS**
: All caps. Expand the abbreviation on first mention.

**FAR**
: All caps.

**gameobj**
: Lowercase except at the beginning of a sentence, heading, or list item.

**ID**
: Not *Id* or *id*, except in string literals or enums.

**Impostor Baker**
: Capitalize each word.

**in-game (adjective)**
: Not *in game*.

**internet**
: Lowercase except at the beginning of a sentence, heading, or list item.

**LOD, LODs**
: All caps. Expand the abbreviation on first mention.

**login (noun or adjective), log in (verb)**
: Not *log-in*.

**microdetails**
: Not *micro-details*. Lowercase except at the beginning of a sentence, heading,
  or list item.

**parent-child or parent/child**
: Not *parent – child* or *parent—child*.

**per**
: To express a rate, use *per* instead of the division slash (`/`).

**please**
: Do not use *please* in the normal course of explaining how to use a product.

  Use *please* only when you're asking for permission or forgiveness. For
  example, when what you're asking for inconveniences a reader, or suggests a
  potential issue with a product.

**plugin (noun), plug-in (adjective), plug in (verb)**

**pop-up window**, **pop-up menu**,
: Use *pop-up window* to describe a window that appears over the main interface
  to provide additional information, request input, or display notifications.

  Use *pop-up menu* to describe the menu that appears when a user right-clicks
  an item.

**prebuilt**
: Not *pre-built*.

**prefab**
: Lowercase except at the beginning of a sentence, heading, or list item.

**read-only**
: Not *read only*.

**rendinst**
: Lowercase except at the beginning of a sentence, heading, or list item.

**screenshot**
: Not *screen shot*.

**sign-in (noun or adjective), sign in (verb)**

**sign-out (noun or adjective), sign out (verb)**

**subdirectory**
: Not *sub-directory*.

**toolkit**
: Not *tool-kit* or *tool kit*.

**utilize**
: Do not use *utilize* when you mean *use*.

  Use *utilize* or *utilization* when referring to the quantity of a resource
  being used.

**vromfs**
: Lowercase except at the beginning of a sentence, heading, or list item.

**War Thunder**
: Capitalize each word.

**War Thunder-based, War Thunder-like**
: Not *WarThunder-based* or *War-Thunder-based*.

## Product Names

This section describes how to use product names.

- **Capitalize Product Names.** When you write about any product, follow the
  official capitalization for the names of brands, companies, software,
  products, services, features, and terms defined by companies and open source
  communities. If an official name begins with a lowercase letter, then put it
  in lowercase even at the start of a sentence. But it's better to revise the
  sentence to avoid putting a lowercase word at the start, if possible.

- **Feature Names.** When you write about a feature, don't capitalize it unless
  the name is officially capitalized. If you're unsure, follow the precedent
  that's set by other documents that describe the feature.

- **Shorten Product Names.** Use the full trademarked product name. Don't
  abbreviate product names, except in cases where you're matching a UI label.

- **Use *The* with Product Names.** Use *the* with product names when referring
  to specific instances, roles, or modified versions. Avoid *the* when the name
  functions as a standalone proper noun or appears in branding.

  ```{seealso}
  For more information, see
  [Use *The*](#use-the).
  ```

## Text Formatting

Text-formatting summary for many of the general conventions covered elsewhere in
the style guide.

### Bold

Use bold formatting, for UI elements, run-in headings and in special cases to
emphasize important phrases in a sentence.

### Italic

Use italics formatting, when drawing attention to a specific word or phrase,
such as when defining terms or using words as words.

### Underline

Do not underline.

### Code Font

Use code font for:

- Filenames, filename extensions, and paths
- Folders and directories
- Parameter names and values
- Placeholder variables
- Text input
- Console output
- Key names and mouse button labels
- Code in text, inline code, and code samples

**Items to put in ordinary (non-code) font:**

- IP addresses
- URLs
- Domain names
- Names of products, services, and organizations

### Font Type, Size, and Color

- Do not override global styles for font type, size, or color.
- Use different text colors in examples only when it is the most effective
  method to clarify a concept.

### Other Punctuation Conventions

- Do not use ampersands (`&`) as conjunctions or shorthand for *and*. Use *and*
  instead.
- Use `&` in cases where you need to refer to a UI element or the name that uses
  `&`.
- Do not use an exclamation mark.

### Units of Measurement

- Put a nonbreaking space `&nbsp;` between the number and the unit.

  {octicon}`check;1.4em` 120&nbsp;GB

  {octicon}`x;1.4em` 120GB

  {octicon}`check;1.4em` 5&nbsp;m

  {octicon}`x;1.4em` 5m

- If the unit of measure is money or percent or degrees of an angle, do not use
  a space.

  {octicon}`check;1.4em` $20

  {octicon}`check;1.4em` 50%

  {octicon}`check;1.4em` 90°

- In a range of numbers, repeat the unit for each number. Use the word *to*
  between the numbers, rather than a hyphen.

  {octicon}`check;1.4em` 0° to 90°

  {octicon}`x;1.4em` 0-90°

  {octicon}`check;1.4em` 5&nbsp;m to 10&nbsp;m

  {octicon}`x;1.4em` 5-10&nbsp;m

### Headings

- Each file should contain a single topic article.
- The article may have one top-level heading, and subsequent headings should
  follow a sequential order, without skipping any levels.
- If the main heading of an article is defined using other tools, section
  headings within the file should begin at the second level.

### Capitalization

Follow the standard capitalization rules for American English. Additionally, do
the following:

- Do not use unnecessary capitalization.
- Do not use all-uppercase, except in the following contexts: in official names,
  in abbreviations that are always written in all-caps, or when referring to
  code that uses all-caps.
- Do not use camel case, except in official names or when referring to code that
  uses camel case.

  ```{seealso}
  For information on how to capitalize specific words, see
  [Word List](#word-list).
  ```

#### Capitalize Product Names

```{seealso}
For more information on how to capitalize product names, see
[Product Names](#product-names).
```

#### Capitalization in Titles and Headings

- In document titles and headings, use **title case**. That is, capitalize the
  first and last words of the title, as well as all major words such as nouns,
  pronouns, verbs, adjectives, adverbs, and proper nouns.
- Articles (*a*, *an*, *the*), conjunctions (*and*, *but*, *or*), and
  prepositions with fewer than four letters are not capitalized unless they
  appear as the first or last word of the title.

#### Capitalization in References to Titles and Headings

- When referencing any title or heading from a document that follows this guide,
  use **title case**.
- For titles or headings from works or sources that do not follow this guide,
  retain the original capitalization.

#### Capitalization for Figures and Tables

- Use **sentence case** for labels, callouts, and other text in images and
  diagrams.
- Use **sentence case** for all the elements in a table: contents, headings,
  labels, and captions.

#### Capitalization and End Punctuation

##### Numbered, Lettered, and Bulleted Lists

Start each list item with a capital letter, unless case is an important part of
the information conveyed by the list.

End each list item with a period, except in the following cases:

- If the item consists of a single word.
- If the item doesn't include a verb.
- If the item is entirely in code font.
- If the item is entirely link text or a document title.

{octicon}`check;1.4em` Recommended:

The interface is organized into three main panels:
1. Control Panel
2. Properties Panel
3. Viewport Settings Panel

{octicon}`check;1.4em` Recommended:

To export an animation, follow these steps:
1. Ensure Visibility.
2. Export Settings.
3. Add Note Tracks.

{octicon}`check;1.4em` Recommended:

Environment textures:
- envi snapshot
- background texture
- paint details texture
- reflection texture

##### Description Lists that Use Run-in Headings

- Start the run-in heading with a capital letter.
- End the run-in heading with a period or a colon, but be consistent within the
  list.
- You can decide whether to bold the punctuation that ends the heading based on
  factors such as on-page consistency.
- For the descriptions that follow the punctuation, capitalize the first letter
  as follows:
  + If the text follows a period, start the text with a capital letter.
  + If the text follows a colon, start the text with a lowercase letter.
- To end the descriptive text, punctuate as follows:
  + If the description follows a period, end the description with a period.
  + If the description follows a colon, do one of the following:
    - If the description is a list of items or short phrases without verbs, do
      not include a period.
    - If the description includes a verb or expresses a standalone thought, end
      the description with a period.
- Do not use a dash to set off a description from an item in a description list.

{octicon}`check;1.4em` Recommended:

This property defines how the object interacts with landscape collision:
- **Ignore Collision:** the object's height matches its pivot point.
- **Place Pivot:** the pivot point is placed directly on the collision surface.
- **Place Pivot and Use Normal:** the object's pivot aligns to the normal of the
  landscape.
- **Place 3-Point (bbox)**: a bounding box is created around the object.
- **Place Foundation (bbox):** all points of the bounding box base align with
  the collision surface.

{octicon}`check;1.4em` Recommended:

There are three types of builds:
- **Full Build.** Builds all resources.
- **Partial Pack Build.** Builds the specific pack containing the resource you
  need.
- **Partial Package Build.** Builds an entire package.

#### Capitalization in Hyphenated Words

Capitalize all main words in hyphenated terms in titles and headings.

{octicon}`check;1.4em` High-Level Architecture Overview

{octicon}`x;1.4em` High-level architecture overview

**Exceptions:**

- **Hyphenated Words with Prefixes:** do not capitalize prefixes in hyphenated
  words unless they start the title or heading.

  {octicon}`check;1.4em` Re-evaluation of System Processes

  {octicon}`x;1.4em` Re-Evaluation of System Processes

- **Hyphenated Words Beginning with Single Letters:** do not capitalize the
  single letter at the start of a hyphenated term unless it begins the title or
  heading.

  {octicon}`check;1.4em` X-ray Analysis Results

  {octicon}`x;1.4em` X-Ray Analysis Results

- **Hyphenated Articles, Prepositions, and Coordinating Conjunctions:** do not
  capitalize hyphenated articles, prepositions, or coordinating conjunctions
  unless they start the title or heading.

  {octicon}`check;1.4em` End-to-end Encryption Setup

  {octicon}`x;1.4em` End-To-End Encryption Setup

#### Capitalization and Colons

Use a lowercase letter to begin the first word of the text immediately following
a colon unless the text falls into one of the exceptions.

{octicon}`check;1.4em` The system processes data in three phases: input,
processing, and output.

{octicon}`x;1.4em` The system processes data in three phases: Input, processing,
and output.

**Exceptions:**

- **Proper Noun:** capitalize the first word if it is a proper noun.
- **Heading:** capitalize the first word if the text following the colon is a
  heading.
- **Quotation:** capitalize the first word if the text following the colon is a
  quotation.

## Language

### Abbreviations

#### Key Guidelines for Abbreviation Usage

- Use standard acronyms that improve readability and save the reader time.
- Spell out abbreviations on first reference.
- Avoid using specialized abbreviations that may be unfamiliar to your audience
  unless they are clearly defined.

#### When to Spell Out a Term

- When an abbreviation is likely to be unfamiliar to the reader, spell it out
  upon first mention and include the abbreviation in parentheses immediately
  afterward. *Italicize* both the spelled-out term and its abbreviation.
- For all subsequent mentions of the term, use only the abbreviation.
- Capitalize the letters which are used directly in the abbreviated form.
- If the first mention of an abbreviation occurs in a heading or title, it is
  acceptable to use the abbreviation directly. Then, spell out and define the
  abbreviation in the first paragraph following the heading.

  {octicon}`check;1.4em` Level of Detail (LOD)

  {octicon}`check;1.4em` Graphical User Interface (GUI)

  {octicon}`check;1.4em` eXtensible Markup Language (XML)

#### Commonly Accepted Abbreviations

The following abbreviations are widely recognized and generally do not need to
be spelled out:

- API
- LOD
- FPS
- File formats such as PNG, JPEG, or OBJ
- Units of measurement like MB, GB, or TB
- URL
- GPU, CPU, RAM

### Articles

#### Use *The*

Using *the* with product names in documentation depends on whether the product
name is treated as a proper noun or a general noun phrase.

##### When to Use *The*

- **Generic References**. Use *the* when referring to a specific instance,
  feature, or role of the product.

  {octicon}`check;1.4em` The Dagor Engine renderer supports advanced real-time
  lighting techniques.

- **Functional Contexts**. Use *the* when describing how the product functions
  or interacts in a specific scenario.

  {octicon}`check;1.4em` The Dagor Engine physics module excels in simulating
  large-scale destructible environments.

- **With Modifiers**. Use *the* when the product name is modified by an
  adjective, clause, or descriptive element.

  {octicon}`check;1.4em` The enhanced Dagor Engine API streamlines shader
  development.

##### When Not to Use *The*

- **As a Standalone Proper Noun**. Avoid *the* when the product name stands
  alone as a proper noun.

  {octicon}`check;1.4em` Dagor Engine integrates seamlessly with custom asset
  pipelines.

- **In Branding or Titles**. Do not use *the* when the product name appears in a
  title or branded term.

  {octicon}`check;1.4em` Explore new possibilities with Dagor Engine 6.

**Exceptions:**

- Some product names may inherently include *the* due to branding or established
  usage.
- In lists or general overviews, omitting *the* is acceptable for brevity:

  {octicon}`check;1.4em` Features include GPU-driven rendering, multi-threaded
  processing, and terrain optimization.


