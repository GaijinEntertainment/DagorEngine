# RFC process

## Background

Whenever Quirrel language changes its syntax or semantics (including behavior of builtin libraries), we need to consider many implications of the changes.

Whenever new syntax is introduced, we need to ask:

- Is it backwards compatible?
- Is it easy for machines and humans to parse?
- Does it follow 'Zen of Quirrel' (based on Zen of Python?)
- Does it create grammar ambiguities for current and future syntax?
- Is it stylistically coherent with the rest of the language?
- Does it present challenges with editor integration like autocomplete?
- Will it affect performance?

For changes in semantics, we should be asking:

- Is behavior easy to understand and non-surprising?
- Can it be implemented performantly today?
- Is it compatible with type checking and other forms of static analysis?

For new standard library functions, we should be asking:

- Is the new functionality used/useful often enough in existing code?
- Does the standard library implementation carry important performance benefits that can't be achieved in user code?
- Is the behavior general and unambiguous, as opposed to solving a problem / providing an interface that's too specific?
- Is the function interface amenable to type checking / linting?

In addition to these questions, we also need to consider that every addition carries a cost, and too many features will result in a language that is harder to learn, harder to implement and ensure consistent implementation quality throughout, slower, etc. In addition, any language is greater than the sum of its parts and features often have non-intuitive interactions with each other.

Since reversing these decisions is incredibly costly and can be impossible due to backwards compatibility implications, all user facing changes to Quirrel language and core libraries must go through an RFC process.

## Process

There is no any special process for RFC review at the moment.

## Implementation

When an RFC gets merged, the feature *can* be implemented; however, there's no set timeline for that implementation. In some cases implementation may land in a matter of days after an RFC is merged, in some it may take months.

To avoid having permanently stale RFCs, in rare cases Quirrel team can *remove* a previously merged RFC when the landscape is believed to change enough for a feature like this to warrant further discussion.

When an RFC is implemented and the implementation is enabled via feature flags, RFC should be updated to include "**Status**: Implemented" at the top level (before *Summary* section).
