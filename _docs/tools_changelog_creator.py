from datetime import datetime, timedelta
from functools import cmp_to_key
from typing import Dict, List, Tuple
import argparse
import glob
import json
import os
import re
import sys

# Assuming that this file is in _docs.
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "../_buildtools/tools"))
import git

import requests

MARKDOWN_FILE_EXTENSION = ".md"
PATHS_TO_CHECK_FOR_CHANGES = "prog/tools/AssetViewer prog/tools/sceneTools/assetExp prog/tools/sceneTools/daEditorX prog/tools/sceneTools/daEditorX_extPlugins prog/tools/libTools prog/tools/sharedInclude skyquake/prog/tools/daEditorX"
DATABASE_DIRECTORY_OLD = "old"
DATABASE_DIRECTORY_EDITED = "edited"
DATABASE_DIRECTORY_TO_EDIT = "to-edit"
CHANGE_TYPE_ADDED = "ADDED"
CHANGE_TYPE_IMPROVED = "IMPROVED"
CHANGE_TYPE_FIXED = "FIXED"
CHANGE_TYPE_IGNORED = "IGNORED"
CHANGE_TYPES = [CHANGE_TYPE_ADDED, CHANGE_TYPE_IMPROVED, CHANGE_TYPE_FIXED, CHANGE_TYPE_IGNORED] # The order in this is also used for ordering the block in the final Markdown files.

class MyException(Exception):
  pass

class Globals:
  databaseDirectory = ""
  gerritUrl = "" # The full URL of Gerrit without a slash suffix. E.g.: https://gerrit.address

class Commit:
  date = ""
  subject = ""
  body = ""

class DatabaseEntry:
  relativeDirectory = "" # Relative to the database directory.
  changeId = "" # e.g.: Ib4f199b0fda030de6fed56623e5ccf3484a87f33
  changeNumber = "" # "591146" for https://gerrit.address/c/dagor/+/591146
  commitDate = "" # e.g.: 2026-02-17T15:15:47+00:00
  tool = "" # AV, DE, AV+DE
  changeType = "" # See CHANGE_TYPES.
  description = "" # The changelog entry.

  def getFullPath(self) -> str:
    return os.path.join(Globals.databaseDirectory, self.relativeDirectory, self.changeNumber + MARKDOWN_FILE_EXTENSION)

  def load(self) -> None:
    fullPath = self.getFullPath()

    with open(fullPath, "r") as f:
      commitDate = f.readline()
      self.commitDate = commitDate.removeprefix("Commit date:")
      if self.commitDate == commitDate:
        raise MyException(f"Expected \"Commit date:\", got \"{commitDate}\" in \"{fullPath}\".")
      self.commitDate = self.commitDate.strip()

      changeId = f.readline()
      self.changeId = changeId.removeprefix("Change ID:")
      if self.changeId == changeId:
        raise MyException(f"Expected \"Change ID:\", got \"{changeId}\" in \"{fullPath}\".")
      self.changeId = self.changeId.strip()
      if len(self.changeId) == 0:
        raise MyException(f"Change ID is empty in \"{fullPath}\".")

      changeType = f.readline()
      self.changeType = changeType.removeprefix("Change type:")
      if self.changeType == changeType:
        raise MyException(f"Expected \"Change type:\", got \"{changeType}\" in \"{fullPath}\".")
      self.changeType = self.changeType.strip().upper()

      if self.changeType == "ADD":
        self.changeType = CHANGE_TYPE_ADDED
      elif self.changeType == "FIX":
        self.changeType = CHANGE_TYPE_FIXED
      elif self.changeType == "IGNORE":
        self.changeType = CHANGE_TYPE_IGNORED
      elif self.changeType == "IMPROVE":
        self.changeType = CHANGE_TYPE_IMPROVED

      tool = f.readline()
      self.tool = tool.removeprefix("Tool:")
      if self.tool == tool:
        raise MyException(f"Expected \"Tool:\", got \"{tool}\" in \"{fullPath}\".")
      self.tool = self.tool.strip().upper()

      while True:
        line = f.readline()
        if len(line) == 0:
          break

        if len(self.description) > 0:
          self.description += "\n"
        self.description += line.rstrip()

  def isIgnored(self) -> bool:
    return self.changeType == CHANGE_TYPE_IGNORED

  def isValidChangeType(self) -> bool:
    return self.changeType in CHANGE_TYPES

  def isValidTool(self) -> bool:
    return self.tool in ["AV", "DE", "AV+DE"]

  def moveToDirectory(self, new_relative_directory: str) -> None:
    newDirectoryPath = os.path.join(Globals.databaseDirectory, new_relative_directory)
    os.makedirs(newDirectoryPath, exist_ok=True)

    newPath = os.path.join(newDirectoryPath, self.changeNumber + MARKDOWN_FILE_EXTENSION)
    os.rename(self.getFullPath(), newPath)
    self.relativeDirectory = new_relative_directory

class Database:
  changeIdToEntryMap = {} # type: Dict[str, DatabaseEntry]
  changeNumberToEntryMap = {} # type: Dict[str, DatabaseEntry]

  def load(self) -> None:
    for stateDirectoryName in [DATABASE_DIRECTORY_OLD, DATABASE_DIRECTORY_EDITED, DATABASE_DIRECTORY_TO_EDIT]:
      stateDirectoryPath = os.path.join(Globals.databaseDirectory, stateDirectoryName)
      for path in glob.glob("**/*.md", recursive=True, root_dir=stateDirectoryPath):
        entry = DatabaseEntry()
        entry.relativeDirectory = os.path.join(stateDirectoryName, os.path.dirname(path)).replace("\\", "/").rstrip("/")
        assert entry.relativeDirectory.find(":") < 0
        assert not entry.relativeDirectory.startswith("/")
        assert not entry.relativeDirectory.endswith("/")
        entry.changeNumber = os.path.splitext(os.path.basename(path))[0] # "123.md" -> "123"
        entry.load()

        if entry.changeId in self.changeIdToEntryMap:
          raise MyException(f"Duplicate change ID.\n  1. {self.changeIdToEntryMap[entry.changeId].getFullPath()}\n  2. {entry.getFullPath()}")

        if entry.changeNumber in self.changeNumberToEntryMap:
          raise MyException(f"duplicate commit ID.\n  1. {self.changeNumberToEntryMap[entry.changeNumber].getFullPath()}\n  2. {entry.getFullPath()}")

        self.changeIdToEntryMap[entry.changeId] = entry
        self.changeNumberToEntryMap[entry.changeNumber] = entry

  def getEntriesByRelativeDirectory(self, relative_directory) -> List[DatabaseEntry]:
    entries = []
    for entry in self.changeIdToEntryMap.values():
      if entry.relativeDirectory == relative_directory:
        entries.append(entry)
    return entries

def get_commits(dagor_dir: str, after_date: str) -> List[Commit]:
  commit_separator = "_changelog_writer_commit_separator_"
  property_separator = "_changelog_writer_property_separator_"
  cmd = f"log --after={after_date} \"--pretty=%cI{property_separator}%s{property_separator}%b{commit_separator}\" {PATHS_TO_CHECK_FOR_CHANGES}"

  result = git.cmd(cmd, dagor_dir, verbose=False)
  raw_commits = result.split(commit_separator)
  commits = [] # type: List[Commit]
  for raw_commit in raw_commits:
    properties = raw_commit.split(property_separator)
    if len(properties) != 3:
      break

    commit = Commit()
    commit.date = properties[0].strip()
    commit.subject = properties[1].strip()
    commit.body = properties[2].strip()

    commits.append(commit)

  return commits

def remove_change_id_from_body(body: str) -> Tuple[str, str]:
  # Example:
  # Change-Id: Iebf870c7f8ba473ea1bae9d52b4285e0669391f8
  match = re.search(r"Change-Id: ([a-zA-Z0-9]+)(?:\n|$)", body)
  if match is None:
    return None

  body = re.sub("Change-Id: [a-zA-Z0-9]+$", "", body)

  return body.strip(), match.group(1)

def find_gerrit_change_number_by_change_id(change_id: str, gerrit_user: str, gerrit_password: str) -> int:
  url = Globals.gerritUrl + "/a/changes"
  qParam = f"change:{change_id}"
  query_params = {"q": qParam}
  response = requests.get(url, query_params, auth = requests.auth.HTTPBasicAuth(gerrit_user, gerrit_password))

  # Gerrit: "To prevent against Cross Site Script Inclusion (XSSI) attacks, the JSON response body starts with
  # a magic prefix line that must be stripped before feeding the rest of the response body to a JSON parser:"
  content = response.content
  content = content[5:] # strip ")]}'\n"

  search_results = json.loads(content)
  final_search_result = None

  for search_result in search_results:
    if search_result["branch"] == "master":
      if final_search_result is not None:
        raise MyException(f"Got multiple results for Change-Id {change_id}!")
      final_search_result = search_result

  if final_search_result is None:
    raise MyException(f"Change-Id {change_id} could not be found on Gerrit!")

  return final_search_result["_number"]

def write_new_commits(database: Database, commits: List[Commit], gerrit_user: str, gerrit_password: str) -> None:
  for commit in commits:
    bodyWithoutChangeId, change_id = remove_change_id_from_body(commit.body)
    if len(change_id) == 0:
      raise MyException(f"Change ID cannot be found in commit's body:\n{commit.subject}\n\n{commit.body}")

    if change_id in database.changeIdToEntryMap:
      print(f"Skipping commit with change ID {change_id}. It already exists in {database.changeIdToEntryMap[change_id].getFullPath()}.")
      continue

    gerrit_change_number = find_gerrit_change_number_by_change_id(change_id, gerrit_user, gerrit_password)

    if str(gerrit_change_number) in database.changeNumberToEntryMap:
      raise MyException(f"Commit {gerrit_change_number} already exists in {database.changeNumberToEntryMap[str(gerrit_change_number)].getFullPath()}, but was not duplicate when searched by change ID {change_id}.")

    filePath = os.path.join(Globals.databaseDirectory, DATABASE_DIRECTORY_TO_EDIT, f"{gerrit_change_number}.md")
    if os.path.exists(filePath):
      raise MyException(f"File {filePath} already exists.")

    with open(filePath, "w") as f:
      f.write(
        f"Commit date: {commit.date}\n" \
        f"Change ID: {change_id}\n" \
        f"Change type:\n" \
        f"Tool:\n\n" \
        f"{commit.subject}\n\n" \
        f"{bodyWithoutChangeId} ([{gerrit_change_number}]({Globals.gerritUrl}/c/dagor/+/{gerrit_change_number}))")

def move_to_edited_directory(database: Database) -> None:
  entries = database.getEntriesByRelativeDirectory(DATABASE_DIRECTORY_TO_EDIT)
  for entry in entries:
    if len(entry.changeType) == 0:
      continue

    if entry.isIgnored():
      entry.moveToDirectory(f"{DATABASE_DIRECTORY_EDITED}/ignored")
      continue

    if not entry.isValidChangeType():
      raise MyException(f"Unsupported change type \"{entry.changeType}\" in {entry.getFullPath()}.")

    if len(entry.tool) == 0:
      continue

    if not entry.isValidTool():
      raise MyException(f"Unsupported tool type \"{entry.tool}\" in {entry.getFullPath()}.")

    entry.moveToDirectory(f"{DATABASE_DIRECTORY_EDITED}/{entry.tool}")

def get_monday_for_date(dateString: str):
  dateTime = datetime.fromisoformat(dateString)
  date = dateTime.date()
  weekday = date.weekday()
  mondayDate = date - timedelta(days = weekday)
  return mondayDate

def get_sort_value_for_change_type(change_type) -> int:
  for index, value in enumerate(CHANGE_TYPES):
    if value == change_type:
      return index
  return len(CHANGE_TYPES)

def compareFunction(a: DatabaseEntry, b: DatabaseEntry) -> int:
  # Sort by the week first in descending order.
  aDate = get_monday_for_date(a.commitDate)
  bDate = get_monday_for_date(b.commitDate)
  if aDate < bDate:
    return 1
  if aDate > bDate:
    return -1

  # Sort by ADDED, IMPROVED and FIXED in that order.
  aChangeTypeInt = get_sort_value_for_change_type(a.changeType)
  bChangeTypeInt = get_sort_value_for_change_type(b.changeType)
  if aChangeTypeInt < bChangeTypeInt:
    return -1
  if aChangeTypeInt > bChangeTypeInt:
    return 1

  # Sort by commit date in a descending order.
  if a.commitDate < b.commitDate:
    return 1
  if a.commitDate > b.commitDate:
    return -1
  return 0

def get_markdown_from_entries(entries: List[DatabaseEntry], for_asset_viewer = True) -> str:
  MARKDOWN_CHANGE_TYPE_MAP = {
    CHANGE_TYPE_ADDED: "{bdg-green}`ADDED`",
    CHANGE_TYPE_IMPROVED: "{bdg-blue}`IMPROVED`",
    CHANGE_TYPE_FIXED: "{bdg-orange}`FIXED`",
  }

  markdown = ""
  lastWeekDate = None

  for entry in entries:
    if entry.tool == "AV":
      if not for_asset_viewer:
        continue
    elif entry.tool == "DE":
      if for_asset_viewer:
        continue
    elif entry.tool != "AV+DE":
      raise MyException(f"Unsupported tool: \"{entry.tool}\".")

    mondayDate = get_monday_for_date(entry.commitDate)
    if (lastWeekDate is None) or mondayDate != lastWeekDate:
      lastWeekDate = mondayDate
      if len(markdown) > 0:
        markdown += "\n"
      markdown += f"## Week of {mondayDate.year}-{mondayDate.month:02d}-{mondayDate.day:02d}\n"

    description = entry.description.strip()
    markdownChangeType = MARKDOWN_CHANGE_TYPE_MAP.get(entry.changeType, None)
    if markdownChangeType is None:
      raise MyException(f"Unsupported change type: \"{entry.changeType}\".")

    markdown += f"\n{markdownChangeType} {description}\n"

  return markdown

def write_markdown_from_database(database: Database, output_directory: str) -> None:
  entries = [] # type: List[DatabaseEntry]

  for relativeDirectory in [f"{DATABASE_DIRECTORY_EDITED}/AV", f"{DATABASE_DIRECTORY_EDITED}/AV+DE", f"{DATABASE_DIRECTORY_EDITED}/DE"]:
    entries += database.getEntriesByRelativeDirectory(relativeDirectory)

  for entry in entries:
    assert not entry.isIgnored()
    assert entry.isValidChangeType()
    assert entry.isValidTool()

  entries.sort(key = cmp_to_key(compareFunction))

  markdown = get_markdown_from_entries(entries, for_asset_viewer = True)
  avFilePath = os.path.join(output_directory, "av.md")
  with open(avFilePath, "w") as f:
    f.write(markdown)
  print(f"Wrote \"{avFilePath}\".")

  markdown = get_markdown_from_entries(entries, for_asset_viewer = False)
  deFilePath = os.path.join(output_directory, "de.md")
  with open(deFilePath, "w") as f:
    f.write(markdown)
  print(f"Wrote \"{deFilePath}\".")

def main_internal():
  parser = argparse.ArgumentParser(
    prog = "tools_changelog_creator",
    epilog = "Examples:\n" \
      "tools_changelog_creator.py --database-directory d:\\changelog --gerrit-url https://gerrit.address update-from-gerrit --dagor-path d:\\dagor --after 2026-01-15 --gerrit-user USER --gerrit-password PASSWORD\n" \
      "tools_changelog_creator.py --database-directory d:\\changelog process-edits")

  parser.add_argument("--database-directory", required = True, help = "Changelog database directory.")

  commandParser = parser.add_subparsers(dest = "command")

  updateFromGerritParser = commandParser.add_parser("update-from-gerrit")
  updateFromGerritParser.add_argument("--dagor-path", required = True)
  updateFromGerritParser.add_argument("--after", required = True, help = "The changelog will be generated from commits after this date. Format: YYYY-MM-DD")
  updateFromGerritParser.add_argument("--gerrit-url", required = True, help = "The URL of Gerrit.")
  updateFromGerritParser.add_argument("--gerrit-user", required = True)
  updateFromGerritParser.add_argument("--gerrit-password", required = True, help = "Not the domain password! Gnerate it in Gerrit settings at HTTP Credentials.")

  commandParser.add_parser("process-edits")

  args = parser.parse_args()

  Globals.databaseDirectory = getattr(args, "database_directory", "")
  if len(Globals.databaseDirectory) == 0:
    raise MyException("The database directory is not set.")

  database = Database()
  database.load()

  command = args.command
  if command == "update-from-gerrit":
    dagorDir = getattr(args, "dagor_path")
    if not os.path.isdir(dagorDir):
      raise MyException(f"The specified path \"{dagorDir}\" does not not exist.")

    after = getattr(args, "after")

    Globals.gerritUrl = getattr(args, "gerrit_url", "").rstrip("/").strip()
    if len(Globals.gerritUrl) == 0:
      raise MyException("The Gerrit URL is not set.")

    gerritUser = getattr(args, "gerrit_user")
    gerritPassword = getattr(args, "gerrit_password")

    commits = get_commits(dagorDir, after)
    write_new_commits(database, commits, gerritUser, gerritPassword)
  elif command == "process-edits":
    move_to_edited_directory(database)

    if len(database.getEntriesByRelativeDirectory(DATABASE_DIRECTORY_TO_EDIT)) == 0:
      print("There are no more files to edit, generating the Markdown files.")
      write_markdown_from_database(database, Globals.databaseDirectory)

def main() -> int:
  try:
    main_internal()
    return 0
  except MyException as e:
    print(f"ERROR: {str(e)}")
    return 1

sys.exit(main())
