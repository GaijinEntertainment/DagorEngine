import * as vscode from 'vscode';
import * as fs from 'fs';

export const testTag = new vscode.TestTag('test');
export const benchmarkTag = new vscode.TestTag('benchmark');

/** Regex to find [test] annotated functions in .das files */
const TEST_ANNOTATION_RE = /^[ \t]*\[test\b[^\]]*\]\s*\r?\n[ \t]*def\s+(\w+)/gm;

/** Regex to find [benchmark] annotated functions in .das files */
const BENCHMARK_ANNOTATION_RE = /^[ \t]*\[benchmark\b[^\]]*\]\s*\r?\n[ \t]*def\s+(\w+)/gm;

/** Quick check: file must require testing_boost to contain [test]/[benchmark] functions */
const TESTING_REQUIRE_RE = /require\s+(?:dastest\/)?testing_boost/;

/** Files with `expect` are negative compilation tests, not runnable */
const EXPECT_RE = /^expect\s+\d+/m;

export type TestKind = 'test' | 'benchmark';

export interface TestFunction {
    name: string;
    line: number; // 0-based line number of the def
    kind: TestKind;
}

/**
 * Parse a .das file for [test] and [benchmark]-annotated functions.
 * Returns empty array if file doesn't use testing_boost.
 */
export function parseTestFunctions(content: string): TestFunction[] {
    if (!TESTING_REQUIRE_RE.test(content) || EXPECT_RE.test(content)) {
        return [];
    }
    const results: TestFunction[] = [];

    function scanWith(re: RegExp, kind: TestKind) {
        let match: RegExpExecArray | null;
        re.lastIndex = 0;
        while ((match = re.exec(content)) !== null) {
            const upToMatch = content.substring(0, match.index);
            const annotationLine = upToMatch.split('\n').length - 1;
            const defLine = annotationLine + 1;
            results.push({ name: match[1], line: defLine, kind });
        }
    }

    scanWith(TEST_ANNOTATION_RE, 'test');
    scanWith(BENCHMARK_ANNOTATION_RE, 'benchmark');

    // Sort by line number so tree order matches file order
    results.sort((a, b) => a.line - b.line);
    return results;
}

/**
 * Check if a file basename should be skipped (underscore-prefixed files are helpers).
 */
function shouldSkipFile(basename: string): boolean {
    return basename.startsWith('_');
}

/**
 * Discover test files and add file-level items. Children are resolved lazily.
 */
export async function discoverTestFiles(
    controller: vscode.TestController,
    workspaceRoot: string,
): Promise<void> {
    const pattern = new vscode.RelativePattern(workspaceRoot, '**/*.das');
    const files = await vscode.workspace.findFiles(pattern);

    for (const fileUri of files) {
        const basename = fileUri.path.split('/').pop() ?? '';
        if (shouldSkipFile(basename)) {
            continue;
        }
        try {
            const content = fs.readFileSync(fileUri.fsPath, 'utf8');
            if (TESTING_REQUIRE_RE.test(content) && !EXPECT_RE.test(content)) {
                getOrCreateFileItem(controller, fileUri);
            }
        } catch {
            // Skip unreadable files
        }
    }
}

/**
 * Parse test functions in a file and add them as children of the file item.
 */
export function discoverTestsInFile(
    controller: vscode.TestController,
    fileItem: vscode.TestItem,
): void {
    const uri = fileItem.uri;
    if (!uri) {
        return;
    }
    try {
        const content = fs.readFileSync(uri.fsPath, 'utf8');
        const tests = parseTestFunctions(content);

        const newChildren: vscode.TestItem[] = [];
        for (const test of tests) {
            const testId = `${fileItem.id}/${test.name}`;
            const existing = fileItem.children.get(testId);
            if (existing) {
                // Reuse existing item to preserve description (benchmark stats)
                existing.range = new vscode.Range(test.line, 0, test.line, 0);
                newChildren.push(existing);
            } else {
                const label = test.kind === 'benchmark' ? `[bench] ${test.name}` : test.name;
                const testItem = controller.createTestItem(testId, label, uri);
                testItem.range = new vscode.Range(test.line, 0, test.line, 0);
                testItem.tags = [test.kind === 'benchmark' ? benchmarkTag : testTag];
                newChildren.push(testItem);
            }
        }
        fileItem.children.replace(newChildren);
    } catch {
        // Skip unreadable files
    }
}

/**
 * Get or create a file-level TestItem. Uses uri.toString() as ID (matches VSCode convention).
 */
export function getOrCreateFileItem(
    controller: vscode.TestController,
    fileUri: vscode.Uri,
): vscode.TestItem {
    const fileId = fileUri.toString();
    let fileItem = controller.items.get(fileId);
    if (!fileItem) {
        const label = vscode.workspace.asRelativePath(fileUri);
        fileItem = controller.createTestItem(fileId, label, fileUri);
        fileItem.canResolveChildren = true;
        controller.items.add(fileItem);
    }
    return fileItem;
}

/**
 * Remove a file-level TestItem if it exists.
 */
export function removeFileItem(
    controller: vscode.TestController,
    fileUri: vscode.Uri,
): void {
    controller.items.delete(fileUri.toString());
}
