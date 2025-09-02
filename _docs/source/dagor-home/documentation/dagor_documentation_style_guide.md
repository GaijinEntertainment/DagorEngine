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

3ds Max
: Always capitalize *Max*.

add-on vs. add on
: Not *addon*.

  Use *add-on* as a noun or adjective.

  {octicon}`check;1.4em` The game includes a new add-on for additional levels.

  Use *add on* as a verb.

  {octicon}`check;1.4em` The developer plans to add on more content next update.

Asset Viewer
: Capitalize each word.

Blender
: Always capitalize.

BLK format
: When referring to a format in general.

`.blk` files
: When referring to specific files with the BLK format. File extensions are
  formatted with a [code font](#code-font).

Building Resources
: Use *Building Resources* when referring to the title or heading.

  Use *resource building process* (or similar) when describing the act of
  compiling resources.

button
: You *click* on-screen buttons and mouse buttons, and *press* keys on the
  keyboard.

changelog
: Not *changeLog* or *ChangeLog*.

  Lowercase except at the beginning of a sentence, heading, or list item.

checkbox
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

D3D, Direct3D, DirectX
: Always capitalize.

daBuild
: Retain the exact casing as defined by the development team. If possible,
  revise the sentence to avoid starting with a lowercase
  [product name](dagor_documentation_style_guide.md#product-names).

daEditor, daEditorX
: Retain the exact casing as defined by the development team. If possible,
  revise the sentence to avoid starting with a lowercase
  [product name](dagor_documentation_style_guide.md#product-names).

daGI
: Expand the abbreviation on first mention: Dagor Global Illumination (daGI).

Dagor Engine
: Capitalize each word.

daNetGame
: Retain the exact casing as defined by the development team. If possible,
  revise the sentence to avoid starting with a lowercase
  [product name](dagor_documentation_style_guide.md#product-names).

daNetGame-based, daNetGame-like
: Not *daNetGame based* or *daNetGame like*.

daRG
: Expand the abbreviation on first mention: Dagor Reactive GUI (daRG).

dynmodel
: Lowercase except at the beginning of a sentence, heading, or list item.

ECS
: All caps. Expand the abbreviation on first mention: Entity-Component-System
  (ECS). When expanded, capitalize each word and hyphenate to form a compound
  noun.

FAR
: All caps.

gameobj
: Lowercase except at the beginning of a sentence, heading, or list item.

ID
: Not *Id* or *id*, except in string literals or enums.

Impostor Baker
: Capitalize each word.

in-game vs. in game
: Not *ingame*.

  Use *in-game* as an adjective.

  {octicon}`check;1.4em` View your in-game settings.

  Use *in game* only when *in* is a preposition followed by the noun *game*.

  {octicon}`check;1.4em` That feature isn't available in game.

internet
: Lowercase except at the beginning of a sentence, heading, or list item.

LOD, LODs
: All caps. Expand the abbreviation on first mention: Level of Detail (LOD).

login vs. log in
: Not *log-in*.

  Use *login* as a noun or adjective.

  {octicon}`check;1.4em` Enter your login credentials.

  Use *log in* as a verb.

  {octicon}`check;1.4em` You need to log in to view your achievements.

microdetails
: Not *micro-details*.

  Lowercase except at the beginning of a sentence, heading, or list item.

parent-child or parent/child
: Not *parent – child* or *parent—child*.

per
: To express a rate, use *per* instead of the division slash (`/`).

please
: Do not use *please* in the normal course of explaining how to use a product.

  Use *please* only when you're asking for permission or forgiveness. For
  example, when what you're asking for inconveniences a reader, or suggests a
  potential issue with a product.

plugin vs. plug-in vs. plug in
: Use *plugin* as a noun.

  {octicon}`check;1.4em` This audio plugin improves spatial sound processing.

  Use *plug-in* as an adjective.

  {octicon}`check;1.4em` Enable the plug-in module for particle effects.

  Use *plug in* as a verb.

  {octicon}`check;1.4em` Please plug in the VR headset to continue.

pop-up window, pop-up menu
: Use *pop-up window* to describe a window that appears over the main interface
  to provide additional information, request input, or display notifications.

  Use *pop-up menu* to describe the menu that appears when a user right-clicks
  an item.

prebuilt
: Not *pre-built*.

prefab
: Lowercase except at the beginning of a sentence, heading, or list item.

read-only
: Not *read only*.

real-time vs. real time
: Use *real-time* with a hyphen when used as an adjective:

  {octicon}`check;1.4em` real-time updates, real-time communication

  Use *real time* without a hyphen when used as a noun.

  {octicon}`check;1.4em` The system operates in real time.

rendinst
: Lowercase except at the beginning of a sentence, heading, or list item.

riExtra
: Lowercase except at the beginning of a sentence, heading, or list item. Always
  capitalize *Extra*.

screenshot
: Not *screen shot*.

sign-in vs. sign in
: Use *sign-in* as a noun or adjective.

  {octicon}`check;1.4em` The sign-in process includes two-factor authentication.

  Use *sign in* as a verb.

  {octicon}`check;1.4em` Players must sign in with their account before
  accessing online services.

sign-out vs. sign out
: Use *sign-out* as a noun or adjective.

  {octicon}`check;1.4em` The sign-out option is in the settings menu.

  Use *sign out* as a verb.

  {octicon}`check;1.4em` The game automatically signs out inactive users.

subdirectory
: Not *sub-directory*.

toolkit
: Not *tool-kit* or *tool kit*.

utilize
: Do not use *utilize* when you mean *use*.

  Use *utilize* or *utilization* when referring to the quantity of a resource
  being used.

vromfs
: Lowercase except at the beginning of a sentence, heading, or list item.

War Thunder
: Capitalize each word.

War Thunder-based, War Thunder-like
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
  [Use *The*](#using-the).
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
  For information about how to capitalize specific words, see
  [Word List](#word-list).
  ```

#### Capitalization of Product Names

```{seealso}
For more information about how to capitalize product names, see
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
- For titles or headings from sources that do not follow this guide, retain the
  original capitalization.

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

  {octicon}`check;1.4em` Artificial Intelligence (AI)

  {octicon}`check;1.4em` Game Engine Architecture (GEA)

#### Commonly Accepted Abbreviations

The following abbreviations are widely recognized and generally do not need to
be spelled out:

- API
- CPU, GPU, RAM
- FPS
- File formats such as PNG, JPEG, or OBJ
- GUI, UI
- Units of measurement like MB, GB, or TB
- URL

### Articles

#### Using *The*

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

#### Omitting Articles in Headings and Titles

It is acceptable to omit articles (such as *a*, *an*, and *the*) in headings and
titles for brevity and clarity. This helps maintain a clean and concise
presentation of content, especially when space is limited or when the title
needs to be more direct.

## Linking

### Cross-references

Cross-references are typically used to link to nonessential information that
enhances the reader's understanding. Whenever possible, provide contextual help
rather than redirecting the reader elsewhere.

Follow these guidelines when creating cross-references to other documents:

- When referring to a document that adheres to this style guide, place the
  reference in a separate paragraph using the documentation builder tools. If
  you are referencing the same document from multiple locations within the
  original document, it is acceptable to insert the link directly within the
  text, without placing it in a separate paragraph.

- When referencing third-party tools or products, linking to the relevant site
  is acceptable. Ensure that any linked site is reputable, reliable, and of high
  quality.

- Do not link to content that is behind a paywall or otherwise restricted. All
  linked materials must be freely accessible to the reader, with no additional
  access requirements.

### Formatting Cross-references

- Introduce the link with the phrase *For more information, see* or *For more
  information about..., see*.

- Use *see* and *about* instead of *read* and *on* when referencing links and
  cross-references.

  {octicon}`check;1.4em` Recommended:

  ```{seealso}
  For more information about how to set up and start developing with the Dagor
  Engine, see
  [Getting Started with Dagor](../../getting-started/index.rst).
  ```

  {octicon}`x;1.4em` For more information on assets, read
  [Introduction to Assets](../../assets/about-assets/about_assets.md).

  {octicon}`x;1.4em` See this
  [article](../documentation/dagor_documentation_style_guide.md).

  {octicon}`x;1.4em` Click
  [here](../documentation/dagor_documentation_style_guide.md).

- When referencing a document title or heading from a document that follows this
  guide, use the full document title in **title case**.

  {octicon}`check;1.4em` Recommended:

  ```{seealso}
  For more information, see
  [Capitalization in References to Titles and Headings](#capitalization-in-references-to-titles-and-headings).
  ```

- If the link points to a downloadable file, clarify this in the link text and
  specify the file type and size if the file is large.

  {octicon}`check;1.4em` Recommended:

  ```{seealso}
  For more information about Dagor ECS, download Anton Yudintsev's video lecture
  {download}`Gameplay-Architecture-and-Design.mp4, 3.65 GB <https://drive.google.com/file/d/1Op_NELH6slRCGsbTPcNF-Tmx18ulnYjC/view?usp=drive_link>`.
  ```

- When including a cross-reference as a link, avoid putting the link text in
  quotation marks.

  {octicon}`check;1.4em` Recommended:

  ```{seealso}
  For more information, see
  [BLK File Format](../../dagor-tools/blk/blk.md).
  ```

  {octicon}`x;1.4em` Not recommended:

  ```{seealso}
  For more information, see
  ["BLK File Format"](../../dagor-tools/blk/blk.md).
  ```

- Do not use a URL as link text. Instead, use the page title or a description of
  the linked page. An exception may be made for certain legal documents (e.g.,
  Terms of Service), where URLs can be used as link text.

- If the link includes an abbreviation in parentheses, include both the full
  form and the abbreviation in the link text.

  {octicon}`check;1.4em` [Physically based rendering (PBR)](https://en.wikipedia.org/wiki/Physically_based_rendering)

  {octicon}`x;1.4em` [Physically based rendering](https://en.wikipedia.org/wiki/Physically_based_rendering) (PBR)

- If the text includes a command or a code element typically represented in code
  font, include a description of the code element in the link text, unless it
  would be redundant or awkward.

  {octicon}`check;1.4em` To set the current camera direction, run the
  [`camera.dir` command](../../dagor-tools/daeditor/daeditor-console-commands/daeditor_console_commands.md#cameradir).

  {octicon}`check;1.4em` The daEditor supports the
  [`list`](../../dagor-tools/daeditor/daeditor-console-commands/daeditor_console_commands.md#list),
  [`help`](../../dagor-tools/daeditor/daeditor-console-commands/daeditor_console_commands.md#help),
  and
  [`clear`](../../dagor-tools/daeditor/daeditor-console-commands/daeditor_console_commands.md#clear)
  console commands.

  {octicon}`x;1.4em` To configure the environment settings, run the
  [`envi.set`](../../dagor-tools/daeditor/daeditor-console-commands/daeditor_console_commands.md#enviset)
  command.

- Do not place links in section headings.

- Do not use images as links.

### Links to Sections on the Same Page

When linking to another section on the same page, indicate that the link will
navigate to a different section of the same document.

{octicon}`check;1.4em` Recommended:

```{seealso}
For more information, see the
[Formatting Cross-references](#formatting-cross-references) section of this document.
```

### Links to Sections on Another Page

When linking to a section on another page, format the link the same way as a
standard cross-reference.

If the section title on the destination page is identical to one on the source
page, provide additional context for clarity.

  {octicon}`check;1.4em` Recommended:

  ```{seealso}
  For more information, see
  [Basic concepts](../../api-references/dagor-ecs/dagor_ecs.md#basic-concepts) in "Dagor ECS".
  ```

### Punctuation with Links

When punctuation appears immediately before or after a link, place the
punctuation outside the link tags where possible.

{octicon}`check;1.4em` For more information, see
[Getting Started with Dagor](../../getting-started/index.rst).

{octicon}`x;1.4em` For more information, see
[Getting Started with Dagor.](../../getting-started/index.rst)

## Punctuation

### Hyphen, Dash, and Minus Sign

[Em dashes (—)](#em-dash-), [en dashes (–)](#en-dash-), [hyphens
(-)](#hyphen--), and [minus signs (−)](#minus-sign) are distinct and should be
handled accordingly, with attention to their proper function.

#### Hyphen (-)

Use the hyphen to join words or split them at the end of a line.

**General Guidelines**

Hyphenation depends on context, readability, and conventions:

- **Position**: Does the term come before a noun or after a verb?
- **Clarity**: Will the sentence be unclear without a hyphen?
- **Rules**: Some terms must always or never be hyphenated, regardless of other
  factors.

Exceptions are common. If unsure, check these sources in order:

1. Existing documentation conventions.
2. This style guide's [Word List](#word-list).
3. [Merriam-Webster dictionary](https://www.merriam-webster.com/).

**Use hyphen for:**

- **Compound Noun Modifiers**

  Use a hyphen between two or more words that work together to modify a noun
  **before** the noun, especially when:

  - **Clarity requires it**

    {octicon}`check;1.4em` floating-point texture

    {octicon}`check;1.4em` frame-by-frame playback

    {octicon}`check;1.4em` up-to-date build

    {octicon}`x;1.4em` frame by frame playback

  - **One word is a participle (-ed or -ing)**

    {octicon}`check;1.4em` GPU-accelerated path

    {octicon}`check;1.4em` baked-in lighting

    {octicon}`check;1.4em` left-aligned text

    {octicon}`check;1.4em` The text is left aligned. *(no hyphen after noun)*

  - **The modifier includes a number or letter**

    {octicon}`check;1.4em` 3-point lighting

    {octicon}`check;1.4em` 5-stage pipeline

    {octicon}`check;1.4em` Y-based sorting

  Avoid hyphens when the meaning is clear without them or the modifier comes
  after the noun.

  If a modifier has more than two words, try to rephrase. If rephrasing isn't
  possible, hyphenate as needed for clarity.

  ```{admonition} Exceptions
  :class: warning

  Don't hyphenate **-ly adverbs** unless clarity suffers:

  {octicon}`check;1.4em` Publicly available assets

  {octicon}`check;1.4em` Highly detailed mesh

  {octicon}`check;1.4em` Visually rich environment

  {octicon}`x;1.4em` Poorly-lit scene

  Don't hyphenate terms that are **commonly unhyphenated**:

  {octicon}`check;1.4em` A managed instance group

  {octicon}`check;1.4em` A machine learning model

  For **number + unit** modifiers, skip the hyphen unless needed:

  {octicon}`check;1.4em` 60&nbsp;FPS animation

  {octicon}`check;1.4em` 200&nbsp;GB disk

  ```{seealso}
  For more information, see [Units of Measurement](#units-of-measurement).
  ```

- **Suspended Compound Modifiers**

  Avoid **suspended compound modifiers**, such as:

  {octicon}`x;1.4em` CPU- and GPU-bound tasks

  {octicon}`x;1.4em` left- and right-aligned elements

  These constructions can reduce clarity. Instead, spell out the full phrase:

  {octicon}`check;1.4em` CPU-bound and GPU-bound tasks

  {octicon}`check;1.4em` left-aligned and right-aligned elements

  If you must use a suspended modifier due to space constraints (e.g. in UI or
  diagrams), include a hyphen after the first word and a space:

  {octicon}`check;1.4em` mouse- or keyboard-based input

  **Don't** use suspension when dealing with **simple one-word adjectives**:

  {octicon}`check;1.4em` high and low settings

  {octicon}`x;1.4em` high- and low settings

- **Compound Numerals and Fractions**

  Use hyphens in **compound numerals** and **fractions** when they function as
  adjectives.

  {octicon}`check;1.4em` a thirty-second frame

  {octicon}`check;1.4em` a one-third resolution scale

  {octicon}`check;1.4em` a sixty-four-bit architecture

  Don't hyphenate if the fraction or number is used as a noun:

  {octicon}`check;1.4em` The frame took one third of a millisecond.

  {octicon}`check;1.4em` Support for sixty four bit is deprecated.

  {octicon}`x;1.4em` Support for sixty-four-bit is deprecated.

- **Compound Nouns**

  It's preferable to use the **closed (one-word)** form for compound nouns
  whenever it's standard in engine or toolchain usage:

  {octicon}`check;1.4em` renderpass, framebuffer, toolchain

  If [Merriam-Webster](https://www.merriam-webster.com/) lists a spaced or
  hyphenated form but the closed version is commonly used in game engine-related
  area or codebase, use the closed form.

  ```{admonition} Exceptions
  :class: warning

  Use hyphens or spaces for established terms:

  {octicon}`check;1.4em` multi-pass rendering, debug layer, camera rig, physics material

  Some words change form by usage:

  {octicon}`check;1.4em` **set up** (verb), **setup** (noun)

  {octicon}`check;1.4em` **log in** (verb), **login** (noun)

  For compound units of measurement or units that multiply, use hyphens:

  {octicon}`check;1.4em` 120&nbsp;render-thread-hours

  {octicon}`check;1.4em` 10&nbsp;AI-agent-hours

  {octicon}`check;1.4em` 5&nbsp;vCPU-hours

  {octicon}`check;1.4em` 40&nbsp;person-hours
  ```

- **Prefixes**

  Avoid creating new, nonstandard words by simply attaching a prefix to an
  existing word. If the resulting form isn't widely recognized, rephrase for
  clarity.

  {octicon}`check;1.4em` reinitialize the renderer

  {octicon}`check;1.4em` non-player entity

  {octicon}`x;1.4em` reinit renderer

  {octicon}`x;1.4em` noninteractiveable UI

  Use a hyphen between a prefix and a stem word:

  - If a confusing or ambiguous word results without the hyphen:

    {octicon}`check;1.4em` **re-sign** (sign again) vs **resign** (quit a job)

    {octicon}`check;1.4em` **co-op** (cooperative) vs **coop** (a cage for chickens)

  - If the stem word begins with a capital letter:

    {octicon}`check;1.4em` non-XML, pre-IPv6

  If the prefixed word is listed in
  [Merriam-Webster](https://www.merriam-webster.com/) or the [Word
  List](#word-list), you can use it. When in doubt, rewrite or hyphenate to
  improve readability.

  In general, don't include a hyphen after the following prefixes unless
  omitting the hyphen could confuse the reader.

  - auto-
  - co-
  - cyber-
  - exa-, giga-, kilo-, mega-
  - inter-, intra-
  - macro-, micro-, mono-
  - non-
  - omni-
  - pre-
  - re-
  - sub-
  - tera-
  - un-, uni-

#### En Dash (–)

**Don't use en dashes.**

Use a hyphen or the word ***to*** instead.

For ranges, use:

{octicon}`check;1.4em` **60–120 FPS** write as **60 to 120 FPS**

{octicon}`check;1.4em` **GPU-bound tasks 1–4** write as **tasks 1 to 4**

**Avoid using dashes with spaces** (–, —, or -) to separate labels from
descriptions. Use a colon or period instead.

{octicon}`check;1.4em` Frame timing: Controls update frequency.

{octicon}`check;1.4em` Example: Use renderpass to group draw calls.

{octicon}`x;1.4em` Frame timing - Controls update frequency

{octicon}`x;1.4em` Example — Use renderpass to group draw calls

#### Em Dash (—)

**Don't use em dashes.**

Em dashes can cause inconsistency in formatting, reduce readability in technical
contexts, and create issues in code snippets, tooling, and localization.

Instead, use:

- A **colon (:)** to introduce explanations or definitions.
- A **hyphen (-)** for compound terms.
- The word ***to*** for ranges.

  {octicon}`check;1.4em` Renderpass: Defines a sequence of draw calls

  {octicon}`check;1.4em` Use a real-time rendering pipeline

  {octicon}`check;1.4em` Set frame rate to 60 to 120 FPS

```{admonition} Why we avoid en/em dashes?
:class: important

To keep formatting consistent and machine-friendly, **Dagor documentation does
not use en (–) or em (—) dashes**. Dashes can cause issues with readability,
localization, and rendering across platforms. Instead, we prefer clear and
stable alternatives:

- Use a **colon (:)** to introduce definitions or descriptions.
- Use the word ***to*** for numeric ranges.
- Use a **hyphen (-)** for compound terms.

  {octicon}`check;1.4em` LOD: Level of Detail system

  {octicon}`check;1.4em` 60 to 120 FPS

  {octicon}`check;1.4em` GPU-bound rendering

This approach improves clarity and avoids formatting issues in tools, editors,
and translations.
```

#### Minus Sign

Use the minus sign (−) only for subtraction and negative numbers in formulas or
code.

Do **not** substitute an en dash (–) for the minus sign, as it can cause
confusion and accessibility issues.

{octicon}`check;1.4em` `Health = MaxHealth − DamageTaken`

{octicon}`check;1.4em` Velocity = −9.8 m/s² (gravity)


