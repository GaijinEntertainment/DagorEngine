import * as vscode from 'vscode';
import { resolveConfig } from './config';
import { createTestController, createFileWatcher } from './testController';

export function activate(context: vscode.ExtensionContext): void {
    const result = resolveConfig();
    if (!result.ok) {
        const binaryName = process.platform === 'win32' ? 'daslang.exe' : 'daslang';
        const messages: Record<string, string> = {
            'no-workspace': 'daslang Test Explorer: no workspace folder open.',
            'no-compiler': `daslang Test Explorer: compiler not found. Set "dascript.compiler" in settings, or open a workspace containing bin/Release/${binaryName}.`,
            'no-dastest': 'daslang Test Explorer: dastest not found. Set "dascript.dastestPath" in settings, or open a workspace containing dastest/dastest.das.',
        };
        vscode.window.showWarningMessage(messages[result.reason]);
        return;
    }

    const controller = createTestController(result.config);
    const watcher = createFileWatcher(controller, result.config);

    context.subscriptions.push(controller);
    context.subscriptions.push(watcher);
}

export function deactivate(): void {
    // Cleanup handled by disposables
}
