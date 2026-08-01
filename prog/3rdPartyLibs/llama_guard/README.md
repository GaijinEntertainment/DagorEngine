# Llama Guard 3 (8B)

Meta's [Llama-Guard-3-8B](https://huggingface.co/meta-llama/Llama-Guard-3-8B)
content moderation model, used by the Active Matter chat moderation backend.

This directory holds **only the license** for source control. The model is not
built from source and its weights are not committed or shipped with the game:
inference runs on backend infrastructure.

`active_matter/prog/jamfile` registers this directory with
`ExplicitLicenseUsed`, so the generated `LICENSE-<exe>` carries the Llama 3.1
Community License for every Active Matter executable.

- License: Llama 3.1 Community License (see `LICENSE`)
- Attribution notice required by section 1.b.iii: `LICENSE_NOTICE.txt`

Note: section 1.b.i of the license also requires prominently displaying
"Built with Llama" on a related website, user interface, or product
documentation; that is a separate obligation not covered by these files.
