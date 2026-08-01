# tools changelog creator

The tools changelog creator script helps creating the tools changelog by keeping track of the already seen commits, making a basic Markdown file (with a link to the Gerrit change) for each commit that are related to tools, and then finally by generating the two Markdown files for Asset Viewer and daEditorX that contain the changes in a weekly breakdown grouped by change type (added, improved, fixed).

The script supports two commands:

## update-from-gerrit

Example:<br>
`tools_changelog_creator.py --database-directory d:\\changelog update-from-gerrit --dagor-path d:\\dagor --after 2026-01-15 --gerrit-url https://gerrit.address --gerrit-user USER --gerrit-password PASSWORD`

The script reads the git log of the dagor directory (specified with `--dagor-path`), collects the new commits that changed files within the tools-related directories (see PATHS_TO_CHECK_FOR_CHANGES in the source Python file), and saves them to Markdown files into the `to-edit` directory within the database directory (specified with `--database-directory`).

You have to edit the Markdown files and fill out the change type (`add`, `improve`, `fix`, `ignore`), determine the affected tool (`av`, `de`, `av+de`), and edit the commit message to make it more user friendly.

The `ignore` change type means that the commit will be not be included in the changelog. For example a code refactoring might not be interesting for the users.

## process-edits

Example:<br>
`tools_changelog_creator.py --database-directory d:\\changelog process-edits`

The script moves the edited Markdown files from the `to-edit` directory into the `edited` directory.

A Markdown file is considered edited if the change type and the tool fields are set.

Once the `to-edit` directory is empty the script generates the two Markdown files `av.md` and `de.md` into the database directory.

The contents of the generated `av.md` and `de.md` files must be prepended to:<br>
`D:\dagor\_docs\source\_internal\dagor-tools\changelog\asset-viewer\asset_viewer_2026.md`<br>
`D:\dagor\_docs\source\_internal\dagor-tools\changelog\daeditorx\daeditorx_2026.md`.