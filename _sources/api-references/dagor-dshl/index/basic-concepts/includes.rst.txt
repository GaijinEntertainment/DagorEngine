.. _includes:

========
Includes
========

DSHL supports:

- ``include``, e.g. ``include "some_shader.dshl"``
- ``include_optional``, e.g. ``include_optional "optional_shader.dshl"``

Includes in ``*.dshl`` files are always included one time,
and should not be confused with ``#include`` directive in hlsl files and blocks,
where they follow the regular preprocessor rules (and can be included multiple times).
