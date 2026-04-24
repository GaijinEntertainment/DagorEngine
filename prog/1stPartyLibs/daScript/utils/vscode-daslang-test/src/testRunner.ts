import * as cp from 'child_process';
import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';
import { DaslangConfig } from './config';

export interface SuiteTestResult {
    name: string;
    passed: boolean;
    skipped: boolean;
    time: number;       // microseconds
    messages: string[];
    location: string;   // "file:line" or empty
}

export interface JsonTestReport {
    file: string;
    total: number;
    passed: number;
    failed: number;
    errors: number;
    skipped: number;
    success: boolean;
    time_usec: number;
    tests: SuiteTestResult[];
}

export interface RunResult {
    report: JsonTestReport | null;
    stdout: string;
    stderr: string;
    exitCode: number | null;
    timedOut: boolean;
}

/**
 * Run dastest on a file, optionally filtering to specific test names.
 */
export async function runTests(
    config: DaslangConfig,
    filePath: string,
    testNames?: string[],
    cancellation?: { readonly isCancelled: boolean; kill?: () => void },
    options?: { benchmark?: boolean; jit?: boolean; isolatedMode?: boolean; onOutput?: (chunk: string) => void },
): Promise<RunResult> {
    const jsonFile = path.join(os.tmpdir(), `dastest_${Date.now()}_${Math.random().toString(36).slice(2)}.json`);

    // Build command: daslang.exe dastest.das [-jit] -- [--isolated-mode] --test file --json-file tmp ...
    // Note: -jit goes AFTER script path so that get_command_line_arguments()[0]=compiler, [1]=script.
    // dastest's isolated mode uses args[0] and args[1] to construct subprocess commands.
    const args: string[] = [];

    // Script path (must be args[1] for isolated mode subprocess construction)
    args.push(config.dastestPath);

    // Top-level daslang flags (after script path, before --)
    if (options?.jit) {
        args.push('-jit');
    }

    // Separator
    args.push('--');

    // dastest flags
    if (options?.isolatedMode) {
        args.push('--isolated-mode');
    }
    args.push('--test', filePath);
    args.push('--json-file', jsonFile);

    if (options?.benchmark) {
        args.push('--bench');
    }
    if (testNames) {
        const nameFlag = options?.benchmark ? '--bench-names' : '--test-names';
        for (const name of testNames) {
            args.push(nameFlag, name);
        }
    }

    return new Promise<RunResult>((resolve) => {
        const proc = cp.spawn(config.compilerPath, args, {
            cwd: config.workspaceRoot,
            stdio: ['ignore', 'pipe', 'pipe'],
        });

        if (cancellation) {
            cancellation.kill = () => proc.kill();
        }

        let stdout = '';
        let stderr = '';
        let timedOut = false;

        proc.stdout.on('data', (data: Buffer) => {
            const chunk = data.toString();
            stdout += chunk;
            options?.onOutput?.(chunk);
        });
        proc.stderr.on('data', (data: Buffer) => {
            const chunk = data.toString();
            stderr += chunk;
            options?.onOutput?.(chunk);
        });

        const timer = setTimeout(() => {
            timedOut = true;
            proc.kill();
        }, config.timeout * 1000);

        proc.on('close', (exitCode) => {
            clearTimeout(timer);

            let report: JsonTestReport | null = null;
            try {
                if (fs.existsSync(jsonFile)) {
                    const content = fs.readFileSync(jsonFile, 'utf8');
                    report = JSON.parse(content) as JsonTestReport;
                    fs.unlinkSync(jsonFile);
                }
            } catch {
                // JSON parse or file read failed
            }

            resolve({ report, stdout, stderr, exitCode, timedOut });
        });
    });
}

/**
 * Parse a "file:line" location string into file path and line number.
 */
export function parseLocation(location: string): { file: string; line: number } | null {
    if (!location) {
        return null;
    }
    const match = location.match(/^(.+):(\d+)$/);
    if (!match) {
        return null;
    }
    return { file: match[1], line: parseInt(match[2], 10) };
}
