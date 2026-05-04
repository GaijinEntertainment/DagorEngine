import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';

export interface DaslangConfig {
    compilerPath: string;
    dastestPath: string;
    workspaceRoot: string;
    timeout: number;
}

/** Read toggle settings fresh (not cached at activation). */
export function getRuntimeFlags(): { isolatedMode: boolean; jit: boolean } {
    const cfg = vscode.workspace.getConfiguration('dascript');
    return {
        isolatedMode: cfg.get<boolean>('isolatedMode', false),
        jit: cfg.get<boolean>('jit', false),
    };
}

export type ConfigResult =
    | { ok: true; config: DaslangConfig }
    | { ok: false; reason: 'no-workspace' | 'no-compiler' | 'no-dastest' };

function findFile(...candidates: string[]): string | undefined {
    for (const c of candidates) {
        if (c && fs.existsSync(c)) {
            return c;
        }
    }
    return undefined;
}

export function resolveConfig(): ConfigResult {
    const folders = vscode.workspace.workspaceFolders;
    if (!folders || folders.length === 0) {
        return { ok: false, reason: 'no-workspace' };
    }
    const workspaceRoot = folders[0].uri.fsPath;
    const cfg = vscode.workspace.getConfiguration('dascript');
    const timeout = cfg.get<number>('testTimeout', 120);

    // Resolve compiler path (platform-aware binary name)
    const binaryName = process.platform === 'win32' ? 'daslang.exe' : 'daslang';
    const compilerSetting = cfg.get<string>('compiler', '');
    const compilerPath = findFile(
        compilerSetting,
        path.join(workspaceRoot, 'bin', 'Release', binaryName),
        path.join(workspaceRoot, 'bin', 'Debug', binaryName),
    );
    if (!compilerPath) {
        return { ok: false, reason: 'no-compiler' };
    }

    // Resolve dastest path
    const dastestSetting = cfg.get<string>('dastestPath', '');
    const dastestPath = findFile(
        dastestSetting,
        path.join(workspaceRoot, 'dastest', 'dastest.das'),
    );
    if (!dastestPath) {
        return { ok: false, reason: 'no-dastest' };
    }

    return { ok: true, config: { compilerPath, dastestPath, workspaceRoot, timeout } };
}
