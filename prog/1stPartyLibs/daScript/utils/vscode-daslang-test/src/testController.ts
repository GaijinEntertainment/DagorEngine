import * as vscode from 'vscode';
import { DaslangConfig, getRuntimeFlags } from './config';
import {
    discoverTestFiles,
    discoverTestsInFile,
    getOrCreateFileItem,
    removeFileItem,
    testTag,
    benchmarkTag,
} from './testDiscovery';
import { runTests, parseLocation } from './testRunner';

const outputChannel = vscode.window.createOutputChannel('daslang Tests');

export function createTestController(config: DaslangConfig): vscode.TestController {
    const controller = vscode.tests.createTestController('daslang-dastest', 'daslang Tests');

    // Resolve handler: root discovers files, file items discover children
    controller.resolveHandler = async (item) => {
        if (!item) {
            await discoverTestFiles(controller, config.workspaceRoot);
        } else {
            discoverTestsInFile(controller, item);
        }
    };

    // Default profile: runs all tests and benchmarks
    controller.createRunProfile('Run All', vscode.TestRunProfileKind.Run, (request, token) => {
        return runHandler(controller, config, request, token, 'all');
    }, true);

    // Tests only
    controller.createRunProfile('Run Tests Only', vscode.TestRunProfileKind.Run, (request, token) => {
        return runHandler(controller, config, request, token, 'test');
    }, false, testTag);

    // Benchmarks only
    controller.createRunProfile('Run Benchmarks Only', vscode.TestRunProfileKind.Run, (request, token) => {
        return runHandler(controller, config, request, token, 'benchmark');
    }, false, benchmarkTag);

    return controller;
}

/**
 * Set up a file watcher that keeps the test tree in sync.
 */
export function createFileWatcher(
    controller: vscode.TestController,
    config: DaslangConfig,
): vscode.FileSystemWatcher {
    const watcher = vscode.workspace.createFileSystemWatcher('**/*.das');

    const refresh = (uri: vscode.Uri) => {
        const basename = uri.path.split('/').pop() ?? '';
        if (basename.startsWith('_')) {
            return;
        }
        const fileItem = getOrCreateFileItem(controller, uri);
        discoverTestsInFile(controller, fileItem);
        // Remove file item if it has no test children (no longer a test file)
        if (fileItem.children.size === 0) {
            removeFileItem(controller, uri);
        }
    };

    watcher.onDidChange(refresh);
    watcher.onDidCreate(refresh);
    watcher.onDidDelete((uri) => removeFileItem(controller, uri));

    return watcher;
}

async function runHandler(
    controller: vscode.TestController,
    config: DaslangConfig,
    request: vscode.TestRunRequest,
    token: vscode.CancellationToken,
    mode: 'test' | 'benchmark' | 'all',
): Promise<void> {
    const run = controller.createTestRun(request);

    // Read toggle settings fresh (user may have changed them since activation)
    const { jit, isolatedMode } = getRuntimeFlags();

    // Show output channel and clear previous output
    outputChannel.clear();
    outputChannel.show(true); // true = preserve focus

    // Log run configuration
    const flags: string[] = [];
    if (jit) { flags.push('JIT'); }
    if (isolatedMode) { flags.push('isolated'); }
    outputChannel.appendLine(`--- daslang test run [${mode}]${flags.length > 0 ? ' (' + flags.join(', ') + ')' : ''} ---`);

    // Collect tests to run, grouped by file path
    const fileGroups = new Map<string, vscode.TestItem[]>();
    const testsToRun = request.include ?? gatherAllTests(controller);
    for (const item of testsToRun) {
        if (token.isCancellationRequested) {
            break;
        }

        if (item.children.size > 0 || item.canResolveChildren) {
            // File-level item — resolve children if needed, then collect matching ones
            if (item.canResolveChildren && item.children.size === 0) {
                discoverTestsInFile(controller, item);
            }
            const filePath = item.uri?.fsPath;
            if (filePath) {
                const children: vscode.TestItem[] = [];
                item.children.forEach(child => {
                    if (mode === 'all' || child.tags.some(t => t.id === (mode === 'benchmark' ? benchmarkTag : testTag).id)) {
                        children.push(child);
                    }
                });
                if (children.length > 0) {
                    fileGroups.set(filePath, children);
                }
            }
        } else {
            // Function-level item — group by parent file
            const filePath = item.uri?.fsPath;
            if (filePath) {
                const group = fileGroups.get(filePath) ?? [];
                group.push(item);
                fileGroups.set(filePath, group);
            }
        }
    }

    // Execute per file
    for (const [filePath, items] of fileGroups) {
        if (token.isCancellationRequested) {
            break;
        }

        // Split items by kind
        const testItems = items.filter(t => t.tags.some(tag => tag.id === testTag.id));
        const benchItems = items.filter(t => t.tags.some(tag => tag.id === benchmarkTag.id));

        // Determine which passes to run
        const passes: { tests: vscode.TestItem[]; benchmark: boolean }[] = [];
        if ((mode === 'test' || mode === 'all') && testItems.length > 0) {
            passes.push({ tests: testItems, benchmark: false });
        }
        if ((mode === 'benchmark' || mode === 'all') && benchItems.length > 0) {
            passes.push({ tests: benchItems, benchmark: true });
        }

        for (const pass of passes) {
            if (token.isCancellationRequested) {
                break;
            }

            // Mark tests as started
            for (const test of pass.tests) {
                run.started(test);
            }

            // Log which file is being tested
            outputChannel.appendLine(`\n> ${filePath}${pass.benchmark ? ' (benchmarks)' : ''}`);

            // Determine if we need to filter to specific tests/benchmarks
            const expectedTag = pass.benchmark ? benchmarkTag : testTag;
            const allMatchingInFile: vscode.TestItem[] = [];
            // Find the file item by checking all top-level items for matching fsPath
            controller.items.forEach(fileItem => {
                if (fileItem.uri?.fsPath === filePath) {
                    fileItem.children.forEach(child => {
                        if (child.tags.some(t => t.id === expectedTag.id)) {
                            allMatchingInFile.push(child);
                        }
                    });
                }
            });
            const runningAll = pass.tests.length >= allMatchingInFile.length;
            const testNames = runningAll ? undefined : pass.tests.map(t => t.id.split('/').pop()!);

            const cancellation = { isCancelled: false, kill: undefined as (() => void) | undefined };
            const tokenListener = token.onCancellationRequested(() => {
                cancellation.isCancelled = true;
                cancellation.kill?.();
            });

            try {
                const result = await runTests(config, filePath, testNames, cancellation, {
                    benchmark: pass.benchmark,
                    jit,
                    isolatedMode,
                    onOutput: (chunk) => {
                        outputChannel.append(chunk);
                    },
                });

                if (result.timedOut) {
                    for (const test of pass.tests) {
                        run.errored(test, new vscode.TestMessage(`Test timed out after ${config.timeout}s`));
                    }
                    continue;
                }

                if (!result.report) {
                    const message = result.stderr || result.stdout || `Exit code: ${result.exitCode}`;
                    for (const test of pass.tests) {
                        run.errored(test, new vscode.TestMessage(message));
                    }
                    continue;
                }

                if (result.report.tests.length === 0 && result.report.total > 0) {
                    // Isolated mode: JSON report has summary but no per-test details.
                    // Use summary to pass/fail all tests as a group.
                    const durationMs = result.report.time_usec / 1000;
                    if (result.report.success) {
                        for (const test of pass.tests) {
                            run.passed(test, durationMs);
                        }
                    } else {
                        const msg = `${result.report.failed} failed, ${result.report.errors} errors (isolated mode)`;
                        for (const test of pass.tests) {
                            run.failed(test, new vscode.TestMessage(msg), durationMs);
                        }
                    }
                } else {
                    // Normal mode: map individual JSON results to TestItems
                    for (const testResult of result.report.tests) {
                        const testItem = pass.tests.find(t => t.id.split('/').pop() === testResult.name);
                        if (!testItem) {
                            continue;
                        }

                        const durationMs = testResult.time / 1000;

                        if (testResult.skipped) {
                            run.skipped(testItem);
                        } else if (testResult.passed) {
                            if (pass.benchmark && testResult.messages.length > 0) {
                                const output = testResult.messages.join('\r\n') + '\r\n';
                                run.appendOutput(output, undefined, testItem);
                                testItem.description = testResult.messages.join(' | ');
                                // Don't pass duration — let description show stats instead of "1.2s"
                                run.passed(testItem);
                            } else {
                                run.passed(testItem, durationMs);
                            }
                        } else {
                            const messageText = testResult.messages.length > 0
                                ? testResult.messages.join('\n')
                                : 'Test failed';
                            const msg = new vscode.TestMessage(messageText);

                            const loc = parseLocation(testResult.location);
                            if (loc && testItem.uri) {
                                msg.location = new vscode.Location(
                                    testItem.uri,
                                    new vscode.Position(loc.line - 1, 0),
                                );
                            }

                            run.failed(testItem, msg, durationMs);
                        }
                    }

                    // Handle tests not in JSON report
                    for (const test of pass.tests) {
                        const found = result.report.tests.some(r => test.id.split('/').pop() === r.name);
                        if (!found) {
                            run.errored(test, new vscode.TestMessage('Test not found in results'));
                        }
                    }
                }
            } finally {
                tokenListener.dispose();
            }
        }
    }

    outputChannel.appendLine('\n--- done ---');
    run.end();
}

function gatherAllTests(controller: vscode.TestController): vscode.TestItem[] {
    const items: vscode.TestItem[] = [];
    controller.items.forEach(item => items.push(item));
    return items;
}
